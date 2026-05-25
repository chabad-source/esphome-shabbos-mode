import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, time
from esphome.const import CONF_ID

DEPENDENCIES = ["time"]

CONF_TIME_ID = "time_id"
CONF_LATITUDE = "latitude"
CONF_LONGITUDE = "longitude"
CONF_ELEVATION = "elevation"
CONF_IN_ISRAEL = "in_israel"
CONF_START_DEGREE = "start_degree"
CONF_START_OFFSET_MINUTES = "start_offset_minutes"
CONF_END_DEGREE = "end_degree"
CONF_END_OFFSET_MINUTES = "end_offset_minutes"

shabbos_mode_ns = cg.esphome_ns.namespace("shabbos_mode")
ShabbosModeBinarySensor = shabbos_mode_ns.class_(
    "ShabbosModeBinarySensor", binary_sensor.BinarySensor, cg.PollingComponent
)

CONFIG_SCHEMA = (
    binary_sensor.BINARY_SENSOR_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(ShabbosModeBinarySensor),
            cv.Required(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
            cv.Required(CONF_LATITUDE): cv.float_range(min=-90.0, max=90.0),
            cv.Required(CONF_LONGITUDE): cv.float_range(min=-180.0, max=180.0),
            cv.Optional(CONF_ELEVATION, default=0.0): cv.float_,
            cv.Optional(CONF_IN_ISRAEL, default=False): cv.boolean,
            cv.Optional(CONF_START_DEGREE, default=0.0): cv.float_range(min=0.0, max=30.0),
            cv.Optional(CONF_START_OFFSET_MINUTES, default=-18): cv.int_range(min=-300, max=300),
            cv.Optional(CONF_END_DEGREE, default=8.5): cv.float_range(min=0.0, max=30.0),
            cv.Optional(CONF_END_OFFSET_MINUTES, default=0): cv.int_range(min=-300, max=300),
        }
    ).extend(cv.polling_component_schema("30s"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await binary_sensor.register_binary_sensor(var, config)

    time_var = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(time_var))
    cg.add(var.set_latitude(config[CONF_LATITUDE]))
    cg.add(var.set_longitude(config[CONF_LONGITUDE]))
    cg.add(var.set_elevation(config[CONF_ELEVATION]))
    cg.add(var.set_in_israel(config[CONF_IN_ISRAEL]))
    cg.add(var.set_start_degree(config[CONF_START_DEGREE]))
    cg.add(var.set_start_offset_minutes(config[CONF_START_OFFSET_MINUTES]))
    cg.add(var.set_end_degree(config[CONF_END_DEGREE]))
    cg.add(var.set_end_offset_minutes(config[CONF_END_OFFSET_MINUTES]))
