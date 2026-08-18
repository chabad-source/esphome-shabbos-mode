import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID, ENTITY_CATEGORY_CONFIG

from .binary_sensor import ShabbosModeBinarySensor, shabbos_mode_ns

CONF_SHABBOS_MODE_ID = "shabbos_mode_id"

ShabbosModeResetButton = shabbos_mode_ns.class_(
    "ShabbosModeResetButton", button.Button, cg.Component
)

CONFIG_SCHEMA = button.button_schema(
    ShabbosModeResetButton,
    entity_category=ENTITY_CATEGORY_CONFIG,
).extend(
    {
        cv.Required(CONF_SHABBOS_MODE_ID): cv.use_id(ShabbosModeBinarySensor),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await button.register_button(var, config)
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_SHABBOS_MODE_ID])
