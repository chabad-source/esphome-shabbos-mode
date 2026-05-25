#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/component.h"

extern "C" {
#include "hebrewcalendar.h"
#include "NOAAcalculator.h"
}

namespace esphome {
namespace shabbos_mode {

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

  void setup() override;
  void update() override;
  void dump_config() override;

 protected:
  bool compute_active_(const ESPTime &now) const;
  hdate calculate_date_event_(hdate date, double degree, int offset_minutes) const;
  hdate calculate_start_event_(hdate date) const;
  hdate calculate_end_event_(hdate date) const;
  hdate get_date_from_utc_time_(hdate current, double time, bool is_sunrise) const;
  int get_antimeridian_adjustment_(hdate current) const;
  long get_local_mean_time_offset_(hdate current) const;
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
};

}  // namespace shabbos_mode
}  // namespace esphome
