import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import sensor, time, number
from esphome.const import (
    CONF_ID,
    CONF_TIME_ID,
    CONF_TOTAL,
    DEVICE_CLASS_GAS,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_CUBIC_METER,
)

# Import your custom ICONS from const.py
from .const import (
    ICON_TODAY,
    ICON_YESTERDAY,
    ICON_WEEK,
    ICON_MONTH,
    ICON_YEAR,
    ICON_QUARTER,
)

CODEOWNERS = ["@roving-ronin"]

DEPENDENCIES = ["time"]

# Note: ICON_QUARTER is "mdi:calendar-multiple", matching the constant that
# already existed (unused) in gas_statistics_mj/const.py for consistency.

CONF_GAS_TODAY = "gas_today"
CONF_GAS_YESTERDAY = "gas_yesterday"
CONF_GAS_WEEK = "gas_week"
CONF_GAS_MONTH = "gas_month"
CONF_GAS_YEAR = "gas_year"
CONF_GAS_QUARTER = "gas_quarter"
# Optional binding to a number entity (e.g. a template number exposed on the
# web_server GUI) that holds the day-of-month (1-31) on which the quarter
# accumulator should reset. Automatically clamped in C++ to the real length
# of the reset month (e.g. 31 falls back to the 28th/29th in February).
CONF_QUARTER_RESET_DAY = "quarter_reset_day"
# Optional binding to a number entity holding the "anchor" month (1-12) for
# the first quarter. The other three quarter-start months are anchor+3,
# anchor+6, anchor+9 (e.g. anchor=2 => Feb/May/Aug/Nov).
CONF_QUARTER_START_MONTH = "quarter_start_month"

gas_statistics_ns = cg.esphome_ns.namespace("gas_statistics")

GasStatistics = gas_statistics_ns.class_("GasStatistics", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(GasStatistics),
        cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
        cv.Required(CONF_TOTAL): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_QUARTER_RESET_DAY): cv.use_id(number.Number),
        cv.Optional(CONF_QUARTER_START_MONTH): cv.use_id(number.Number),
        cv.Optional(CONF_GAS_TODAY): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER,
            icon=ICON_TODAY,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_GAS,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_YESTERDAY): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER,
            icon=ICON_YESTERDAY,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_GAS,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_WEEK): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER,
            icon=ICON_WEEK,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_GAS,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_MONTH): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER,
            icon=ICON_MONTH,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_GAS,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_YEAR): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER,
            icon=ICON_YEAR,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_GAS,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_QUARTER): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER,
            icon=ICON_QUARTER,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_GAS,
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

    # input sensors CHECK JBH
    await setup_input(config, CONF_TOTAL, var.set_total)

    # optional input number entities controlling the quarter reset day/month
    await setup_input(config, CONF_QUARTER_RESET_DAY, var.set_quarter_reset_day)
    await setup_input(config, CONF_QUARTER_START_MONTH, var.set_quarter_start_month)

    # exposed sensors
    await setup_sensor(config, CONF_GAS_TODAY, var.set_gas_today)
    await setup_sensor(config, CONF_GAS_YESTERDAY, var.set_gas_yesterday)
    await setup_sensor(config, CONF_GAS_WEEK, var.set_gas_week)
    await setup_sensor(config, CONF_GAS_MONTH, var.set_gas_month)
    await setup_sensor(config, CONF_GAS_YEAR, var.set_gas_year)
    await setup_sensor(config, CONF_GAS_QUARTER, var.set_gas_quarter)
