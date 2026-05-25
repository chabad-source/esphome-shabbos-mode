#include "shabbos_mode_controls.h"

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
  this->traits.set_options({"baal_hatanya", "gra", "mga"});
  this->publish_state(this->parent_->get_plag_opinion_name());
}

void ShabbosModePlagOpinionSelect::dump_config() { LOG_SELECT("", "Shabbos Mode Plag Opinion Select", this); }

void ShabbosModePlagOpinionSelect::control(const std::string &value) {
  this->parent_->set_early_take_in_plag_opinion(value);
  this->publish_state(this->parent_->get_plag_opinion_name());
}

}  // namespace shabbos_mode
}  // namespace esphome
