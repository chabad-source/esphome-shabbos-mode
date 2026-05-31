#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

extern "C" {
#include "hebrewcalendar.h"
#include "NOAAcalculator.h"
}

namespace esphome {
namespace shabbos_mode {

class ShabbosModeEventTextSensor;

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

enum SettingTextType {
  SETTING_TEXT_LOCATION = 0,
  SETTING_TEXT_EARLY_TAKE_IN_TIME = 1,
  SETTING_TEXT_EARLY_TAKE_IN_RANGE = 2,
};

class ShabbosModeBinarySensor : public binary_sensor::BinarySensor, public PollingComponent {
 public:
  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_latitude(double latitude);
  void set_longitude(double longitude);
  void set_elevation(double elevation);
  void set_in_israel(bool in_israel) { this->in_israel_ = in_israel; }
  void set_start_degree(double start_degree);
  void set_start_offset_minutes(int start_offset_minutes);
  void set_end_degree(double end_degree);
  void set_end_offset_minutes(int end_offset_minutes);
  void set_early_take_in_time(int hour, int minute);
  void set_early_take_in_offset_minutes(int early_take_in_offset_minutes);
  void set_early_take_in_for_yom_tov(bool early_take_in_for_yom_tov) {
    this->has_early_take_in_ = true;
    this->early_take_in_for_yom_tov_ = early_take_in_for_yom_tov;
  }
  void set_early_take_in_plag_opinion(const std::string &plag_opinion);
  void set_early_take_in_enabled(bool early_take_in_enabled) {
    this->has_early_take_in_ = true;
    this->early_take_in_enabled_ = early_take_in_enabled;
  }
  void set_early_take_in_from(int month, int day);
  void set_early_take_in_to(int month, int day);
  float get_setting_number_value(SettingNumberType type) const;
  void set_setting_number_value(SettingNumberType type, float value);
  bool get_setting_switch_value(SettingSwitchType type) const;
  void set_setting_switch_value(SettingSwitchType type, bool value);
  std::string get_setting_text_value(SettingTextType type) const;
  bool set_setting_text_value(SettingTextType type, const std::string &value);
  std::string get_plag_opinion_name() const;
  std::string get_next_turn_on_text() const;
  std::string get_next_turn_off_text() const;
  std::string get_current_hebrew_date_text() const;
  void save_runtime_settings();
  void register_event_text_sensor(ShabbosModeEventTextSensor *sensor);

  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

 protected:
  bool compute_active_(const ESPTime &now) const;
  bool compute_active_(hdate current) const;
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
  std::string trim_(const std::string &value) const;
  bool parse_location_(const std::string &value, double &latitude, double &longitude) const;
  bool parse_time_of_day_(const std::string &value, int &hour, int &minute) const;
  bool parse_month_day_(const std::string &value, int &month, int &day) const;
  void load_runtime_settings_();
  void save_runtime_settings_();
  void request_runtime_settings_save_();
  void notify_runtime_settings_changed_();
  std::string normalize_plag_opinion_(const std::string &plag_opinion) const;
  int get_antimeridian_adjustment_(hdate current) const;
  long get_local_mean_time_offset_(hdate current) const;
  bool should_apply_early_take_in_(hdate date, int current_month, int current_day) const;
  bool is_in_early_take_in_range_(int current_month, int current_day) const;
  bool is_month_day_before_or_equal_(int left_month, int left_day, int right_month, int right_day) const;
  bool is_valid_month_day_(int month, int day) const;
  int max_day_for_month_(int month) const;
  int clamp_day_for_month_(int month, int day) const;
  int clamp_int_(int value, int min_value, int max_value) const;
  double clamp_double_(double value, double min_value, double max_value) const;
  void sanitize_runtime_settings_();
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
  bool settings_loaded_{false};
  struct RuntimeSettings {
    double latitude;
    double longitude;
    double elevation;
    bool in_israel;
    double start_degree;
    int start_offset_minutes;
    double end_degree;
    int end_offset_minutes;
    bool has_early_take_in;
    bool has_early_take_in_time;
    int early_take_in_hour;
    int early_take_in_minute;
    int early_take_in_offset_minutes;
    bool early_take_in_enabled;
    bool early_take_in_for_yom_tov;
    uint8_t early_take_in_plag_opinion;
    bool has_early_take_in_from;
    int early_take_in_from_month;
    int early_take_in_from_day;
    bool has_early_take_in_to;
    int early_take_in_to_month;
    int early_take_in_to_day;
  };
  ESPPreferenceObject settings_pref_;
  bool settings_dirty_{false};
  uint32_t settings_dirty_at_{0};
  std::vector<ShabbosModeEventTextSensor *> event_text_sensors_;
};

}  // namespace shabbos_mode
}  // namespace esphome
