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
}

void UsageTracker::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Usage Tracker...");

  this->lifetime_use_pref_ = global_preferences->make_preference<float>(fnv1_hash("usage_tracker_lifetime"));
  if (!this->lifetime_use_pref_.load(&this->total_lifetime_use_)) {
    ESP_LOGI(TAG, "Usage Tracker - No saved lifetime use found, starting at 0");
    this->total_lifetime_use_ = 0;
  } else {
    ESP_LOGI(TAG, "Usage Tracker - Loaded lifetime use: %.0f s", this->total_lifetime_use_);
  }

  // Publish restored value after boot
  if (this->lifetime_use_sensor_ != nullptr) {
    this->lifetime_use_sensor_->publish_state(this->total_lifetime_use_);
  }

  this->last_save_time_ = millis();
}


void UsageTracker::loop() {
  const uint32_t now = millis();

  if (this->sensor_ == nullptr) {
    ESP_LOGE(TAG, "Usage Tracker - No sensor configured!");
    return;
  }

  bool is_on = this->sensor_->state;

  // Detect transition ON -> OFF
  if (this->was_on_ && !is_on) {
    this->was_on_ = false;
    uint32_t duration = (now - this->on_start_time_) / 1000;
    // Handle potential millis overflow
    //uint32_t duration = (now >= this->on_start_time_)
    //                       ? (now - this->on_start_time_) / 1000
    //                       : (0xFFFFFFFF - this->on_start_time_ + now + 1) / 1000;
    ESP_LOGD(TAG, "Usage Tracker - Sensor OFF, last on duration: %u s", duration);

    if (this->last_use_sensor_ != nullptr) {
      this->last_use_sensor_->publish_state(duration);
    }

    this->total_lifetime_use_ += duration;

    if (this->lifetime_use_sensor_ != nullptr) {
      this->lifetime_use_sensor_->publish_state(this->total_lifetime_use_);
    }
  }

  // Detect transition OFF -> ON
  if (!this->was_on_ && is_on) {
    this->was_on_ = true;
    this->on_start_time_ = now;
    ESP_LOGD(TAG, "Usage Tracker - Source sensor turned ON");
  }

  // Periodic save to NVS
  if ((now - this->last_save_time_) > 300000) {  // 5 minutes
    this->last_save_time_ = now;
    this->lifetime_use_pref_.save(&this->total_lifetime_use_);
    ESP_LOGD(TAG, "Usage Tracker - Saved lifetime use to NVS: %.0f s", this->total_lifetime_use_);
  }
}

}  // namespace usage_tracker
}  // namespace esphome
