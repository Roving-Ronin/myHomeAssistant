#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "gas_statistics_mj.h"

namespace esphome {
namespace gas_statistics_mj {

static const char *const TAG = "gas_statistics_mj";
static const char *const GAP = "  ";

void GasStatisticsMJ::dump_config() {
  ESP_LOGCONFIG(TAG, "Gas Statistics (MJ) - Sensors");
    if (this->gas_today_ && !this->gas_today_->is_internal()) {
    LOG_SENSOR(GAP, "Gas (MJ) Today", this->gas_today_);
  }
  if (this->gas_yesterday_ && !this->gas_yesterday_->is_internal()) {
    LOG_SENSOR(GAP, "Gas (MJ) Yesterday", this->gas_yesterday_);
  }
  if (this->gas_week_ && !this->gas_week_->is_internal()) {
    LOG_SENSOR(GAP, "Gas (MJ) Week", this->gas_week_);
  }
  if (this->gas_month_ && !this->gas_month_->is_internal()) {
    LOG_SENSOR(GAP, "Gas (MJ) Month", this->gas_month_);
  }
  if (this->gas_year_ && !this->gas_year_->is_internal()) {
    LOG_SENSOR(GAP, "Gas (MJ) Year", this->gas_year_);
  }
}

void GasStatisticsMJ::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });
  
  this->pref_ = global_preferences->make_preference<gas_data_mj_t>(fnv1_hash(TAG));

  gas_data_mj_t loaded{};
  if (this->pref_.load(&loaded)) {
    this->gas_ = loaded;
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

  this->gas_.current_day_of_year = t.day_of_year;

  this->process_(total);
}


void GasStatisticsMJ::process_(float total) {
  // Publish today's gas
  if (this->gas_today_ && !std::isnan(this->gas_.start_today)) {
    this->gas_today_->publish_state(total - this->gas_.start_today);
  } else if (std::isnan(this->gas_.start_today)) {
    // Initialize today's start value
    this->gas_.start_today = total;
    if (this->gas_today_) {
      this->gas_today_->publish_state(0);  // Publish initial value as 0
    }
  }

  // Publish yesterday's gas
  if (this->gas_yesterday_ && !std::isnan(this->gas_.start_yesterday)) {
    this->gas_yesterday_->publish_state(this->gas_.start_today - this->gas_.start_yesterday);
  }

  // Publish weekly gas (partial or full)
  if (this->gas_week_ && !std::isnan(this->gas_.start_week)) {
    if (this->gas_.full_week_started) {
      // Publish full calendar week value
      this->gas_week_->publish_state(total - this->gas_.start_week);
    } else {
      // Publish partial week value
      this->gas_week_->publish_state(total - this->gas_.start_week);
    }
  } else if (std::isnan(this->gas_.start_week)) {
    // Initialize start_week for the first time
    this->gas_.start_week = total;
    if (this->gas_week_) {
      this->gas_week_->publish_state(0);  // Publish initial value as 0
    }
  }

  // Publish monthly gas (partial or full)
  if (this->gas_month_ && !std::isnan(this->gas_.start_month)) {
    if (this->gas_.full_month_started) {
      // Publish full calendar month value
      this->gas_month_->publish_state(total - this->gas_.start_month);
    } else {
      // Publish partial month value
      this->gas_month_->publish_state(total - this->gas_.start_month);
    }
  } else if (std::isnan(this->gas_.start_month)) {
    // Initialize start_month for the first time
    this->gas_.start_month = total;
    if (this->gas_month_) {
      this->gas_month_->publish_state(0);  // Publish initial value as 0
    }
  }

  // Publish yearly gas (partial or full)
  if (this->gas_year_ && !std::isnan(this->gas_.start_year)) {
    if (this->gas_.full_year_started) {
      // Publish full calendar year value
      this->gas_year_->publish_state(total - this->gas_.start_year);
    } else {
      // Publish partial year value
      this->gas_year_->publish_state(total - this->gas_.start_year);
    }
  } else if (std::isnan(this->gas_.start_year)) {
    // Initialize start_year for the first time
    this->gas_.start_year = total;
    if (this->gas_year_) {
      this->gas_year_->publish_state(0);  // Publish initial value as 0
    }
  }

  // Save the current state
  this->save_();
}

void GasStatisticsMJ::save_() { this->pref_.save(&this->gas_); } // Save to flash memory

}  // namespace gas_statistics
}  // namespace esphome
