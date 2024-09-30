#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/core/schema.h"
#include "energy_statistics.h"

namespace esphome {
namespace energy_statistics {

static const char *const TAG = "energy_statistics";

// Define the configuration schema for the component
const auto EnergyStatistics::CONFIG_SCHEMA = sensor::SENSOR_SCHEMA.extend(
  Schema::Field<Schema::String>("save_frequency", "5m", "Save Frequency (default 5 minutes)")
);

void EnergyStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Energy statistics sensors");
  if (this->energy_today_) {
    LOG_SENSOR("  ", "Energy Today", this->energy_today_);
  }
  if (this->energy_yesterday_) {
    LOG_SENSOR("  ", "Energy Yesterday", this->energy_yesterday_);
  }
  if (this->energy_week_) {
    LOG_SENSOR("  ", "Energy Week", this->energy_week_);
  }
  if (this->energy_month_) {
    LOG_SENSOR("  ", "Energy Month", this->energy_month_);
  }
  if (this->energy_year_) {
    LOG_SENSOR("  ", "Energy Year", this->energy_year_);
  }

  ESP_LOGCONFIG(TAG, "Restored Energy Today: %.3f", this->energy_.energy_today);
  ESP_LOGCONFIG(TAG, "Restored Energy Yesterday: %.3f", this->energy_.energy_yesterday);
  ESP_LOGCONFIG(TAG, "Restored Energy Week: %.3f", this->energy_.energy_week);
  ESP_LOGCONFIG(TAG, "Restored Energy Month: %.3f", this->energy_.energy_month);
  ESP_LOGCONFIG(TAG, "Restored Energy Year: %.3f", this->energy_.energy_year);
}

void EnergyStatistics::setup() {
  // Fetch the save frequency from the YAML configuration
  std::string save_frequency_value = this->config_->get<std::string>("save_frequency", "5m");
  set_save_frequency(save_frequency_value);  // Set save frequency from the YAML configuration

  // Load preferences and energy data
  this->pref_ = global_preferences->make_preference<energy_data_t>(fnv1_hash(TAG));

  energy_data_t loaded{};
  if (this->pref_.load(&loaded) && !this->is_resetting_) {
    this->energy_ = loaded;
    // Publish existing states if available
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
    reset_statistics();  // Ensure reset if resetting
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
    // Update start points for new day, week, month, year
    this->energy_.start_yesterday = this->energy_.start_today;
    this->energy_.start_today = total;

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

// Remaining methods...

}  // namespace energy_statistics
}  // namespace esphome
