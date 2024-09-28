# Gas Statistics (m3)

Gather statistics in cubic metres (m3) for:
* today
* yesterday
* week
* month
* year

> Sample of usage of gas* components in configuartion for `NAME OF METER/MODEL` (1 imp = 0.5L) gas meter based on Kincony KC868-A16 (ESP32) : [kincony-kc868-a16.yaml](../../esphome/kincony-kc868-a16.yaml). Main section starts at line 972.

Note: To reduce wear on the ESP devices flash memory, so as to increase the devices lifespan, the readings are only written to flash every 60 seconds.

```yaml
# Example configuration entry
...
external_components:
  - source: github://roving-ronin/myhomeassistant/components
    refresh: 0s
...
sensor:
  - platform: "gas_statistics"
    id: gas_statistics_component
    total: gas_consumed_m3

    gas_today:
      name: "Gas Consumed Today"
      id: gas_consumed_today

    gas_yesterday:
      name: "Gas Consumed Yesterday"
      id: gas_consumed_yesterday

    gas_week:
      name: "Gas Consumed Week"
      id: gas_consumed_week

    gas_month:
      name: "Gas Consumed Month"
      id: gas_consumed_month

    gas_year:
      name: "Gas Consumed Year"
      id: gas_consumed_year
```

## Configuration variables:
* **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types.html#config-id)): Manually specify the ID used for code generation.
* **total** (**Required**, [ID](https://esphome.io/guides/configuration-types.html#config-id)): The ID of the total power sensor.
* **gas_today** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **gas_yesterday** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **gas_week** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **gas_month** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **gas_year** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
