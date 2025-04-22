import esphome.config_validation as cv
import esphome.codegen as cg
from esphome import automation
from esphome.components import sensor, binary_sensor
from esphome.components import sensor, time
from esphome.const import CONF_ID, CONF_SENSOR

# Import your custom ICONS from const.py
from .const import (
    DOMAIN,
    ICON_LAST_USE,
    ICON_LIFETIME_USE,
)

CODEOWNERS = ["@roving-ronin"]
DEPENDENCIES = ["time"]

CONF_ON_OFF_SENSOR = "on_off_sensor"
CONF_LAST_ON_DURATION = "last_on_duration"
CONF_LIFETIME_USE = "lifetime_use"

usage_tracker_ns = cg.esphome_ns.namespace("usage_tracker")
UsageTracker = usage_tracker_ns.class_("UsageTracker", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(UsageTracker),
        cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
        cv.Required(CONF_ON_OFF_SENSOR): cv.use_id(binary_sensor.BinarySensor),
        cv.Required(CONF_LAST_ON_DURATION): sensor.sensor_schema(
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_LAST_USE,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Required(CONF_LIFETIME_USE): sensor.sensor_schema(
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_LIFETIME_USE,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
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
