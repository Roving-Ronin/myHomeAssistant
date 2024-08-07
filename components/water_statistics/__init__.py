import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import sensor, time
from esphome.const import (
    CONF_ID,
    CONF_TIME_ID,
    CONF_POWER,
    CONF_TOTAL,
    DEVICE_CLASS_WATER,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_LITRES,
)

CODEOWNERS = ["@dentra"]

DEPENDENCIES = ["time"]

CONF_WATER_TODAY = "water_today"
CONF_water_YESTERDAY = "water_yesterday"
CONF_WATER_WEEK = "water_week"
CONF_WATER_MONTH = "water_month"

water_statistics_ns = cg.esphome_ns.namespace("water_statistics")

WaterStatistics = water_statistics_ns.class_("WaterStatistics", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WaterStatistics),
        cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
        cv.Required(CONF_TOTAL): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_WATER_TODAY): sensor.sensor_schema(
            unit_of_measurement=LITRES,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_WATER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_WATER_YESTERDAY): sensor.sensor_schema(
            unit_of_measurement=LITRES,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_WATER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_WATER_WEEK): sensor.sensor_schema(
            unit_of_measurement=LITRES,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_WATER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_WATER_MONTH): sensor.sensor_schema(
            unit_of_measurement=LITRES,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_WATER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def setup_sensor(config, key, setter):
    """setting up sensor"""
    if key not in config:
        return None
    var = await sensor.new_sensor(config[key])
    cg.add(setter(var))
    return var


async def setup_input(config, key, setter):
    """setting up input"""
    if key not in config:
        return None
    var = await cg.get_variable(config[key])
    cg.add(setter(var))
    return var


# code generation entry point
async def to_code(config):
    """Code generation entry point"""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    await setup_input(config, CONF_TIME_ID, var.set_time)

    # input sensors
    await setup_input(config, CONF_POWER, var.set_power)
    await setup_input(config, CONF_TOTAL, var.set_total)

    # exposed sensors
    await setup_sensor(config, CONF_WATER_TODAY, var.set_water_today)
    await setup_sensor(config, CONF_WATER_YESTERDAY, var.set_water_yesterday)
    await setup_sensor(config, CONF_WATER_WEEK, var.set_water_week)
    await setup_sensor(config, CONF_WATER_MONTH, var.set_water_month)
