import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import sensor, time
from esphome.const import (
    CONF_ID,
    CONF_TIME_ID,
    CONF_TOTAL,
    DEVICE_CLASS_ENERGY,
    STATE_CLASS_TOTAL_INCREASING,
)

# Import your custom ICONS from const.py
from .const import (
    ICON_TODAY,
    ICON_YESTERDAY,
    ICON_WEEK,
    ICON_MONTH,
    ICON_YEAR,
)

# Import your custom UNIT_MEGAJOULE from const.py
from .const import UNIT_MEGAJOULE

CODEOWNERS = ["@dentra", "@roving-ronin"]

DEPENDENCIES = ["time"]

CONF_GAS_TODAY = "gas_today"
CONF_GAS_YESTERDAY = "gas_yesterday"
CONF_GAS_WEEK = "gas_week"
CONF_GAS_MONTH = "gas_month"
CONF_GAS_YEAR = "gas_year"

gas_statistics_mj_ns = cg.esphome_ns.namespace("gas_statistics_mj")

GasStatisticsMJ = gas_statistics_mj_ns.class_("GasStatisticsMJ", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(GasStatisticsMJ),
        cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
        cv.Required(CONF_TOTAL): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_GAS_TODAY): sensor.sensor_schema(
            unit_of_measurement=UNIT_MEGAJOULE,
            icon=ICON_TODAY,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_YESTERDAY): sensor.sensor_schema(
            unit_of_measurement=UNIT_MEGAJOULE,
            icon=ICON_YESTERDAY,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_WEEK): sensor.sensor_schema(
            unit_of_measurement=UNIT_MEGAJOULE,
            icon=ICON_WEEK,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_MONTH): sensor.sensor_schema(
            unit_of_measurement=UNIT_MEGAJOULE,
            icon=ICON_MONTH,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_YEAR): sensor.sensor_schema(
            unit_of_measurement=UNIT_MEGAJOULE,
            icon=ICON_YEAR,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
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
    await setup_input(config, CONF_TOTAL, var.set_total)

    # exposed sensors
    await setup_sensor(config, CONF_GAS_TODAY, var.set_gas_today)
    await setup_sensor(config, CONF_GAS_YESTERDAY, var.set_gas_yesterday)
    await setup_sensor(config, CONF_GAS_WEEK, var.set_gas_week)
    await setup_sensor(config, CONF_GAS_MONTH, var.set_gas_month)
    await setup_sensor(config, CONF_GAS_YEAR, var.set_gas_year)
