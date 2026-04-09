# EMHASS Events Card

A custom Home Assistant Lovelace card for [EMHASS](https://emhass.readthedocs.io/) (Energy Management for Home Assistant) that displays MPC optimizer forecasts and historical energy events in a rich, colour-coded table.

Designed for the **Sigenergy + EMHASS MPC** setup documented at [sigenergy.annable.me](https://sigenergy.annable.me/emhass/), with three-tier sensor fallback so it also works with standard EMHASS installations.

![EMHASS Events Card screenshot](screenshot.png)

---

## Features

- **📅 Future Decisions tab** — full MPC forecast horizon showing every 5-minute optimization decision
- **📋 Past Events tab** — historical sensor data with configurable lookback (Today, Yesterday, 24h–7 days)
- **Colour-coded event rows** — instantly see what the optimizer is doing at each timestep
- **HAEO-style flow labels** — descriptive events like `🌞 Solar → 🏠 Home + 🔋 Battery + ⚡ Grid`
- **Status bar** — current SoC, morning/peak SoC forecast, buy/sell prices, net cost, grid alerts
- **Daily summary rows** — kWh totals per day for Load, PV, Grid and Battery
- **Cost/Profit column** — per-row estimated cost or earnings
- **Three-tier sensor fallback** — card YAML config → MPC/Sigenergy sensors → standard EMHASS sensors
- **Auto-refresh** — updates at :01, :06, :11 ... past each hour to align with MPC optimization cycles
- **Sensor debug panel** — expandable diagnostic showing exactly which sensor attributes were found

---

## Requirements

- Home Assistant with a working [EMHASS](https://github.com/davidusb-geek/emhass) installation running `naive-mpc-optim`
- [custom-sidebar](https://github.com/thomasloven/lovelace-card-mod) or any custom card loader (HACS or manual)
- No additional frontend dependencies — pure vanilla JS

---

## Installation

### Manual

1. Copy `emhass-events-card.js` to `/config/www/emhass-events-card.js`
2. In Home Assistant go to **Settings → Dashboards → Resources** and add:
   - URL: `/local/emhass-events-card.js`
   - Type: **JavaScript Module**
3. Hard-refresh your browser (`Ctrl+Shift+R` / `Cmd+Shift+R`)

### HACS (not yet listed)

Manual installation only for now. HACS submission planned.

---

## Basic Usage

```yaml
type: custom:emhass-events-card
grid_options:
  columns: full
  rows: auto
```

That's it for a standard Sigenergy + EMHASS MPC install. The card auto-detects sensors using its three-tier fallback (see below).

---

## Sensor Configuration

The card resolves each sensor through three tiers in priority order:

| Priority | Source | Description |
|---|---|---|
| 1 | Card YAML config | Explicit overrides you provide in the card config |
| 2 | MPC / Sigenergy defaults | `sensor.mpc_*` sensors from the annable.me setup |
| 3 | Standard EMHASS defaults | `sensor.p_batt_forecast`, `sensor.unit_load_cost` etc. |

### Default sensor mapping (Tier 2 — MPC/Sigenergy)

| Role | Default sensor | Attribute | Value key |
|---|---|---|---|
| Battery power forecast | `sensor.mpc_batt_power` | `battery_scheduled_power` | `mpc_batt_power` |
| Battery SoC forecast | `sensor.mpc_batt_soc` | `battery_scheduled_soc` | `mpc_batt_soc` |
| Grid power forecast | `sensor.mpc_grid_power` | `forecasts` | `mpc_grid_power` |
| PV power forecast | `sensor.mpc_pv_power` | `forecasts` | `mpc_pv_power` |
| Load power forecast | `sensor.mpc_load_power` | `forecasts` | `mpc_load_power` |
| Inverter power | `sensor.mpc_inverter_power` | `forecasts` | `mpc_inverter_power` |
| Buy price | `sensor.mpc_general_price` | `unit_load_cost_forecasts` | `mpc_general_price` |
| Sell price | `sensor.mpc_feed_in_price` | `unit_prod_price_forecasts` | `mpc_feed_in_price` |
| Net cost (state only) | `sensor.mpc_cost_fun` | — | — |

### Full YAML override example

```yaml
type: custom:emhass-events-card
grid_options:
  columns: full
  rows: auto

# Power forecast sensors (override Tier 2/3 defaults)
p_batt_forecast:   sensor.mpc_batt_power
p_grid_forecast:   sensor.mpc_grid_power
p_pv_forecast:     sensor.mpc_pv_power
p_load_forecast:   sensor.mpc_load_power
p_inverter:        sensor.mpc_inverter_power
soc_forecast:      sensor.mpc_batt_soc
buy_price:         sensor.mpc_general_price
sell_price:        sensor.mpc_feed_in_price
net_cost:          sensor.mpc_cost_fun

# Energy sensors for kWh delta columns in Past Events tab
# Use lifetime/total sensors wherever possible to avoid midnight reset gaps
energy_load:           sensor.sigen_plant_total_load_consumption
energy_solar:          sensor.sigen_plant_total_pv_generation
energy_grid_import:    sensor.sigen_plant_total_imported_energy
energy_grid_export:    sensor.sigen_plant_total_exported_energy
energy_batt_charge:    sensor.sigen_plant_daily_battery_charge_energy
energy_batt_discharge: sensor.sigen_plant_daily_battery_discharge_energy
```

> **Note:** Energy sensors are optional. Without them, the kWh columns in the Past Events tab will show `—`. The kW (instantaneous power) columns will still work from the MPC sensor state history.

---

## Sign Conventions

The card is built around the Sigenergy + annable.me MPC sensor conventions:

| Sensor | Positive | Negative |
|---|---|---|
| Battery (`mpc_batt_power`) | Discharging (SoC falls) | Charging (SoC rises) |
| Grid (`mpc_grid_power`) | Import (buying) | Export (selling) |
| PV (`mpc_pv_power`) | Always positive | — |
| Load (`mpc_load_power`) | Always positive | — |

The battery column display **negates** the raw value for intuitive reading:
- Discharging → displayed as `-x.xx kW` in **red**
- Charging → displayed as `x.xx kW` in **green**

---

## Event Classification

Events are classified from the combination of PV, battery, grid and load power at each timestep using a 50 W threshold. Events follow HAEO-style flow notation:

### Solar present

| Event label | Colour | Meaning |
|---|---|---|
| `🌞 Solar → 🏠 Home` | 🟡 Yellow-green | Solar self-consumption only |
| `🌞 Solar → 🏠 Home + 🔋 Battery` | 🟡 Yellow-green | Solar charging battery, no grid |
| `🌞 Solar → 🏠 Home + ⚡ Grid` | 🟡 Yellow-green | Solar surplus exported |
| `🌞 Solar → 🏠 Home + 🔋 Battery + ⚡ Grid` | 🟡 Yellow-green | Solar charging battery and exporting |
| `🌞 Solar + 🔋 Battery → 🏠 Home` | 🩵 Teal | Solar + battery, no grid |
| `🌞 Solar + ⚡ Grid → 🏠 Home` | 🔴 Pink-red | Solar insufficient, grid topping up |
| `🌞 Solar + ⚡ Grid → 🏠 Home + 🔋 Battery (Force)` | 🔴 Pink-red | Forced grid charge alongside solar |
| `🌞 Solar + 🔋 Battery + ⚡ Grid → 🏠 Home` | 🔴 Pink-red | All three sources needed |
| `🌞 Solar + 🔋 Battery → 🏠 Home + ⚡ Grid (Force)` | 🟢 Green | Forced export: solar + battery |

### No solar (night / overcast)

| Event label | Colour | Meaning |
|---|---|---|
| `🔋 Battery → 🏠 Home` | 🩵 Teal | Battery powering home, no grid |
| `🔋 Battery → 🏠 Home + ⚡ Grid (Force)` | 🟡 Yellow | Forced battery export to grid |
| `🔋 Battery + ⚡ Grid → 🏠 Home` | 🔴 Pink-red | Battery + grid covering load |
| `⚡ Grid → 🏠 Home` | 🔴 Red | Grid only, battery idle |
| `⚡ Grid → 🏠 Home + 🔋 Battery (Force)` | 🔴 Red | Forced grid charge (cheap rate) |

---

## Recorder Configuration

For the **Past Events tab** to work, the MPC sensors must be recorded by HA. Add the following to your `recorder.yaml` (or `configuration.yaml` recorder section):

```yaml
recorder:
  include:
    entity_globs:
      - sensor.mpc_*
```

> **Important:** HA recorder only stores data from when the entity is included — there is no backfill of historical data. The Past tab will show increasing data the longer the sensors have been recorded.

If your recorder uses an `exclude:` section, check it doesn't accidentally override the include with a broader glob (e.g. `sensor.*_power`).

---

## Modes Column

The Mode column describes the optimizer's inferred control action at each timestep, derived from the power values:

| Mode | Description |
|---|---|
| `Maximum Self Consumption` | Optimizer prioritising self-use of solar/battery |
| `Command Charging (PV First)` | Forced battery charge — grid import + solar |
| `Command Charging` | Forced battery charge from grid only |
| `Command Discharging (PV First)` | Forced battery discharge to grid with solar |
| `Command Discharging` | Forced battery discharge to grid, no solar |
| `Standby` | Battery idle, no active charging or discharging |

---

## Versioning

| Version | Changes |
|---|---|
| v2.0.4 | Fix battery daily sum sign and colour in both tabs |
| v2.0.3 | HAEO-style event labels, battery display negated (discharge = -kW red), threshold 50 W |
| v2.0.2 | Fix battery sign convention (raw +ve = discharging confirmed by SoC trend) |
| v2.0.1 | Three-tier sensor fallback, auto-detect attribute keys, sensor debug panel |
| v2.0.0 | Initial release — Future Decisions + Past Events tabs, MPC sensor support |

---

## Related Projects

- [EMHASS](https://github.com/davidusb-geek/emhass) — Energy Management for Home Assistant
- [Sigenergy + EMHASS guide](https://sigenergy.annable.me/emhass/) — annable.me setup guide this card is designed for
- [HAEO Events Card](https://github.com/) — the HAEO optimizer card this was modelled on

---

## License

MIT License — see [LICENSE](LICENSE) for details.
