import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID, ENTITY_CATEGORY_CONFIG

from .binary_sensor import ShabbosModeBinarySensor, shabbos_mode_ns

CONF_SHABBOS_MODE_ID = "shabbos_mode_id"
CONF_TYPE = "type"

ShabbosModeSettingSwitch = shabbos_mode_ns.class_("ShabbosModeSettingSwitch", switch.Switch, cg.Component)
SettingSwitchType = shabbos_mode_ns.enum("SettingSwitchType")

SWITCH_TYPES = {
    "in_israel": "SETTING_SWITCH_IN_ISRAEL",
    "early_take_in_enabled": "SETTING_SWITCH_EARLY_TAKE_IN_ENABLED",
    "early_take_in_applies_to_yom_tov": "SETTING_SWITCH_EARLY_TAKE_IN_APPLIES_TO_YOM_TOV",
}

CONFIG_SCHEMA = switch.switch_schema(
    ShabbosModeSettingSwitch,
    entity_category=ENTITY_CATEGORY_CONFIG,
).extend(
    {
        cv.Required(CONF_SHABBOS_MODE_ID): cv.use_id(ShabbosModeBinarySensor),
        cv.Required(CONF_TYPE): cv.one_of(*SWITCH_TYPES, lower=True),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await switch.register_switch(var, config)
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_SHABBOS_MODE_ID])
    cg.add(var.set_setting_type(getattr(SettingSwitchType, SWITCH_TYPES[config[CONF_TYPE]])))
