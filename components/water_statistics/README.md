# Water Statistics

Gather statistics for:
* today
* yesterday
* week
* month

> You can take a look at sample of usage of water* components in configuartion for `ZMAi-90` water meter based on `TYWE3S`: [zmai90.yaml](../zmai90.yaml)

```yaml
# Example configuration entry
...
external_components:
  - source: github://roving-ronin/myHomeAssistant/components
...
sensor:
  - platform: "water_statistics"
    total: water_consumed_litres
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
```

## Configuration variables:
* **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types.html#config-id)): Manually specify the ID used for code generation.
* **total** (**Required**, [ID](https://esphome.io/guides/configuration-types.html#config-id)): The ID of the total power sensor.
* **water_today** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **water_yesterday** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **water_week** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **water_month** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
