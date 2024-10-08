#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "gas_statistics.h"

namespace esphome {
namespace gas_statistics {

static const char *const TAG = "gas_statistics";
static const char *const GAP = "  ";

// Time between warning log messages being repeated (in milliseconds)
static const uint32_t WARNING_LOG_INTERVAL = 60000;  // 60 seconds

void GasStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Gas Statistics (m³) - Sensors");
  if (this->gas_today_) {
    LOG_SENSOR(GAP, "Gas (m³) Today", this->gas_today_);
  }
  if (this->gas_yesterday_) {
    LOG_SENSOR(GAP, "Gas (m³) Yesterday", this->gas_yesterday_);
  }
  if (this->gas_week_) {
    LOG_SENSOR(GAP, "Gas (m³) Week", this->gas_week_);
  }
  if (this->gas_month_) {
    LOG_SENSOR(GAP, "Gas (m³) Month", this->gas_month_);
  }
  if (this->gas_year_) {
    LOG_SENSOR(GAP, "Gas (m³) Year", this->gas_year_);
  }

  ESP_LOGCONFIG(TAG, "Restored Gas Today (m³): %.3f", this->gas_.gas_today);
  ESP_LOGCONFIG(TAG, "Restored Gas Yesterday (m³): %.3f", this->gas_.gas_yesterday);
  ESP_LOGCONFIG(TAG, "Restored Gas Week (m³): %.3f", this->gas_.gas_week);
  ESP_LOGCONFIG(TAG, "Restored Gas Month (m³): %.3f", this->gas_.gas_month);
  ESP_LOGCONFIG(TAG, "Restored Gas Year (m³): %.3f", this->gas_.gas_year);
}

void GasStatistics::setup() {
  this->pref_ = global_preferences->make_preference<gas_data_t>(fnv1_hash(TAG));

  gas_data_t loaded{};
  if (this->pref_.load(&loaded) && !this->is_resetting_) {
    this->gas_ = loaded;

    if (this->gas_today_ && !std::isnan(this->gas_.gas_today)) {
      this->gas_today_->publish_state(this->gas_.gas_today);
    }
    if (this->gas_yesterday_ && !std::isnan(this->gas_.gas_yesterday)) {
      this->gas_yesterday_->publish_state(this->gas_.gas_yesterday);
    }
    if (this->gas_week_ && !std::isnan(this->gas_.gas_week)) {
      this->gas_week_->publish_state(this->gas_.gas_week);
    }
    if (this->gas_month_ && !std::isnan(this->gas_.gas_month)) {
      this->gas_month_->publish_state(this->gas_.gas_month);
    }
    if (this->gas_year_ && !std::isnan(this->gas_.gas_year)) {
      this->gas_year_->publish_state(this->gas_.gas_year);
    }

    auto total = this->total_->get_state();
    if (!std::isnan(total)) {
      this->process_(total);
    }
  } else {
    reset_statistics(); // Ensure reset if resetting
  }
}

void GasStatistics::loop() {
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
  if (t.day_of_year != this->gas_.current_day_of_year) {
    // Transfer today's gas usage to yesterday before resetting today
    if (this->gas_yesterday_) {
      this->gas_.gas_yesterday = this->gas_.gas_today;  // Transfer today's gas usage to yesterday
      this->gas_yesterday_->publish_state(this->gas_.gas_yesterday);  // Publish the updated yesterday value
    }

    // Reset today's gas usage and update start points for new day, week, month, year
    this->gas_.start_yesterday = this->gas_.start_today;
    this->gas_.start_today = total;
    this->gas_.gas_today = 0.0;  // Reset today's gas usage to 0

    // Update week, month, year start points as necessary
    if (t.day_of_week == this->gas_week_start_day_) {
      this->gas_.start_week = total;
    }
    if (t.day_of_month == this->gas_month_start_day_) {
      this->gas_.start_month = total;
    }
    if (t.day_of_year == this->gas_year_start_day_) {
      this->gas_.start_year = total;
    }

    this->gas_.current_day_of_year = t.day_of_year;  // Update the current day
    this->process_(total);  // Process new gas values
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



void GasStatistics::process_(float total) {
  uint32_t now = millis();  // Get the current time

  // If we're waiting for the sensor to update, skip calculation until valid
  if (this->waiting_for_sensor_read_) {
    if (std::isnan(total) || total == 0.0) {
      // Only log the warning once per minute
      if (now - this->last_warning_time_ >= WARNING_LOG_INTERVAL) {
        ESP_LOGW(TAG, "Gas Statistics (m³) - Skipping sensor reading update, waiting for valid sensor reading.");
        this->last_warning_time_ = now;  // Update the last warning log time
      }
      return;
    }

    // Set the start values to the current total when a valid reading is received
    this->gas_.start_today = total;
    this->gas_.start_yesterday = total;
    this->gas_.start_week = total;
    this->gas_.start_month = total;
    this->gas_.start_year = total;

    this->waiting_for_sensor_read_ = false;  // Disable the wait flag
    ESP_LOGI(TAG, "Gas Statistics (m³) - Valid sensor reading obtained: %.3f", total);
  }

  // Ensure total is greater than or equal to start points
  if (total < this->gas_.start_today) {
    ESP_LOGW(TAG, "Gas Statistics (m³) - Total is less than start point, resetting start_today.");
    this->gas_.start_today = total;
    return;  // Return here to ensure the negative value does not get calculated
  }

  if (total < this->gas_.start_week) {
    ESP_LOGW(TAG, "Gas Statistics (m³) - Total is less than start point, resetting start_week.");
    this->gas_.start_week = total;
    return;
  }

  if (total < this->gas_.start_month) {
    ESP_LOGW(TAG, "Gas Statistics (m³) - Total is less than start point, resetting start_month.");
    this->gas_.start_month = total;
    return;
  }

  if (total < this->gas_.start_year) {
    ESP_LOGW(TAG, "Gas Statistics (m³) - Total is less than start point, resetting start_year.");
    this->gas_.start_year = total;
    return;
  }

  // Add time-based checks
  const auto t = this->time_->now();

  // Update gas today only if the value has changed
  if (this->gas_today_ && !std::isnan(this->gas_.start_today)) {
    float new_gas_today = total - this->gas_.start_today;

    // Handle negative usage
    if (new_gas_today < 0.0) {
      ESP_LOGW(TAG, "Gas Statistics (m³) - Negative gas usage detected, resetting start_today.");
      this->gas_.start_today = total;  // Reset to avoid negative value
      new_gas_today = 0.0;
    }

    // Publish only if the new value differs from the current state
    if (this->gas_today_->get_state() != new_gas_today) {
      this->gas_.gas_today = new_gas_today;
      this->gas_today_->publish_state(this->gas_.gas_today);
    }
  }

  // Ensure gas week only updates at the start of a new week
  if (this->gas_week_ && t.day_of_week == this->gas_week_start_day_ && !std::isnan(this->gas_.start_week)) {
    float new_gas_week = total - this->gas_.start_week;

    // Handle negative usage
    if (new_gas_week < 0.0) {
      ESP_LOGW(TAG, "Gas Statistics (m³) - Negative gas usage detected, resetting start_week.");
      this->gas_.start_week = total;  // Reset to avoid negative value
      new_gas_week = 0.0;
    }

    // Publish only if the new value differs from the current state
    if (this->gas_week_->get_state() != new_gas_week) {
      this->gas_.gas_week = new_gas_week;
      this->gas_week_->publish_state(this->gas_.gas_week);
    }
  }

  // Ensure gas month only updates at the start of a new month
  if (this->gas_month_ && t.day_of_month == this->gas_month_start_day_ && !std::isnan(this->gas_.start_month)) {
    float new_gas_month = total - this->gas_.start_month;

    // Handle negative usage
    if (new_gas_month < 0.0) {
      ESP_LOGW(TAG, "Gas Statistics (m³) - Negative gas usage detected, resetting start_month.");
      this->gas_.start_month = total;  // Reset to avoid negative value
      new_gas_month = 0.0;
    }

    // Publish only if the new value differs from the current state
    if (this->gas_month_->get_state() != new_gas_month) {
      this->gas_.gas_month = new_gas_month;
      this->gas_month_->publish_state(this->gas_.gas_month);
    }
  }

  // Ensure gas year only updates at the start of a new year
  if (this->gas_year_ && t.day_of_year == this->gas_year_start_day_ && !std::isnan(this->gas_.start_year)) {
    float new_gas_year = total - this->gas_.start_year;

    // Handle negative usage
    if (new_gas_year < 0.0) {
      ESP_LOGW(TAG, "Gas Statistics (m³) - Negative gas usage detected, resetting start_year.");
      this->gas_.start_year = total;  // Reset to avoid negative value
      new_gas_year = 0.0;
    }

    // Publish only if the new value differs from the current state
    if (this->gas_year_->get_state() != new_gas_year) {
      this->gas_.gas_year = new_gas_year;
      this->gas_year_->publish_state(this->gas_.gas_year);
    }
  }

  // Only save to flash if necessary
  if (now - last_save_time_ >= save_interval_ * 1000) {
    this->save_();
    last_save_time_ = now;  // Update the last save time
  }
}


void GasStatistics::reset_statistics() {
  uint32_t now = millis();        // Get the current time
  ESP_LOGI(TAG, "Gas Statistics (m³) - Resetting values to 0.0");

  // Reset today’s gas value to 0.0 (this is the only one that changes immediately)
  this->gas_.gas_today = 0.0;
  this->gas_.gas_yesterday = 0.0;
  
  // Reset week, month, and year values to 0.0 (these should remain 0 until their time periods start)
  this->gas_.gas_week = 0.0;
  this->gas_.gas_month = 0.0;
  this->gas_.gas_year = 0.0;

  // Get the current total value
  const auto total = this->total_->get_state();
  
  if (!std::isnan(total) && total != 0.0) {
    // Only set today’s start point to the current total
    this->gas_.start_today = total;
    this->gas_.start_yesterday = total;

    ESP_LOGI(TAG, "Gas Statistics (m³) - Start points set after reset: %.3f", total);
  } else {
    // If total is not valid, flag to wait for a valid reading
    this->waiting_for_sensor_read_ = true;
    if (now - this->last_warning_time_ >= WARNING_LOG_INTERVAL) {
      ESP_LOGW(TAG, "Gas Statistics (m³) - Total is invalid, waiting for valid sensor reading.");
      this->last_warning_time_ = now;  // Update the last warning log time
    }
  }

  // Publish the reset values to sensors
  if (this->gas_today_) this->gas_today_->publish_state(0.0);
  if (this->gas_yesterday_) this->gas_yesterday_->publish_state(0.0);
  if (this->gas_week_) this->gas_week_->publish_state(0.0);
  if (this->gas_month_) this->gas_month_->publish_state(0.0);
  if (this->gas_year_) this->gas_year_->publish_state(0.0);

  // Save reset state to flash memory
  this->save_();
}


void GasStatistics::save_() {
  this->pref_.save(&this->gas_); // Save to flash memory
  ESP_LOGD(TAG, "Gas Statistics (m³) - Values saved to flash memory (NVS)."); // Log message indicating save action
}

}  // namespace gas_statistics
}  // namespace esphome
