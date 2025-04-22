from esphome import automation
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import sensor, binary_sensor
from esphome.const import CONF_ID, CONF_SENSOR

CODEOWNERS = ["@roving-ronin"]
DEPENDENCIES = []

CONF_ON_OFF_SENSOR = "on_off_sensor"
CONF_LAST_ON_DURATION = "last_on_duration"
CONF_LIFETIME_USE = "lifetime_use"

usage_timer_ns = cg.esphome_ns.namespace("usage_timer")
UsageTimer = usage_timer_ns.class_("UsageTimer", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(UsageTimer),
        cv.Required(CONF_ON_OFF_SENSOR): cv.use_id(binary_sensor.BinarySensor),
        cv.Required(CONF_LAST_ON_DURATION): sensor.sensor_schema(),
        cv.Required(CONF_LIFETIME_USE): sensor.sensor_schema(),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    sens = await cg.get_variable(config[CONF_ON_OFF_SENSOR])
    cg.add(var.set_sensor(sens))

    last_on = await sensor.new_sensor(config[CONF_LAST_ON_DURATION])
    cg.add(var.set_last_on_duration_sensor(last_on))

    lifetime = await sensor.new_sensor(config[CONF_LIFETIME_USE])
    cg.add(var.set_lifetime_use_sensor(lifetime))
