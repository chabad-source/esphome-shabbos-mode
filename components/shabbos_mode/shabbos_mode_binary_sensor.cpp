#include "shabbos_mode_binary_sensor.h"
#include "shabbos_mode_controls.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <sstream>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace shabbos_mode {

static const char *const TAG = "shabbos_mode.binary_sensor";
static const double GEOMETRIC_ZENITH = 90.0;
static const uint32_t SETTINGS_SAVE_DEBOUNCE_MS = 1000;

void ShabbosModeBinarySensor::setup() {
  if (this->time_ == nullptr) {
    ESP_LOGW(TAG, "No time source configured");
    return;
  }

  this->settings_pref_ = this->make_entity_preference<RuntimeSettings>(0x534d0101);
  this->load_runtime_settings_();

  auto now = this->time_->now();
  if (!now.is_valid()) {
    ESP_LOGD(TAG, "Time is not valid yet, waiting for the first sync");
    return;
  }

  this->publish_state(this->compute_active_(now));
}

void ShabbosModeBinarySensor::loop() {
  if (this->settings_dirty_ && millis() - this->settings_dirty_at_ >= SETTINGS_SAVE_DEBOUNCE_MS) {
    this->save_runtime_settings_();
  }
}

void ShabbosModeBinarySensor::update() {
  if (this->settings_dirty_) {
    this->save_runtime_settings_();
  }

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
  return this->compute_active_(current);
}

bool ShabbosModeBinarySensor::compute_active_(hdate current) const {
  struct tm current_tm = hdategregorian(current);
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

  int from_month = this->has_early_take_in_from_ ? this->early_take_in_from_month_ : 1;
  int from_day = this->has_early_take_in_from_ ? this->early_take_in_from_day_ : 1;
  int to_month = this->has_early_take_in_to_ ? this->early_take_in_to_month_ : 12;
  int to_day = this->has_early_take_in_to_ ? this->early_take_in_to_day_ : 31;

  bool current_after_from = this->is_month_day_before_or_equal_(from_month, from_day, current_month, current_day);
  bool current_before_to = this->is_month_day_before_or_equal_(current_month, current_day, to_month, to_day);
  if (this->is_month_day_before_or_equal_(from_month, from_day, to_month, to_day)) {
    return current_after_from && current_before_to;
  }
  return current_after_from || current_before_to;
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

hdate ShabbosModeBinarySensor::calculate_next_transition_(const ESPTime &now, bool want_turn_on) const {
  auto now_copy = now;
  struct tm base_tm = now_copy.to_c_tm();
  hdate current = convertDate(base_tm);
  current.offset = ESPTime::timezone_offset();
  setEY(&current, this->in_israel_);
  bool tracked_active = this->compute_active_(current);

  for (int offset_days = 0; offset_days < 370; offset_days++) {
    struct tm search_tm = base_tm;
    search_tm.tm_mday += offset_days;
    if (mktime(&search_tm) == -1) {
      continue;
    }

    hdate search_date = convertDate(search_tm);
    search_date.offset = current.offset;
    setEY(&search_date, this->in_israel_);

    hdate candidates[2] = {{0}, {0}};
    int candidate_count = 0;
    int candlelighting = iscandlelighting(search_date);
    if (candlelighting == 1 || candlelighting == 2) {
      candidates[candidate_count++] = this->calculate_start_event_(search_date, search_tm.tm_mon + 1, search_tm.tm_mday);
    }
    if (isassurbemelachah(search_date)) {
      candidates[candidate_count++] = this->calculate_end_event_(search_date);
    }
    if (candidate_count == 2 && hdatecompare(candidates[0], candidates[1]) == -1) {
      std::swap(candidates[0], candidates[1]);
    }

    for (int i = 0; i < candidate_count; i++) {
      hdate event = candidates[i];
      if (!this->is_valid_event_(event) || hdatecompare(current, event) != 1) {
        continue;
      }

      hdate after = event;
      hdateaddsecond(&after, 1);
      bool active_after = this->compute_active_(after);
      if (active_after == tracked_active) {
        continue;
      }

      tracked_active = active_after;
      if (tracked_active == want_turn_on) {
        return event;
      }
    }
  }

  return (hdate) {0};
}

void ShabbosModeBinarySensor::set_latitude(double latitude) {
  this->latitude_ = this->clamp_double_(latitude, -90.0, 90.0);
}

void ShabbosModeBinarySensor::set_longitude(double longitude) {
  this->longitude_ = this->clamp_double_(longitude, -180.0, 180.0);
}

void ShabbosModeBinarySensor::set_elevation(double elevation) {
  this->elevation_ = this->clamp_double_(elevation, -500.0, 10000.0);
}

void ShabbosModeBinarySensor::set_start_degree(double start_degree) {
  this->start_degree_ = this->clamp_double_(start_degree, 0.0, 30.0);
}

void ShabbosModeBinarySensor::set_start_offset_minutes(int start_offset_minutes) {
  this->start_offset_minutes_ = this->clamp_int_(start_offset_minutes, -999, 999);
}

void ShabbosModeBinarySensor::set_end_degree(double end_degree) {
  this->end_degree_ = this->clamp_double_(end_degree, 0.0, 30.0);
}

void ShabbosModeBinarySensor::set_end_offset_minutes(int end_offset_minutes) {
  this->end_offset_minutes_ = this->clamp_int_(end_offset_minutes, -999, 999);
}

void ShabbosModeBinarySensor::set_early_take_in_time(int hour, int minute) {
  this->has_early_take_in_ = true;
  this->early_take_in_enabled_ = true;
  this->has_early_take_in_time_ = true;
  this->early_take_in_hour_ = this->clamp_int_(hour, 0, 23);
  this->early_take_in_minute_ = this->clamp_int_(minute, 0, 59);
}

void ShabbosModeBinarySensor::set_early_take_in_offset_minutes(int early_take_in_offset_minutes) {
  this->has_early_take_in_ = true;
  this->early_take_in_enabled_ = true;
  this->early_take_in_offset_minutes_ = this->clamp_int_(early_take_in_offset_minutes, -999, 999);
}

void ShabbosModeBinarySensor::set_early_take_in_from(int month, int day) {
  this->has_early_take_in_ = true;
  this->early_take_in_enabled_ = true;
  this->has_early_take_in_from_ = true;
  this->early_take_in_from_month_ = this->clamp_int_(month, 1, 12);
  this->early_take_in_from_day_ =
      this->clamp_day_for_month_(this->early_take_in_from_month_, this->clamp_int_(day, 1, 31));
}

void ShabbosModeBinarySensor::set_early_take_in_to(int month, int day) {
  this->has_early_take_in_ = true;
  this->early_take_in_enabled_ = true;
  this->has_early_take_in_to_ = true;
  this->early_take_in_to_month_ = this->clamp_int_(month, 1, 12);
  this->early_take_in_to_day_ =
      this->clamp_day_for_month_(this->early_take_in_to_month_, this->clamp_int_(day, 1, 31));
}

void ShabbosModeBinarySensor::set_early_take_in_plag_opinion(const std::string &plag_opinion) {
  this->has_early_take_in_ = true;
  this->early_take_in_enabled_ = true;
  std::string normalized = this->normalize_plag_opinion_(plag_opinion);
  if (normalized == "gra") {
    this->early_take_in_plag_opinion_ = PLAG_OPINION_GRA;
  } else if (normalized == "mga") {
    this->early_take_in_plag_opinion_ = PLAG_OPINION_MGA;
  } else {
    this->early_take_in_plag_opinion_ = PLAG_OPINION_BAAL_HATANYA;
  }
  this->request_runtime_settings_save_();
  this->notify_runtime_settings_changed_();
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
      this->set_latitude(value);
      break;
    case SETTING_NUMBER_LONGITUDE:
      this->set_longitude(value);
      break;
    case SETTING_NUMBER_ELEVATION:
      this->set_elevation(value);
      break;
    case SETTING_NUMBER_START_DEGREE:
      this->set_start_degree(value);
      break;
    case SETTING_NUMBER_START_OFFSET_MINUTES:
      this->set_start_offset_minutes(static_cast<int>(value));
      break;
    case SETTING_NUMBER_END_DEGREE:
      this->set_end_degree(value);
      break;
    case SETTING_NUMBER_END_OFFSET_MINUTES:
      this->set_end_offset_minutes(static_cast<int>(value));
      break;
    case SETTING_NUMBER_EARLY_TAKE_IN_HOUR:
      this->set_early_take_in_time(static_cast<int>(value), this->early_take_in_minute_);
      break;
    case SETTING_NUMBER_EARLY_TAKE_IN_MINUTE:
      this->set_early_take_in_time(this->early_take_in_hour_, static_cast<int>(value));
      break;
    case SETTING_NUMBER_EARLY_TAKE_IN_OFFSET_MINUTES:
      this->set_early_take_in_offset_minutes(static_cast<int>(value));
      break;
    case SETTING_NUMBER_EARLY_TAKE_IN_FROM_MONTH:
      this->set_early_take_in_from(static_cast<int>(value), this->early_take_in_from_day_);
      break;
    case SETTING_NUMBER_EARLY_TAKE_IN_FROM_DAY:
      this->set_early_take_in_from(this->early_take_in_from_month_, static_cast<int>(value));
      break;
    case SETTING_NUMBER_EARLY_TAKE_IN_TO_MONTH:
      this->set_early_take_in_to(static_cast<int>(value), this->early_take_in_to_day_);
      break;
    case SETTING_NUMBER_EARLY_TAKE_IN_TO_DAY:
      this->set_early_take_in_to(this->early_take_in_to_month_, static_cast<int>(value));
      break;
  }
  this->request_runtime_settings_save_();
  this->notify_runtime_settings_changed_();
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
  this->request_runtime_settings_save_();
  this->notify_runtime_settings_changed_();
}

std::string ShabbosModeBinarySensor::get_setting_text_value(SettingTextType type) const {
  char buffer[48];
  switch (type) {
    case SETTING_TEXT_LOCATION:
      snprintf(buffer, sizeof(buffer), "%.5f,%.5f", this->latitude_, this->longitude_);
      return std::string(buffer);
    case SETTING_TEXT_EARLY_TAKE_IN_TIME:
      if (!this->has_early_take_in_time_) {
        return "";
      }
      snprintf(buffer, sizeof(buffer), "%02d:%02d", this->early_take_in_hour_, this->early_take_in_minute_);
      return std::string(buffer);
    case SETTING_TEXT_EARLY_TAKE_IN_RANGE:
      if (!this->has_early_take_in_from_ && !this->has_early_take_in_to_) {
        return "";
      }
      snprintf(buffer, sizeof(buffer), "%d/%d-%d/%d", this->early_take_in_from_month_, this->early_take_in_from_day_,
               this->early_take_in_to_month_, this->early_take_in_to_day_);
      return std::string(buffer);
  }
  return "";
}

bool ShabbosModeBinarySensor::set_setting_text_value(SettingTextType type, const std::string &value) {
  std::string trimmed = this->trim_(value);
  switch (type) {
    case SETTING_TEXT_LOCATION: {
      double latitude = 0.0;
      double longitude = 0.0;
      if (!this->parse_location_(trimmed, latitude, longitude)) {
        ESP_LOGW(TAG, "Invalid location '%s'. Expected 'latitude,longitude'", value.c_str());
        return false;
      }
      this->set_latitude(latitude);
      this->set_longitude(longitude);
      break;
    }
    case SETTING_TEXT_EARLY_TAKE_IN_TIME: {
      this->has_early_take_in_ = true;
      this->early_take_in_enabled_ = true;
      if (trimmed.empty()) {
        this->has_early_take_in_time_ = false;
        break;
      }
      int hour = 0;
      int minute = 0;
      if (!this->parse_time_of_day_(trimmed, hour, minute)) {
        ESP_LOGW(TAG, "Invalid early take-in time '%s'. Expected 'HH:MM' or blank", value.c_str());
        return false;
      }
      this->has_early_take_in_time_ = true;
      this->early_take_in_hour_ = hour;
      this->early_take_in_minute_ = minute;
      break;
    }
    case SETTING_TEXT_EARLY_TAKE_IN_RANGE: {
      this->has_early_take_in_ = true;
      this->early_take_in_enabled_ = true;
      if (trimmed.empty()) {
        this->has_early_take_in_from_ = false;
        this->has_early_take_in_to_ = false;
        break;
      }
      int from_month = 0;
      int from_day = 0;
      int to_month = 0;
      int to_day = 0;
      bool parsed = false;

      size_t dots_separator = trimmed.find("..");
      if (dots_separator != std::string::npos) {
        parsed = this->parse_month_day_(trimmed.substr(0, dots_separator), from_month, from_day) &&
                 this->parse_month_day_(trimmed.substr(dots_separator + 2), to_month, to_day);
      } else {
        size_t dash_separator = trimmed.find("-");
        if (dash_separator != std::string::npos) {
          parsed = this->parse_month_day_(trimmed.substr(0, dash_separator), from_month, from_day) &&
                   this->parse_month_day_(trimmed.substr(dash_separator + 1), to_month, to_day);
        }
      }

      if (!parsed) {
        ESP_LOGW(TAG, "Invalid early take-in range '%s'. Expected '5/1-9/15', '05-01..09-15', or blank",
                 value.c_str());
        return false;
      }
      this->has_early_take_in_from_ = true;
      this->has_early_take_in_to_ = true;
      this->early_take_in_from_month_ = from_month;
      this->early_take_in_from_day_ = from_day;
      this->early_take_in_to_month_ = to_month;
      this->early_take_in_to_day_ = to_day;
      break;
    }
  }

  this->request_runtime_settings_save_();
  this->notify_runtime_settings_changed_();
  return true;
}

std::string ShabbosModeBinarySensor::get_plag_opinion_name() const {
  switch (this->early_take_in_plag_opinion_) {
    case PLAG_OPINION_GRA:
      return "Gra";
    case PLAG_OPINION_MGA:
      return "MGA";
    case PLAG_OPINION_BAAL_HATANYA:
    default:
      return "Baal HaTanya";
  }
}

std::string ShabbosModeBinarySensor::get_next_turn_on_text() const {
  if (this->time_ == nullptr) {
    return "";
  }
  auto now = this->time_->now();
  if (!now.is_valid()) {
    return "";
  }
  return this->format_hdate_(this->calculate_next_transition_(now, true));
}

std::string ShabbosModeBinarySensor::get_next_turn_off_text() const {
  if (this->time_ == nullptr) {
    return "";
  }
  auto now = this->time_->now();
  if (!now.is_valid()) {
    return "";
  }
  return this->format_hdate_(this->calculate_next_transition_(now, false));
}

std::string ShabbosModeBinarySensor::get_current_hebrew_date_text() const {
  if (this->time_ == nullptr) {
    return "";
  }
  auto now = this->time_->now();
  if (!now.is_valid()) {
    return "";
  }
  auto now_copy = now;
  struct tm current_tm = now_copy.to_c_tm();
  hdate current = convertDate(current_tm);
  current.offset = ESPTime::timezone_offset();
  setEY(&current, this->in_israel_);
  return this->format_hebrew_date_(current);
}

std::string ShabbosModeBinarySensor::format_hdate_(const hdate &date) const {
  if (!this->is_valid_event_(date)) {
    return "";
  }
  time_t time = hdatetime_t(date);
  struct tm *local = localtime(&time);
  if (local == nullptr) {
    return "";
  }
  static const char *const weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char *const months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  int hour_24 = local->tm_hour;
  int hour_12 = hour_24 % 12;
  if (hour_12 == 0) {
    hour_12 = 12;
  }
  char buffer[48];
  snprintf(buffer, sizeof(buffer), "%s, %s %d, %d:%02d %s", weekdays[local->tm_wday], months[local->tm_mon],
           local->tm_mday, hour_12, local->tm_min, hour_24 >= 12 ? "PM" : "AM");
  return std::string(buffer);
}

std::string ShabbosModeBinarySensor::format_hebrew_date_(const hdate &date) const {
  if (!this->is_valid_event_(date)) {
    return "";
  }

  static const char *const regular_months[] = {"",       "Nissan", "Iyar",  "Sivan", "Tamuz", "Av",    "Elul",
                                               "Tishrei", "Cheshvan", "Kislev", "Teves", "Shevat", "Adar"};
  static const char *const leap_months[] = {"",       "Nissan", "Iyar",  "Sivan", "Tamuz", "Av",      "Elul",
                                            "Tishrei", "Cheshvan", "Kislev", "Teves", "Shevat", "Adar I", "Adar II"};

  const char *month_name = "";
  if (date.leap) {
    if (date.month >= 1 && date.month <= 13) {
      month_name = leap_months[date.month];
    }
  } else {
    if (date.month >= 1 && date.month <= 12) {
      month_name = regular_months[date.month];
    }
  }

  char buffer[48];
  snprintf(buffer, sizeof(buffer), "%d %s %d", date.day, month_name, date.year);
  return std::string(buffer);
}

std::string ShabbosModeBinarySensor::trim_(const std::string &value) const {
  auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
  auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();
  if (begin >= end) {
    return "";
  }
  return std::string(begin, end);
}

bool ShabbosModeBinarySensor::parse_location_(const std::string &value, double &latitude, double &longitude) const {
  size_t separator = value.find(",");
  if (separator == std::string::npos) {
    return false;
  }
  std::string latitude_part = this->trim_(value.substr(0, separator));
  std::string longitude_part = this->trim_(value.substr(separator + 1));
  if (latitude_part.empty() || longitude_part.empty()) {
    return false;
  }
  char *end_ptr = nullptr;
  latitude = std::strtod(latitude_part.c_str(), &end_ptr);
  if (end_ptr == latitude_part.c_str() || *end_ptr != '\0') {
    return false;
  }
  longitude = std::strtod(longitude_part.c_str(), &end_ptr);
  if (end_ptr == longitude_part.c_str() || *end_ptr != '\0') {
    return false;
  }
  return latitude >= -90.0 && latitude <= 90.0 && longitude >= -180.0 && longitude <= 180.0;
}

bool ShabbosModeBinarySensor::parse_time_of_day_(const std::string &value, int &hour, int &minute) const {
  size_t separator = value.find(":");
  if (separator == std::string::npos) {
    return false;
  }
  std::string hour_part = this->trim_(value.substr(0, separator));
  std::string minute_part = this->trim_(value.substr(separator + 1));
  if (hour_part.empty() || minute_part.empty()) {
    return false;
  }
  for (char c : hour_part) {
    if (!std::isdigit(static_cast<unsigned char>(c))) {
      return false;
    }
  }
  for (char c : minute_part) {
    if (!std::isdigit(static_cast<unsigned char>(c))) {
      return false;
    }
  }
  hour = std::atoi(hour_part.c_str());
  minute = std::atoi(minute_part.c_str());
  return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59;
}

bool ShabbosModeBinarySensor::parse_month_day_(const std::string &value, int &month, int &day) const {
  size_t separator = value.find("-");
  if (separator == std::string::npos) {
    separator = value.find("/");
  }
  if (separator == std::string::npos) {
    return false;
  }
  std::string month_part = this->trim_(value.substr(0, separator));
  std::string day_part = this->trim_(value.substr(separator + 1));
  if (month_part.empty() || day_part.empty()) {
    return false;
  }
  for (char c : month_part) {
    if (!std::isdigit(static_cast<unsigned char>(c))) {
      return false;
    }
  }
  for (char c : day_part) {
    if (!std::isdigit(static_cast<unsigned char>(c))) {
      return false;
    }
  }
  month = std::atoi(month_part.c_str());
  day = std::atoi(day_part.c_str());
  return this->is_valid_month_day_(month, day);
}

void ShabbosModeBinarySensor::load_runtime_settings_() {
  RuntimeSettings settings{};
  if (!this->settings_pref_.load(&settings)) {
    this->settings_loaded_ = true;
    return;
  }

  this->latitude_ = settings.latitude;
  this->longitude_ = settings.longitude;
  this->elevation_ = settings.elevation;
  this->in_israel_ = settings.in_israel;
  this->start_degree_ = settings.start_degree;
  this->start_offset_minutes_ = settings.start_offset_minutes;
  this->end_degree_ = settings.end_degree;
  this->end_offset_minutes_ = settings.end_offset_minutes;
  this->has_early_take_in_ = settings.has_early_take_in;
  this->has_early_take_in_time_ = settings.has_early_take_in_time;
  this->early_take_in_hour_ = settings.early_take_in_hour;
  this->early_take_in_minute_ = settings.early_take_in_minute;
  this->early_take_in_offset_minutes_ = settings.early_take_in_offset_minutes;
  this->early_take_in_enabled_ = settings.early_take_in_enabled;
  this->early_take_in_for_yom_tov_ = settings.early_take_in_for_yom_tov;
  this->early_take_in_plag_opinion_ = static_cast<PlagOpinion>(settings.early_take_in_plag_opinion);
  this->has_early_take_in_from_ = settings.has_early_take_in_from;
  this->early_take_in_from_month_ = settings.early_take_in_from_month;
  this->early_take_in_from_day_ = settings.early_take_in_from_day;
  this->has_early_take_in_to_ = settings.has_early_take_in_to;
  this->early_take_in_to_month_ = settings.early_take_in_to_month;
  this->early_take_in_to_day_ = settings.early_take_in_to_day;
  this->sanitize_runtime_settings_();
  this->settings_loaded_ = true;
}

void ShabbosModeBinarySensor::save_runtime_settings_() {
  if (!this->settings_loaded_) {
    return;
  }
  this->sanitize_runtime_settings_();

  RuntimeSettings settings{};
  settings.latitude = this->latitude_;
  settings.longitude = this->longitude_;
  settings.elevation = this->elevation_;
  settings.in_israel = this->in_israel_;
  settings.start_degree = this->start_degree_;
  settings.start_offset_minutes = this->start_offset_minutes_;
  settings.end_degree = this->end_degree_;
  settings.end_offset_minutes = this->end_offset_minutes_;
  settings.has_early_take_in = this->has_early_take_in_;
  settings.has_early_take_in_time = this->has_early_take_in_time_;
  settings.early_take_in_hour = this->early_take_in_hour_;
  settings.early_take_in_minute = this->early_take_in_minute_;
  settings.early_take_in_offset_minutes = this->early_take_in_offset_minutes_;
  settings.early_take_in_enabled = this->early_take_in_enabled_;
  settings.early_take_in_for_yom_tov = this->early_take_in_for_yom_tov_;
  settings.early_take_in_plag_opinion = static_cast<uint8_t>(this->early_take_in_plag_opinion_);
  settings.has_early_take_in_from = this->has_early_take_in_from_;
  settings.early_take_in_from_month = this->early_take_in_from_month_;
  settings.early_take_in_from_day = this->early_take_in_from_day_;
  settings.has_early_take_in_to = this->has_early_take_in_to_;
  settings.early_take_in_to_month = this->early_take_in_to_month_;
  settings.early_take_in_to_day = this->early_take_in_to_day_;
  this->settings_pref_.save(&settings);
  this->settings_dirty_ = false;
}

void ShabbosModeBinarySensor::request_runtime_settings_save_() {
  if (!this->settings_loaded_) {
    return;
  }
  this->settings_dirty_ = true;
  this->settings_dirty_at_ = millis();
}

void ShabbosModeBinarySensor::save_runtime_settings() { this->save_runtime_settings_(); }

void ShabbosModeBinarySensor::register_event_text_sensor(ShabbosModeEventTextSensor *sensor) {
  this->event_text_sensors_.push_back(sensor);
}

std::string ShabbosModeBinarySensor::normalize_plag_opinion_(const std::string &plag_opinion) const {
  std::string normalized;
  normalized.reserve(plag_opinion.size());
  for (char c : plag_opinion) {
    if (c == ' ' || c == '-') {
      normalized.push_back('_');
    } else {
      normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
  }
  return normalized;
}

void ShabbosModeBinarySensor::notify_runtime_settings_changed_() {
  if (this->time_ != nullptr) {
    auto now = this->time_->now();
    if (now.is_valid()) {
      this->publish_state(this->compute_active_(now));
    }
  }

  for (auto *sensor : this->event_text_sensors_) {
    if (sensor != nullptr) {
      sensor->update();
    }
  }
}

bool ShabbosModeBinarySensor::is_valid_event_(const hdate &date) const { return date.year != 0; }

bool ShabbosModeBinarySensor::is_month_day_before_or_equal_(int left_month, int left_day, int right_month,
                                                            int right_day) const {
  if (left_month != right_month) {
    return left_month < right_month;
  }
  return left_day <= right_day;
}

bool ShabbosModeBinarySensor::is_valid_month_day_(int month, int day) const {
  return month >= 1 && month <= 12 && day >= 1 && day <= this->max_day_for_month_(month);
}

int ShabbosModeBinarySensor::max_day_for_month_(int month) const {
  static const int days_by_month[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) {
    return 31;
  }
  return days_by_month[month - 1];
}

int ShabbosModeBinarySensor::clamp_day_for_month_(int month, int day) const {
  return this->clamp_int_(day, 1, this->max_day_for_month_(month));
}

int ShabbosModeBinarySensor::clamp_int_(int value, int min_value, int max_value) const {
  return std::min(std::max(value, min_value), max_value);
}

double ShabbosModeBinarySensor::clamp_double_(double value, double min_value, double max_value) const {
  return std::min(std::max(value, min_value), max_value);
}

void ShabbosModeBinarySensor::sanitize_runtime_settings_() {
  this->set_latitude(this->latitude_);
  this->set_longitude(this->longitude_);
  this->set_elevation(this->elevation_);
  this->set_start_degree(this->start_degree_);
  this->set_start_offset_minutes(this->start_offset_minutes_);
  this->set_end_degree(this->end_degree_);
  this->set_end_offset_minutes(this->end_offset_minutes_);
  this->early_take_in_hour_ = this->clamp_int_(this->early_take_in_hour_, 0, 23);
  this->early_take_in_minute_ = this->clamp_int_(this->early_take_in_minute_, 0, 59);
  this->early_take_in_enabled_ = this->has_early_take_in_ && this->early_take_in_enabled_;
  this->early_take_in_offset_minutes_ = this->clamp_int_(this->early_take_in_offset_minutes_, -999, 999);
  this->early_take_in_plag_opinion_ =
      this->early_take_in_plag_opinion_ == PLAG_OPINION_GRA || this->early_take_in_plag_opinion_ == PLAG_OPINION_MGA
          ? this->early_take_in_plag_opinion_
          : PLAG_OPINION_BAAL_HATANYA;
  this->early_take_in_from_month_ = this->clamp_int_(this->early_take_in_from_month_, 1, 12);
  this->early_take_in_from_day_ =
      this->clamp_day_for_month_(this->early_take_in_from_month_, this->early_take_in_from_day_);
  this->early_take_in_to_month_ = this->clamp_int_(this->early_take_in_to_month_, 1, 12);
  this->early_take_in_to_day_ = this->clamp_day_for_month_(this->early_take_in_to_month_, this->early_take_in_to_day_);
}

}  // namespace shabbos_mode
}  // namespace esphome
