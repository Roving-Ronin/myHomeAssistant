#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/core/schema.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace energy_statistics {

using sensor::Sensor;

class EnergyStatistics : public Component {
 public:
  float get_setup_priority() const override { return setup_priority::DATA; }

  // ESPHome function override to set up the component
  void dump_config() override;
  void setup() override;
  void loop() override;

  // Function to reset statistics
  void reset_statistics();

  // Setters to bind sensors and time components
  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_total(Sensor *sensor) { this->total_ = sensor; }

  // Setter for the save_frequency, called from YAML configuration
  void set_save_frequency(const std::string &save_frequency) {
    this->save_interval_ = parse_save_frequency(save_frequency);
  }

  // Setter for the statistic sensors
  void set_energy_today(Sensor *sensor) { this->energy_today_ = sensor; }
  void set_energy_yesterday(Sensor *sensor) { this->energy_yesterday_ = sensor; }
  void set_energy_week(Sensor *sensor) { this->energy_week_ = sensor; }
  void set_energy_month(Sensor *sensor) { this->energy_month_ = sensor; }
  void set_energy_year(Sensor *sensor) { this->energy_year_ = sensor; }

  // Define the configuration schema to accept the `save_frequency` option
  static const auto CONFIG_SCHEMA;

protected:
  uint32_t save_interval_{300};        // Save every 5min (adjust as needed, based on seconds)
  uint32_t last_save_time_{0};         // Timestamp of the last save
  uint32_t last_warning_time_{0};      // Timestamp of the last warning log

  ESPPreferenceObject pref_;
  time::RealTimeClock *time_;

  // Input sensor
  Sensor *total_{nullptr};

  // Exposed sensors
  Sensor *energy_today_{nullptr};
  Sensor *energy_yesterday_{nullptr};
  Sensor *energy_week_{nullptr};
  Sensor *energy_month_{nullptr};
  Sensor *energy_year_{nullptr};

  bool is_resetting_{false};              // Resetting state flag
  bool prevent_sensor_update_{false};     // To prevent sensor updates
  bool waiting_for_sensor_read_{false};   // Flag to wait for a valid sensor reading after reset

  int energy_week_start_day_{2};          // start day of week configuration
  int energy_month_start_day_{1};         // start day of month configuration
  int energy_year_start_day_{1};          // start day of year configuration

  struct energy_data_t {
    uint16_t current_day_of_year{0};
    float start_today{NAN};
    float start_yesterday{NAN};
    float start_week{NAN};
    float start_month{NAN};
    float start_year{NAN};

    // Add fields to store sensor values in globals
    float energy_today{NAN};
    float energy_yesterday{NAN};
    float energy_week{NAN};
    float energy_month{NAN};
    float energy_year{NAN};
  } energy_;

  void process_(float total);
  void save_();

  // Helper to convert user-supplied save_frequency into seconds
  uint32_t parse_save_frequency(const std::string &frequency) {
    char unit = frequency.back();
    std::string numeric_part = frequency.substr(0, frequency.size() - 1);
    uint32_t value = std::stoul(numeric_part);

    switch (unit) {
      case 's':  // seconds
        return value;
      case 'm':  // minutes
        return value * 60;        // 1 minute = 60 seconds
      case 'h':  // hours
        return value * 3600;      // 1 hour = 3600 seconds
      case 'd':  // days
        return value * 86400;     // 1 day = 86400 seconds
      default:
        ESP_LOGW(TAG, "Invalid save_frequency unit. Using default of 5 minutes.");
        return 300;  // Default to 5 minutes (300 seconds) if invalid unit
    }
  }
};

}  // namespace energy_statistics
}  // namespace esphome
