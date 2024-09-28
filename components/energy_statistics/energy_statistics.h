#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace energy_statistics {

using sensor::Sensor;

class EnergyStatistics : public Component {  // Inherit only from Component
 public:
  // Set up priority to ensure this component is set up early
  float get_setup_priority() const override { return setup_priority::DATA; }
  
  // Override component lifecycle methods
  void dump_config() override;
  void setup() override;
  void loop() override;

  // Methods for resetting and handling reset service call
  void on_reset_called();
  void reset();

  // Setters for time and total sensor
  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_total(Sensor *sensor) { this->total_ = sensor; }

  // Setters for energy sensors
  void set_energy_today(Sensor *sensor) { this->energy_today_ = sensor; }
  void set_energy_yesterday(Sensor *sensor) { this->energy_yesterday_ = sensor; }
  void set_energy_week(Sensor *sensor) { this->energy_week_ = sensor; }
  void set_energy_month(Sensor *sensor) { this->energy_month_ = sensor; }
  void set_energy_year(Sensor *sensor) { this->energy_year_ = sensor; }

 protected:
  // Preferences for storing persistent data
  ESPPreferenceObject pref_;
  time::RealTimeClock *time_;

  // Input sensor for total energy
  Sensor *total_{nullptr};

  // Output sensors for energy tracking
  Sensor *energy_today_{nullptr};
  Sensor *energy_yesterday_{nullptr};
  Sensor *energy_week_{nullptr};
  Sensor *energy_month_{nullptr};
  Sensor *energy_year_{nullptr};

  // Configurable start days for week, month, and year
  int energy_week_start_day_{2};
  int energy_month_start_day_{1};
  int energy_year_start_day_{1};

  // Structure to store energy data
  struct energy_data_t {
    uint16_t current_day_of_year{0};  // Track current day of the year
    float start_today{NAN};           // Starting total energy for today
    float start_yesterday{NAN};       // Starting total energy for yesterday
    float start_week{NAN};            // Starting total energy for the week
    float start_month{NAN};           // Starting total energy for the month
    float start_year{NAN};            // Starting total energy for the year

    // Sensor values for energy metrics
    float energy_today{NAN};
    float energy_yesterday{NAN};
    float energy_week{NAN};
    float energy_month{NAN};
    float energy_year{NAN};
  } energy_;

  // Process new total energy and update values
  void process_(float total);
  
  // Save energy data to persistent storage
  void save_();
};  // class EnergyStatistics

}  // namespace energy_statistics
}  // namespace esphome
