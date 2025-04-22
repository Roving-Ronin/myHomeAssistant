#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace usage_tracker {

using sensor::Sensor;
using time::RealTimeClock;

class UsageTracker : public Component {
 public:
  // Setup and loop
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Setters
  void set_sensor(binary_sensor::BinarySensor *sensor) { this->sensor_ = sensor; }
  void set_last_use_sensor(sensor::Sensor *sensor) { this->last_use_sensor_ = sensor; }
  void set_lifetime_use_sensor(sensor::Sensor *sensor) { this->lifetime_use_sensor_ = sensor; }
  void set_time(time::RealTimeClock *time) { this->time_ = time; }

 protected:
  // Components
  binary_sensor::BinarySensor *sensor_{nullptr};
  sensor::Sensor *last_use_sensor_{nullptr};
  sensor::Sensor *lifetime_use_sensor_{nullptr};
  RealTimeClock *time_{nullptr};

  // Internal state
  bool was_on_{false};
  uint32_t on_start_time_{0};  // Epoch time in seconds
  float total_lifetime_use_{0};

  // NVS storage
  ESPPreferenceObject lifetime_use_pref_;
  ESPPreferenceObject on_start_time_pref_;
  uint32_t last_save_time_{0};
};

}  // namespace usage_tracker
}  // namespace esphome
