#include "usage_tracker.h"
#include "esphome/core/log.h"

namespace esphome {
namespace usage_tracker {

static const char *const TAG = "usage_tracker";

void UsageTracker::setup() {
  // Restore previous total lifetime usage
  lifetime_use_pref_ = global_preferences.make_preference<float>(this->get_object_id_hash());
  lifetime_use_pref_.load(&total_lifetime_use_);
  ESP_LOGD(TAG, "Restored lifetime use: %.1f seconds", total_lifetime_use_);

  // Initialize state
  this->was_on_ = this->sensor_->state;
  if (this->was_on_) {
    this->on_start_time_ = millis();
    ESP_LOGD(TAG, "Initial sensor state is ON");
  }
}

void UsageTracker::loop() {
  bool is_on = this->sensor_->state;

  if (is_on && !this->was_on_) {
    // Turned ON
    this->on_start_time_ = millis();
    this->was_on_ = true;
    ESP_LOGD(TAG, "Sensor turned ON");
  } else if (!is_on && this->was_on_) {
    // Turned OFF
    uint32_t now = millis();
    float duration = (now - this->on_start_time_) / 1000.0f;

    if (this->last_on_duration_sensor_ != nullptr)
      this->last_on_duration_sensor_->publish_state(duration);

    this->total_lifetime_use_ += duration;
    ESP_LOGD(TAG, "Sensor was ON for %.1f seconds. Total: %.1f", duration, total_lifetime_use_);

    // Save and publish
    if (this->lifetime_use_sensor_ != nullptr)
      this->lifetime_use_sensor_->publish_state(total_lifetime_use_);
    this->lifetime_use_pref_.save(&total_lifetime_use_);

    this->was_on_ = false;
  }
}

}  // namespace usage_tracker
}  // namespace esphome
