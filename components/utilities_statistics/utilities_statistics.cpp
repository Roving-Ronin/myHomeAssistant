#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#include "utilities_statistics.h"

namespace esphome {
namespace utilities_statistics {

void UtilitiesStatistics::dump_config() {
  ESP_LOGCONFIG("UtilitiesStatistics", "Gas (m³), Gas (MJ), and Water (L) Statistics");
}

void UtilitiesStatistics::setup() {
  // Load saved state from preferences
  this->pref_.load(&this->gas_m3_, sizeof(this->gas_m3_));
  this->pref_.load(&this->gas_mj_, sizeof(this->gas_mj_));
  this->pref_.load(&this->water_, sizeof(this->water_));
}

void UtilitiesStatistics::loop() {
  // If resetting or waiting for valid sensor readings, skip the loop
  if (this->is_resetting_ || this->waiting_for_sensor_read_) {
    return;
  }

  // Process gas statistics (m³) if the total sensor is available and updated
  if (this->gas_m3_total_ != nullptr && this->gas_m3_total_->has_state()) {
    this->process_gas_m3_(this->gas_m3_total_->state);
    this->process_gas_mj_();  // Convert m³ to MJ
  }

  // Process water statistics if the total sensor is available and updated
  if (this->water_total_ != nullptr && this->water_total_->has_state()) {
    this->process_water_(this->water_total_->state);
  }

  // Save data periodically
  uint32_t now = millis();
  if (now - this->last_save_time_ > this->save_interval_ * 1000) {
    this->save_();
    this->last_save_time_ = now;
  }
}

void UtilitiesStatistics::reset_gas_m3_statistics() {
  this->is_resetting_ = true;
  this->gas_m3_ = {};
  this->save_();
  this->is_resetting_ = false;
}

void UtilitiesStatistics::reset_gas_mj_statistics() {
  this->is_resetting_ = true;
  this->gas_mj_ = {};
  this->save_();
  this->is_resetting_ = false;
}

void UtilitiesStatistics::reset_water_statistics() {
  this->is_resetting_ = true;
  this->water_ = {};
  this->save_();
  this->is_resetting_ = false;
}

// Process gas statistics (m³)
void UtilitiesStatistics::process_gas_m3_(float total) {
  // Process gas statistics for today, yesterday, week, month, year
  if (this->gas_m3_today_ != nullptr) {
    this->gas_m3_today_->publish_state(total - this->gas_m3_.start_today);
  }
  if (this->gas_m3_yesterday_ != nullptr) {
    this->gas_m3_yesterday_->publish_state(this->gas_m3_.gas_m3_yesterday);
  }
  if (this->gas_m3_week_ != nullptr) {
    this->gas_m3_week_->publish_state(this->gas_m3_.gas_m3_week);
  }
  if (this->gas_m3_month_ != nullptr) {
    this->gas_m3_month_->publish_state(this->gas_m3_.gas_m3_month);
  }
  if (this->gas_m3_year_ != nullptr) {
    this->gas_m3_year_->publish_state(this->gas_m3_.gas_m3_year);
  }
}

// Process gas statistics (MJ) based on the total gas in m³
void UtilitiesStatistics::process_gas_mj_() {
  if (this->gas_mj_total_ != nullptr && this->gas_m3_total_ != nullptr) {
    float gas_mj = this->gas_m3_total_->state * 1.022; // Example conversion factor
    this->gas_mj_total_->publish_state(gas_mj);

    // Update exposed MJ sensors (today, yesterday, etc.)
    if (this->gas_mj_today_ != nullptr) {
      this->gas_mj_today_->publish_state(gas_mj - this->gas_mj_.start_today);
    }
    if (this->gas_mj_yesterday_ != nullptr) {
      this->gas_mj_yesterday_->publish_state(this->gas_mj_.gas_mj_yesterday);
    }
    if (this->gas_mj_week_ != nullptr) {
      this->gas_mj_week_->publish_state(this->gas_mj_.gas_mj_week);
    }
    if (this->gas_mj_month_ != nullptr) {
      this->gas_mj_month_->publish_state(this->gas_mj_.gas_mj_month);
    }
    if (this->gas_mj_year_ != nullptr) {
      this->gas_mj_year_->publish_state(this->gas_mj_.gas_mj_year);
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

void UtilitiesStatistics::save_() {
  // Save gas_m3, gas_mj, and water data to preferences
  this->pref_.save(&this->gas_m3_, sizeof(this->gas_m3_));
  this->pref_.save(&this->gas_mj_, sizeof(this->gas_mj_));
  this->pref_.save(&this->water_, sizeof(this->water_));
}

}  // namespace utilities_statistics
}  // namespace esphome
