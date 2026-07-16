#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/select/select.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace gas_statistics_mj {

using sensor::Sensor;
using select::Select;

class GasStatisticsMJ : public Component {
 public:
  float get_setup_priority() const override { return setup_priority::DATA; }
  void dump_config() override;
  void setup() override;
  void loop() override;
  void on_shutdown() override;

  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_total(Sensor *sensor) { this->total_ = sensor; }

  // Optional select entity (e.g. a template select rendered as a dropdown
  // on the web_server GUI) holding the day-of-month (options "1".."31") on
  // which the quarter accumulator resets. Automatically clamped to the real
  // length of the reset month (e.g. 31 falls back to the 28th/29th in
  // February). Defaults to day 1 if unset or unparsable.
  void set_quarter_reset_day(Select *select) { this->quarter_reset_day_ = select; }

  // Optional select entity holding the "anchor" month (options "January"..
  // "December") for the first quarter. The other three quarter-start months
  // are anchor+3, anchor+6, anchor+9 (e.g. anchor=February => Feb/May/Aug/
  // Nov). Defaults to January if unset or unparsable.
  void set_quarter_start_month(Select *select) { this->quarter_start_month_ = select; }

  void set_gas_today(Sensor *sensor) { this->gas_today_ = sensor; }
  void set_gas_yesterday(Sensor *sensor) { this->gas_yesterday_ = sensor; }
  void set_gas_week(Sensor *sensor) { this->gas_week_ = sensor; }
  void set_gas_month(Sensor *sensor) { this->gas_month_ = sensor; }
  void set_gas_quarter(Sensor *sensor) { this->gas_quarter_ = sensor; }
  void set_gas_year(Sensor *sensor) { this->gas_year_ = sensor; }

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
  Select *quarter_reset_day_{nullptr};
  Select *quarter_start_month_{nullptr};

  // Exposed sensors
  Sensor *gas_today_{nullptr};
  Sensor *gas_yesterday_{nullptr};
  Sensor *gas_week_{nullptr};
  Sensor *gas_month_{nullptr};
  Sensor *gas_quarter_{nullptr};
  Sensor *gas_year_{nullptr};

  // Start day of week configuration
  int gas_week_start_day_{2};
  // Start day of month configuration
  int gas_month_start_day_{1};
  // Start day of year configuration
  int gas_year_start_day_{1};
  // Fallback reset day-of-month used when quarter_reset_day_ is not set
  // or has not yet published a valid state. Range 1-31; automatically
  // clamped to the number of days in the current reset month.
  int gas_quarter_reset_day_default_{1};
  // Fallback "anchor" month (1-12) for the first quarter-start month, used
  // when quarter_start_month_ is not set or has not yet published a valid
  // state. The other three quarter-start months are anchor+3, anchor+6,
  // anchor+9.
  int gas_quarter_start_month_default_{1};

  // Structure for storing gas statistics in megajoules
  struct gas_mj_data_t {
    uint16_t current_day_of_year{0};
    float start_today{NAN};
    float start_yesterday{NAN};
    float start_week{NAN};
    float start_month{NAN};
    float start_quarter{NAN};
    float start_year{NAN};
  } gas_;

  // Store last published values for change detection
  float last_today_{NAN};
  float last_yesterday_{NAN};
  float last_week_{NAN};
  float last_month_{NAN};
  float last_quarter_{NAN};
  float last_year_{NAN};

  int get_quarter_start_month_();
  int get_quarter_reset_day_(int year, int month);
  bool is_quarter_start_month_(int month);
  static int days_in_month_(int year, int month);
  static int month_name_to_number_(const std::string &name);
  void process_(float total, bool is_initial_restore = false);
  void retry_sntp_sync_();
};

}  // namespace gas_statistics_mj
}  // namespace esphome
