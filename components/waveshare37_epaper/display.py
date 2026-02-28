import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import display
from esphome.const import CONF_ID, CONF_CS_PIN, CONF_DC_PIN, CONF_BUSY_PIN, CONF_RESET_PIN, CONF_LAMBDA
from esphome import pins

AUTO_LOAD = ["display"]

waveshare37_ns = cg.esphome_ns.namespace("waveshare37_epaper")
Waveshare37EPaperDisplay = waveshare37_ns.class_("Waveshare37EPaperDisplay", display.DisplayBuffer)

CONF_CLK_PIN = "clk_pin"
CONF_MOSI_PIN = "mosi_pin"
CONF_FULL_UPDATE_EVERY = "full_update_every"

CONFIG_SCHEMA = display.FULL_DISPLAY_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(Waveshare37EPaperDisplay),
    cv.Required(CONF_CLK_PIN):   pins.gpio_output_pin_schema,
    cv.Required(CONF_MOSI_PIN):  pins.gpio_output_pin_schema,
    cv.Required(CONF_CS_PIN):    pins.gpio_output_pin_schema,
    cv.Required(CONF_DC_PIN):    pins.gpio_output_pin_schema,
    cv.Required(CONF_RESET_PIN): pins.gpio_output_pin_schema,
    cv.Required(CONF_BUSY_PIN):  pins.gpio_input_pin_schema,
    cv.Optional(CONF_FULL_UPDATE_EVERY, default=10): cv.positive_int,
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await display.register_display(var, config)

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA],
            [(display.DisplayRef, "it")],
            return_type=cg.void,
        )
        cg.add(var.set_writer(lambda_))

    cg.add(var.set_full_update_every(config[CONF_FULL_UPDATE_EVERY]))

    cg.add(var.set_pins(
        await cg.gpio_pin_expression(config[CONF_CLK_PIN]),
        await cg.gpio_pin_expression(config[CONF_MOSI_PIN]),
        await cg.gpio_pin_expression(config[CONF_CS_PIN]),
        await cg.gpio_pin_expression(config[CONF_DC_PIN]),
        await cg.gpio_pin_expression(config[CONF_RESET_PIN]),
        await cg.gpio_pin_expression(config[CONF_BUSY_PIN]),
    ))