from esphome import pins
import esphome.codegen as cg
from esphome.components import i2c, touchscreen
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_INTERRUPT_PIN, CONF_RESET_PIN, CONF_PIN

from .. import spd2010_ns

CODEOWNERS = ["@gjdodd"]
DEPENDENCIES = ["i2c"]

Spd2010Touchscreen = spd2010_ns.class_(
    "Spd2010Touchscreen",
    touchscreen.Touchscreen,
    i2c.I2CDevice,
)

CONF_SPD2010_TOUCHSCREEN_ID = "spd2010_touchscreen_id"

CONFIG_SCHEMA = touchscreen.TOUCHSCREEN_SCHEMA.extend(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Spd2010Touchscreen),
            cv.Required(CONF_INTERRUPT_PIN): cv.All(
                pins.internal_gpio_input_pin_schema
            ),
            cv.Required(CONF_RESET_PIN): cv.All(
                pins.gpio_output_pin_schema,
            ),
            ,
            cv.Required("enable_pin"): cv.All(
                pins.gpio_output_pin_schema,
            ),
        }
    ).extend(i2c.i2c_device_schema(0x53))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await touchscreen.register_touchscreen(var, config)
    await i2c.register_i2c_device(var, config)

    if interrupt_pin_config := config.get(CONF_INTERRUPT_PIN):
        interrupt_pin = await cg.gpio_pin_expression(interrupt_pin_config)
        cg.add(var.set_interrupt_pin(interrupt_pin))
    if reset_pin_config := config.get(CONF_RESET_PIN):
        reset_pin = await cg.gpio_pin_expression(reset_pin_config)
        cg.add(var.set_reset_pin(reset_pin))