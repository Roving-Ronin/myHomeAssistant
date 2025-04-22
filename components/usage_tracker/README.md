UNDER CONSTRUCTION - NEEDS UPDATING.

# Usage Tracker Statistics

Gather statistics for, how long a device such as a light or a shower was:
* last used for
* used for in its lifetime (accumulated across reboots)


> You can take a look at sample of usage of the component in configuartion for the Aldi Casalux Smart LED (CWWWW or RGBW) Athom Plug devices in the: [athom-power-plugs.yaml](../../esphome/sensors/athom-power-plugs.yaml)  <--- UPDATE 

```yaml
# Example configuration entry
...
external_components:
  - source: github://roving-ronin/myhomeassistant/components
    components: [usage_tracker]
    refresh: 0s
...
sensor:
  - platform: "usage_tracker"
    id: usage_tracker_component
    on_off_sensor: state_device_such_as_light_switch

    last_runtime:
      name: "$name Last Runtime"

    lifetime_runtime:
      name: "$name Lifetime Runtime"

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
