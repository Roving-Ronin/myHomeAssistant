#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace usage_tracker {

using sensor::Sensor;

class UsageTracker : public Component {
 public:
  void set_sensor(binary_sensor::BinarySensor *sensor) { this->sensor_ = sensor; }
  void set_last_on_duration_sensor(Sensor *sensor) { this->last_on_duration_sensor_ = sensor; }
  void set_lifetime_use_sensor(Sensor *sensor) { this->lifetime_use_sensor_ = sensor; }

  void setup() override;
  void loop() override;

 protected:
  binary_sensor::BinarySensor *sensor_{nullptr};
  Sensor *last_on_duration_sensor_{nullptr};
  Sensor *lifetime_use_sensor_{nullptr};

  bool was_on_{false};
  uint32_t on_start_time_{0};
  float total_lifetime_use_{0};

  ESPPreferenceObject lifetime_use_pref_;
  uint32_t last_save_time_{0};
};

}  // namespace usage_tracker
}  // namespace esphome
