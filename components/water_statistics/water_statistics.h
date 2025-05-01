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
  } water_;

  // Store last published values for change detection
  float last_today_{NAN};
  float last_yesterday_{NAN};
  float last_week_{NAN};
  float last_month_{NAN};
  float last_year_{NAN};

  void process_(float total, bool is_initial_restore = false);
  void retry_sntp_sync_();
};

}  // namespace water_statistics
}  // namespace esphome
