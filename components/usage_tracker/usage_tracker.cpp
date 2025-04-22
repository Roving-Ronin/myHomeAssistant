#include "usage_tracker.h"
#include "esphome/core/log.h"

namespace esphome {
namespace usage_tracker {

static const char *const TAG = "usage_tracker";

static const uint32_t SAVE_INTERVAL_MS = 5 * 60 * 1000;  // 5 minutes

void UsageTracker::setup() {
  // Restore previous total lifetime usage
  lifetime_use_pref_ = global_preferences.make_preference<float>(this->get_object_id_hash());
  lifetime_use_pref_.load(&total_lifetime_use_);
  ESP_LOGD(TAG, "Restored lifetime use: %.1f seconds", total_lifetime_use_);

  this->was_on_ = this->sensor_->state;
  if (this->was_on_) {
    this->on_start_time_ = millis();
    ESP_LOGD(TAG, "Initial sensor state is ON");
  }

  this->last_save_time_ = millis();
}

void UsageTracker::loop() {
  uint32_t now = millis();
  bool is_on = this->sensor_->state;

  // Handle state transition
  if (is_on && !this->was_on_) {
    this->on_start_time_ = now;
    this->was_on_ = true;
    ESP_LOGD(TAG, "Sensor turned ON");
  } else if (!is_on && this->was_on_) {
    float duration = (now - this->on_start_time_) / 1000.0f;

    if (this->last_on_duration_sensor_ != nullptr)
      this->last_on_duration_sensor_->publish_state(duration);

    this->total_lifetime_use_ += duration;
    ESP_LOGD(TAG, "Sensor was ON for %.1f seconds. Total: %.1f", duration, total_lifetime_use_);

    if (this->lifetime_use_sensor_ != nullptr)
      this->lifetime_use_sensor_->publish_state(total_lifetime_use_);

    this->was_on_ = false;
  }

  // Save to NVS every 5 minutes if there's been any usage
  if ((now - this->last_save_time_) >= SAVE_INTERVAL_MS) {
    this->lifetime_use_pref_.save(&total_lifetime_use_);
    this->last_save_time_ = now;
    ESP_LOGD(TAG, "Saved lifetime use to NVS: %.1f seconds", total_lifetime_use_);
  }
}
}  // namespace usage_tracker
}  // namespace esphome
