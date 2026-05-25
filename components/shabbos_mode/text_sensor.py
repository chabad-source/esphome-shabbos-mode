import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, ENTITY_CATEGORY_DIAGNOSTIC

from .binary_sensor import ShabbosModeBinarySensor, shabbos_mode_ns

CONF_SHABBOS_MODE_ID = "shabbos_mode_id"
CONF_TYPE = "type"

ShabbosModeEventTextSensor = shabbos_mode_ns.class_(
    "ShabbosModeEventTextSensor", text_sensor.TextSensor, cg.PollingComponent
)
SettingTextSensorType = shabbos_mode_ns.enum("SettingTextSensorType")

TEXT_SENSOR_TYPES = {
    "next_turn_on": "SETTING_TEXT_SENSOR_NEXT_TURN_ON",
    "next_turn_off": "SETTING_TEXT_SENSOR_NEXT_TURN_OFF",
    "current_hebrew_date": "SETTING_TEXT_SENSOR_CURRENT_HEBREW_DATE",
}

CONFIG_SCHEMA = text_sensor.text_sensor_schema(
    ShabbosModeEventTextSensor,
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
).extend(
    {
        cv.Required(CONF_SHABBOS_MODE_ID): cv.use_id(ShabbosModeBinarySensor),
        cv.Required(CONF_TYPE): cv.one_of(*TEXT_SENSOR_TYPES, lower=True),
    }
).extend(cv.polling_component_schema("30s"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await text_sensor.register_text_sensor(var, config)
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_SHABBOS_MODE_ID])
    cg.add(var.set_setting_type(getattr(SettingTextSensorType, TEXT_SENSOR_TYPES[config[CONF_TYPE]])))
