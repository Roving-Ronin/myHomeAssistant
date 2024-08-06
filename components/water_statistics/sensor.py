import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome import pins

water_statistics_ns = cg.esphome_ns.namespace('water_statistics')
WaterStatistics = water_statistics_ns.class_('WaterStatistics', cg.PollingComponent, sensor.Sensor)

CONF_QUARTERLY_OFFSET_DAYS = "quarterly_offset_days"

CONFIG_SCHEMA = sensor.SENSOR_SCHEMA.extend({
    cv.Required("daily"): sensor.sensor_schema(),
    cv.Required("weekly"): sensor.sensor_schema(),
    cv.Required("monthly"): sensor.sensor_schema(),
    cv.Required("quarterly"): sensor.sensor_schema(),
    cv.Required("yearly"): sensor.sensor_schema(),
    cv.Optional(CONF_QUARTERLY_OFFSET_DAYS, default=1): cv.int_,
}).extend(cv.polling_component_schema('60s'))

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield sensor.register_sensor(var, config)

    if "daily" in config:
        sens = yield sensor.new_sensor(config["daily"])
        cg.add(var.set_daily_sensor(sens))

    if "weekly" in config:
        sens = yield sensor.new_sensor(config["weekly"])
        cg.add(var.set_weekly_sensor(sens))

    if "monthly" in config:
        sens = yield sensor.new_sensor(config["monthly"])
        cg.add(var.set_monthly_sensor(sens))

    if "quarterly" in config:
        sens = yield sensor.new_sensor(config["quarterly"])
        cg.add(var.set_quarterly_sensor(sens))

    if "yearly" in config:
        sens = yield sensor.new_sensor(config["yearly"])
        cg.add(var.set_yearly_sensor(sens))

    if CONF_QUARTERLY_OFFSET_DAYS in config:
        cg.add(var.quarterly_offset_days = config[CONF_QUARTERLY_OFFSET_DAYS])
