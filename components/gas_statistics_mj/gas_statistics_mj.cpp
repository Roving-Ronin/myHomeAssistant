#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "gas_statistics_mj.h"

namespace esphome {
namespace gas_statistics_mj {

static const char *const TAG = "gas_statistics_mj";
static const char *const GAP = "  ";

void GasStatisticsMJ::dump_config() {
  ESP_LOGCONFIG(TAG, "Gas statistics sensors (MJ)");
  if (this->gas_today_) {
    LOG_SENSOR(GAP, "Gas Today (MJ)", this->gas_today_);
  }
  if (this->gas_yesterday_) {
    LOG_SENSOR(GAP, "Gas Yesterday (MJ)", this->gas_yesterday_);
  }
  if (this->gas_week_) {
    LOG_SENSOR(GAP, "Gas Week (MJ)", this->gas_week_);
  }
  if (this->gas_month_) {
    LOG_SENSOR(GAP, "Gas Month (MJ)", this->gas_month_);
  }
  if (this->gas_year_) {
    LOG_SENSOR(GAP, "Gas Year (MJ)", this->gas_year_);
  }

  // Log restored values for debugging
  ESP_LOGCONFIG(TAG, "Restored Gas Today (MJ): %.3f", this->gas_.gas_today);
  ESP_LOGCONFIG(TAG, "Restored Gas Yesterday (MJ): %.3f", this->gas_.gas_yesterday);
  ESP_LOGCONFIG(TAG, "Restored Gas Week (MJ): %.3f", this->gas_.gas_week);
  ESP_LOGCONFIG(TAG, "Restored Gas Month (MJ): %.3f", this->gas_.gas_month);
  ESP_LOGCONFIG(TAG, "Restored Gas Year (MJ): %.3f", this->gas_.gas_year);
}

void GasStatisticsMJ::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });

  this->pref_ = global_preferences->make_preference<gas_data_mj_t>(fnv1_hash(TAG));

  gas_data_mj_t loaded{};
  if (this->pref_.load(&loaded)) {
    this->gas_ = loaded;

    // Restore sensor values from preferences
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
  }
}


void GasStatisticsMJ::loop() {
  const auto t = this->time_->now();
  if (!t.is_valid()) {
    // time is not synced yet
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
  // Gas Today
  if (this->gas_today_ && !std::isnan(this->gas_.start_today)) {
    this->gas_.gas_today = total - this->gas_.start_today;
    this->gas_today_->publish_state(this->gas_.gas_today);
  } else if (this->gas_today_) {
    // Show 0 instead of NA if no valid data
    this->gas_today_->publish_state(0.0);
  }

  // Gas Yesterday
  if (this->gas_yesterday_ && !std::isnan(this->gas_.start_yesterday)) {
    this->gas_.gas_yesterday = this->gas_.start_today - this->gas_.start_yesterday;
    this->gas_yesterday_->publish_state(this->gas_.gas_yesterday);
  } else if (this->gas_yesterday_) {
    this->gas_yesterday_->publish_state(0.0);
  }

  // Gas Week
  if (this->gas_week_ && !std::isnan(this->gas_.start_week)) {
    this->gas_.gas_week = total - this->gas_.start_week;
    this->gas_week_->publish_state(this->gas_.gas_week);
  } else if (this->gas_week_) {
    this->gas_week_->publish_state(0.0);
  }

  // Gas Month
  if (this->gas_month_ && !std::isnan(this->gas_.start_month)) {
    this->gas_.gas_month = total - this->gas_.start_month;
    this->gas_month_->publish_state(this->gas_.gas_month);
  } else if (this->gas_month_) {
    this->gas_month_->publish_state(0.0);
  }

  // Gas Year
  if (this->gas_year_ && !std::isnan(this->gas_.start_year)) {
    this->gas_.gas_year = total - this->gas_.start_year;
    this->gas_year_->publish_state(this->gas_.gas_year);
  } else if (this->gas_year_) {
    this->gas_year_->publish_state(0.0);
  }

  // Save the updated values to preferences
  this->save_();
}

void GasStatisticsMJ::save_() { this->pref_.save(&(this->gas_)); 
}

}  // namespace gas_statistics_mj
}  // namespace esphome
