#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "water_statistics.h"

namespace esphome {
namespace water_statistics {

static const char *const TAG = "water_statistics";
static const char *const GAP = "  ";

// Time between warning log messages being repeated (in milliseconds)
static const uint32_t WARNING_LOG_INTERVAL = 60000;  // 60 seconds

void WaterStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Water Statistics (L) - Sensors");
  if (this->water_today_) {
    LOG_SENSOR(GAP, "Water Today", this->water_today_);
  }
  if (this->water_yesterday_) {
    LOG_SENSOR(GAP, "Water Yesterday", this->water_yesterday_);
  }
  if (this->water_week_) {
    LOG_SENSOR(GAP, "Water Week", this->water_week_);
  }
  if (this->water_month_) {
    LOG_SENSOR(GAP, "Water Month", this->water_month_);
  }
  if (this->water_year_) {
    LOG_SENSOR(GAP, "Water Year", this->water_year_);
  }

  ESP_LOGCONFIG(TAG, "Restored Water Today (L): %.3f", this->water_.water_today);
  ESP_LOGCONFIG(TAG, "Restored Water Yesterday (L): %.3f", this->water_.water_yesterday);
  ESP_LOGCONFIG(TAG, "Restored Water Week (L): %.3f", this->water_.water_week);
  ESP_LOGCONFIG(TAG, "Restored Water Month (L): %.3f", this->water_.water_month);
  ESP_LOGCONFIG(TAG, "Restored Water Year (L): %.3f", this->water_.water_year);
}

void WaterStatistics::setup() {
  this->pref_ = global_preferences->make_preference<water_data_t>(fnv1_hash(TAG));

  water_data_t loaded{};
  if (this->pref_.load(&loaded) && !this->is_resetting_) {
    this->water_ = loaded;

    if (this->water_today_ && !std::isnan(this->water_.water_today)) {
      this->water_today_->publish_state(this->water_.water_today);
    }
    if (this->water_yesterday_ && !std::isnan(this->water_.water_yesterday)) {
      this->water_yesterday_->publish_state(this->water_.water_yesterday);
    }
    if (this->water_week_ && !std::isnan(this->water_.water_week)) {
      this->water_week_->publish_state(this->water_.water_week);
    }
    if (this->water_month_ && !std::isnan(this->water_.water_month)) {
      this->water_month_->publish_state(this->water_.water_month);
    }
    if (this->water_year_ && !std::isnan(this->water_.water_year)) {
      this->water_year_->publish_state(this->water_.water_year);
    }

    auto total = this->total_->get_state();
    if (!std::isnan(total)) {
      this->process_(total);
    }
  } else {
    reset_statistics(); // Ensure reset if resetting
  }
}

void WaterStatistics::loop() {
  const auto t = this->time_->now();
  if (!t.is_valid()) {
    return;
  }

  const auto total = this->total_->get_state();
  if (std::isnan(total)) {
    return;
  }

  // If sensor updates are prevented, skip this loop
  if (this->prevent_sensor_update_) {
    this->prevent_sensor_update_ = false;
    return;
  }

  // Check if a new day has started
  if (t.day_of_year != this->water_.current_day_of_year) {
    // Update start points for new day, week, month, year
    this->water_.start_yesterday = this->water_.start_today;
    this->water_.start_today = total;
    ESP_LOGI(TAG, "Water Statistics (L) - New DAY detected, resetting start_today to %.3f", total);

    if (t.day_of_week == this->water_week_start_day_) {
      this->water_.start_week = total;
      ESP_LOGI(TAG, "Water Statistics - New WEEK detected, resetting start_week to %.3f", total);
    }
    if (t.day_of_month == this->water_month_start_day_) {
      this->water_.start_month = total;
      ESP_LOGI(TAG, "Water Statistics - New MONTH detected, resetting start_week to %.3f", total);
    }
    if (t.day_of_year == this->water_year_start_day_) {
      this->water_.start_year = total;
      ESP_LOGI(TAG, "Water Statistics - New YEAR detected, resetting start_week to %.3f", total);
    }

    this->water_.current_day_of_year = t.day_of_year;  // Update the current day
    this->process_(total);  // Process new water values
  }

  // Continue processing the total sensor value
  this->process_(total);

  // Only save if save interval has passed
  uint32_t now = millis();
  if (now - last_save_time_ >= save_interval_ * 1000) {
    this->save_();
    last_save_time_ = now;
  }
}


void WaterStatistics::process_(float total) {
  uint32_t now = millis();  // Get the current time

  // If we're waiting for the sensor to update, skip calculation until valid
  if (this->waiting_for_sensor_read_) {
    if (std::isnan(total) || total == 0.0) {
      // Only log the warning once per minute
      if (now - this->last_warning_time_ >= WARNING_LOG_INTERVAL) {
        ESP_LOGW(TAG, "Water Statistics (L) - Skipping sensor reading update, waiting for valid sensor reading.");
        this->last_warning_time_ = now;  // Update the last warning log time
      }
      return;
    }

    // Set the start values to the current total when a valid reading is received
    this->water_.start_today = total;
    this->water_.start_yesterday = total;
    this->water_.start_week = total;
    this->water_.start_month = total;
    this->water_.start_year = total;

    this->waiting_for_sensor_read_ = false;  // Disable the wait flag
    ESP_LOGI(TAG, "Water Statistics (L) - Valid sensor reading obtained: %.3f", total);
  }

  // Add grace period to avoid immediate reset on small fluctuations
  static uint32_t last_reset_attempt = 0;
  if (total < this->water_.start_today || total < this->water_.start_week || total < this->water_.start_month || total < this->water_.start_year) {
    if (now - last_reset_attempt >= 30000) {  // Grace period of 30 seconds before reset
      ESP_LOGW(TAG, "Water Statistics (L) - Total is less than start points, resetting all start values after grace period.");
      this->water_.start_today = total;
      this->water_.start_week = total;
      this->water_.start_month = total;
      this->water_.start_year = total;
      last_reset_attempt = now;
    } else {
      ESP_LOGI(TAG, "Water Statistics (L) - Ignoring minor fluctuation, not resetting within grace period.");
    }
    return;
  }

  
  // Ensure total is greater than or equal to start points
  if (total < this->water_.start_today) {
    ESP_LOGW(TAG, "Water Statistics (L) - Total is less than start point, resetting start_today.");
    this->water_.start_today = total;
    return;  // Return here to ensure the negative value does not get calculated
  }

  if (total < this->water_.start_week) {
    ESP_LOGW(TAG, "Water Statistics (L) - Total is less than start point, resetting start_week.");
    this->water_.start_week = total;
    return;
  }

  if (total < this->water_.start_month) {
    ESP_LOGW(TAG, "Water Statistics (L) - Total is less than start point, resetting start_month.");
    this->water_.start_month = total;
    return;
  }

  if (total < this->water_.start_year) {
    ESP_LOGW(TAG, "Water Statistics (L) - Total is less than start point, resetting start_year.");
    this->water_.start_year = total;
    return;
  }

  // Log the current start points for debugging
  ESP_LOGI(TAG, "Water Statistics - Start Values: Today: %.3f, Week: %.3f, Month: %.3f, Year: %.3f", 
            this->water_.start_today, this->water_.start_week, this->water_.start_month, this->water_.start_year);
  
  // Calculate and update water usage for each period (Today, Week, Month, Year) individually
  if (this->water_today_ && !std::isnan(this->water_.start_today)) {
    float new_water_today = total - this->water_.start_today;
    if (new_water_today >= 0.0) {
      this->water_.water_today = new_water_today;
      this->water_today_->publish_state(this->water_.water_today);
      ESP_LOGI(TAG, "Water Statistics (L) - Water Usage - Today: %.3f L", new_water_today);
    }
  }

  if (this->water_week_ && !std::isnan(this->water_.start_week)) {
    float new_water_week = total - this->water_.start_week;
    if (new_water_week >= 0.0) {
      this->water_.water_week = new_water_week;
      this->water_week_->publish_state(this->water_.water_week);
      ESP_LOGI(TAG, "Water Statistics (L) - Water Usage - Week: %.3f L", new_water_week);
    }
  }

  if (this->water_month_ && !std::isnan(this->water_.start_month)) {
    float new_water_month = total - this->water_.start_month;
    if (new_water_month >= 0.0) {
      this->water_.water_month = new_water_month;
      this->water_month_->publish_state(this->water_.water_month);
      ESP_LOGI(TAG, "Water Statistics (L) - Water Usage - Month: %.3f L", new_water_month);
    }
  }

  if (this->water_year_ && !std::isnan(this->water_.start_year)) {
    float new_water_year = total - this->water_.start_year;
    if (new_water_year >= 0.0) {
      this->water_.water_year = new_water_year;
      this->water_year_->publish_state(this->water_.water_year);
      ESP_LOGI(TAG, "Water Statistics (L) - Water Usage - Year: %.3f L", new_water_year);
    }
  }

  // Save to flash memory if necessary
  if (now - last_save_time_ >= save_interval_ * 1000) {
    this->save_();
    last_save_time_ = now;
    ESP_LOGI(TAG, "Water Statistics - Values saved to flash (NVS) memory.");
  }
}



void WaterStatistics::reset_statistics() {
  uint32_t now = millis();        // Get the current time
  ESP_LOGI(TAG, "Water Statistics (L) - Resetting values to 0.0");

  // Reset water values to 0.0
  this->water_.water_today = 0.0;
  this->water_.water_yesterday = 0.0;
  this->water_.water_week = 0.0;
  this->water_.water_month = 0.0;
  this->water_.water_year = 0.0;

  // Get the current total value
  const auto total = this->total_->get_state();
  
  if (!std::isnan(total) && total != 0.0) {
    // Use the current total value as the new start points
    this->water_.start_today = total;
    this->water_.start_yesterday = total;
    this->water_.start_week = total;
    this->water_.start_month = total;
    this->water_.start_year = total;
    ESP_LOGI(TAG, "Water Statistics (L) - Start points set after reset: %.3f", total);
  } else {
    // If total is not valid, flag to wait for a valid reading
    this->waiting_for_sensor_read_ = true;
    if (now - this->last_warning_time_ >= WARNING_LOG_INTERVAL) {
      ESP_LOGW(TAG, "Water Statistics (L) - Total is invalid, waiting for valid sensor reading.");
      this->last_warning_time_ = now;  // Update the last warning log time
    }
  }

  // Publish the reset values to sensors
  if (this->water_today_) this->water_today_->publish_state(0.0);
  if (this->water_yesterday_) this->water_yesterday_->publish_state(0.0);
  if (this->water_week_) this->water_week_->publish_state(0.0);
  if (this->water_month_) this->water_month_->publish_state(0.0);
  if (this->water_year_) this->water_year_->publish_state(0.0);

  // Save reset state to flash memory
  this->save_();
}


void WaterStatistics::save_() {
  this->pref_.save(&this->water_); // Save to flash memory
  ESP_LOGD(TAG, "Water Statistics (L) - Values saved to flash memory (NVS)."); // Log message indicating save action
}

}  // namespace water_statistics
}  // namespace esphome
