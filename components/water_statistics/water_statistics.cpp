#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "water_statistics.h"

namespace esphome {
namespace water_statistics {

static const char *const TAG = "water_statistics";
static const char *const GAP = "  ";

void WaterStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Water Statistics (L) - Sensors");
  if (this->water_today_) {
    LOG_SENSOR(GAP, "Water (L) Today", this->water_today_);
  }
  if (this->water_yesterday_) {
    LOG_SENSOR(GAP, "Water (L) Yesterday", this->water_yesterday_);
  }
  if (this->water_week_) {
    LOG_SENSOR(GAP, "Water (L) Week", this->water_week_);
  }
  if (this->water_month_) {
    LOG_SENSOR(GAP, "Water (L) Month", this->water_month_);
  }
  if (this->water_year_) {
    LOG_SENSOR(GAP, "Water (L) Year", this->water_year_);
  }
}

void WaterStatistics::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });
  
  this->pref_ = global_preferences->make_preference<water_data_t>(fnv1_hash(TAG));

  water_data_t loaded{};
  if (this->pref_.load(&loaded)) {
    this->water_ = loaded;
    auto total = this->total_->get_state();
    if (!std::isnan(total)) {
      this->process_(total);
    }
  }
}

void WaterStatistics::loop() {
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

  this->water_.current_day_of_year = t.day_of_year;

  this->process_(total);
}


void WaterStatistics::process_(float total) {
  // Publish today's water
  if (this->water_today_ && !std::isnan(this->water_.start_today)) {
    this->water_today_->publish_state(total - this->water_.start_today);
  } else if (std::isnan(this->water_.start_today)) {
    // Initialize today's start value
    this->water_.start_today = total;
    if (this->water_today_) {
      this->water_today_->publish_state(0);  // Publish initial value as 0
    }
  }

  // Publish yesterday's water
  if (this->water_yesterday_ && !std::isnan(this->water_.start_yesterday)) {
    this->water_yesterday_->publish_state(this->water_.start_today - this->water_.start_yesterday);
  }

  // Publish weekly water (partial or full)
  if (this->water_week_ && !std::isnan(this->water_.start_week)) {
    if (this->water_.full_week_started) {
      // Publish full calendar week value
      this->water_week_->publish_state(total - this->water_.start_week);
    } else {
      // Publish partial week value
      this->water_week_->publish_state(total - this->water_.start_week);
    }
  } else if (std::isnan(this->water_.start_week)) {
    // Initialize start_week for the first time
    this->water_.start_week = total;
    if (this->water_week_) {
      this->water_week_->publish_state(0);  // Publish initial value as 0
    }
  }

  // Publish monthly water (partial or full)
  if (this->water_month_ && !std::isnan(this->water_.start_month)) {
    if (this->water_.full_month_started) {
      // Publish full calendar month value
      this->water_month_->publish_state(total - this->water_.start_month);
    } else {
      // Publish partial month value
      this->water_month_->publish_state(total - this->water_.start_month);
    }
  } else if (std::isnan(this->water_.start_month)) {
    // Initialize start_month for the first time
    this->water_.start_month = total;
    if (this->water_month_) {
      this->water_month_->publish_state(0);  // Publish initial value as 0
    }
  }

  // Publish yearly water (partial or full)
  if (this->water_year_ && !std::isnan(this->water_.start_year)) {
    if (this->water_.full_year_started) {
      // Publish full calendar year value
      this->water_year_->publish_state(total - this->water_.start_year);
    } else {
      // Publish partial year value
      this->water_year_->publish_state(total - this->water_.start_year);
    }
  } else if (std::isnan(this->water_.start_year)) {
    // Initialize start_year for the first time
    this->water_.start_year = total;
    if (this->water_year_) {
      this->water_year_->publish_state(0);  // Publish initial value as 0
    }
  }

  // Save the current state
  this->save_();
}

void WaterStatistics::save_() { this->pref_.save(&this->water_); } // Save to flash memory

}  // namespace water_statistics
}  // namespace esphome
