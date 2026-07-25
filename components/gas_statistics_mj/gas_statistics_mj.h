#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace gas_statistics_mj {

using sensor::Sensor;

class GasStatisticsMJ : public Component {
 public:
  float get_setup_priority() const override { return setup_priority::DATA; }
  void dump_config() override;
  void setup() override;
  void loop() override;
  void on_shutdown() override;

  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_total(Sensor *sensor) { this->total_ = sensor; }

  void set_gas_today(Sensor *sensor) { this->gas_today_ = sensor; }
  void set_gas_yesterday(Sensor *sensor) { this->gas_yesterday_ = sensor; }
  void set_gas_week(Sensor *sensor) { this->gas_week_ = sensor; }
  void set_gas_month(Sensor *sensor) { this->gas_month_ = sensor; }
  void set_gas_year(Sensor *sensor) { this->gas_year_ = sensor; }
  void set_gas_quarter(Sensor *sensor) { this->gas_quarter_ = sensor; }

  /** Manually (re)start the quarter accumulator - e.g. from the max-quarter-
   * length fallback in gas.yaml. If already_consumed is 0 (the default), the
   * quarter baseline snaps to the current total, so Gas - Quarter (MJ) reads
   * 0 going forward. If already_consumed is non-zero (e.g. computed from a
   * last-bill-reading / current-meter-reading pair converted to MJ), the
   * baseline is backdated so Gas - Quarter (MJ) immediately reflects that
   * real consumption-so-far figure instead. Also stamps "today" as the
   * quarter start date (see reset_quarter_from_date() for backdating to a
   * specific date instead). Callable directly from a YAML lambda via
   * id(component).reset_quarter(...).
   *
   * Note: unlike the m3 component, there is no calibrate_total() here - the
   * lifetime MJ total is never recalibrated to a single "physical reading",
   * since it's built from per-pulse conversions using whatever Pressure
   * Factor / Heating Value was set at the time, and those change every
   * quarter. It's left to accumulate exactly as it always has.
   */
  void reset_quarter(float already_consumed = 0.0f);

  /** Same as reset_quarter(), but backdates the quarter start to a specific
   * calendar date instead of "today" - used when the "Gas - Quarter Start
   * Date" datetime entity is set to the real billing period start date,
   * which is often only known once the actual bill arrives, days or weeks
   * into the new quarter. Backdating this way means Gas - Quarter (MJ) and
   * the day-scaled pricing thresholds are correct immediately, rather than
   * assuming the quarter only started "today". Callable directly from a
   * YAML lambda via id(component).reset_quarter_from_date(...).
   */
  void reset_quarter_from_date(float already_consumed, uint16_t year, uint8_t month, uint8_t day);

  /** Number of days elapsed since the current quarter started (the day the
   * quarter started counts as day 1). Used by pricing lambdas to scale a
   * per-day MJ threshold (e.g. "20.71 MJ/day") into a cumulative MJ figure
   * for the quarter so far, since the gas retailer's tiers are themselves
   * per-day rates multiplied out by the exact number of days in each bill.
   * Returns 0 if the quarter start date hasn't been established yet (e.g.
   * before "Gas - Quarter Start Date" has ever been set after flashing).
   */
  int days_into_quarter();

 protected:
  ESPPreferenceObject pref_;
  time::RealTimeClock *time_;

  // Non-blocking approach to check and load NVS
  int initial_total_retries_{0};
  bool has_loaded_nvs_{false};
  bool initial_processing_started_{false};
  int sntp_retries_{0};
  bool has_value_changed_{false}; // Tracks if any sensor value changed

  // Input sensors
  Sensor *total_{nullptr};

  // Exposed sensors
  Sensor *gas_today_{nullptr};
  Sensor *gas_yesterday_{nullptr};
  Sensor *gas_week_{nullptr};
  Sensor *gas_month_{nullptr};
  Sensor *gas_year_{nullptr};
  Sensor *gas_quarter_{nullptr};

  // Start day of week configuration
  int gas_week_start_day_{2};
  // Start day of month configuration
  int gas_month_start_day_{1};
  // Start day of year configuration
  int gas_year_start_day_{1};

  // Structure for storing gas statistics in megajoules
  struct gas_mj_data_t {
    uint16_t current_day_of_year{0};
    float start_today{NAN};
    float start_yesterday{NAN};
    float start_week{NAN};
    float start_month{NAN};
    float start_year{NAN};
    float start_quarter{NAN};
    // Calendar date the current quarter baseline was established, used by
    // days_into_quarter(). 0 means "not yet established".
    uint16_t quarter_start_day_of_year{0};
    uint16_t quarter_start_year{0};
  } gas_;

  // Store last published values for change detection
  float last_today_{NAN};
  float last_yesterday_{NAN};
  float last_week_{NAN};
  float last_month_{NAN};
  float last_year_{NAN};
  float last_quarter_{NAN};

  // Shared by reset_quarter() and reset_quarter_from_date(). start_day_of_year
  // / start_year of 0/0 means "use the RTC's current date" (the plain
  // reset_quarter() case); any other value backdates the quarter start to
  // that specific calendar date instead.
  void set_quarter_baseline_(float already_consumed, uint16_t start_day_of_year, uint16_t start_year);
  void process_(float total, bool is_initial_restore = false);
  void retry_sntp_sync_();
};

}  // namespace gas_statistics_mj
}  // namespace esphome
