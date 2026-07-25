#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace water_statistics {

using sensor::Sensor;

class WaterStatistics : public Component {
 public:
  float get_setup_priority() const override { return setup_priority::DATA; }
  void dump_config() override;
  void setup() override;
  void loop() override;
  void on_shutdown() override;

  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_total(Sensor *sensor) { this->total_ = sensor; }

  void set_water_today(Sensor *sensor) { this->water_today_ = sensor; }
  void set_water_yesterday(Sensor *sensor) { this->water_yesterday_ = sensor; }
  void set_water_week(Sensor *sensor) { this->water_week_ = sensor; }
  void set_water_month(Sensor *sensor) { this->water_month_ = sensor; }
  void set_water_year(Sensor *sensor) { this->water_year_ = sensor; }

  // Optional - only wire this up for instances that track a billing quarter
  // (e.g. Town Water). Unlike gas, there's no automatic day/month reset
  // pattern here: real water billing periods read once per quarter but on
  // irregular dates (meter-reading-route dependent, not a fixed calendar
  // day), so the quarter only ever advances via a manual reset_quarter()
  // call (e.g. a "Reset Quarter" button pressed whenever a new bill/meter
  // read arrives). If this isn't set, the quarter machinery is simply
  // unused for that instance (e.g. Tank Water).
  void set_water_quarter(Sensor *sensor) { this->water_quarter_ = sensor; }

  /** Manually (re)start the quarter accumulator - e.g. from a "Reset
   * Quarter" button, pressed whenever a new water bill/meter read period
   * begins. If already_consumed is 0 (the default), the quarter baseline
   * snaps to the current total, so the quarter sensor reads 0 going
   * forward. If already_consumed is non-zero (e.g. computed from a
   * last-bill-reading / current-meter-reading pair), the baseline is
   * backdated so the quarter sensor immediately reflects that real
   * consumption-so-far figure. Also records "now" as the quarter's start
   * date, used by days_into_quarter(). Callable directly from a YAML lambda
   * via id(component).reset_quarter(...).
   */
  void reset_quarter(float already_consumed = 0.0f);

  /** Same as reset_quarter(), but backdates the quarter start to a specific
   * calendar date instead of "now" - used when a "Quarter Start Date"
   * datetime entity is set to the real billing period start date, which is
   * often only known once the actual bill arrives, days or weeks into the
   * new period. Callable directly from a YAML lambda via
   * id(component).reset_quarter_from_date(...).
   */
  void reset_quarter_from_date(float already_consumed, uint16_t year, uint8_t month, uint8_t day);

  /** Calibrate the lifetime total to match a physical meter reading. Shifts
   * the today/yesterday/week/month/year baselines by the same delta as the
   * total, so those sensors keep reporting the consumption they'd already
   * tracked instead of showing the calibration jump as a spurious spike.
   * start_quarter is intentionally left untouched - call reset_quarter()
   * separately (typically right after this) if the quarter baseline also
   * needs to move. The caller is still responsible for actually updating
   * the total_ sensor itself (e.g. via a global + .update()).
   */
  void calibrate_total(float new_total);

  /** Number of days elapsed since the current quarter started (the day
   * reset_quarter() was called counts as day 1). Used by pricing lambdas to
   * scale a per-day rate/limit (e.g. "0.80 kL/day") into a cumulative
   * figure for the quarter so far. Returns 0 if reset_quarter() has never
   * been called on this instance.
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
  Sensor *water_today_{nullptr};
  Sensor *water_yesterday_{nullptr};
  Sensor *water_week_{nullptr};
  Sensor *water_month_{nullptr};
  Sensor *water_year_{nullptr};
  Sensor *water_quarter_{nullptr};

  // Start day of week configuration
  int water_week_start_day_{2};
  // Start day of month configuration
  int water_month_start_day_{1};
  // Start day of year configuration
  int water_year_start_day_{1};

  // Structure for storing water statistics in Litres
  struct water_data_t {
    uint16_t current_day_of_year{0};
    float start_today{NAN};
    float start_yesterday{NAN};
    float start_week{NAN};
    float start_month{NAN};
    float start_year{NAN};
    float start_quarter{NAN};
    // Calendar date the current quarter baseline was established (via
    // reset_quarter()), used by days_into_quarter(). 0 means "never set".
    uint16_t quarter_start_day_of_year{0};
    uint16_t quarter_start_year{0};
  } water_;

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

}  // namespace water_statistics
}  // namespace esphome
