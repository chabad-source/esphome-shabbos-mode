#pragma once

#include <string>
#include <vector>

#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text/text.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include "shabbos_mode_binary_sensor.h"

namespace esphome {
namespace shabbos_mode {

class ShabbosModeSettingNumber : public number::Number, public Component, public Parented<ShabbosModeBinarySensor> {
 public:
  void set_setting_type(SettingNumberType setting_type) { this->setting_type_ = setting_type; }

  void setup() override;
  void dump_config() override;

 protected:
  void control(float value) override;

  SettingNumberType setting_type_{SETTING_NUMBER_LATITUDE};
};

class ShabbosModeSettingSwitch : public switch_::Switch, public Component, public Parented<ShabbosModeBinarySensor> {
 public:
  void set_setting_type(SettingSwitchType setting_type) { this->setting_type_ = setting_type; }

  void setup() override;
  void dump_config() override;

 protected:
  void write_state(bool state) override;

  SettingSwitchType setting_type_{SETTING_SWITCH_IN_ISRAEL};
};

class ShabbosModePlagOpinionSelect : public select::Select, public Component, public Parented<ShabbosModeBinarySensor> {
 public:
  void setup() override;
  void dump_config() override;

 protected:
  void control(const std::string &value) override;
};

class ShabbosModeSettingText : public text::Text, public Component, public Parented<ShabbosModeBinarySensor> {
 public:
  void set_setting_type(SettingTextType setting_type) { this->setting_type_ = setting_type; }

  void setup() override;
  void dump_config() override;

 protected:
  void control(const std::string &value) override;

  SettingTextType setting_type_{SETTING_TEXT_LOCATION};
};

class ShabbosModeEventTextSensor
    : public text_sensor::TextSensor,
      public PollingComponent,
      public Parented<ShabbosModeBinarySensor> {
 public:
  void set_setting_type(SettingTextSensorType setting_type) { this->setting_type_ = setting_type; }

  void setup() override;
  void update() override;
  void dump_config() override;

 protected:
  SettingTextSensorType setting_type_{SETTING_TEXT_SENSOR_NEXT_TURN_ON};
};

}  // namespace shabbos_mode
}  // namespace esphome
