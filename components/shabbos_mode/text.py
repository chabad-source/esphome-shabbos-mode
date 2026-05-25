import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text
from esphome.const import CONF_ID, ENTITY_CATEGORY_CONFIG

from .binary_sensor import ShabbosModeBinarySensor, shabbos_mode_ns

CONF_SHABBOS_MODE_ID = "shabbos_mode_id"
CONF_TYPE = "type"

ShabbosModeSettingText = shabbos_mode_ns.class_("ShabbosModeSettingText", text.Text, cg.Component)
SettingTextType = shabbos_mode_ns.enum("SettingTextType")

TEXT_TYPES = {
    "location": "SETTING_TEXT_LOCATION",
    "early_take_in_time": "SETTING_TEXT_EARLY_TAKE_IN_TIME",
    "early_take_in_range": "SETTING_TEXT_EARLY_TAKE_IN_RANGE",
}

CONFIG_SCHEMA = text.text_schema(
    ShabbosModeSettingText,
    entity_category=ENTITY_CATEGORY_CONFIG,
).extend(
    {
        cv.Required(CONF_SHABBOS_MODE_ID): cv.use_id(ShabbosModeBinarySensor),
        cv.Required(CONF_TYPE): cv.one_of(*TEXT_TYPES, lower=True),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await text.register_text(var, config)
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_SHABBOS_MODE_ID])
    cg.add(var.set_setting_type(getattr(SettingTextType, TEXT_TYPES[config[CONF_TYPE]])))
