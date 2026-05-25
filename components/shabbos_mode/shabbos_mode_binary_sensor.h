#pragma once

#include <string>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/component.h"

extern "C" {
#include "hebrewcalendar.h"
#include "NOAAcalculator.h"
}

namespace esphome {
namespace shabbos_mode {

enum PlagOpinion {
  PLAG_OPINION_BAAL_HATANYA = 0,
  PLAG_OPINION_GRA = 1,
  PLAG_OPINION_MGA = 2,
};

enum SettingNumberType {
  SETTING_NUMBER_LATITUDE = 0,
  SETTING_NUMBER_LONGITUDE = 1,
  SETTING_NUMBER_ELEVATION = 2,
  SETTING_NUMBER_START_DEGREE = 3,
  SETTING_NUMBER_START_OFFSET_MINUTES = 4,
  SETTING_NUMBER_END_DEGREE = 5,
  SETTING_NUMBER_END_OFFSET_MINUTES = 6,
  SETTING_NUMBER_EARLY_TAKE_IN_HOUR = 7,
  SETTING_NUMBER_EARLY_TAKE_IN_MINUTE = 8,
  SETTING_NUMBER_EARLY_TAKE_IN_OFFSET_MINUTES = 9,
  SETTING_NUMBER_EARLY_TAKE_IN_FROM_MONTH = 10,
  SETTING_NUMBER_EARLY_TAKE_IN_FROM_DAY = 11,
  SETTING_NUMBER_EARLY_TAKE_IN_TO_MONTH = 12,
  SETTING_NUMBER_EARLY_TAKE_IN_TO_DAY = 13,
};

enum SettingSwitchType {
  SETTING_SWITCH_IN_ISRAEL = 0,
  SETTING_SWITCH_EARLY_TAKE_IN_ENABLED = 1,
  SETTING_SWITCH_EARLY_TAKE_IN_APPLIES_TO_YOM_TOV = 2,
};

enum SettingTextSensorType {
  SETTING_TEXT_SENSOR_NEXT_TURN_ON = 0,
  SETTING_TEXT_SENSOR_NEXT_TURN_OFF = 1,
  SETTING_TEXT_SENSOR_CURRENT_HEBREW_DATE = 2,
};

class ShabbosModeBinarySensor : public binary_sensor::BinarySensor, public PollingComponent {
 public:
  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_latitude(double latitude) { this->latitude_ = latitude; }
  void set_longitude(double longitude) { this->longitude_ = longitude; }
  void set_elevation(double elevation) { this->elevation_ = elevation; }
  void set_in_israel(bool in_israel) { this->in_israel_ = in_israel; }
  void set_start_degree(double start_degree) { this->start_degree_ = start_degree; }
  void set_start_offset_minutes(int start_offset_minutes) { this->start_offset_minutes_ = start_offset_minutes; }
  void set_end_degree(double end_degree) { this->end_degree_ = end_degree; }
  void set_end_offset_minutes(int end_offset_minutes) { this->end_offset_minutes_ = end_offset_minutes; }
  void set_early_take_in_time(int hour, int minute) {
    this->has_early_take_in_ = true;
    this->early_take_in_enabled_ = true;
    this->has_early_take_in_time_ = true;
    this->early_take_in_hour_ = hour;
    this->early_take_in_minute_ = minute;
  }
  void set_early_take_in_offset_minutes(int early_take_in_offset_minutes) {
    this->has_early_take_in_ = true;
    this->early_take_in_enabled_ = true;
    this->early_take_in_offset_minutes_ = early_take_in_offset_minutes;
  }
  void set_early_take_in_for_yom_tov(bool early_take_in_for_yom_tov) {
    this->has_early_take_in_ = true;
    this->early_take_in_for_yom_tov_ = early_take_in_for_yom_tov;
  }
  void set_early_take_in_plag_opinion(const std::string &plag_opinion);
  void set_early_take_in_enabled(bool early_take_in_enabled) {
    this->has_early_take_in_ = true;
    this->early_take_in_enabled_ = early_take_in_enabled;
  }
  void set_early_take_in_from(int month, int day) {
    this->has_early_take_in_ = true;
    this->early_take_in_enabled_ = true;
    this->has_early_take_in_from_ = true;
    this->early_take_in_from_month_ = month;
    this->early_take_in_from_day_ = day;
  }
  void set_early_take_in_to(int month, int day) {
    this->has_early_take_in_ = true;
    this->early_take_in_enabled_ = true;
    this->has_early_take_in_to_ = true;
    this->early_take_in_to_month_ = month;
    this->early_take_in_to_day_ = day;
  }
  float get_setting_number_value(SettingNumberType type) const;
  void set_setting_number_value(SettingNumberType type, float value);
  bool get_setting_switch_value(SettingSwitchType type) const;
  void set_setting_switch_value(SettingSwitchType type, bool value);
  std::string get_plag_opinion_name() const;
  std::string get_next_turn_on_text() const;
  std::string get_next_turn_off_text() const;
  std::string get_current_hebrew_date_text() const;

  void setup() override;
  void update() override;
  void dump_config() override;

 protected:
  bool compute_active_(const ESPTime &now) const;
  hdate calculate_date_event_(hdate date, double degree, int offset_minutes) const;
  hdate calculate_start_event_(hdate date, int current_month, int current_day) const;
  hdate calculate_end_event_(hdate date) const;
  hdate calculate_early_take_in_event_(hdate date) const;
  hdate calculate_plag_event_(hdate date) const;
  long calculate_shaah_zmanis_(hdate startday, hdate endday) const;
  hdate calculate_next_transition_(const ESPTime &now, bool want_turn_on) const;
  hdate get_date_from_utc_time_(hdate current, double time, bool is_sunrise) const;
  std::string format_hdate_(const hdate &date) const;
  std::string format_hebrew_date_(const hdate &date) const;
  int get_antimeridian_adjustment_(hdate current) const;
  long get_local_mean_time_offset_(hdate current) const;
  bool should_apply_early_take_in_(hdate date, int current_month, int current_day) const;
  bool is_in_early_take_in_range_(int current_month, int current_day) const;
  int month_day_to_ordinal_(int month, int day) const;
  bool is_valid_event_(const hdate &date) const;

  time::RealTimeClock *time_{nullptr};
  double latitude_{0.0};
  double longitude_{0.0};
  double elevation_{0.0};
  bool in_israel_{false};
  double start_degree_{0.0};
  int start_offset_minutes_{-18};
  double end_degree_{8.5};
  int end_offset_minutes_{0};
  bool has_early_take_in_{false};
  bool has_early_take_in_time_{false};
  int early_take_in_hour_{0};
  int early_take_in_minute_{0};
  int early_take_in_offset_minutes_{0};
  bool early_take_in_enabled_{false};
  bool early_take_in_for_yom_tov_{false};
  PlagOpinion early_take_in_plag_opinion_{PLAG_OPINION_BAAL_HATANYA};
  bool has_early_take_in_from_{false};
  int early_take_in_from_month_{1};
  int early_take_in_from_day_{1};
  bool has_early_take_in_to_{false};
  int early_take_in_to_month_{12};
  int early_take_in_to_day_{31};
};

}  // namespace shabbos_mode
}  // namespace esphome
