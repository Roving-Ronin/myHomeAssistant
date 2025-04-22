#pragma once
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace usage_timer {

class UsageTimer : public Component {
 public:
  void set_sensor(binary_sensor::BinarySensor *sensor) { this->sensor_ = sensor; }
  void set_last_on_duration_sensor(sensor::Sensor *s) { this->last_on_duration_sensor_ = s; }
  void set_lifetime_use_sensor(sensor::Sensor *s) { this->lifetime_use_sensor_ = s; }

  void setup() override;
  void loop() override;

 protected:
  binary_sensor::BinarySensor *sensor_{nullptr};
  sensor::Sensor *last_on_duration_sensor_{nullptr};
  sensor::Sensor *lifetime_use_sensor_{nullptr};

  bool was_on_ = false;
  uint32_t on_start_time_ = 0;
  float total_lifetime_use_sec_ = 0;
};

}  // namespace usage_timer
}  // namespace esphome
