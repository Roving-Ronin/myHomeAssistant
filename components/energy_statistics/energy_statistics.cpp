#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "energy_statistics.h"

namespace esphome {
namespace energy_statistics {

static const char *const TAG = "energy_statistics";
static const char *const GAP = "  ";

void EnergyStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Energy statistics sensors");
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

  // Add logs for the loaded values
  ESP_LOGCONFIG(TAG, "Restored Energy Today: %.3f", this->energy_.energy_today);
  ESP_LOGCONFIG(TAG, "Restored Energy Yesterday: %.3f", this->energy_.energy_yesterday);
  ESP_LOGCONFIG(TAG, "Restored Energy Week: %.3f", this->energy_.energy_week);
  ESP_LOGCONFIG(TAG, "Restored Energy Month: %.3f", this->energy_.energy_month);
  ESP_LOGCONFIG(TAG, "Restored Energy Year: %.3f", this->energy_.energy_year);
}

void EnergyStatistics::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });

  this->pref_ = global_preferences->make_preference<energy_data_t>(fnv1_hash(TAG));

  energy_data_t loaded{};
  if (this->pref_.load(&loaded)) {
    this->energy_ = loaded;

    // Load the sensor values from preferences if available
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
  }
}

void EnergyStatistics::loop() {
  const auto t = this->time_->now();
  if (!t.is_valid()) {
    // time is not sync yet
    return;
  }

  // Reset the prevent update flag after a certain condition
  if (this->prevent_sensor_update_) {
    this->prevent_sensor_update_ = false; // Reset after your condition
    return; // Skip the rest of the loop for now
  }

  const auto total = this->total_->get_state();
  if (std::isnan(total)) {
    // total is not published yet
    return;
  }

  if (t.day_of_year == this->energy_.current_day_of_year) {
    // nothing to do
    return;
  }

  this->energy_.start_yesterday = this->energy_.start_today;

  this->energy_.start_today = total;

  if (this->energy_.current_day_of_year != 0) {
    // at specified day of week we start a new week calculation
    if (t.day_of_week == this->energy_week_start_day_) {
      this->energy_.start_week = total;
    }
    // at first day of month we start a new month calculation
    if (t.day_of_month == 1) {
      this->energy_.start_month = total;
    }
    // at first day of year we start a new year calculation
    if (t.day_of_year == 1) {
      this->energy_.start_year = total;
    }
  }

  this->energy_.current_day_of_year = t.day_of_year;

  this->process_(total);
}

void EnergyStatistics::process_(float total) {
  // Energy Today
  if (this->energy_today_ && !std::isnan(this->energy_.start_today)) {
    this->energy_.energy_today = total - this->energy_.start_today;
    this->energy_today_->publish_state(this->energy_.energy_today);
  } else if (this->energy_today_) {
    // Show 0 instead of NA if no valid data
    this->energy_today_->publish_state(0.0);
  }

  // Energy Yesterday
  if (this->energy_yesterday_ && !std::isnan(this->energy_.start_yesterday)) {
    this->energy_.energy_yesterday = this->energy_.start_today - this->energy_.start_yesterday;
    this->energy_yesterday_->publish_state(this->energy_.energy_yesterday);
  } else if (this->energy_yesterday_) {
    this->energy_yesterday_->publish_state(0.0);
  }

  // Energy Week
  if (this->energy_week_ && !std::isnan(this->energy_.start_week)) {
    this->energy_.energy_week = total - this->energy_.start_week;
    this->energy_week_->publish_state(this->energy_.energy_week);
  } else if (this->energy_week_) {
    this->energy_week_->publish_state(0.0);
  }

  // Energy Month
  if (this->energy_month_ && !std::isnan(this->energy_.start_month)) {
    this->energy_.energy_month = total - this->energy_.start_month;
    this->energy_month_->publish_state(this->energy_.energy_month);
  } else if (this->energy_month_) {
    this->energy_month_->publish_state(0.0);
  }

  // Energy Year
  if (this->energy_year_ && !std::isnan(this->energy_.start_year)) {
    this->energy_.energy_year = total - this->energy_.start_year;
    this->energy_year_->publish_state(this->energy_.energy_year);
  } else if (this->energy_year_) {
    this->energy_year_->publish_state(0.0);
  }

  // Save the updated values to preferences every minute
  this->save_();
}

void EnergyStatistics::reset_statistics() {
  ESP_LOGI(TAG, "Resetting Energy Statistics to 0.0"); // Log message at the info level

  // API reset of statistics to ZERO.
  this->energy_.energy_today = 0.0;
  this->energy_.energy_yesterday = 0.0;
  this->energy_.energy_week = 0.0;
  this->energy_.energy_month = 0.0;
  this->energy_.energy_year = 0.0;

  if (this->energy_today_) this->energy_today_->publish_state(0.0);
  if (this->energy_yesterday_) this->energy_yesterday_->publish_state(0.0);
  if (this->energy_week_) this->energy_week_->publish_state(0.0);
  if (this->energy_month_) this->energy_month_->publish_state(0.0);
  if (this->energy_year_) this->energy_year_->publish_state(0.0);

  // Save the reset values to preferences
  this->save_();

  // Prevent sensor callbacks from overwriting reset values for a short time
  this->prevent_sensor_update_ = true;  // New flag to prevent updates
}

void EnergyStatistics::save_() {
  static uint32_t last_save_time_ = 0; // Variable to store the last save time
  const uint32_t save_interval_ = 60 * 1000; // Save every 60 seconds

  uint32_t current_time = millis(); // Get the current time in milliseconds

  // Check if enough time has passed since the last save
  if (current_time - last_save_time_ >= save_interval_) {
    this->pref_.save(&(this->energy_)); // Save the statistics to flash
    last_save_time_ = current_time; // Update the last save time
    ESP_LOGI(TAG, "Energy Statistics saved to ESP flash."); // Log message indicating save action
  }
}

}  // namespace energy_statistics
}  // namespace esphome
