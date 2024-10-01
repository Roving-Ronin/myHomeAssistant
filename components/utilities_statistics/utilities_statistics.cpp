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

  // If sensor updates are prevented, skip this loop
  if (this->prevent_sensor_update_) {
    this->prevent_sensor_update_ = false;
    return;
  }

  // Process Gas (m³)
  const auto gas_total_m3 = this->gas_m3_total_->get_state();
  if (std::isnan(gas_total_m3)) {
    return;
  }

  // Check if a new day has started for Gas (m³)
  if (t.day_of_year != this->gas_m3_.current_day_of_year) {
    // Update start points for new day, week, month, year
    this->gas_m3_.start_yesterday = this->gas_m3_.start_today;
    this->gas_m3_.start_today = gas_total_m3;

    if (t.day_of_week == this->gas_m3_week_start_day_) {
      this->gas_m3_.start_week = gas_total_m3;
    }
    if (t.day_of_month == this->gas_m3_month_start_day_) {
      this->gas_m3_.start_month = gas_total_m3;
    }
    if (t.day_of_year == this->gas_m3_year_start_day_) {
      this->gas_m3_.start_year = gas_total_m3;
    }

    this->gas_m3_.current_day_of_year = t.day_of_year;  // Update the current day
  }

  // Continue processing the total gas (m³) sensor value
  this->process_gas_m3_(gas_total_m3, t);

  // Process Gas (MJ)
  const auto gas_total_mj = this->gas_mj_total_->get_state();
  if (std::isnan(gas_total_mj)) {
    return;
  }

  // Check if a new day has started for Gas (MJ)
  if (t.day_of_year != this->gas_mj_.current_day_of_year) {
    // Update start points for new day, week, month, year
    this->gas_mj_.start_yesterday = this->gas_mj_.start_today;
    this->gas_mj_.start_today = gas_total_mj;

    if (t.day_of_week == this->gas_mj_week_start_day_) {
      this->gas_mj_.start_week = gas_total_mj;
    }
    if (t.day_of_month == this->gas_mj_month_start_day_) {
      this->gas_mj_.start_month = gas_total_mj;
    }
    if (t.day_of_year == this->gas_mj_year_start_day_) {
      this->gas_mj_.start_year = gas_total_mj;
    }

    this->gas_mj_.current_day_of_year = t.day_of_year;  // Update the current day
  }

  // Continue processing the total gas (MJ) sensor value
  this->process_gas_mj_();

  // Process Water
  const auto water_total = this->water_total_->get_state();
  if (std::isnan(water_total)) {
    return;
  }

  // Check if a new day has started for Water
  if (t.day_of_year != this->water_.current_day_of_year) {
    // Update start points for new day, week, month, year
    this->water_.start_yesterday = this->water_.start_today;
    this->water_.start_today = water_total;

    if (t.day_of_week == this->water_week_start_day_) {
      this->water_.start_week = water_total;
    }
    if (t.day_of_month == this->water_month_start_day_) {
      this->water_.start_month = water_total;
    }
    if (t.day_of_year == this->water_year_start_day_) {
      this->water_.start_year = water_total;
    }

    this->water_.current_day_of_year = t.day_of_year;  // Update the current day
  }

  // Continue processing the total water sensor value
  this->process_water_(water_total, t);

  // Save state at intervals
  uint32_t now = millis();
  if (now - last_save_time_ >= save_interval_ * 1000) {
    this->save_();
    last_save_time_ = now;
  }
}














