#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "energy_statistics.h"

namespace esphome {
namespace energy_statistics {

static const char *const TAG = "energy_statistics";
static const char *const GAP = "  ";

// Time between warning log messages being repeated (in milliseconds)
static const uint32_t WARNING_LOG_INTERVAL = 60000;  // 60 seconds

void EnergyStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Energy Statistics - Sensors");
  if (this->energy_today_) {
    LOG_SENSOR(GAP, "Energy Today", this->energy_today_);
  }
  if (this->energy_yesterday_) {
    LOG_SENSOR(GAP, "Energy Yesterday", this->energy_yesterday_);
  }
  if (this->energy_week_) {
    LOG_SENSOR(GAP, "Energy Week", this->energy_week_);
  }
  if (this->energy_month_) {
    LOG_SENSOR(GAP, "Energy Month", this->energy_month_);
  }
  if (this->energy_year_) {
    LOG_SENSOR(GAP, "Energy Year", this->energy_year_);
  }

  ESP_LOGCONFIG(TAG, "Restored Energy Today : %.3f", this->energy_.energy_today);
  ESP_LOGCONFIG(TAG, "Restored Energy Yesterday : %.3f", this->energy_.energy_yesterday);
  ESP_LOGCONFIG(TAG, "Restored Energy Week : %.3f", this->energy_.energy_week);
  ESP_LOGCONFIG(TAG, "Restored Energy Month : %.3f", this->energy_.energy_month);
  ESP_LOGCONFIG(TAG, "Restored Energy Year : %.3f", this->energy_.energy_year);
}

void EnergyStatistics::setup() {
  this->pref_ = global_preferences->make_preference<energy_data_t>(fnv1_hash(TAG));

  energy_data_t loaded{};
  if (this->pref_.load(&loaded) && !this->is_resetting_) {
    this->energy_ = loaded;

    if (this->energy_today_ && !std::isnan(this->energy_.energy_today)) {
      this->energy_today_->publish_state(this->energy_.energy_today);
    }
    if (this->energy_yesterday_ && !std::isnan(this->energy_.energy_yesterday)) {
      this->energy_yesterday_->publish_state(this->energy_.energy_yesterday);
    }
    if (this->energy_week_ && !std::isnan(this->energy_.energy_week)) {
      this->energy_week_->publish_state(this->energy_.energy_week);
    }
    if (this->energy_month_ && !std::isnan(this->energy_.energy_month)) {
      this->energy_month_->publish_state(this->energy_.energy_month);
    }
    if (this->energy_year_ && !std::isnan(this->energy_.energy_year)) {
      this->energy_year_->publish_state(this->energy_.energy_year);
    }

    auto total = this->total_->get_state();
    if (!std::isnan(total)) {
      this->process_(total);
    }
  } else {
    reset_statistics(); // Ensure reset if resetting
  }
}

void EnergyStatistics::loop() {
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
  if (t.day_of_year != this->energy_.current_day_of_year) {
    // Transfer today's energy usage to yesterday before resetting today
    if (this->energy_yesterday_) {
      this->energy_.energy_yesterday = this->energy_.energy_today;  // Transfer today's energy usage to yesterday
      this->energy_yesterday_->publish_state(this->energy_.energy_yesterday);  // Publish the updated yesterday value
    }

    // Reset today's energy usage and update start points for new day, week, month, year
    this->energy_.start_yesterday = this->energy_.start_today;
    this->energy_.start_today = total;
    this->energy_.energy_today = 0.0;  // Reset today's energy usage to 0

    // Update week, month, year start points as necessary
    if (t.day_of_week == this->energy_week_start_day_) {
      this->energy_.start_week = total;
    }
    if (t.day_of_month == this->energy_month_start_day_) {
      this->energy_.start_month = total;
    }
    if (t.day_of_year == this->energy_year_start_day_) {
      this->energy_.start_year = total;
    }

    this->energy_.current_day_of_year = t.day_of_year;  // Update the current day
    this->process_(total);  // Process new energy values
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



void EnergyStatistics::process_(float total) {
  uint32_t now = millis();  // Get the current time

  // If we're waiting for the sensor to update, skip calculation until valid
  if (this->waiting_for_sensor_read_) {
    if (std::isnan(total) || total == 0.0) {
      // Only log the warning once per minute
      if (now - this->last_warning_time_ >= WARNING_LOG_INTERVAL) {
        ESP_LOGW(TAG, "Energy Statistics - Skipping sensor reading update, waiting for valid sensor reading.");
        this->last_warning_time_ = now;  // Update the last warning log time
      }
      return;
    }

    // Set the start values to the current total when a valid reading is received
    this->energy_.start_today = total;
    this->energy_.start_yesterday = total;
    this->energy_.start_week = total;
    this->energy_.start_month = total;
    this->energy_.start_year = total;

    this->waiting_for_sensor_read_ = false;  // Disable the wait flag
    ESP_LOGI(TAG, "Energy Statistics - Valid sensor reading obtained: %.3f", total);
  }

  // Ensure total is greater than or equal to start points
  if (total < this->energy_.start_today) {
    ESP_LOGW(TAG, "Energy Statistics - Total is less than start point, resetting start_today.");
    this->energy_.start_today = total;
    return;  // Return here to ensure the negative value does not get calculated
  }

  if (total < this->energy_.start_week) {
    ESP_LOGW(TAG, "Energy Statistics - Total is less than start point, resetting start_week.");
    this->energy_.start_week = total;
    return;
  }

  if (total < this->energy_.start_month) {
    ESP_LOGW(TAG, "Energy Statistics - Total is less than start point, resetting start_month.");
    this->energy_.start_month = total;
    return;
  }

  if (total < this->energy_.start_year) {
    ESP_LOGW(TAG, "Energy Statistics - Total is less than start point, resetting start_year.");
    this->energy_.start_year = total;
    return;
  }

  // Add time-based checks
  const auto t = this->time_->now();

  // Update energy today only if the value has changed
  if (this->energy_today_ && !std::isnan(this->energy_.start_today)) {
    float new_energy_today = total - this->energy_.start_today;

    // Handle negative usage
    if (new_energy_today < 0.0) {
      ESP_LOGW(TAG, "Energy Statistics - Negative energy usage detected, resetting start_today.");
      this->energy_.start_today = total;  // Reset to avoid negative value
      new_energy_today = 0.0;
    }

    // Publish only if the new value differs from the current state
    if (this->energy_today_->get_state() != new_energy_today) {
      this->energy_.energy_today = new_energy_today;
      this->energy_today_->publish_state(this->energy_.energy_today);
    }
  }

  // Ensure energy week only updates at the start of a new week
  if (this->energy_week_ && t.day_of_week == this->energy_week_start_day_ && !std::isnan(this->energy_.start_week)) {
    float new_energy_week = total - this->energy_.start_week;

    // Handle negative usage
    if (new_energy_week < 0.0) {
      ESP_LOGW(TAG, "Energy Statistics - Negative energy usage detected, resetting start_week.");
      this->energy_.start_week = total;  // Reset to avoid negative value
      new_energy_week = 0.0;
    }

    // Publish only if the new value differs from the current state
    if (this->energy_week_->get_state() != new_energy_week) {
      this->energy_.energy_week = new_energy_week;
      this->energy_week_->publish_state(this->energy_.energy_week);
    }
  }

  // Ensure energy month only updates at the start of a new month
  if (this->energy_month_ && t.day_of_month == this->energy_month_start_day_ && !std::isnan(this->energy_.start_month)) {
    float new_energy_month = total - this->energy_.start_month;

    // Handle negative usage
    if (new_energy_month < 0.0) {
      ESP_LOGW(TAG, "Energy Statistics - Negative energy usage detected, resetting start_month.");
      this->energy_.start_month = total;  // Reset to avoid negative value
      new_energy_month = 0.0;
    }

    // Publish only if the new value differs from the current state
    if (this->energy_month_->get_state() != new_energy_month) {
      this->energy_.energy_month = new_energy_month;
      this->energy_month_->publish_state(this->energy_.energy_month);
    }
  }

  // Ensure energy year only updates at the start of a new year
  if (this->energy_year_ && t.day_of_year == this->energy_year_start_day_ && !std::isnan(this->energy_.start_year)) {
    float new_energy_year = total - this->energy_.start_year;

    // Handle negative usage
    if (new_energy_year < 0.0) {
      ESP_LOGW(TAG, "Energy Statistics - Negative energy usage detected, resetting start_year.");
      this->energy_.start_year = total;  // Reset to avoid negative value
      new_energy_year = 0.0;
    }

    // Publish only if the new value differs from the current state
    if (this->energy_year_->get_state() != new_energy_year) {
      this->energy_.energy_year = new_energy_year;
      this->energy_year_->publish_state(this->energy_.energy_year);
    }
  }

  // Only save to flash if necessary
  if (now - last_save_time_ >= save_interval_ * 1000) {
    this->save_();
    last_save_time_ = now;  // Update the last save time
  }
}


void EnergyStatistics::reset_statistics() {
  uint32_t now = millis();        // Get the current time
  ESP_LOGI(TAG, "Energy Statistics - Resetting values to 0.0");

  // Reset today’s energy value to 0.0 (this is the only one that changes immediately)
  this->energy_.energy_today = 0.0;
  this->energy_.energy_yesterday = 0.0;
  
  // Reset week, month, and year values to 0.0 (these should remain 0 until their time periods start)
  this->energy_.energy_week = 0.0;
  this->energy_.energy_month = 0.0;
  this->energy_.energy_year = 0.0;

  // Get the current total value
  const auto total = this->total_->get_state();
  
  if (!std::isnan(total) && total != 0.0) {
    // Only set today’s start point to the current total
    this->energy_.start_today = total;
    this->energy_.start_yesterday = total;

    ESP_LOGI(TAG, "Energy Statistics  - Start points set after reset: %.3f", total);
  } else {
    // If total is not valid, flag to wait for a valid reading
    this->waiting_for_sensor_read_ = true;
    if (now - this->last_warning_time_ >= WARNING_LOG_INTERVAL) {
      ESP_LOGW(TAG, "Energy Statistics - Total is invalid, waiting for valid sensor reading.");
      this->last_warning_time_ = now;  // Update the last warning log time
    }
  }

  // Publish the reset values to sensors
  if (this->energy_today_) this->energy_today_->publish_state(0.0);
  if (this->energy_yesterday_) this->energy_yesterday_->publish_state(0.0);
  if (this->energy_week_) this->energy_week_->publish_state(0.0);
  if (this->energy_month_) this->energy_month_->publish_state(0.0);
  if (this->energy_year_) this->energy_year_->publish_state(0.0);

  // Save reset state to flash memory
  this->save_();
}


void EnergyStatistics::save_() {
  this->pref_.save(&this->energy_); // Save to flash memory
  ESP_LOGD(TAG, "Energy Statistics - Values saved to flash memory (NVS)."); // Log message indicating save action
}

}  // namespace energy_statistics
}  // namespace esphome
