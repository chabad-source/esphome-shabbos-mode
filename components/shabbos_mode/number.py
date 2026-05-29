import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ENTITY_CATEGORY, CONF_ID, ENTITY_CATEGORY_CONFIG

from .binary_sensor import ShabbosModeBinarySensor, shabbos_mode_ns

CONF_SHABBOS_MODE_ID = "shabbos_mode_id"
CONF_TYPE = "type"

ShabbosModeSettingNumber = shabbos_mode_ns.class_("ShabbosModeSettingNumber", number.Number, cg.Component)
SettingNumberType = shabbos_mode_ns.enum("SettingNumberType")

NUMBER_TYPES = {
    "latitude": ("SETTING_NUMBER_LATITUDE", -90.0, 90.0, 0.0001),
    "longitude": ("SETTING_NUMBER_LONGITUDE", -180.0, 180.0, 0.0001),
    "elevation": ("SETTING_NUMBER_ELEVATION", -500.0, 10000.0, 1.0),
    "start_degree": ("SETTING_NUMBER_START_DEGREE", 0.0, 30.0, 0.1),
    "start_offset_minutes": ("SETTING_NUMBER_START_OFFSET_MINUTES", -999.0, 999.0, 1.0),
    "end_degree": ("SETTING_NUMBER_END_DEGREE", 0.0, 30.0, 0.1),
    "end_offset_minutes": ("SETTING_NUMBER_END_OFFSET_MINUTES", -999.0, 999.0, 1.0),
    "early_take_in_hour": ("SETTING_NUMBER_EARLY_TAKE_IN_HOUR", 0.0, 23.0, 1.0),
    "early_take_in_minute": ("SETTING_NUMBER_EARLY_TAKE_IN_MINUTE", 0.0, 59.0, 1.0),
    "early_take_in_offset_minutes": ("SETTING_NUMBER_EARLY_TAKE_IN_OFFSET_MINUTES", -999.0, 999.0, 1.0),
    "early_take_in_from_month": ("SETTING_NUMBER_EARLY_TAKE_IN_FROM_MONTH", 1.0, 12.0, 1.0),
    "early_take_in_from_day": ("SETTING_NUMBER_EARLY_TAKE_IN_FROM_DAY", 1.0, 31.0, 1.0),
    "early_take_in_to_month": ("SETTING_NUMBER_EARLY_TAKE_IN_TO_MONTH", 1.0, 12.0, 1.0),
    "early_take_in_to_day": ("SETTING_NUMBER_EARLY_TAKE_IN_TO_DAY", 1.0, 31.0, 1.0),
}

CONFIG_SCHEMA = number.number_schema(
    ShabbosModeSettingNumber,
    entity_category=ENTITY_CATEGORY_CONFIG,
).extend(
    {
        cv.Required(CONF_SHABBOS_MODE_ID): cv.use_id(ShabbosModeBinarySensor),
        cv.Required(CONF_TYPE): cv.one_of(*NUMBER_TYPES, lower=True),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    enum_name, min_value, max_value, step = NUMBER_TYPES[config[CONF_TYPE]]
    var = cg.new_Pvariable(config[CONF_ID])
    await number.register_number(var, config, min_value=min_value, max_value=max_value, step=step)
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_SHABBOS_MODE_ID])
    cg.add(var.set_setting_type(getattr(SettingNumberType, enum_name)))
