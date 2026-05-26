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
CONF_EARLY_TAKE_IN = "early_take_in"
CONF_TIME = "time"
CONF_OFFSET_MINUTES = "offset_minutes"
CONF_FROM = "from"
CONF_TO = "to"
CONF_PLAG_OPINION = "plag_opinion"
CONF_APPLIES_TO_YOM_TOV = "applies_to_yom_tov"

PLAG_OPINIONS = {
    "baal_hatanya": "baal_hatanya",
    "gra": "gra",
    "mga": "mga",
}

MAX_DAYS_BY_MONTH = {
    1: 31,
    2: 29,
    3: 31,
    4: 30,
    5: 31,
    6: 30,
    7: 31,
    8: 31,
    9: 30,
    10: 31,
    11: 30,
    12: 31,
}


def validate_time_of_day(value):
    value = cv.string_strict(value)
    parts = value.split(":")
    if len(parts) != 2 or not all(part.isdigit() for part in parts):
        raise cv.Invalid("time must be in HH:MM format")
    hour = int(parts[0])
    minute = int(parts[1])
    if hour < 0 or hour > 23 or minute < 0 or minute > 59:
        raise cv.Invalid("time must be in HH:MM format")
    return f"{hour:02d}:{minute:02d}"


def validate_month_day(value):
    value = cv.string_strict(value)
    parts = value.split("-")
    if len(parts) != 2 or not all(part.isdigit() for part in parts):
        raise cv.Invalid("date must be in MM-DD format")
    month = int(parts[0])
    day = int(parts[1])
    max_day = MAX_DAYS_BY_MONTH.get(month)
    if max_day is None or day < 1 or day > max_day:
        raise cv.Invalid("date must be a real calendar date in MM-DD format")
    return f"{month:02d}-{day:02d}"

shabbos_mode_ns = cg.esphome_ns.namespace("shabbos_mode")
ShabbosModeBinarySensor = shabbos_mode_ns.class_(
    "ShabbosModeBinarySensor", binary_sensor.BinarySensor, cg.PollingComponent
)

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(ShabbosModeBinarySensor)
    .extend(
        {
            cv.Required(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
            cv.Required(CONF_LATITUDE): cv.float_range(min=-90.0, max=90.0),
            cv.Required(CONF_LONGITUDE): cv.float_range(min=-180.0, max=180.0),
            cv.Optional(CONF_ELEVATION, default=0.0): cv.float_,
            cv.Optional(CONF_IN_ISRAEL, default=False): cv.boolean,
            cv.Optional(CONF_START_DEGREE, default=0.0): cv.float_range(min=0.0, max=30.0),
            cv.Optional(CONF_START_OFFSET_MINUTES, default=-18): cv.int_range(min=-300, max=300),
            cv.Optional(CONF_END_DEGREE, default=8.5): cv.float_range(min=0.0, max=30.0),
            cv.Optional(CONF_END_OFFSET_MINUTES, default=0): cv.int_range(min=-300, max=300),
            cv.Optional(CONF_EARLY_TAKE_IN): cv.Schema(
                {
                    cv.Optional(CONF_TIME): validate_time_of_day,
                    cv.Optional(CONF_OFFSET_MINUTES, default=0): cv.int_range(min=-300, max=300),
                    cv.Optional(CONF_FROM): validate_month_day,
                    cv.Optional(CONF_TO): validate_month_day,
                    cv.Optional(CONF_PLAG_OPINION, default="baal_hatanya"): cv.enum(PLAG_OPINIONS, lower=True),
                    cv.Optional(CONF_APPLIES_TO_YOM_TOV, default=False): cv.boolean,
                }
            ),
        }
    )
    .extend(cv.polling_component_schema("30s"))
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
    if CONF_EARLY_TAKE_IN in config:
        early_take_in = config[CONF_EARLY_TAKE_IN]
        if CONF_TIME in early_take_in:
            hour, minute = [int(part) for part in early_take_in[CONF_TIME].split(":")]
            cg.add(var.set_early_take_in_time(hour, minute))
        cg.add(var.set_early_take_in_offset_minutes(early_take_in[CONF_OFFSET_MINUTES]))
        cg.add(var.set_early_take_in_plag_opinion(early_take_in[CONF_PLAG_OPINION]))
        cg.add(var.set_early_take_in_for_yom_tov(early_take_in[CONF_APPLIES_TO_YOM_TOV]))
        if CONF_FROM in early_take_in:
            month, day = [int(part) for part in early_take_in[CONF_FROM].split("-")]
            cg.add(var.set_early_take_in_from(month, day))
        if CONF_TO in early_take_in:
            month, day = [int(part) for part in early_take_in[CONF_TO].split("-")]
            cg.add(var.set_early_take_in_to(month, day))
