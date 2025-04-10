#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "water_statistics.h"

namespace esphome {
namespace water_statistics {

static const char *const TAG = "water_statistics";

static const char *const PREF_V1 = "water_statistics";
static const char *const PREF_V2 = "water_statistics_v2";

void WaterStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Water Statistics (L) - Sensors");
  if (this->water_today_) {
    LOG_SENSOR("  ", "Water (L) Today", this->water_today_);
  }
  if (this->water_yesterday_) {
    LOG_SENSOR("  ", "Water (L) Yesterday", this->water_yesterday_);
  }
  if (this->water_week_) {
    LOG_SENSOR("  ", "Water (L) Week", this->water_week_);
  }
  if (this->water_month_) {
    LOG_SENSOR("  ", "Water (L) Month", this->water_month_);
  }
  if (this->water_year_) {
    LOG_SENSOR("  ", "Water (L) Year", this->water_year_);
  }
}

void WaterStatistics::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });
  
  this->pref_ = global_preferences->make_preference<water_data_t>(fnv1_hash(PREF_V2));
  bool loaded = this->pref_.load(&this->water_);
  if (!loaded) {
    // migrating from v1 data
    loaded = global_preferences->make_preference<water_data_v1_t>(fnv1_hash(PREF_V1)).load(&this->water_);
    if (loaded) {
      this->water_.start_year = this->water_.start_month;
      // save as v2
      this->pref_.save(&this->water_);
      global_preferences->sync();
    }
  }
  if (loaded) {
    this->initial_total_retries_ = 40; // Try for 5 seconds (checked in loop)
    this->has_loaded_nvs_ = true;
  } else {
    ESP_LOGW(TAG, "No previous data loaded from NVS");
  }
}


void WaterStatistics::loop() {
// Handle initial total check non-blocking
  if (this->has_loaded_nvs_ && this->initial_total_retries_ > 0) {
    float total = this->total_->state;
    if (!std::isnan(total) && total > 0.0f) {
      ESP_LOGD(TAG, "Processing restored total: %f", total);
      this->process_(total);
      this->initial_total_retries_ = 0; // Done
      this->has_loaded_nvs_ = false;
    } else {
      ESP_LOGD(TAG, "Waiting for valid total: %f, retries: %d", total, this->initial_total_retries_);
      this->initial_total_retries_--;
      if (this->initial_total_retries_ == 0) {
        ESP_LOGW(TAG, "Total invalid after 5s: %f, retaining prior stats", total);
        this->has_loaded_nvs_ = false;
      }
      return; // Yield to avoid blocking
    }
  }  
  
  const auto t = this->time_->now();
  if (!t.is_valid()) {
    // time is not sync yet
    return;
  }

  const auto total = this->total_->get_state();
  if (std::isnan(total)) {
    // total is not published yet
    return;
  }

  // update stats first time or on next day
  if (t.day_of_year == this->water_.current_day_of_year) {
    // nothing to do
    return;
  }

  // Save the current day's data
  this->water_.start_yesterday = this->water_.start_today;
  
  this->water_.start_today = total;

  if (this->water_.current_day_of_year != 0) {
    // at specified day of week we start a new week calculation
    if (t.day_of_week == this->water_week_start_day_) {
      this->water_.start_week = total;
    }
    // at first day of month we start a new month calculation
    if (t.day_of_month == 1) {
      this->water_.start_month = total;
    }
    // at first day of year we start a new year calculation
    if (t.day_of_year == 1) {
      this->water_.start_year = total;
    }
  }

  // Intitialize all sensors. https://github.com/dentra/esphome-components/issues/65
  if (this->water_week_ && std::isnan(this->water_.start_week)) {
    this->water_.start_week = this->water_.start_yesterday;
  }
  if (this->water_month_ && std::isnan(this->water_.start_month)) {
    this->water_.start_month = this->water_.start_yesterday;
  }
  if (this->water_year_ && std::isnan(this->water_.start_year)) {
    this->water_.start_year = this->water_.start_yesterday;
  }

  this->water_.current_day_of_year = t.day_of_year;

  this->process_(total);
}


void WaterStatistics::process_(float total) {
  // Publish today's water
  if (this->water_today_ && !std::isnan(this->water_.start_today)) {
    this->water_today_->publish_state(total - this->water_.start_today);
  }
  
  // Publish yesterday's water
  if (this->water_yesterday_ && !std::isnan(this->water_.start_yesterday)) {
    this->water_yesterday_->publish_state(this->water_.start_today - this->water_.start_yesterday);
  } else if (this->water_yesterday_) {
    // If there's no value for yesterday (NaN), publish 0
    this->water_yesterday_->publish_state(0);
  }

  // Publish weekly water
  if (this->water_week_ && !std::isnan(this->water_.start_week)) {
    this->water_week_->publish_state(total - this->water_.start_week);
  } else if (this->water_week_) {
    // If there's no value for week (NaN), publish 0
    this->water_week_->publish_state(0);
  }

  // Publish monthly water
  if (this->water_month_ && !std::isnan(this->water_.start_month)) {
    this->water_month_->publish_state(total - this->water_.start_month);
  } else if (this->water_month_) {
    // If there's no value for month (NaN), publish 0
    this->water_month_->publish_state(0);
  }

  // Publish yearly water
  if (this->water_year_ && !std::isnan(this->water_.start_year)) {
    this->water_year_->publish_state(total - this->water_.start_year);
  } else if (this->water_year_) {
    // If there's no value for year (NaN), publish 0
    this->water_year_->publish_state(0);
  }

  this->pref_.save(&this->water_);
}

}  // namespace water_statistics
}  // namespace esphome
