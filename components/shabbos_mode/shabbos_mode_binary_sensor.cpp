#include "shabbos_mode_binary_sensor.h"

#include <cmath>

#include "esphome/core/log.h"

namespace esphome {
namespace shabbos_mode {

static const char *const TAG = "shabbos_mode.binary_sensor";
static const double GEOMETRIC_ZENITH = 90.0;

void ShabbosModeBinarySensor::setup() {
  if (this->time_ == nullptr) {
    ESP_LOGW(TAG, "No time source configured");
    return;
  }

  auto now = this->time_->now();
  if (!now.is_valid()) {
    ESP_LOGD(TAG, "Time is not valid yet, waiting for the first sync");
    return;
  }

  this->publish_state(this->compute_active_(now));
}

void ShabbosModeBinarySensor::update() {
  if (this->time_ == nullptr) {
    return;
  }

  auto now = this->time_->now();
  if (!now.is_valid()) {
    ESP_LOGD(TAG, "Skipping update because the current time is not valid");
    return;
  }

  this->publish_state(this->compute_active_(now));
}

void ShabbosModeBinarySensor::dump_config() {
  LOG_BINARY_SENSOR("", "Shabbos Mode Binary Sensor", this);
  ESP_LOGCONFIG(TAG, "  Latitude: %.6f", this->latitude_);
  ESP_LOGCONFIG(TAG, "  Longitude: %.6f", this->longitude_);
  ESP_LOGCONFIG(TAG, "  Elevation: %.2f m", this->elevation_);
  ESP_LOGCONFIG(TAG, "  In Israel: %s", YESNO(this->in_israel_));
  ESP_LOGCONFIG(TAG, "  Start degree: %.2f", this->start_degree_);
  ESP_LOGCONFIG(TAG, "  Start offset: %d min", this->start_offset_minutes_);
  ESP_LOGCONFIG(TAG, "  End degree: %.2f", this->end_degree_);
  ESP_LOGCONFIG(TAG, "  End offset: %d min", this->end_offset_minutes_);
  if (this->has_early_take_in_) {
    ESP_LOGCONFIG(TAG, "  Early take-in time: %02d:%02d", this->early_take_in_hour_, this->early_take_in_minute_);
    ESP_LOGCONFIG(TAG, "  Early take-in applies to Yom Tov: %s", YESNO(this->early_take_in_for_yom_tov_));
  }
  LOG_UPDATE_INTERVAL(this);
}

bool ShabbosModeBinarySensor::compute_active_(const ESPTime &now) const {
  auto now_copy = now;
  struct tm current_tm = now_copy.to_c_tm();
  hdate current = convertDate(current_tm);
  current.offset = ESPTime::timezone_offset();
  setEY(&current, this->in_israel_);

  bool active = false;
  if (isassurbemelachah(current)) {
    hdate end = this->calculate_end_event_(current);
    if (this->is_valid_event_(end)) {
      active = hdatecompare(current, end) == 1;
    }
  }

  int candlelighting = iscandlelighting(current);
  if (!active && (candlelighting == 1 || candlelighting == 2)) {
    hdate start = this->calculate_start_event_(current);
    if (this->is_valid_event_(start)) {
      active = hdatecompare(current, start) != 1;
    }
  }

  return active;
}

hdate ShabbosModeBinarySensor::calculate_start_event_(hdate date) const {
  if (iscandlelighting(date) == 2) {
    return this->calculate_date_event_(date, this->end_degree_, this->end_offset_minutes_);
  }

  hdate start = this->calculate_date_event_(date, this->start_degree_, this->start_offset_minutes_);
  if (!this->should_apply_early_take_in_(date)) {
    return start;
  }

  hdate early_start = this->calculate_early_take_in_event_(date);
  if (!this->is_valid_event_(early_start)) {
    return start;
  }

  if (!this->is_valid_event_(start) || hdatecompare(early_start, start) == 1) {
    return early_start;
  }
  return start;
}

hdate ShabbosModeBinarySensor::calculate_end_event_(hdate date) const {
  return this->calculate_date_event_(date, this->end_degree_, this->end_offset_minutes_);
}

hdate ShabbosModeBinarySensor::calculate_early_take_in_event_(hdate date) const {
  hdate result = hdatenew(date.year, date.month, date.day, this->early_take_in_hour_, this->early_take_in_minute_, 0, 0,
                          date.offset);
  setEY(&result, date.EY);
  return result;
}

hdate ShabbosModeBinarySensor::calculate_date_event_(hdate date, double degree, int offset_minutes) const {
  location here{this->latitude_, this->longitude_, this->elevation_};
  const double zenith = GEOMETRIC_ZENITH + degree;
  const double utc_time = getUTCSunset(hdatejulian(date), here, zenith, degree > 0.0);

  hdate result = this->get_date_from_utc_time_(date, utc_time, false);
  if (this->is_valid_event_(result) && offset_minutes != 0) {
    hdateaddminute(&result, offset_minutes);
  }
  return result;
}

hdate ShabbosModeBinarySensor::get_date_from_utc_time_(hdate current, double time, bool is_sunrise) const {
  hdate result = {0};
  if (std::isnan(time)) {
    return result;
  }

  int adjustment = this->get_antimeridian_adjustment_(current);
  double calculated_time = time;
  result.year = current.year;
  result.EY = current.EY;
  result.offset = current.offset;
  result.month = current.month;
  result.day = current.day;
  if (adjustment != 0) {
    hdateaddday(&result, adjustment);
  }

  int hours = static_cast<int>(calculated_time);
  calculated_time -= hours;
  int minutes = static_cast<int>(calculated_time *= 60);
  calculated_time -= minutes;
  int seconds = static_cast<int>(calculated_time *= 60);
  calculated_time -= seconds;
  int milliseconds = static_cast<int>(calculated_time * 1000);
  int local_time_hours = static_cast<int>(this->longitude_) / 15;

  if (is_sunrise && local_time_hours + hours > 18) {
    hdateaddday(&result, -1);
  } else if (!is_sunrise && local_time_hours + hours < 6) {
    hdateaddday(&result, 1);
  }

  result.hour = hours;
  result.min = minutes;
  result.sec = seconds;
  result.msec = milliseconds;
  hdateaddsecond(&result, current.offset);
  return result;
}

int ShabbosModeBinarySensor::get_antimeridian_adjustment_(hdate current) const {
  const double local_hours_offset = this->get_local_mean_time_offset_(current) / 3600.0;
  if (local_hours_offset >= 20.0) {
    return 1;
  }
  if (local_hours_offset <= -20.0) {
    return -1;
  }
  return 0;
}

long ShabbosModeBinarySensor::get_local_mean_time_offset_(hdate current) const {
  return static_cast<long>(this->longitude_ * 4 * 60 - current.offset);
}

bool ShabbosModeBinarySensor::should_apply_early_take_in_(hdate date) const {
  if (!this->has_early_take_in_ || iscandlelighting(date) != 1) {
    return false;
  }
  if (date.wday == 6) {
    return true;
  }
  return this->early_take_in_for_yom_tov_;
}

bool ShabbosModeBinarySensor::is_valid_event_(const hdate &date) const { return date.year != 0; }

}  // namespace shabbos_mode
}  // namespace esphome
