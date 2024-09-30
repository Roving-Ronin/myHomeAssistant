#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace energy_statistics {

using sensor::Sensor;

class EnergyStatistics : public Component {
 public:
  float get_setup_priority() const override { return setup_priority::DATA; }
  void dump_config() override;
  void setup() override;
  void loop() override;

  void reset_statistics();

  // Add option for save (to flash memory) frequency
  void set_save_frequency(uint32_t save_frequency_in_seconds) {
    this->save_interval_ = this->parse_save_frequency(frequency_str);
  }

  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_total(Sensor *sensor) { this->total_ = sensor; }

  void set_energy_today(Sensor *sensor) { this->energy_today_ = sensor; }
  void set_energy_yesterday(Sensor *sensor) { this->energy_yesterday_ = sensor; }
  void set_energy_week(Sensor *sensor) { this->energy_week_ = sensor; }
  void set_energy_month(Sensor *sensor) { this->energy_month_ = sensor; }
  void set_energy_year(Sensor *sensor) { this->energy_year_ = sensor; }

protected:
  // Method to parse the save_frequency (e.g., "5m", "2h") from YAML
  uint32_t parse_save_frequency(const std::string &str);
  // Save every 5min (adjust as needed, based on seconds)
  uint32_t save_interval_{300};
  // Timestamp of the last save
  uint32_t last_save_time_{0};

  ESPPreferenceObject pref_;
  time::RealTimeClock *time_;

  // input sensors
  Sensor *total_{nullptr};

  // exposed sensors
  Sensor *energy_today_{nullptr};
  Sensor *energy_yesterday_{nullptr};
  Sensor *energy_week_{nullptr};
  Sensor *energy_month_{nullptr};
  Sensor *energy_year_{nullptr};

  // Resetting state flag
  bool is_resetting_{false};
  // To prevent sensor updates, whilst resetting
  bool prevent_sensor_update_{false}; // Add this line if you want to keep this functionality

  // start day of week configuration
  int energy_week_start_day_{2};
  // start day of month configuration
  int energy_month_start_day_{1};
  // start day of year configuration
  int energy_year_start_day_{1};

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
};  // class EnergyStatistics

}  // namespace energy_statistics
}  // namespace esphome
