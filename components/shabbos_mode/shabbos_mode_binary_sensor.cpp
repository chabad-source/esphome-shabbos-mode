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
    if (this->has_early_take_in_time_) {
      ESP_LOGCONFIG(TAG, "  Early take-in requested time: %02d:%02d", this->early_take_in_hour_,
                    this->early_take_in_minute_);
    } else {
      ESP_LOGCONFIG(TAG, "  Early take-in requested time: plag");
    }
    ESP_LOGCONFIG(TAG, "  Early take-in offset: %d min", this->early_take_in_offset_minutes_);
    ESP_LOGCONFIG(TAG, "  Early take-in enabled: %s", YESNO(this->early_take_in_enabled_));
    ESP_LOGCONFIG(TAG, "  Early take-in applies to Yom Tov: %s", YESNO(this->early_take_in_for_yom_tov_));
    const char *plag_opinion = "baal_hatanya";
    if (this->early_take_in_plag_opinion_ == PLAG_OPINION_GRA) {
      plag_opinion = "gra";
    } else if (this->early_take_in_plag_opinion_ == PLAG_OPINION_MGA) {
      plag_opinion = "mga";
    }
    ESP_LOGCONFIG(TAG, "  Early take-in plag opinion: %s", plag_opinion);
    if (this->has_early_take_in_from_) {
      ESP_LOGCONFIG(TAG, "  Early take-in from: %02d-%02d", this->early_take_in_from_month_, this->early_take_in_from_day_);
    } else {
      ESP_LOGCONFIG(TAG, "  Early take-in from: always");
    }
    if (this->has_early_take_in_to_) {
      ESP_LOGCONFIG(TAG, "  Early take-in to: %02d-%02d", this->early_take_in_to_month_, this->early_take_in_to_day_);
    } else {
      ESP_LOGCONFIG(TAG, "  Early take-in to: always");
    }
  }
  LOG_UPDATE_INTERVAL(this);
}

bool ShabbosModeBinarySensor::compute_active_(const ESPTime &now) const {
  auto now_copy = now;
  struct tm current_tm = now_copy.to_c_tm();
  hdate current = convertDate(current_tm);
  current.offset = ESPTime::timezone_offset();
  setEY(&current, this->in_israel_);
  int current_month = current_tm.tm_mon + 1;
  int current_day = current_tm.tm_mday;

  bool active = false;
  if (isassurbemelachah(current)) {
    hdate end = this->calculate_end_event_(current);
    if (this->is_valid_event_(end)) {
      active = hdatecompare(current, end) == 1;
    }
  }

  int candlelighting = iscandlelighting(current);
  if (!active && (candlelighting == 1 || candlelighting == 2)) {
    hdate start = this->calculate_start_event_(current, current_month, current_day);
    if (this->is_valid_event_(start)) {
      active = hdatecompare(current, start) != 1;
    }
  }

  return active;
}

hdate ShabbosModeBinarySensor::calculate_start_event_(hdate date, int current_month, int current_day) const {
  if (iscandlelighting(date) == 2) {
    return this->calculate_date_event_(date, this->end_degree_, this->end_offset_minutes_);
  }

  hdate start = this->calculate_date_event_(date, this->start_degree_, this->start_offset_minutes_);
  if (!this->should_apply_early_take_in_(date, current_month, current_day)) {
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
  hdate plag = this->calculate_plag_event_(date);
  if (!this->is_valid_event_(plag)) {
    return plag;
  }

  hdate candidate = plag;
  if (this->has_early_take_in_time_) {
    candidate = hdatenew(date.year, date.month, date.day, this->early_take_in_hour_, this->early_take_in_minute_, 0, 0,
                         date.offset);
    setEY(&candidate, date.EY);
  }
  if (this->early_take_in_offset_minutes_ != 0) {
    hdateaddminute(&candidate, this->early_take_in_offset_minutes_);
  }
  if (hdatecompare(candidate, plag) == 1) {
    return plag;
  }
  return candidate;
}

hdate ShabbosModeBinarySensor::calculate_plag_event_(hdate date) const {
  location here{this->latitude_, this->longitude_, this->elevation_};
  hdate startday = {0};
  hdate endday = {0};

  switch (this->early_take_in_plag_opinion_) {
    case PLAG_OPINION_GRA:
      startday = this->get_date_from_utc_time_(date, getUTCSunrise(hdatejulian(date), here, 90.0, 0), true);
      endday = this->get_date_from_utc_time_(date, getUTCSunset(hdatejulian(date), here, 90.0, 0), false);
      break;
    case PLAG_OPINION_MGA:
      startday = this->get_date_from_utc_time_(date, getUTCSunrise(hdatejulian(date), here, 90.0, 0), true);
      if (this->is_valid_event_(startday)) {
        hdateaddminute(&startday, -72);
      }
      endday = this->get_date_from_utc_time_(date, getUTCSunset(hdatejulian(date), here, 90.0, 0), false);
      if (this->is_valid_event_(endday)) {
        hdateaddminute(&endday, 72);
      }
      break;
    case PLAG_OPINION_BAAL_HATANYA:
    default:
      startday = this->get_date_from_utc_time_(date, getUTCSunrise(hdatejulian(date), here, 91.583, 0), true);
      endday = this->get_date_from_utc_time_(date, getUTCSunset(hdatejulian(date), here, 91.583, 0), false);
      break;
  }

  long shaah_zmanis = this->calculate_shaah_zmanis_(startday, endday);
  if (shaah_zmanis == 0) {
    return (hdate) {0};
  }
  hdate result = startday;
  hdateaddmsecond(&result, static_cast<long>(shaah_zmanis * 10.75));
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

bool ShabbosModeBinarySensor::should_apply_early_take_in_(hdate date, int current_month, int current_day) const {
  if (!this->has_early_take_in_ || !this->early_take_in_enabled_ || iscandlelighting(date) != 1) {
    return false;
  }
  if (!this->is_in_early_take_in_range_(current_month, current_day)) {
    return false;
  }
  if (date.wday == 6) {
    return true;
  }
  return this->early_take_in_for_yom_tov_;
}

bool ShabbosModeBinarySensor::is_in_early_take_in_range_(int current_month, int current_day) const {
  if (!this->has_early_take_in_from_ && !this->has_early_take_in_to_) {
    return true;
  }

  int current = this->month_day_to_ordinal_(current_month, current_day);
  int from = this->has_early_take_in_from_ ? this->month_day_to_ordinal_(this->early_take_in_from_month_, this->early_take_in_from_day_) : 1;
  int to = this->has_early_take_in_to_ ? this->month_day_to_ordinal_(this->early_take_in_to_month_, this->early_take_in_to_day_) : 366;

  if (from <= to) {
    return current >= from && current <= to;
  }
  return current >= from || current <= to;
}

int ShabbosModeBinarySensor::month_day_to_ordinal_(int month, int day) const {
  static const int offsets[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  return offsets[month - 1] + day;
}

long ShabbosModeBinarySensor::calculate_shaah_zmanis_(hdate startday, hdate endday) const {
  long diff = 0;
  long start = HebrewCalendarElapsedDays(startday.year) + (startday.dayofyear - 1);
  long end = HebrewCalendarElapsedDays(endday.year) + (endday.dayofyear - 1);
  diff = end - start;
  diff = (diff * 24) + (endday.hour - startday.hour);
  diff = (diff * 60) + (endday.min - startday.min);
  diff = (diff * 60) + (endday.sec - startday.sec);
  diff = (diff * 1000) + (endday.msec - startday.msec);
  if (startday.year == 0 || endday.year == 0) {
    return 0;
  }
  return diff / 12;
}

void ShabbosModeBinarySensor::set_early_take_in_plag_opinion(const std::string &plag_opinion) {
  this->has_early_take_in_ = true;
  this->early_take_in_enabled_ = true;
  if (plag_opinion == "gra") {
    this->early_take_in_plag_opinion_ = PLAG_OPINION_GRA;
  } else if (plag_opinion == "mga") {
    this->early_take_in_plag_opinion_ = PLAG_OPINION_MGA;
  } else {
    this->early_take_in_plag_opinion_ = PLAG_OPINION_BAAL_HATANYA;
  }
}

float ShabbosModeBinarySensor::get_setting_number_value(SettingNumberType type) const {
  switch (type) {
    case SETTING_NUMBER_LATITUDE:
      return this->latitude_;
    case SETTING_NUMBER_LONGITUDE:
      return this->longitude_;
    case SETTING_NUMBER_ELEVATION:
      return this->elevation_;
    case SETTING_NUMBER_START_DEGREE:
      return this->start_degree_;
    case SETTING_NUMBER_START_OFFSET_MINUTES:
      return this->start_offset_minutes_;
    case SETTING_NUMBER_END_DEGREE:
      return this->end_degree_;
    case SETTING_NUMBER_END_OFFSET_MINUTES:
      return this->end_offset_minutes_;
    case SETTING_NUMBER_EARLY_TAKE_IN_HOUR:
      return this->early_take_in_hour_;
    case SETTING_NUMBER_EARLY_TAKE_IN_MINUTE:
      return this->early_take_in_minute_;
    case SETTING_NUMBER_EARLY_TAKE_IN_OFFSET_MINUTES:
      return this->early_take_in_offset_minutes_;
    case SETTING_NUMBER_EARLY_TAKE_IN_FROM_MONTH:
      return this->early_take_in_from_month_;
    case SETTING_NUMBER_EARLY_TAKE_IN_FROM_DAY:
      return this->early_take_in_from_day_;
    case SETTING_NUMBER_EARLY_TAKE_IN_TO_MONTH:
      return this->early_take_in_to_month_;
    case SETTING_NUMBER_EARLY_TAKE_IN_TO_DAY:
      return this->early_take_in_to_day_;
  }
  return 0.0f;
}

void ShabbosModeBinarySensor::set_setting_number_value(SettingNumberType type, float value) {
  switch (type) {
    case SETTING_NUMBER_LATITUDE:
      this->latitude_ = value;
      break;
    case SETTING_NUMBER_LONGITUDE:
      this->longitude_ = value;
      break;
    case SETTING_NUMBER_ELEVATION:
      this->elevation_ = value;
      break;
    case SETTING_NUMBER_START_DEGREE:
      this->start_degree_ = value;
      break;
    case SETTING_NUMBER_START_OFFSET_MINUTES:
      this->start_offset_minutes_ = static_cast<int>(value);
      break;
    case SETTING_NUMBER_END_DEGREE:
      this->end_degree_ = value;
      break;
    case SETTING_NUMBER_END_OFFSET_MINUTES:
      this->end_offset_minutes_ = static_cast<int>(value);
      break;
    case SETTING_NUMBER_EARLY_TAKE_IN_HOUR:
      this->has_early_take_in_ = true;
      this->has_early_take_in_time_ = true;
      this->early_take_in_hour_ = static_cast<int>(value);
      break;
    case SETTING_NUMBER_EARLY_TAKE_IN_MINUTE:
      this->has_early_take_in_ = true;
      this->has_early_take_in_time_ = true;
      this->early_take_in_minute_ = static_cast<int>(value);
      break;
    case SETTING_NUMBER_EARLY_TAKE_IN_OFFSET_MINUTES:
      this->has_early_take_in_ = true;
      this->early_take_in_offset_minutes_ = static_cast<int>(value);
      break;
    case SETTING_NUMBER_EARLY_TAKE_IN_FROM_MONTH:
      this->has_early_take_in_ = true;
      this->has_early_take_in_from_ = true;
      this->early_take_in_from_month_ = static_cast<int>(value);
      break;
    case SETTING_NUMBER_EARLY_TAKE_IN_FROM_DAY:
      this->has_early_take_in_ = true;
      this->has_early_take_in_from_ = true;
      this->early_take_in_from_day_ = static_cast<int>(value);
      break;
    case SETTING_NUMBER_EARLY_TAKE_IN_TO_MONTH:
      this->has_early_take_in_ = true;
      this->has_early_take_in_to_ = true;
      this->early_take_in_to_month_ = static_cast<int>(value);
      break;
    case SETTING_NUMBER_EARLY_TAKE_IN_TO_DAY:
      this->has_early_take_in_ = true;
      this->has_early_take_in_to_ = true;
      this->early_take_in_to_day_ = static_cast<int>(value);
      break;
  }
}

bool ShabbosModeBinarySensor::get_setting_switch_value(SettingSwitchType type) const {
  switch (type) {
    case SETTING_SWITCH_IN_ISRAEL:
      return this->in_israel_;
    case SETTING_SWITCH_EARLY_TAKE_IN_ENABLED:
      return this->early_take_in_enabled_;
    case SETTING_SWITCH_EARLY_TAKE_IN_APPLIES_TO_YOM_TOV:
      return this->early_take_in_for_yom_tov_;
  }
  return false;
}

void ShabbosModeBinarySensor::set_setting_switch_value(SettingSwitchType type, bool value) {
  switch (type) {
    case SETTING_SWITCH_IN_ISRAEL:
      this->in_israel_ = value;
      break;
    case SETTING_SWITCH_EARLY_TAKE_IN_ENABLED:
      this->has_early_take_in_ = true;
      this->early_take_in_enabled_ = value;
      break;
    case SETTING_SWITCH_EARLY_TAKE_IN_APPLIES_TO_YOM_TOV:
      this->has_early_take_in_ = true;
      this->early_take_in_for_yom_tov_ = value;
      break;
  }
}

std::string ShabbosModeBinarySensor::get_plag_opinion_name() const {
  switch (this->early_take_in_plag_opinion_) {
    case PLAG_OPINION_GRA:
      return "gra";
    case PLAG_OPINION_MGA:
      return "mga";
    case PLAG_OPINION_BAAL_HATANYA:
    default:
      return "baal_hatanya";
  }
}

bool ShabbosModeBinarySensor::is_valid_event_(const hdate &date) const { return date.year != 0; }

}  // namespace shabbos_mode
}  // namespace esphome
