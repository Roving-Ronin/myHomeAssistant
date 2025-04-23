UNDER CONSTRUCTION - NEEDS UPDATING.

# Usage Tracker Statistics

Gather statistics for, how long a device such as a light or a shower was:
* last used
* used for in its lifetime (accumulated across reboots)


> You can take a look at sample of usage of the component in configuartion for the Aldi Casalux Smart LED (CWWWW or RGBW) Athom Plug devices in the: [aldi-casalux-smart-led-rgbw.yaml](../../esphome/sensors/aldi-casalux-smart-led-rgbw.yaml)
> in this, these sensors are used to show how long the light was last on for and the total runtime the light has had, to allow tracking for predicted failure as compared to manufacturers published x,000 hour lifespan.

```yaml
# Example configuration entry

external_components:
  - source: github://roving-ronin/myhomeassistant/components
    components: [usage_tracker]
    refresh: 0s

sensor:
  - platform: "usage_tracker"
    id: usage_tracker_component
    on_off_sensor: downlight_state

    last_use:
      name: "Duration Last Use"

    lifetime_use:
      name: "Duration Lifetime Use"

```

> As the manufactuers always publish the anticipated lifetime in hours, the last_use and lifetime_use sensors have a lambda that converts the seconds output into hours, with 3 decimal places, whilst also filtering the sensor to only update when used. This is shown below:

```yaml
# Example configuration entry for use with lights

external_components:
  - source: github://roving-ronin/myhomeassistant/components
    components: [usage_tracker]
    refresh: 0s

binary_sensor:
  - platform: template
    name: "Downlight State"
    id: downlight_state
    lambda: |-
      return id(downlight).current_values.is_on();
    internal: true

sensor:
  - platform: "usage_tracker"
    id: usage_tracker_component
    on_off_sensor: downlight_state

    last_use:
      name: "Duration Last Use"
      id: duration_last_use
      unit_of_measurement: h
      accuracy_decimals: 3
      filters:
        - lambda: |-
            return x / 3600.0;
        - delta: 0.001
      web_server:
        sorting_group_id: group_light_usage
        sorting_weight: 1

    lifetime_use:
      name: "Duration Lifetime Use"
      id: duration_lifetime_use
      unit_of_measurement: h
      accuracy_decimals: 3
      filters:
        - lambda: |-
            return x / 3600.0;
        - delta: 0.001
      web_server:
        sorting_group_id: group_light_usage
        sorting_weight: 2

```


## Configuration variables:
* **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types.html#config-id)): Manually specify the ID used for code generation.
* **on_off_sensor** (**Required**, [ID](https://esphome.io/guides/configuration-types.html#config-id)): The ID of the sensor tracking the On/Off state of the device.
* **last_use** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **lifetime_use** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).

Note: To reduce wear on the ESP devices flash memory, so as to increase the devices lifespan, the readings are only written to flash every 5 minutes. To change this edit the value of "if (millis() - this->last_save_time_ > 300000) {" in usage_tracker.h
