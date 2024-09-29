#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "gas_statistics.h"

namespace esphome {
namespace gas_statistics {

static const char *const TAG = "gas_statistics";
static const char *const GAP = "  ";

void GasStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Gas statistics sensors");
  if (this->gas_today_) {
    LOG_SENSOR(GAP, "Gas Today", this->gas_today_);
  }
  if (this->gas_yesterday_) {
    LOG_SENSOR(GAP, "Gas Yesterday", this->gas_yesterday_);
  }
  if (this->gas_week_) {
    LOG_SENSOR(GAP, "Gas Week", this->gas_week_);
  }
  if (this->gas_month_) {
    LOG_SENSOR(GAP, "Gas Month", this->gas_month_);
  }
  if (this->gas_year_) {
    LOG_SENSOR(GAP, "Gas Year", this->gas_year_);
  }

  ESP_LOGCONFIG(TAG, "Restored Gas Today: %.3f", this->gas_.gas_today);
  ESP_LOGCONFIG(TAG, "Restored Gas Yesterday: %.3f", this->gas_.gas_yesterday);
  ESP_LOGCONFIG(TAG, "Restored Gas Week: %.3f", this->gas_.gas_week);
  ESP_LOGCONFIG(TAG, "Restored Gas Month: %.3f", this->gas_.gas_month);
  ESP_LOGCONFIG(TAG, "Restored Gas Year: %.3f", this->gas_.gas_year);
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
    // Update start points for new day, week, month, year
    this->gas_.start_yesterday = this->gas_.start_today;
    this->gas_.start_today = total;

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
  // Update gas today only if the value has changed
  if (this->gas_today_ && !std::isnan(this->gas_.start_today)) {
    float new_gas_today = total - this->gas_.start_today;
    if (this->gas_today_->get_state() != new_gas_today) {
      this->gas_.gas_today = new_gas_today;
      this->gas_today_->publish_state(this->gas_.gas_today);
    }
  } else if (this->gas_today_ && this->gas_today_->get_state() != 0.0) {
    this->gas_today_->publish_state(0.0);
  }

  // Update gas yesterday only if the value has changed
  if (this->gas_yesterday_ && !std::isnan(this->gas_.start_yesterday)) {
    float new_gas_yesterday = this->gas_.start_today - this->gas_.start_yesterday;
    if (this->gas_yesterday_->get_state() != new_gas_yesterday) {
      this->gas_.gas_yesterday = new_gas_yesterday;
      this->gas_yesterday_->publish_state(this->gas_.gas_yesterday);
    }
  } else if (this->gas_yesterday_ && this->gas_yesterday_->get_state() != 0.0) {
    this->gas_yesterday_->publish_state(0.0);
  }

  // Update gas week only if the value has changed
  if (this->gas_week_ && !std::isnan(this->gas_.start_week)) {
    float new_gas_week = total - this->gas_.start_week;
    if (this->gas_week_->get_state() != new_gas_week) {
      this->gas_.gas_week = new_gas_week;
      this->gas_week_->publish_state(this->gas_.gas_week);
    }
  } else if (this->gas_week_ && this->gas_week_->get_state() != 0.0) {
    this->gas_week_->publish_state(0.0);
  }

  // Update gas month only if the value has changed
  if (this->gas_month_ && !std::isnan(this->gas_.start_month)) {
    float new_gas_month = total - this->gas_.start_month;
    if (this->gas_month_->get_state() != new_gas_month) {
      this->gas_.gas_month = new_gas_month;
      this->gas_month_->publish_state(this->gas_.gas_month);
    }
  } else if (this->gas_month_ && this->gas_month_->get_state() != 0.0) {
    this->gas_month_->publish_state(0.0);
  }

  // Update gas year only if the value has changed
  if (this->gas_year_ && !std::isnan(this->gas_.start_year)) {
    float new_gas_year = total - this->gas_.start_year;
    if (this->gas_year_->get_state() != new_gas_year) {
      this->gas_.gas_year = new_gas_year;
      this->gas_year_->publish_state(this->gas_.gas_year);
    }
  } else if (this->gas_year_ && this->gas_year_->get_state() != 0.0) {
    this->gas_year_->publish_state(0.0);
  }

  // Only save to flash if necessary
  uint32_t now = millis();
  if (now - last_save_time_ >= save_interval_ * 1000) {
    this->save_();
    last_save_time_ = now;  // Update the last save time
  }
}


void GasStatistics::reset_statistics() {
  ESP_LOGI(TAG, "Resetting Gas Statistics to 0.0");

  // Reset gas values to 0.0
  this->gas_.gas_today = 0.0;
  this->gas_.gas_yesterday = 0.0;
  this->gas_.gas_week = 0.0;
  this->gas_.gas_month = 0.0;
  this->gas_.gas_year = 0.0;

  // Reset start points for gas calculations
  const auto total = this->total_->get_state();
  this->gas_.start_today = total;
  this->gas_.start_yesterday = total;
  this->gas_.start_week = total;
  this->gas_.start_month = total;
  this->gas_.start_year = total;

  // Publish the reset values to sensors
  if (this->gas_today_) this->gas_today_->publish_state(0.0);
  if (this->gas_yesterday_) this->gas_yesterday_->publish_state(0.0);
  if (this->gas_week_) this->gas_week_->publish_state(0.0);
  if (this->gas_month_) this->gas_month_->publish_state(0.0);
  if (this->gas_year_) this->gas_year_->publish_state(0.0);

  // Save reset state to flash memory
  this->save_();
  this->is_resetting_ = false;  // Clear flag after reset
}


void GasStatistics::save_() {
  this->pref_.save(&this->gas_); // Save to flash memory
  ESP_LOGD(TAG, "Gas Statistics - Values saved to flash memory."); // Log message indicating save action
}

}  // namespace gas_statistics
}  // namespace esphome
