#include "shabbos_mode_controls.h"

#include <string>
#include <vector>

#include "esphome/core/log.h"

namespace esphome {
namespace shabbos_mode {

static const char *const TAG = "shabbos_mode.controls";

void ShabbosModeSettingNumber::setup() { this->publish_state(this->parent_->get_setting_number_value(this->setting_type_)); }

void ShabbosModeSettingNumber::dump_config() { LOG_NUMBER("", "Shabbos Mode Setting Number", this); }

void ShabbosModeSettingNumber::control(float value) {
  this->parent_->set_setting_number_value(this->setting_type_, value);
  this->publish_state(this->parent_->get_setting_number_value(this->setting_type_));
}

void ShabbosModeSettingSwitch::setup() { this->publish_state(this->parent_->get_setting_switch_value(this->setting_type_)); }

void ShabbosModeSettingSwitch::dump_config() { LOG_SWITCH("", "Shabbos Mode Setting Switch", this); }

void ShabbosModeSettingSwitch::write_state(bool state) {
  this->parent_->set_setting_switch_value(this->setting_type_, state);
  this->publish_state(this->parent_->get_setting_switch_value(this->setting_type_));
}

void ShabbosModePlagOpinionSelect::setup() {
  this->traits.set_options({"Baal HaTanya", "Gra", "MGA"});
  this->publish_state(this->parent_->get_plag_opinion_name());
}

void ShabbosModePlagOpinionSelect::dump_config() { LOG_SELECT("", "Shabbos Mode Plag Opinion Select", this); }

void ShabbosModePlagOpinionSelect::control(const std::string &value) {
  this->parent_->set_early_take_in_plag_opinion(value);
  this->publish_state(this->parent_->get_plag_opinion_name());
}

void ShabbosModeSettingText::setup() { this->publish_state(this->parent_->get_setting_text_value(this->setting_type_)); }

void ShabbosModeSettingText::dump_config() { LOG_TEXT("", "Shabbos Mode Setting Text", this); }

void ShabbosModeSettingText::control(const std::string &value) {
  if (!this->parent_->set_setting_text_value(this->setting_type_, value)) {
    this->publish_state(this->parent_->get_setting_text_value(this->setting_type_));
    return;
  }
  this->publish_state(this->parent_->get_setting_text_value(this->setting_type_));
}

void ShabbosModeEventTextSensor::setup() { this->update(); }

void ShabbosModeEventTextSensor::update() {
  if (this->setting_type_ == SETTING_TEXT_SENSOR_NEXT_TURN_OFF) {
    this->publish_state(this->parent_->get_next_turn_off_text());
    return;
  }
  if (this->setting_type_ == SETTING_TEXT_SENSOR_CURRENT_HEBREW_DATE) {
    this->publish_state(this->parent_->get_current_hebrew_date_text());
    return;
  }
  this->publish_state(this->parent_->get_next_turn_on_text());
}

void ShabbosModeEventTextSensor::dump_config() { LOG_TEXT_SENSOR("", "Shabbos Mode Event Text Sensor", this); }

}  // namespace shabbos_mode
}  // namespace esphome
