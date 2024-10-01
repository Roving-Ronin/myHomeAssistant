#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "utilities_statistics.h"

namespace esphome {
namespace utilities_statistics {

static const char *const TAG = "utilities_statistics";
static const char *const GAP = "  ";

// Time between warning log messages being repeated (in milliseconds)
static const uint32_t WARNING_LOG_INTERVAL = 60000;  // 60 seconds

void UtilitiesStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Gas (m³), Gas (MJ), and Water Statistics - Sensors");

  // Gas (m³) sensors
  if (this->gas_m3_today_) {
    LOG_SENSOR(GAP, "Gas (m³) Today", this->gas_m3_today_);
  }
  if (this->gas_m3_yesterday_) {
    LOG_SENSOR(GAP, "Gas (m³) Yesterday", this->gas_m3_yesterday_);
  }
  if (this->gas_m3_week_) {
    LOG_SENSOR(GAP, "Gas (m³) Week", this->gas_m3_week_);
  }
  if (this->gas_m3_month_) {
    LOG_SENSOR(GAP, "Gas (m³) Month", this->gas_m3_month_);
  }
  if (this->gas_m3_year_) {
    LOG_SENSOR(GAP, "Gas (m³) Year", this->gas_m3_year_);
  }

  // Gas MJ sensors
  if (this->gas_mj_today_) {
    LOG_SENSOR(GAP, "Gas MJ Today", this->gas_mj_today_);
  }
  if (this->gas_mj_yesterday_) {
    LOG_SENSOR(GAP, "Gas MJ Yesterday", this->gas_mj_yesterday_);
  }
  if (this->gas_mj_week_) {
    LOG_SENSOR(GAP, "Gas MJ Week", this->gas_mj_week_);
  }
  if (this->gas_mj_month_) {
    LOG_SENSOR(GAP, "Gas MJ Month", this->gas_mj_month_);
  }
  if (this->gas_mj_year_) {
    LOG_SENSOR(GAP, "Gas MJ Year", this->gas_mj_year_);
  }

  // Water sensors
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
}

void UtilitiesStatistics::setup() {
  // Load saved state from preferences
  this->pref_ = global_preferences->make_preference<gas_m3_data_t>(fnv1_hash(TAG));

  gas_m3_data_t loaded{};
  if (this->pref_.load(&loaded) && !this->is_resetting_) {
    this->gas_m3_ = loaded;

    if (this->gas_m3_today_ && !std::isnan(this->gas_m3_.gas_m3_today)) {
      this->gas_m3_today_->publish_state(this->gas_m3_.gas_m3_today);
    }
    if (this->gas_m3_yesterday_ && !std::isnan(this->gas_m3_.gas_m3_yesterday)) {
      this->gas_m3_yesterday_->publish_state(this->gas_m3_.gas_m3_yesterday);
    }
    if (this->gas_m3_week_ && !std::isnan(this->gas_m3_.gas_m3_week)) {
      this->gas_m3_week_->publish_state(this->gas_m3_.gas_m3_week);
    }
    if (this->gas_m3_month_ && !std::isnan(this->gas_m3_.gas_m3_month)) {
      this->gas_m3_month_->publish_state(this->gas_m3_.gas_m3_month);
    }
    if (this->gas_m3_year_ && !std::isnan(this->gas_m3_.gas_m3_year)) {
      this->gas_m3_year_->publish_state(this->gas_m3_.gas_m3_year);
    }

    auto total = this->gas_m3_total_->get_state();
    if (!std::isnan(total)) {
      this->process_gas_m3_(total);
    }
  } else {
    reset_gas_m3_statistics();  // Reset if the load fails or reset is requested
  }
}

void UtilitiesStatistics::loop() {
  const auto t = this->time_->now();
  if (!t.is_valid()) {
    return;
  }

  const auto total = this->gas_m3_total_->get_state();
  if (std::isnan(total)) {
    return;
  }

  // If sensor updates are prevented, skip this loop
  if (this->prevent_sensor_update_) {
    this->prevent_sensor_update_ = false;
    return;
  }

  // Check if a new day has started
  if (t.day_of_year != this->gas_m3_.current_day_of_year) {
    // Update start points for the new day, week, month, year
    this->gas_m3_.start_yesterday = this->gas_m3_.start_today;
    this->gas_m3_.start_today = total;

    if (t.day_of_week == this->gas_m3_week_start_day_) {
      this->gas_m3_.start_week = total;
    }
    if (t.day_of_month == this->gas_m3_month_start_day_) {
      this->gas_m3_.start_month = total;
    }
    if (t.day_of_year == this->gas_m3_year_start_day_) {
      this->gas_m3_.start_year = total;
    }

    this->gas_m3_.current_day_of_year = t.day_of_year;  // Update the current day
    this->process_gas_m3_(total);  // Process new gas m3 values
  }

  // Continue processing the total sensor value
  this->process_gas_m3_(total);

  // Save data periodically
  uint32_t now = millis();
  if (now - last_save_time_ >= save_interval_ * 1000) {
    this->save_();
    last_save_time_ = now;
  }
}

// Process gas statistics (m³)
void UtilitiesStatistics::process_gas_m3_(float total) {
  uint32_t now = millis();  // Get the current time

  // If waiting for a valid sensor reading, skip the process
  if (this->waiting_for_sensor_read_) {
    if (std::isnan(total) || total == 0.0) {
      // Log a warning if no valid reading is found
      if (now - this->last_warning_time_ >= WARNING_LOG_INTERVAL) {
        ESP_LOGW(TAG, "Gas Statistics (m³) - Waiting for valid sensor reading.");
        this->last_warning_time_ = now;
      }
      return;
    }

    // Once a valid reading is found, initialize the start points
    this->gas_m3_.start_today = total;
    this->gas_m3_.start_yesterday = total;
    this->gas_m3_.start_week = total;
    this->gas_m3_.start_month = total;
    this->gas_m3_.start_year = total;

    this->waiting_for_sensor_read_ = false;
    ESP_LOGI(TAG, "Gas Statistics (m³) - Valid sensor reading obtained: %.3f", total);
  }

  // Ensure the total is valid and greater than the start points
  if (total < this->gas_m3_.start_today || std::isnan(this->gas_m3_.start_today)) {
    if (now - this->last_warning_time_ >= WARNING_LOG_INTERVAL) {
      ESP_LOGW(TAG, "Gas Statistics (m³) - 'Total Gas' sensor value is invalid.");
      this->last_warning_time_ = now;
    }
    return;
  }

  // Update today, yesterday, week, month, year statistics
  if (this->gas_m3_today_ && !std::isnan(this->gas_m3_.start_today)) {
    float new_gas_today = total - this->gas_m3_.start_today;
    if (this->gas_m3_today_->get_state() != new_gas_today) {
      this->gas_m3_.gas_m3_today = new_gas_today;
      this->gas_m3_today_->publish_state(new_gas_today);
    }
  }

  if (this->gas_m3_yesterday_ && !std::isnan(this->gas_m3_.start_yesterday)) {
    float new_gas_yesterday = this->gas_m3_.start_today - this->gas_m3_.start_yesterday;
    this->gas_m3_yesterday_->publish_state(new_gas_yesterday);
  }

  if (this->gas_m3_week_ && !std::isnan(this->gas_m3_.start_week)) {
    float new_gas_week = total - this->gas_m3_.start_week;
    this->gas_m3_week_->publish_state(new_gas_week);
  }

  if (this->gas_m3_month_ && !std::isnan(this->gas_m3_.start_month)) {
    float new_gas_month = total - this->gas_m3_.start_month;
    this->gas_m3_month_->publish_state(new_gas_month);
  }

  if (this->gas_m3_year_ && !std::isnan(this->gas_m3_.start_year)) {
    float new_gas_year = total - this->gas_m3_.start_year;
    this->gas_m3_year_->publish_state(new_gas_year);
  }

  // Only save data when necessary
  uint32_t now = millis();
  if (now - last_save_time_ >= save_interval_ * 1000) {
    this->save_();
    last_save_time_ = now;
  }
}

// Process gas statistics (MJ) based on gas in m³
void UtilitiesStatistics::process_gas_mj_() {
  if (this->gas_mj_total_ != nullptr && this->gas_m3_total_ != nullptr) {
    float gas_mj = this->gas_m3_total_->state * 1.022; // Conversion factor
    this->gas_mj_total_->publish_state(gas_mj);

    // Update MJ sensors for today, yesterday, etc.
    if (this->gas_mj_today_ != nullptr) {
      this->gas_mj_today_->publish_state(gas_mj - this->gas_mj_.start_today);
    }
  }
}

// Process water statistics
void UtilitiesStatistics::process_water_(float total) {
  if (this->water_today_ != nullptr) {
    this->water_today_->publish_state(total - this->water_.start_today);
  }
  if (this->water_yesterday_ != nullptr) {
    this->water_yesterday_->publish_state(this->water_.water_yesterday);
  }
  if (this->water_week_ != nullptr) {
    this->water_week_->publish_state(this->water_.water_week);
  }
  if (this->water_month_ != nullptr) {
    this->water_month_->publish_state(this->water_.water_month);
  }
  if (this->water_year_ != nullptr) {
    this->water_year_->publish_state(this->water_.water_year);
  }
}

// Reset gas m3 statistics
void UtilitiesStatistics::reset_gas_m3_statistics() {
  ESP_LOGI(TAG, "Resetting Gas (m³) statistics.");
  this->gas_m3_ = {};  // Reset the gas_m3 structure
  this->save_();       // Save reset data to NVS
}

// Save the current statistics to NVS
void UtilitiesStatistics::save_() {
  this->pref_.save(&this->gas_m3_);  // Save gas_m3 data to flash
  ESP_LOGD(TAG, "Gas (m³) statistics saved to NVS.");
}

}  // namespace utilities_statistics
}  // namespace esphome
