# Utility Statistics

To save on writes to Non-Volatile Storage (NVS) flash memory, this component combines the Gas (m3), Gas (MJ) and Water (L) components into a single component.

It publishes statistics for, each of Gas (m3), Gas (MJ) and Water (L):
* today
* yesterday
* week
* month
* year

> Sample of usage of water* components in configuartion for `Elster V100` (1 imp = 10L) water meter based on Kincony KC868-A16 (ESP32) : [kincony-kc868-a16.yaml](../../esphome/kincony-kc868-a16.yaml). Main section starts at line 972.

Note: To reduce wear on the ESP devices flash memory, so as to increase the devices lifespan, the readings are only written to flash every 60 seconds.

```yaml
# Example configuration entry
...
external_components:
  - source: github://roving-ronin/myhomeassistant/components
    refresh: 0s
...
sensor:
  - platform: "utility_statistics"
    id: utility_statistics_component

    # Time source
    time_id: sntp_time  # Make sure you define this time integration elsewhere

    # Gas (m³) statistics
    gas_m3_total: gas_meter_m3_total  # The id of the sensor tracking total consumed Gas (m3)
    gas_m3_today:
      name: "Gas (m³) Today"
      id: gas_m3_today
    gas_m3_yesterday:
      name: "Gas (m³) Yesterday"
      id: gas_m3_yesterday
    gas_m3_week:
      name: "Gas (m³) Week"
      id: gas_m3_week
    gas_m3_month:
      name: "Gas (m³) Month"
      id: gas_m3_month
    gas_m3_year:
      name: "Gas (m³) Year"
      id: gas_m3_year

    # Gas (MJ) statistics
    gas_mj_total: gas_meter_mj_total  # The id of the sensor tracking total consumed Gas (MJ)
    gas_mj_today:
      name: "Gas MJ Today"
      id: gas_mj_today
    gas_mj_yesterday:
      name: "Gas MJ Yesterday"
      id: gas_mj_yesterday
    gas_mj_week:
      name: "Gas MJ Week"
      id: gas_mj_week
    gas_mj_month:
      name: "Gas MJ Month"
      id: gas_mj_month
    gas_mj_year:
      name: "Gas MJ Year"
      id: gas_mj_year

    # Water statistics
    water_total: water_consumed_litres  # The id of the sensor tracking total consumed water (litres)
    water_today:
      name: "Water Consumed Today"
      id: water_consumed_today
    water_yesterday:
      name: "Water Consumed Yesterday"
      id: water_consumed_yesterday
    water_week:
      name: "Water Consumed Week"
      id: water_consumed_week
    water_month:
      name: "Water Consumed Month"
      id: water_consumed_month
    water_year:
      name: "Water Consumed Year"
      id: water_consumed_year
```

TO UPDATE STILL


## Configuration variables:
* **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types.html#config-id)): Manually specify the ID used for code generation.
* **total** (**Required**, [ID](https://esphome.io/guides/configuration-types.html#config-id)): The ID of the total water consumed (litres) sensor.
* **water_today** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **water_yesterday** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **water_week** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **water_month** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **water_year** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
