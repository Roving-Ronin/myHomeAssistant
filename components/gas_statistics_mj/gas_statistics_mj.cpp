#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "gas_statistics_mj.h"

namespace esphome {
namespace gas_statistics_mj {

static const char *const TAG = "gas_statistics_mj";
static const char *const GAP = "  ";

void GasStatisticsMJ::dump_config() {
  ESP_LOGCONFIG(TAG, "Gas statistics MJ sensors");
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
}

void GasStatisticsMJ::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });

  this->pref_ = global_preferences->make_preference<gas_data_t>(fnv1_hash(TAG));

  gas_data_t loaded{};
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
    return;
  }

  const auto total = this->total_->get_state();
  if (std::isnan(total)) {
    return;
  }

  if (t.day_of_year == this->gas_.current_day_of_year) {
    return;
  }

  this->gas_.start_yesterday = this->gas_.start_today;

  this->gas_.start_today = total;

  if (this->gas_.current_day_of_year != 0) {
    if (t.day_of_week == this->gas_week_start_day_) {
      this->gas_.start_week = total;
    }
    if (t.day_of_month == 1) {
      this->gas_.start_month = total;
    }
    if (t.day_of_year == 1) {
      this->gas_.start_year = total;
    }
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
  }

  if (this->gas_week_ && !std::isnan(this->gas_.start_week)) {
    this->gas_week_->publish_state(total - this->gas_.start_week);
  }

  if (this->gas_month_ && !std::isnan(this->gas_.start_month)) {
    this->gas_month_->publish_state(total - this->gas_.start_month);
  }

  if (this->gas_year_ && !std::isnan(this->gas_.start_year)) {
    this->gas_year_->publish_state(total - this->gas_.start_year);
  }
  this->save_();
}

void GasStatisticsMJ::save_() { this->pref_.save(&(this->gas_)); }

}  // namespace gas_statistics_mj
}  // namespace esphome