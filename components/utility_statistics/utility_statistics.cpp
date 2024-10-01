#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "utility_statistics.h"

namespace esphome {
namespace utility_statistics {

static const char *const TAG = "utility_statistics";
static const char *const GAP = "  ";

// Time between warning log messages being repeated (in milliseconds)
static const uint32_t WARNING_LOG_INTERVAL = 60000;  // 60 seconds

void UtilityStatistics::dump_config() {
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


void UtilityStatistics::setup() {
  // Gas (m³) preferences setup
  this->pref_ = global_preferences->make_preference<gas_data_m3_t>(fnv1_hash("gas_m3"));
  gas_data_m3_t loaded_gas_m3{};
  if (this->pref_.load(&loaded_gas_m3) && !this->is_resetting_) {
    this->gas_m3_ = loaded_gas_m3;

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
      this->process_gas_m3_(total, this->time_->now());
    }
  } else {
    reset_gas_m3_statistics();  // Reset if the load fails or reset is requested
  }

  // Gas (MJ) preferences setup
  this->pref_mj_ = global_preferences->make_preference<gas_data_mj_t>(fnv1_hash("gas_mj"));
  gas_data_mj_t loaded_gas_mj{};
  if (this->pref_mj_.load(&loaded_gas_mj) && !this->is_resetting_) {
    this->gas_mj_ = loaded_gas_mj;

    if (this->gas_mj_today_ && !std::isnan(this->gas_mj_.gas_mj_today)) {
      this->gas_mj_today_->publish_state(this->gas_mj_.gas_mj_today);
    }
    if (this->gas_mj_yesterday_ && !std::isnan(this->gas_mj_.gas_mj_yesterday)) {
      this->gas_mj_yesterday_->publish_state(this->gas_mj_.gas_mj_yesterday);
    }
    if (this->gas_mj_week_ && !std::isnan(this->gas_mj_.gas_mj_week)) {
      this->gas_mj_week_->publish_state(this->gas_mj_.gas_mj_week);
    }
    if (this->gas_mj_month_ && !std::isnan(this->gas_mj_.gas_mj_month)) {
      this->gas_mj_month_->publish_state(this->gas_mj_.gas_mj_month);
    }
    if (this->gas_mj_year_ && !std::isnan(this->gas_mj_.gas_mj_year)) {
      this->gas_mj_year_->publish_state(this->gas_mj_.gas_mj_year);
    }

    auto total_mj = this->gas_mj_total_->get_state();
    if (!std::isnan(total_mj)) {
      this->process_gas_mj_();
    }
  } else {
    reset_gas_mj_statistics();  // Reset if the load fails or reset is requested
  }

  // Water preferences setup
  this->pref_water_ = global_preferences->make_preference<water_data_t>(fnv1_hash("water"));
  water_data_t loaded_water{};
  if (this->pref_water_.load(&loaded_water) && !this->is_resetting_) {
    this->water_ = loaded_water;

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

    auto total_water = this->water_total_->get_state();
    if (!std::isnan(total_water)) {
      this->process_water_(total_water, this->time_->now());
    }
  } else {
    reset_water_statistics();  // Reset if the load fails or reset is requested
  }
}


void UtilityStatistics::loop() {
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
  if (data_changed && (now - last_save_time_ >= save_interval_ * 1000)) {
    this->save_();  // Save the changed data to NVS
    last_save_time_ = now;  // Update the last save time
    data_changed = false;   // Reset the flag after saving
  }
}


// Process Gas (m³)
void UtilityStatistics::process_gas_m3_(float total, const esphome::time::ESPTime &t) {
  uint32_t now = millis();

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

    // Once we have a valid reading, set the start values
    this->gas_m3_.start_today = total;
    this->gas_m3_.start_yesterday = total;
    this->gas_m3_.start_week = total;
    this->gas_m3_.start_month = total;
    this->gas_m3_.start_year = total;

    this->waiting_for_sensor_read_ = false;  // Disable the wait flag
    ESP_LOGI(TAG, "Gas Statistics (m³) - Valid sensor reading obtained: %.3f", total);
    data_changed = true;  // Data has changed, so set the flag
  }

  // Ensure total is greater than or equal to start points and clamp negative values
  if (total < this->gas_m3_.start_today || std::isnan(this->gas_m3_.start_today)) {
    // Only log the warning once per minute
    if (now - this->last_warning_time_ >= WARNING_LOG_INTERVAL) {
      ESP_LOGW(TAG, "Gas Statistics (m³) - 'Total Gas' sensor total is less than start point or invalid. Skipping.");
      this->last_warning_time_ = now;  // Update the last warning log time
    }
    return;
  }

  // Update gas today only if the value has changed
  if (this->gas_m3_today_ && !std::isnan(this->gas_m3_.start_today)) {
    float new_gas_today = total - this->gas_m3_.start_today;
    if (this->gas_m3_today_->get_state() != new_gas_today) {
      this->gas_m3_.gas_m3_today = new_gas_today;
      this->gas_m3_today_->publish_state(this->gas_m3_.gas_m3_today);
      data_changed = true;  // Mark that data has changed
    }
  } else if (this->gas_m3_today_ && (std::isnan(this->gas_m3_today_->get_state()) || this->gas_m3_today_->get_state() != 0.0)) {
    // If gas_today_ is NaN or not 0.0, publish 0.0
    this->gas_m3_today_->publish_state(0.0);
  }

  // Update gas yesterday only if the value has changed
  if (this->gas_m3_yesterday_ && !std::isnan(this->gas_m3_.start_yesterday)) {
    float new_gas_yesterday = this->gas_m3_.start_today - this->gas_m3_.start_yesterday;
    if (this->gas_m3_yesterday_->get_state() != new_gas_yesterday) {
      this->gas_m3_.gas_m3_yesterday = new_gas_yesterday;
      this->gas_m3_yesterday_->publish_state(this->gas_m3_.gas_m3_yesterday);
      data_changed = true;  // Data has changed, so set the flag
    }
  } else if (this->gas_m3_yesterday_ && (std::isnan(this->gas_m3_yesterday_->get_state()) || this->gas_m3_yesterday_->get_state() != 0.0)) {
    // If gas_yesterday_ is NaN or not 0.0, publish 0.0
    this->gas_m3_yesterday_->publish_state(0.0);
  }

  // Update gas week only if the value has changed
  if (this->gas_m3_week_ && !std::isnan(this->gas_m3_.start_week)) {
    float new_gas_week = total - this->gas_m3_.start_week;
    if (this->gas_m3_week_->get_state() != new_gas_week) {
      this->gas_m3_.gas_m3_week = new_gas_week;
      this->gas_m3_week_->publish_state(this->gas_m3_.gas_m3_week);
      data_changed = true;  // Data has changed, so set the flag
    }
  } else if (this->gas_m3_week_ && (std::isnan(this->gas_m3_week_->get_state()) || this->gas_m3_week_->get_state() != 0.0)) {
    // If gas_week_ is NaN or not 0.0, publish 0.0
    this->gas_m3_week_->publish_state(0.0);
  }

  // Update gas month only if the value has changed
  if (this->gas_m3_month_ && !std::isnan(this->gas_m3_.start_month)) {
    float new_gas_month = total - this->gas_m3_.start_month;
    if (this->gas_m3_month_->get_state() != new_gas_month) {
      this->gas_m3_.gas_m3_month = new_gas_month;
      this->gas_m3_month_->publish_state(this->gas_m3_.gas_m3_month);
      data_changed = true;  // Data has changed, so set the flag
    }
  } else if (this->gas_m3_month_ && (std::isnan(this->gas_m3_month_->get_state()) || this->gas_m3_month_->get_state() != 0.0)) {
    // If gas_month_ is NaN or not 0.0, publish 0.0
    this->gas_m3_month_->publish_state(0.0);
  }

  // Update gas year only if the value has changed
  if (this->gas_m3_year_ && !std::isnan(this->gas_m3_.start_year)) {
    float new_gas_year = total - this->gas_m3_.start_year;
    if (this->gas_m3_year_->get_state() != new_gas_year) {
      this->gas_m3_.gas_m3_year = new_gas_year;
      this->gas_m3_year_->publish_state(this->gas_m3_.gas_m3_year);
      data_changed = true;  // Data has changed, so set the flag
    }
  } else if (this->gas_m3_year_ && (std::isnan(this->gas_m3_year_->get_state()) || this->gas_m3_year_->get_state() != 0.0)) {
    // If gas_year_ is NaN or not 0.0, publish 0.0
    this->gas_m3_year_->publish_state(0.0);
  }
}


// Process Gas (MJ)
void UtilityStatistics::process_gas_mj_() {
  float gas_mj = this->gas_mj_total_->get_state();  // Get the total in MJ
  uint32_t now = millis();

  // If we're waiting for the sensor to update, skip calculation until valid
  if (this->waiting_for_sensor_read_) {
    if (std::isnan(gas_mj) || gas_mj == 0.0) {
      // Only log the warning once per minute
      if (now - this->last_warning_time_ >= WARNING_LOG_INTERVAL) {
        ESP_LOGW(TAG, "Gas Statistics (MJ) - Skipping sensor reading update, waiting for valid sensor reading.");
        this->last_warning_time_ = now;  // Update the last warning log time
      }
      return;
    }

    // Once we have a valid reading, set the start values
    this->gas_mj_.start_today = gas_mj;
    this->gas_mj_.start_yesterday = gas_mj;
    this->gas_mj_.start_week = gas_mj;
    this->gas_mj_.start_month = gas_mj;
    this->gas_mj_.start_year = gas_mj;

    this->waiting_for_sensor_read_ = false;  // Disable the wait flag
    ESP_LOGI(TAG, "Gas Statistics (MJ) - Valid sensor reading obtained: %.3f", gas_mj);
    data_changed = true;  // Data has changed, so set the flag
  }

  // Ensure total is greater than or equal to start points and clamp negative values
  if (gas_mj < this->gas_mj_.start_today || std::isnan(this->gas_mj_.start_today)) {
    // Only log the warning once per minute
    if (now - this->last_warning_time_ >= WARNING_LOG_INTERVAL) {
      ESP_LOGW(TAG, "Gas Statistics (MJ) - 'Total Gas' sensor total is less than start point or invalid. Skipping.");
      this->last_warning_time_ = now;  // Update the last warning log time
    }
    return;
  }

  // Update gas (MJ) today only if the value has changed
  if (this->gas_mj_today_ && !std::isnan(this->gas_mj_.start_today)) {
    float new_gas_today = gas_mj - this->gas_mj_.start_today;
    if (this->gas_mj_today_->get_state() != new_gas_today) {
      this->gas_mj_.gas_mj_today = new_gas_today;
      this->gas_mj_today_->publish_state(this->gas_mj_.gas_mj_today);
      data_changed = true;  // Data has changed, so set the flag
    }
  } else if (this->gas_mj_today_ && (std::isnan(this->gas_mj_today_->get_state()) || this->gas_mj_today_->get_state() != 0.0)) {
    // If gas_today_ is NaN or not 0.0, publish 0.0
    this->gas_mj_today_->publish_state(0.0);
  }

  // Update gas (MJ) yesterday only if the value has changed
  if (this->gas_mj_yesterday_ && !std::isnan(this->gas_mj_.start_yesterday)) {
    float new_gas_yesterday = this->gas_mj_.start_today - this->gas_mj_.start_yesterday;
    if (this->gas_mj_yesterday_->get_state() != new_gas_yesterday) {
      this->gas_mj_.gas_mj_yesterday = new_gas_yesterday;
      this->gas_mj_yesterday_->publish_state(this->gas_mj_.gas_mj_yesterday);
      data_changed = true;  // Data has changed, so set the flag
    }
  } else if (this->gas_mj_yesterday_ && (std::isnan(this->gas_mj_yesterday_->get_state()) || this->gas_mj_yesterday_->get_state() != 0.0)) {
    this->gas_mj_yesterday_->publish_state(0.0);
  }

  // Update gas (MJ) week only if the value has changed
  if (this->gas_mj_week_ && !std::isnan(this->gas_mj_.start_week)) {
    float new_gas_week = gas_mj - this->gas_mj_.start_week;
    if (this->gas_mj_week_->get_state() != new_gas_week) {
      this->gas_mj_.gas_mj_week = new_gas_week;
      this->gas_mj_week_->publish_state(this->gas_mj_.gas_mj_week);
      data_changed = true;  // Data has changed, so set the flag
    }
  } else if (this->gas_mj_week_ && (std::isnan(this->gas_mj_week_->get_state()) || this->gas_mj_week_->get_state() != 0.0)) {
    this->gas_mj_week_->publish_state(0.0);
  }

  // Update gas (MJ) month only if the value has changed
  if (this->gas_mj_month_ && !std::isnan(this->gas_mj_.start_month)) {
    float new_gas_month = gas_mj - this->gas_mj_.start_month;
    if (this->gas_mj_month_->get_state() != new_gas_month) {
      this->gas_mj_.gas_mj_month = new_gas_month;
      this->gas_mj_month_->publish_state(this->gas_mj_.gas_mj_month);
      data_changed = true;  // Data has changed, so set the flag
    }
  } else if (this->gas_mj_month_ && (std::isnan(this->gas_mj_month_->get_state()) || this->gas_mj_month_->get_state() != 0.0)) {
    this->gas_mj_month_->publish_state(0.0);
  }

  // Update gas (MJ) year only if the value has changed
  if (this->gas_mj_year_ && !std::isnan(this->gas_mj_.start_year)) {
    float new_gas_year = gas_mj - this->gas_mj_.start_year;
    if (this->gas_mj_year_->get_state() != new_gas_year) {
      this->gas_mj_.gas_mj_year = new_gas_year;
      this->gas_mj_year_->publish_state(this->gas_mj_.gas_mj_year);
      data_changed = true;  // Data has changed, so set the flag
    }
  } else if (this->gas_mj_year_ && (std::isnan(this->gas_mj_year_->get_state()) || this->gas_mj_year_->get_state() != 0.0)) {
    this->gas_mj_year_->publish_state(0.0);
  }
}






void UtilityStatistics::process_water_(float total, const esphome::time::ESPTime &t) {
  uint32_t now = millis();

  // If we're waiting for the sensor to update, skip calculation until valid
  if (this->waiting_for_sensor_read_) {
    if (std::isnan(total) || total == 0.0) {
      // Only log the warning once per minute
      if (now - this->last_warning_time_ >= WARNING_LOG_INTERVAL) {
        ESP_LOGW(TAG, "Water Statistics - Skipping sensor reading update, waiting for valid sensor reading.");
        this->last_warning_time_ = now;  // Update the last warning log time
      }
      return;
    }

    // Once we have a valid reading, set the start values
    this->water_.start_today = total;
    this->water_.start_yesterday = total;
    this->water_.start_week = total;
    this->water_.start_month = total;
    this->water_.start_year = total;

    this->waiting_for_sensor_read_ = false;  // Disable the wait flag
    ESP_LOGI(TAG, "Water Statistics - Valid sensor reading obtained: %.3f", total);
    data_changed = true;  // Data has changed, so set the flag
  }

  // Ensure total is greater than or equal to start points and clamp negative values
  if (total < this->water_.start_today || std::isnan(this->water_.start_today)) {
    // Only log the warning once per minute
    if (now - this->last_warning_time_ >= WARNING_LOG_INTERVAL) {
      ESP_LOGW(TAG, "Water Statistics - 'Total Water' sensor total is less than start point or invalid. Skipping.");
      this->last_warning_time_ = now;  // Update the last warning log time
    }
    return;
  }

  // Update water today only if the value has changed
  if (this->water_today_ && !std::isnan(this->water_.start_today)) {
    float new_water_today = total - this->water_.start_today;
    if (this->water_today_->get_state() != new_water_today) {
      this->water_.water_today = new_water_today;
      this->water_today_->publish_state(this->water_.water_today);
      data_changed = true;  // Data has changed, so set the flag
    }
  } else if (this->water_today_ && (std::isnan(this->water_today_->get_state()) || this->water_today_->get_state() != 0.0)) {
    // If water_today_ is NaN or not 0.0, publish 0.0
    this->water_today_->publish_state(0.0);
  }

  // Update water yesterday only if the value has changed
  if (this->water_yesterday_ && !std::isnan(this->water_.start_yesterday)) {
    float new_water_yesterday = this->water_.start_today - this->water_.start_yesterday;
    if (this->water_yesterday_->get_state() != new_water_yesterday) {
      this->water_.water_yesterday = new_water_yesterday;
      this->water_yesterday_->publish_state(this->water_.water_yesterday);
      data_changed = true;  // Data has changed, so set the flag
    }
  } else if (this->water_yesterday_ && (std::isnan(this->water_yesterday_->get_state()) || this->water_yesterday_->get_state() != 0.0)) {
    this->water_yesterday_->publish_state(0.0);
  }

  // Update water week only if the value has changed
  if (this->water_week_ && !std::isnan(this->water_.start_week)) {
    float new_water_week = total - this->water_.start_week;
    if (this->water_week_->get_state() != new_water_week) {
      this->water_.water_week = new_water_week;
      this->water_week_->publish_state(this->water_.water_week);
      data_changed = true;  // Data has changed, so set the flag
    }
  } else if (this->water_week_ && (std::isnan(this->water_week_->get_state()) || this->water_week_->get_state() != 0.0)) {
    this->water_week_->publish_state(0.0);
  }

  // Update water month only if the value has changed
  if (this->water_month_ && !std::isnan(this->water_.start_month)) {
    float new_water_month = total - this->water_.start_month;
    if (this->water_month_->get_state() != new_water_month) {
      this->water_.water_month = new_water_month;
      this->water_month_->publish_state(this->water_.water_month);
      data_changed = true;  // Data has changed, so set the flag
    }
  } else if (this->water_month_ && (std::isnan(this->water_month_->get_state()) || this->water_month_->get_state() != 0.0)) {
    this->water_month_->publish_state(0.0);
  }

  // Update water year only if the value has changed
  if (this->water_year_ && !std::isnan(this->water_.start_year)) {
    float new_water_year = total - this->water_.start_year;
    if (this->water_year_->get_state() != new_water_year) {
      this->water_.water_year = new_water_year;
      this->water_year_->publish_state(this->water_.water_year);
      data_changed = true;  // Data has changed, so set the flag
    }
  } else if (this->water_year_ && (std::isnan(this->water_year_->get_state()) || this->water_year_->get_state() != 0.0)) {
    this->water_year_->publish_state(0.0);
  }
}


void UtilityStatistics::loop() {
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
  if (!std::isnan(gas_total_m3)) {
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
      this->process_gas_m3_(gas_total_m3, t);  // Process new gas values
    }

    // Continue processing the total gas (m³) sensor value
    this->process_gas_m3_(gas_total_m3, t);
  }

  // Process Gas (MJ)
  const auto gas_total_mj = this->gas_mj_total_->get_state();
  if (!std::isnan(gas_total_mj)) {
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
      this->process_gas_mj_();  // Process new gas values (MJ)
    }

    // Continue processing the total gas (MJ) sensor value
    this->process_gas_mj_();
  }

  // Process Water
  const auto water_total = this->water_total_->get_state();
  if (!std::isnan(water_total)) {
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
      this->process_water_(water_total, t);  // Process new water values
    }

    // Continue processing the total water sensor value
    this->process_water_(water_total, t);
  }

  // Only save to flash if necessary, for all utility at once
  uint32_t now = millis();
  if (now - last_save_time_ >= save_interval_ * 1000) {
    this->save_();
    last_save_time_ = now;  // Update the last save time
  }
}


void UtilityStatistics::reset_gas_m3_statistics() {
  uint32_t now = millis();  // Get the current time
  ESP_LOGI(TAG, "Gas Statistics (m³) - Resetting values to 0.0");

  // Reset Gas (m³) values
  this->gas_m3_.gas_m3_today = 0.0;
  this->gas_m3_.gas_m3_yesterday = 0.0;
  this->gas_m3_.gas_m3_week = 0.0;
  this->gas_m3_.gas_m3_month = 0.0;
  this->gas_m3_.gas_m3_year = 0.0;

  const auto gas_total_m3 = this->gas_m3_total_->get_state();
  if (!std::isnan(gas_total_m3) && gas_total_m3 != 0.0) {
    // Use the current total value as the new start points
    this->gas_m3_.start_today = gas_total_m3;
    this->gas_m3_.start_yesterday = gas_total_m3;
    this->gas_m3_.start_week = gas_total_m3;
    this->gas_m3_.start_month = gas_total_m3;
    this->gas_m3_.start_year = gas_total_m3;
    ESP_LOGI(TAG, "Gas Statistics (m³) - Start points set after reset: %.3f", gas_total_m3);
  } else {
    this->waiting_for_sensor_read_ = true;
    ESP_LOGW(TAG, "Gas Statistics (m³) - Total is invalid, waiting for valid sensor reading.");
  }

  // Publish the reset values to sensors
  if (this->gas_m3_today_) this->gas_m3_today_->publish_state(0.0);
  if (this->gas_m3_yesterday_) this->gas_m3_yesterday_->publish_state(0.0);
  if (this->gas_m3_week_) this->gas_m3_week_->publish_state(0.0);
  if (this->gas_m3_month_) this->gas_m3_month_->publish_state(0.0);
  if (this->gas_m3_year_) this->gas_m3_year_->publish_state(0.0);

  // Save reset state to flash memory
  this->save_();
}


void UtilityStatistics::reset_gas_mj_statistics() {
  uint32_t now = millis();  // Get the current time
  ESP_LOGI(TAG, "Gas Statistics (MJ) - Resetting values to 0.0");

  // Reset Gas (MJ) values
  this->gas_mj_.gas_mj_today = 0.0;
  this->gas_mj_.gas_mj_yesterday = 0.0;
  this->gas_mj_.gas_mj_week = 0.0;
  this->gas_mj_.gas_mj_month = 0.0;
  this->gas_mj_.gas_mj_year = 0.0;

  const auto gas_total_mj = this->gas_mj_total_->get_state();
  if (!std::isnan(gas_total_mj) && gas_total_mj != 0.0) {
    this->gas_mj_.start_today = gas_total_mj;
    this->gas_mj_.start_yesterday = gas_total_mj;
    this->gas_mj_.start_week = gas_total_mj;
    this->gas_mj_.start_month = gas_total_mj;
    this->gas_mj_.start_year = gas_total_mj;
    ESP_LOGI(TAG, "Gas Statistics (MJ) - Start points set after reset: %.3f", gas_total_mj);
  } else {
    this->waiting_for_sensor_read_ = true;
    ESP_LOGW(TAG, "Gas Statistics (MJ) - Total is invalid, waiting for valid sensor reading.");
  }

  // Publish the reset values to sensors
  if (this->gas_mj_today_) this->gas_mj_today_->publish_state(0.0);
  if (this->gas_mj_yesterday_) this->gas_mj_yesterday_->publish_state(0.0);
  if (this->gas_mj_week_) this->gas_mj_week_->publish_state(0.0);
  if (this->gas_mj_month_) this->gas_mj_month_->publish_state(0.0);
  if (this->gas_mj_year_) this->gas_mj_year_->publish_state(0.0);

  // Save reset state to flash memory
  this->save_();
}


void UtilityStatistics::reset_water_statistics() {
  uint32_t now = millis();  // Get the current time
  ESP_LOGI(TAG, "Water Statistics - Resetting values to 0.0");

  // Reset Water values
  this->water_.water_today = 0.0;
  this->water_.water_yesterday = 0.0;
  this->water_.water_week = 0.0;
  this->water_.water_month = 0.0;
  this->water_.water_year = 0.0;

  const auto water_total = this->water_total_->get_state();
  if (!std::isnan(water_total) && water_total != 0.0) {
    this->water_.start_today = water_total;
    this->water_.start_yesterday = water_total;
    this->water_.start_week = water_total;
    this->water_.start_month = water_total;
    this->water_.start_year = water_total;
    ESP_LOGI(TAG, "Water Statistics - Start points set after reset: %.3f", water_total);
  } else {
    this->waiting_for_sensor_read_ = true;
    ESP_LOGW(TAG, "Water Statistics - Total is invalid, waiting for valid sensor reading.");
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


void UtilityStatistics::save_() {
  // Save Gas (m³) data
  if (this->gas_m3_total_) {
    this->pref_.save(&this->gas_m3_);  // Save the Gas (m³) data to NVS
    ESP_LOGD(TAG, "Gas Statistics (m³) - Values saved to flash memory (NVS).");
  }

  // Save Gas (MJ) data
  if (this->gas_mj_total_) {
    this->pref_mj_.save(&this->gas_mj_);  // Save the Gas (MJ) data to NVS
    ESP_LOGD(TAG, "Gas Statistics (MJ) - Values saved to flash memory (NVS).");
  }

  // Save Water data
  if (this->water_total_) {
    this->pref_water_.save(&this->water_);  // Save the Water data to NVS
    ESP_LOGD(TAG, "Water Statistics - Values saved to flash memory (NVS).");
  }
}

}  // namespace utility_statistics
}  // namespace esphome
