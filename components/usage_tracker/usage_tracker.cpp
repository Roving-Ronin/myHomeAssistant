#include "usage_timer.h"
#include "esphome/core/log.h"

namespace esphome {
namespace usage_timer {

static const char *const TAG = "usage_timer";

void UsageTimer::setup() {
  this->was_on_ = this->sensor_->state;
  if (this->was_on_) {
    this->on_start_time_ = millis();
  }
}

void UsageTimer::loop() {
  bool is_on = this->sensor_->state;

  if (is_on && !was_on_) {
    // Just turned on
    on_start_time_ = millis();
    was_on_ = true;
    ESP_LOGD(TAG, "Sensor turned ON");
  } else if (!is_on && was_on_) {
    // Just turned off
    uint32_t duration = millis() - on_start_time_;
    float seconds = duration / 1000.0f;
    ESP_LOGD(TAG, "Sensor turned OFF after %.1fs", seconds);

    if (last_on_duration_sensor_ != nullptr)
      last_on_duration_sensor_->publish_state(seconds);

    total_lifetime_use_sec_ += seconds;
    if (lifetime_use_sensor_ != nullptr)
      lifetime_use_sensor_->publish_state(total_lifetime_use_sec_);

    was_on_ = false;
  }
}

}  // namespace usage_timer
}  // namespace esphome
