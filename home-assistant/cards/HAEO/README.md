# HAEO Events Card (`haeo-events-card.js`)

A custom Home Assistant Lovelace card for the **Home Assistant Energy Optimiser (HAEO)** integration. Displays the optimizer's forecast decisions in a **Future Decisions** tab and your inverter's historical energy data in a **Past Events** tab — both in a single, scrollable table with grouped kW and kWh columns.

**Current version:** `v2.1.5`

---

## Features

- **Future Decisions tab** — shows the HAEO optimizer's forecast for the next several days, reading directly from the `forecast` attributes on your HAEO sensors. Includes:
  - Event classification (Solar → Home, Battery → Home, Grid charging, Force Export, etc.)
  - Buy and Sell price per slot
  - Load, PV, Grid and Battery kW and kWh columns
  - SoC % with colour warnings at low levels
  - Cost/Profit per slot and estimated daily totals
  - Status bar: SoC now, Morning/Peak SoC, current buy/sell prices, grid import/export warnings
  - Smart auto-refresh timed to HA's 5-minute update boundary

- **Past Events tab** — reads history from the HA recorder for the same sensors, aligned to 5-minute slots. Includes:
  - Event classification based on actual recorded power values
  - kWh delta columns from `total_increasing` energy sensors
  - Daily kWh totals (Load, PV, Grid, Battery) and daily Cost/Profit
  - Range selector: Today / Yesterday / Last 24h / 48h / 72h / 96h / 7 days
  - Auto-switches to Last 24h if Today has no data yet

- **Colour coding** — row backgrounds reflect the event type; kW/kWh values are individually coloured:
  - Grid: export = green (earning), import = red (costing)
  - Battery: charging from solar = green, charging from grid = amber, discharging = red

- **Configurable sensor mappings** — defaults to HAEO optimizer sensors and Sigenergy Local Modbus energy sensors; any sensor can be overridden via card YAML

- **Auto unit detection** — reads `unit_of_measurement` from live sensor state and normalises to kW / kWh automatically (supports W, kW, MW, Wh, kWh, MWh, GWh)

- **Full-width layout** — designed for HA Sections dashboard with `grid_options: columns: full`

---

## Requirements

- Home Assistant with the [HAEO (Home Assistant Energy Optimiser)](https://github.com/energypatrikf/home-assistant-energy-optimizer) integration installed and running
- An inverter integration providing `total_increasing` energy sensors for kWh columns (Sigenergy Local Modbus defaults provided; any integration can be used)
- HA recorder enabled (for Past Events history queries)

---

## Installation

1. Copy `haeo-events-card.js` to your HA config directory:
   ```
   /config/www/haeo-events-card.js
   ```

2. Add it as a Lovelace resource. In HA go to **Settings → Dashboards → ⋮ → Manage resources** and add:
   - **URL:** `/local/haeo-events-card.js`
   - **Type:** JavaScript module

   Or add it manually to your `configuration.yaml` / `ui-lovelace.yaml`:
   ```yaml
   resources:
     - url: /local/haeo-events-card.js
       type: module
   ```

3. Hard-refresh your browser (`Ctrl+Shift+R` / `Cmd+Shift+R`) to load the new resource.

---

## Basic Usage

The simplest configuration — uses all HAEO default sensor names and Sigenergy energy sensor defaults:

```yaml
type: custom:haeo-events-card
grid_options:
  columns: full
```

---

## Sensor Configuration

The card uses two groups of sensors.

### Power Sensors (HAEO optimizer)

These are provided by the HAEO integration and are the same for all installs. **No configuration is needed** unless you have renamed them.

| Config key | Default entity | Notes |
|---|---|---|
| `entity_battery` | `sensor.battery_active_power` | kW, positive=discharge, negative=charge |
| `entity_grid` | `sensor.grid_active_power` | kW, negative=export, positive=import (HAEO forecast convention) |
| `entity_load` | `sensor.load_power` | kW |
| `entity_solar` | `sensor.solar_power` | kW |
| `entity_soc` | `sensor.battery_state_of_charge` | % |
| `entity_buy_price` | `number.grid_import_price` | $/kWh |
| `entity_sell_price` | `number.grid_export_price` | $/kWh |
| `entity_grid_net_cost` | `sensor.grid_net_cost` | $ per period |

### Energy Sensors (inverter integration)

These provide the kWh delta columns in the Past Events tab. Defaults are for the [Sigenergy Local Modbus](https://github.com/TypQxQ/Sigenergy-Local-Modbus) integration.

> **Important:** Use **lifetime/total** sensors wherever possible. Daily or monthly sensors reset at midnight or month-end, causing gaps (shown as `—`) when the Past Events range spans a reset boundary. The battery sensors below are daily-reset — lifetime variants are preferred if your integration provides them.

| Config key | Default entity | Notes |
|---|---|---|
| `entity_energy_load` | `sensor.sigen_plant_total_load_consumption` | Lifetime total |
| `entity_energy_solar` | `sensor.sigen_plant_total_pv_generation` | Lifetime total |
| `entity_energy_grid_import` | `sensor.sigen_plant_total_imported_energy` | Lifetime total |
| `entity_energy_grid_export` | `sensor.sigen_plant_total_exported_energy` | Lifetime total |
| `entity_energy_batt_charge` | `sensor.sigen_plant_daily_battery_charge_energy` | Daily reset — gaps at midnight |
| `entity_energy_batt_discharge` | `sensor.sigen_plant_daily_battery_discharge_energy` | Daily reset — gaps at midnight |

---

## Full Configuration Example

```yaml
type: custom:haeo-events-card
grid_options:
  columns: full

# ── Override HAEO power sensors (only needed if you have renamed them) ──
# entity_battery:       sensor.battery_active_power
# entity_grid:          sensor.grid_active_power
# entity_load:          sensor.load_power
# entity_solar:         sensor.solar_power
# entity_soc:           sensor.battery_state_of_charge
# entity_buy_price:     number.grid_import_price
# entity_sell_price:    number.grid_export_price
# entity_grid_net_cost: sensor.grid_net_cost

# ── Override energy sensors for a non-Sigenergy inverter ──
entity_energy_load:           sensor.my_inverter_total_load_consumption
entity_energy_solar:          sensor.my_inverter_total_pv_generation
entity_energy_grid_import:    sensor.my_inverter_total_imported_energy
entity_energy_grid_export:    sensor.my_inverter_total_exported_energy
entity_energy_batt_charge:    sensor.my_inverter_total_battery_charge
entity_energy_batt_discharge: sensor.my_inverter_total_battery_discharge
```

---

## Column Reference

| Column | Description |
|---|---|
| Time | Slot start time |
| Event | Classified activity label |
| Buy $/kWh | Grid import price for this slot |
| Sell $/kWh | Grid export price for this slot |
| Load kW / kWh | Home consumption |
| PV kW / kWh | Solar generation |
| Grid kW / kWh | Negative = export (green), Positive = import (red) |
| Battery kW / kWh | Negative = discharging (red), Positive = charging (green/amber) |
| SoC % | Battery state of charge |
| Cost/Profit | Net grid cost for the slot (`-$` = expense, `$` = earning) |

Day header rows show daily kWh totals for each column and the estimated net cost/profit for the day.

---

## Event Classification

Events are classified purely from power values — HAEO does not expose a mode field. The classifier uses 50 W (0.05 kW) thresholds for the Future tab and 100 W (0.10 kW) for the Past tab (to filter inverter noise in recorded history).

| Row colour | Meaning |
|---|---|
| 🟢 Green (light) | Solar self-consumption |
| 🩵 Teal | Battery + Solar self-consumption |
| 🟡 Yellow | Battery-only self-consumption |
| 🔴 Red (light) | Grid import or forced grid charge |
| 🟩 Dark green | Force export to grid (earning) |
| 🌸 Pink | Mixed sources including grid import |

---

## Notes

- The card auto-refreshes at 1 minute past each 5-minute HA update boundary (`:01`, `:06`, `:11`... past the hour) and catches up if the browser tab was hidden
- `sensor.grid_net_cost` is used as the source for Cost/Profit values; it is only shown for slots where grid flow exceeds 0.05 kW to avoid stale interpolated values appearing on battery-only rows
- Units are auto-detected from `unit_of_measurement` on each sensor's live state. Supported: W/kW/MW (power) and Wh/kWh/MWh/GWh (energy). A browser console warning is logged if a power value exceeds 500 kW after unit conversion, which may indicate a unit detection issue
- The card is designed for the HA **Sections** dashboard type with `grid_options: columns: full`

---

## Related

- [HAEO Integration](https://github.com/energypatrikf/home-assistant-energy-optimizer)
- [Sigenergy Local Modbus Integration](https://github.com/TypQxQ/Sigenergy-Local-Modbus)
- [Energy Manager Card](../EM) — equivalent card for the EMHASS energy manager
