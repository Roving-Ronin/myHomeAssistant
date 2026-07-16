# Gas Statistics (MJ)

Gather statistics in megajoules (MJ) for:
* today
* yesterday
* week
* month
* quarter
* year

> Sample of usage of gas* components in configuartion for `NAME OF METER/MODEL` (1 imp = 10L) gas meter based on Kincony KC868-A16 (ESP32) : [kincony-kc868-a16.yaml](../../esphome/kincony-kc868-a16.yaml). Main section starts at line 972.

Note: To reduce wear on the ESP devices flash memory, so as to increase the devices lifespan, the readings are only written to flash every 60 seconds.

The quarter reset day and start month can be set either at compile time (defaults built into the component) or, more usefully, bound to `select` entities so they show up as dropdowns you can change at runtime from the Home Assistant / web_server GUI without reflashing. If you're also running `gas_statistics` (m3) alongside this component, point both at the same `quarter_reset_day` / `quarter_start_month` select entities so the m3 and MJ quarter totals reset together.

```yaml
# Example configuration entry
...
external_components:
  - source: github://roving-ronin/myhomeassistant/components
    refresh: 0s
...
# Optional: expose the quarter reset day/month as dropdowns in the web_server
# GUI. If you skip this, the component falls back to day 1 of January.
select:
  - platform: template
    name: "Gas - Quarter Reset Day"
    id: input_gas_quarter_reset_day
    optimistic: true
    restore_value: true
    entity_category: config
    options:
      - "1"
      - "2"
      # ... "3" through "30" ...
      - "31"
    initial_option: "1"

  - platform: template
    name: "Gas - Quarter Start Month"
    id: input_gas_quarter_start_month
    optimistic: true
    restore_value: true
    entity_category: config
    options:
      - "January"
      - "February"
      - "March"
      - "April"
      - "May"
      - "June"
      - "July"
      - "August"
      - "September"
      - "October"
      - "November"
      - "December"
    initial_option: "January"

sensor:
  - platform: "gas_statistics_mj"
    id: gas_statistics_mj_component
    total: gas_consumed_mj
    quarter_reset_day: input_gas_quarter_reset_day
    quarter_start_month: input_gas_quarter_start_month

    gas_today:
      name: "Gas Consumed Today"
      id: gas_consumed_today_mj

    gas_yesterday:
      name: "Gas Consumed Yesterday"
      id: gas_consumed_yesterday_mj

    gas_week:
      name: "Gas Consumed Week"
      id: gas_consumed_week_mj

    gas_month:
      name: "Gas Consumed Month"
      id: gas_consumed_month_mj

    gas_quarter:
      name: "Gas Consumed Quarter"
      id: gas_consumed_quarter_mj

    gas_year:
      name: "Gas Consumed Year"
      id: gas_consumed_year_mj
```

## Configuration variables:
* **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types.html#config-id)): Manually specify the ID used for code generation.
* **total** (**Required**, [ID](https://esphome.io/guides/configuration-types.html#config-id)): The ID of the total power sensor.
* **quarter_reset_day** (*Optional*, [ID](https://esphome.io/guides/configuration-types.html#config-id)): The ID of a `select` entity with options `"1"` through `"31"` holding the day-of-month on which the quarter accumulator resets. Automatically clamped to the real length of the reset month (e.g. a value of 31 falls back to the 28th/29th in February), so it's safe to leave set at 31 year-round. If omitted, defaults to day 1.
* **quarter_start_month** (*Optional*, [ID](https://esphome.io/guides/configuration-types.html#config-id)): The ID of a `select` entity with options `"January"` through `"December"` holding the "anchor" month for the first quarter of the year. The other three quarter-start months are anchor+3, anchor+6, and anchor+9 (e.g. anchor=February gives Feb/May/Aug/Nov; anchor=January gives the calendar-quarter default of Jan/Apr/Jul/Oct). If omitted, defaults to January.
* **gas_today** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **gas_yesterday** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **gas_week** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **gas_month** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **gas_quarter** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
* **gas_year** (*Optional*, Sensor):
  * Any options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
