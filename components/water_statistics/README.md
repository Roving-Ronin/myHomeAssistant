# Water Statistics

Gather statistics for:
* today
* yesterday
* week
* month
* year



```yaml
# Example configuration entry
...
external_components:
  - source: github://Roving-Ronin/myHomeAssistant/components
    components: [ water_statistics ]

custom_component:
  - lambda: |-
      auto water_statistics = new WaterStatistics();
      App.register_component(water_statistics);
      water_statistics->daily_sensor = id(daily_water_usage);
      water_statistics->weekly_sensor = id(weekly_water_usage);
      water_statistics->monthly_sensor = id(monthly_water_usage);
      water_statistics->quarterly_sensor = id(quarterly_water_usage);
      water_statistics->yearly_sensor = id(yearly_water_usage);
      water_statistics->quarterly_offset_days = 15;  # Set your offset here

sensor:
  - platform: custom
    lambda: |-
      return {water_statistics->daily_sensor, water_statistics->weekly_sensor, water_statistics->monthly_sensor, water_statistics->quarterly_sensor, water_statistics->yearly_sensor};
    sensors:
      - name: "Daily Water Usage"
      - name: "Weekly Water Usage"
      - name: "Monthly Water Usage"
      - name: "Quarterly Water Usage"
      - name: "Yearly Water Usage"

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
