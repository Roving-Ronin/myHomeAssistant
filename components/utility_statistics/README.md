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
      name: "Gas - Used Today"
      id: gas_m3_used_today
    gas_m3_yesterday:
      name: "Gas - Used Yesterday"
      id: gas_m3_used_yesterday
    gas_m3_week:
      name: "Gas - Used Week"
      id: gas_m3_used_week
    gas_m3_month:
      name: "Gas - Used Month"
      id: gas_m3_used_month
    gas_m3_year:
      name: "Gas - Used Year"
      id: gas_m3_used_year

    # Gas (MJ) statistics
    gas_mj_total: gas_meter_mj_total  # The id of the sensor tracking total consumed Gas (MJ)
    gas_mj_today:
      name: "Gas - MJ Used Today"
      id: gas_mj_used_today
    gas_mj_yesterday:
      name: "Gas - MJ Used Yesterday"
      id: gas_mj_used_yesterday
    gas_mj_week:
      name: "Gas - MJ Used Week"
      id: gas_mj_used_week
    gas_mj_month:
      name: "Gas - MJ Used Month"
      id: gas_mj_used_month
    gas_mj_year:
      name: "Gas - MJ Used Year"
      id: gas_mj_used_year

    # Water (L) statistics
    water_total: water_used_litres  # The id of the sensor tracking total consumed water (litres)
    water_today:
      name: "Water - Used Today"
      id: water_used_today
    water_yesterday:
      name: "Water - Used Yesterday"
      id: water_used_yesterday
    water_week:
      name: "Water - Used Week"
      id: water_used_week
    water_month:
      name: "Water - Used  Month"
      id: water_used_month
    water_year:
      name: "Water - Used  Year"
      id: water_used_year
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
