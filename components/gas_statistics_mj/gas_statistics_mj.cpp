#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "gas_statistics_mj.h"

namespace esphome {
namespace gas_statistics_mj {

static const char *const TAG = "gas_statistics_mj";

static const char *const PREF_V1 = "gas_statistics_mj";
static const char *const PREF_V2 = "gas_statistics_mj_v2";

void GasStatisticsMJ::dump_config() {
  ESP_LOGCONFIG(TAG, "Gas Statistics (MJ) - Sensors");
  if (this->gas_today_) {
    LOG_SENSOR("  ", "Gas (MJ) Today", this->gas_today_);
  }
  if (this->gas_yesterday_) {
    LOG_SENSOR("  ", "Gas (MJ) Yesterday", this->gas_yesterday_);
  }
  if (this->gas_week_) {
    LOG_SENSOR("  ", "Gas (MJ) Week", this->gas_week_);
  }
  if (this->gas_month_) {
    LOG_SENSOR("  ", "Gas (MJ) Month", this->gas_month_);
  }
  if (this->gas_year_) {
    LOG_SENSOR("  ", "Gas (MJ) Year", this->gas_year_);
  }
}


void GasStatisticsMJ::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });

  this->pref_ = global_preferences->make_preference<gas_mj_data_t>(fnv1_hash(PREF_V2));
  bool loaded = this->pref_.load(&this->gas_);
  if (!loaded) {
    // migrating from v1 data
    loaded = global_preferences->make_preference<gas_mj_data_v1_t>(fnv1_hash(PREF_V1)).load(&this->gas_);
    if (loaded) {
      this->gas_.start_year = this->gas_.start_month;
      // save as v2
      this->pref_.save(&this->gas_);
      global_preferences->sync();
    }
  }
  if (loaded) {
    auto total = this->total_->get_state();
    if (!std::isnan(total)) {
      this->process_(total);
    }
  }
}


void GasStatisticsMJ::loop() {
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
  if (t.day_of_year == this->gas_.current_day_of_year) {
    // nothing to do
    return;
  }

  this->gas_.start_yesterday = this->gas_.start_today;

  this->gas_.start_today = total;

  if (this->gas_.current_day_of_year != 0) {
    // at specified day of week we start a new week calculation
    if (t.day_of_week == this->gas_week_start_day_) {
      this->gas_.start_week = total;
    }
    // at first day of month we start a new month calculation
    if (t.day_of_month == 1) {
      this->gas_.start_month = total;
    }
    // at first day of year we start a new year calculation
    if (t.day_of_year == 1) {
      this->gas_.start_year = total;
    }
  }

  // Intitialize all sensors. https://github.com/dentra/esphome-components/issues/65
  if (this->gas_week_ && std::isnan(this->gas_.start_week)) {
    this->gas_.start_week = this->gas_.start_yesterday;
  }
  if (this->gas_month_ && std::isnan(this->gas_.start_month)) {
    this->gas_.start_month = this->gas_.start_yesterday;
  }
  if (this->gas_year_ && std::isnan(this->gas_.start_year)) {
    this->gas_.start_year = this->gas_.start_yesterday;
  }
  
  this->gas_.current_day_of_year = t.day_of_year;

  this->process_(total);
}


void GasStatisticsMJ::process_(float total) {
  if (this->gas_today_ && !std::isnan(this->gas_.start_today)) {
    this->gas_today_->publish_state(total - this->gas_.start_today);
  }

  if (this->gas_yesterday_ && !std::isnan(this->gas_.start_yesterday)) {
    this->gas_yesterday_->publish_state(this->gas_.start_today - this->gas_.start_yesterday);
  } else if (this->gas_yesterday_) {
    // If there's no value for yesterday (NaN), publish 0
    this->gas_yesterday_->publish_state(0);
  }

  if (this->gas_week_ && !std::isnan(this->gas_.start_week)) {
    this->gas_week_->publish_state(total - this->gas_.start_week);
  } else if (this->gas_week_) {
    // If there's no value for week (NaN), publish 0
    this->gas_week_->publish_state(0);
  }

  if (this->gas_month_ && !std::isnan(this->gas_.start_month)) {
    this->gas_month_->publish_state(total - this->gas_.start_month);
  } else if (this->gas_month_) {
    // If there's no value for month (NaN), publish 0
    this->gas_month_->publish_state(0);
  }

  if (this->gas_year_ && !std::isnan(this->gas_.start_year)) {
    this->gas_year_->publish_state(total - this->gas_.start_year);
  } else if (this->gas_year_) {
    // If there's no value for year (NaN), publish 0
    this->gas_year_->publish_state(0);
  }

  this->pref_.save(&this->gas_);
}

}  // namespace gas_statistics
}  // namespace esphome
