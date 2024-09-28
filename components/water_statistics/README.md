# Water Statistics

Gather statistics in litres for:
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
  - platform: "water_statistics"
    id: water_statistics_component
    total: water_consumed_litres       # The id of the sensor tracking total consumed water (litres)

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
