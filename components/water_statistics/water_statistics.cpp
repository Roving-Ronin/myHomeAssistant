#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "water_statistics.h"

namespace esphome {
namespace water_statistics {

static const char *const TAG = "water_statistics";
static const char *const GAP = "  ";

void WaterStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Water statistics sensors");
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

  // Add logs for the loaded values
  ESP_LOGCONFIG(TAG, "Restored Water Today: %.3f", this->water_.water_today);
  ESP_LOGCONFIG(TAG, "Restored Water Yesterday: %.3f", this->water_.water_yesterday);
  ESP_LOGCONFIG(TAG, "Restored Water Week: %.3f", this->water_.water_week);
  ESP_LOGCONFIG(TAG, "Restored Water Month: %.3f", this->water_.water_month);
  ESP_LOGCONFIG(TAG, "Restored Water Year: %.3f", this->water_.water_year);
}

void WaterStatistics::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });

  this->pref_ = global_preferences->make_preference<water_data_t>(fnv1_hash(TAG));

  water_data_t loaded{};
  if (this->pref_.load(&loaded)) {
    this->water_ = loaded;

    // Load the sensor values from preferences if available
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
  // Water Today
  if (this->water_today_ && !std::isnan(this->water_.start_today)) {
    this->water_.water_today = total - this->water_.start_today;
    this->water_today_->publish_state(this->water_.water_today);
  } else if (this->water_today_) {
    // Show 0 instead of NA if no valid data
    this->water_today_->publish_state(0.0);
  }

  // Water Yesterday
  if (this->water_yesterday_ && !std::isnan(this->water_.start_yesterday)) {
    this->water_.water_yesterday = this->water_.start_today - this->water_.start_yesterday;
    this->water_yesterday_->publish_state(this->water_.water_yesterday);
  } else if (this->water_yesterday_) {
    this->water_yesterday_->publish_state(0.0);
  }

  // Water Week
  if (this->water_week_ && !std::isnan(this->water_.start_week)) {
    this->water_.water_week = total - this->water_.start_week;
    this->water_week_->publish_state(this->water_.water_week);
  } else if (this->water_week_) {
    this->water_week_->publish_state(0.0);
  }

  // Water Month
  if (this->water_month_ && !std::isnan(this->water_.start_month)) {
    this->water_.water_month = total - this->water_.start_month;
    this->water_month_->publish_state(this->water_.water_month);
  } else if (this->water_month_) {
    this->water_month_->publish_state(0.0);
  }

  // Water Year
  if (this->water_year_ && !std::isnan(this->water_.start_year)) {
    this->water_.water_year = total - this->water_.start_year;
    this->water_year_->publish_state(this->water_.water_year);
  } else if (this->water_year_) {
    this->water_year_->publish_state(0.0);
  }

  // Save the updated values to preferences
  this->save_();
}

void WaterStatistics::save_() { this->pref_.save(&(this->water_)); 
}

}  // namespace water_statistics
}  // namespace esphome
