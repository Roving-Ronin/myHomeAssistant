import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import sensor, time
from esphome.const import (
    CONF_ID,
    CONF_TIME_ID,
    CONF_TOTAL,
    DEVICE_CLASS_WATER,
    DEVICE_CLASS_GAS,
    DEVICE_CLASS_ENERGY,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_LITRE,
    UNIT_CUBIC_METER,
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

CODEOWNERS = ["@roving-ronin"]

DEPENDENCIES = ["time"]

# Water configuration constants
CONF_WATER_TODAY = "water_today"
CONF_WATER_YESTERDAY = "water_yesterday"
CONF_WATER_WEEK = "water_week"
CONF_WATER_MONTH = "water_month"
CONF_WATER_YEAR = "water_year"

# Gas m³ configuration constants
CONF_GAS_M3_TODAY = "gas_m3_today"
CONF_GAS_M3_YESTERDAY = "gas_m3_yesterday"
CONF_GAS_M3_WEEK = "gas_m3_week"
CONF_GAS_M3_MONTH = "gas_m3_month"
CONF_GAS_M3_YEAR = "gas_m3_year"

# Gas MJ configuration constants
CONF_GAS_MJ_TODAY = "gas_mj_today"
CONF_GAS_MJ_YESTERDAY = "gas_mj_yesterday"
CONF_GAS_MJ_WEEK = "gas_mj_week"
CONF_GAS_MJ_MONTH = "gas_mj_month"
CONF_GAS_MJ_YEAR = "gas_mj_year"

CONF_SAVE_FREQUENCY = "save_frequency"

utilities_statistics_ns = cg.esphome_ns.namespace("utilities_statistics")
UtilitiesStatistics = utilities_statistics_ns.class_("UtilitiesStatistics", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(UtilitiesStatistics),
        cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
        
        # Water configuration
        cv.Required(CONF_TOTAL, default="water_total"): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_WATER_TODAY): sensor.sensor_schema(
            unit_of_measurement=UNIT_LITRE,
            icon=ICON_TODAY,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_WATER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_WATER_YESTERDAY): sensor.sensor_schema(
            unit_of_measurement=UNIT_LITRE,
            icon=ICON_YESTERDAY,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_WATER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_WATER_WEEK): sensor.sensor_schema(
            unit_of_measurement=UNIT_LITRE,
            icon=ICON_WEEK,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_WATER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_WATER_MONTH): sensor.sensor_schema(
            unit_of_measurement=UNIT_LITRE,
            icon=ICON_MONTH,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_WATER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_WATER_YEAR): sensor.sensor_schema(
            unit_of_measurement=UNIT_LITRE,
            icon=ICON_YEAR,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_WATER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        
        # Gas m³ configuration
        cv.Required(CONF_TOTAL, default="gas_m3_total"): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_GAS_M3_TODAY): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER,
            icon=ICON_TODAY,
            accuracy_decimals=3,  # Corrected to 3 decimals
            device_class=DEVICE_CLASS_GAS,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_M3_YESTERDAY): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER,
            icon=ICON_YESTERDAY,
            accuracy_decimals=3,  # Corrected to 3 decimals
            device_class=DEVICE_CLASS_GAS,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_M3_WEEK): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER,
            icon=ICON_WEEK,
            accuracy_decimals=3,  # Corrected to 3 decimals
            device_class=DEVICE_CLASS_GAS,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_M3_MONTH): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER,
            icon=ICON_MONTH,
            accuracy_decimals=3,  # Corrected to 3 decimals
            device_class=DEVICE_CLASS_GAS,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_M3_YEAR): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER,
            icon=ICON_YEAR,
            accuracy_decimals=3,  # Corrected to 3 decimals
            device_class=DEVICE_CLASS_GAS,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),

        # Gas MJ configuration
        cv.Required(CONF_TOTAL, default="gas_mj_total"): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_GAS_MJ_TODAY): sensor.sensor_schema(
            unit_of_measurement=UNIT_MEGAJOULE,
            icon=ICON_TODAY,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_MJ_YESTERDAY): sensor.sensor_schema(
            unit_of_measurement=UNIT_MEGAJOULE,
            icon=ICON_YESTERDAY,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_MJ_WEEK): sensor.sensor_schema(
            unit_of_measurement=UNIT_MEGAJOULE,
            icon=ICON_WEEK,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_MJ_MONTH): sensor.sensor_schema(
            unit_of_measurement=UNIT_MEGAJOULE,
            icon=ICON_MONTH,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_GAS_MJ_YEAR): sensor.sensor_schema(
            unit_of_measurement=UNIT_MEGAJOULE,
            icon=ICON_YEAR,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def setup_sensor(config, key, setter):
    """Setting up sensor"""
    if key not in config:
        return None
    var = await sensor.new_sensor(config[key])
    cg.add(setter(var))
    return var


async def setup_input(config, key, setter):
    """Setting up input"""
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

    # Time input
    await setup_input(config, CONF_TIME_ID, var.set_time)

    # Water statistics inputs
    await setup_input(config, CONF_TOTAL, var.set_water_total)
    # Water statistics sensors
    await setup_sensor(config, CONF_WATER_TODAY, var.set_water_today)
    await setup_sensor(config, CONF_WATER_YESTERDAY, var.set_water_yesterday)
    await setup_sensor(config, CONF_WATER_WEEK, var.set_water_week)
    await setup_sensor(config, CONF_WATER_MONTH, var.set_water_month)
    await setup_sensor(config, CONF_WATER_YEAR, var.set_water_year)

    # Gas m³ statistics inputs
    await setup_input(config, CONF_TOTAL, var.set_gas_m3_total)
    # Gas m³ statistics sensors
    await setup_sensor(config, CONF_GAS_M3_TODAY, var.set_gas_m3_today)
    await setup_sensor(config, CONF_GAS_M3_YESTERDAY, var.set_gas_m3_yesterday)
    await setup_sensor(config, CONF_GAS_M3_WEEK, var.set_gas_m3_week)
    await setup_sensor(config, CONF_GAS_M3_MONTH, var.set_gas_m3_month)
    await setup_sensor(config, CONF_GAS_M3_YEAR, var.set_gas_m3_year)

    # Gas MJ statistics inputs
    await setup_input(config, CONF_TOTAL, var.set_gas_mj_total)
    # Gas MJ statistics sensors
    await setup_sensor(config, CONF_GAS_MJ_TODAY, var.set_gas_mj_today)
    await setup_sensor(config, CONF_GAS_MJ_YESTERDAY, var.set_gas_mj_yesterday)
    await setup_sensor(config, CONF_GAS_MJ_WEEK, var.set_gas_mj_week)
    await setup_sensor(config, CONF_GAS_MJ_MONTH, var.set_gas_mj_month)
    await setup_sensor(config, CONF_GAS_MJ_YEAR, var.set_gas_mj_year)
