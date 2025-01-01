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

  this->water_.start_yesterday = this->water_.start_today;
  this->water_.start_today = total;

  // Adjust start of week/month/year based on partial period
  if (this->water_.current_day_of_year != 0) {
    // Start week on installation/reset, not just on specific day
    if (t.day_of_week == this->water_week_start_day_ || this->water_.start_week == NAN) {
      this->water_.start_week = total;
    }
    // Start month on installation/reset, not just on the 1st
    if (t.day_of_month == 1 || this->water_.start_month == NAN) {
      this->water_.start_month = total;
    }
    // Start year on installation/reset, not just on the 1st
    if (t.day_of_year == 1 || this->water_.start_year == NAN) {
      this->water_.start_year = total;
    }
  }

  this->water_.current_day_of_year = t.day_of_year;

  this->process_(total);
}

void WaterStatistics::process_(float total) {
  if (this->water_today_ && !std::isnan(this->water_.start_today)) {
    this->water_today_->publish_state(total - this->water_.start_today);
  }

  if (this->water_yesterday_ && !std::isnan(this->water_.start_yesterday)) {
    this->water_yesterday_->publish_state(this->water_.start_today - this->water_.start_yesterday);
  }

  if (this->water_week_ && !std::isnan(this->water_.start_week)) {
    this->water_week_->publish_state(total - this->water_.start_week);
  }

  if (this->water_month_ && !std::isnan(this->water_.start_month)) {
    this->water_month_->publish_state(total - this->water_.start_month);
  }

  if (this->water_year_ && !std::isnan(this->water_.start_year)) {
    this->water_year_->publish_state(total - this->water_.start_year);
  }
  
  this->save_();
}

void WaterStatistics::save_() { this->pref_.save(&this->water_); } // Save to flash memory

}  // namespace water_statistics
}  // namespace esphome
