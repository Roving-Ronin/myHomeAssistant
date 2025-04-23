#include "usage_tracker.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome {
namespace usage_tracker {

static const char *const TAG = "usage_tracker";

void UsageTracker::dump_config() {
  ESP_LOGCONFIG(TAG, "Usage Tracker:");
  if (this->sensor_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Watched binary sensor: %s", this->sensor_->get_name().c_str());
  }
  if (this->last_use_sensor_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Last ON duration sensor: %s", this->last_use_sensor_->get_name().c_str());
  }
  if (this->lifetime_use_sensor_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Lifetime use sensor: %s", this->lifetime_use_sensor_->get_name().c_str());
    ESP_LOGCONFIG(TAG, "  Current lifetime total: %.0f s", this->total_lifetime_use_);
  }
  if (this->time_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Time component configured");
  }
}

void UsageTracker::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Usage Tracker...");

  if (this->time_ == nullptr) {
    ESP_LOGE(TAG, "Usage Tracker - No time component configured!");
    return;
  }

  // Initialize NVS preferences
  this->lifetime_use_pref_ = global_preferences->make_preference<float>(fnv1_hash("usage_tracker_lifetime"));
  this->on_start_time_pref_ = global_preferences->make_preference<uint32_t>(fnv1_hash("usage_tracker_on_start"));

  // Load lifetime use
  if (!this->lifetime_use_pref_.load(&this->total_lifetime_use_)) {
    ESP_LOGI(TAG, "Usage Tracker - No saved lifetime use found, starting at 0");
    this->total_lifetime_use_ = 0;
  } else {
    ESP_LOGI(TAG, "Usage Tracker - Loaded lifetime use: %.0f s", this->total_lifetime_use_);
  }

  // Load ON start time and handle reboot
  uint32_t saved_on_start_time = 0;
  if (this->on_start_time_pref_.load(&saved_on_start_time)) {
    if (saved_on_start_time > 0 && this->sensor_ != nullptr && this->sensor_->state) {
      // Device was ON during reboot
      this->was_on_ = true;
      this->on_start_time_ = saved_on_start_time;
      ESP_LOGI(TAG, "Usage Tracker - Restored ON state from reboot, start time: %u", this->on_start_time_);
    } else {
      // Clear saved start time if device was OFF
      saved_on_start_time = 0;
      this->on_start_time_pref_.save(&saved_on_start_time);
    }
  }

  // Publish restored lifetime value
  if (this->lifetime_use_sensor_ != nullptr) {
    this->lifetime_use_sensor_->publish_state(this->total_lifetime_use_);
  }

  this->last_save_time_ = millis();
}

void UsageTracker::loop() {
  if (this->sensor_ == nullptr || this->time_ == nullptr) {
    ESP_LOGE(TAG, "Usage Tracker - Missing sensor or time component!");
    return;
  }

  const uint32_t now = this->time_->now().timestamp;  // Epoch time in seconds
  bool is_on = this->sensor_->state;

  // Detect transition ON -> OFF
  if (this->was_on_ && !is_on) {
    this->was_on_ = false;
    uint32_t duration = now - this->on_start_time_;

    ESP_LOGD(TAG, "Usage Tracker - Sensor OFF, last on duration: %u s", duration);

    if (this->last_use_sensor_ != nullptr) {
      this->last_use_sensor_->publish_state(duration);
    }

    this->total_lifetime_use_ += duration;

    if (this->lifetime_use_sensor_ != nullptr) {
      this->lifetime_use_sensor_->publish_state(this->total_lifetime_use_);
    }

    // Clear saved ON start time
    uint32_t zero = 0;
    this->on_start_time_pref_.save(&zero);
  }

  // Detect transition OFF -> ON
  if (!this->was_on_ && is_on) {
    this->was_on_ = true;
    this->on_start_time_ = now;
    this->on_start_time_pref_.save(&this->on_start_time_);  // Save start time to NVS
    ESP_LOGD(TAG, "Usage Tracker - Source sensor turned ON at %u", this->on_start_time_);
  }

  // Periodic save to NVS
  if (millis() - this->last_save_time_ > 300000) {  // 5 minutes
    this->last_save_time_ = millis();
    this->lifetime_use_pref_.save(&this->total_lifetime_use_);
    ESP_LOGD(TAG, "Usage Tracker - Saved lifetime use to NVS: %.0f s", this->total_lifetime_use_);
  }
}

}  // namespace usage_tracker
}  // namespace esphome
