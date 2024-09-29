# Energy Statistics

Gather statistics for:
* today
* yesterday
* week
* month
* year

> You can take a look at sample of usage of Energy* components in configuartion for Athom Plug devices in the: [athom-power-plugs.yaml](../../esphome/sensors/athom-power-plugs.yaml)

```yaml
# Example configuration entry
...
external_components:
  - source: github://roving-ronin/myhomeassistant/components
    refresh: 0s
...
sensor:
  - platform: "energy_statistics"
    id: energy_statistics_component
    total: total

    energy_today:
      name: "$name Energy Today"

    energy_yesterday:
      name: "$name Energy Yesterday"

    energy_week:
      name: "$name Energy Week"

    energy_month:
      name: "$name Energy Month"

    energy_year:
      name: "$name Energy Year"
```

## Configuration variables:
* **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types.html#config-id)): Manually specify the ID used for code generation.
* **total** (**Required**, [ID](https://esphome.io/guides/configuration-types.html#config-id)): The ID of the total power sensor.
* **energy_today** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **energy_yesterday** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **energy_week** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **energy_month** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **energy_year** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).

Note: To reduce wear on the ESP devices flash memory, so as to increase the devices lifespan, the readings are only written to flash every 60 seconds. To change this edit the value of "uint32_t save_interval_{300};" in energy_statistics.h
