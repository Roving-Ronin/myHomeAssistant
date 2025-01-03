#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "energy_statistics.h"

namespace esphome {
namespace energy_statistics {

static const char *const TAG = "energy_statistics";
static const char *const GAP = "  ";

void EnergyStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Energy statistics sensors");
  if (this->energy_today_) {
    LOG_SENSOR(GAP, "Energy Today", this->energy_today_);
  }
  if (this->energy_yesterday_) {
    LOG_SENSOR(GAP, "Energy Yesterday", this->energy_yesterday_);
  }
  if (this->energy_week_) {
    LOG_SENSOR(GAP, "Energy Week", this->energy_week_);
  }
  if (this->energy_month_) {
    LOG_SENSOR(GAP, "Energy Month", this->energy_month_);
  }
  if (this->energy_year_) {
    LOG_SENSOR(GAP, "Energy Year", this->energy_year_);
  }
}

void EnergyStatistics::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });

  this->pref_ = global_preferences->make_preference<energy_data_t>(fnv1_hash(TAG));

  energy_data_t loaded{};
  if (this->pref_.load(&loaded)) {
    this->energy_ = loaded;
    auto total = this->total_->get_state();
    if (!std::isnan(total)) {
      this->process_(total);
    }
  }
}

void EnergyStatistics::loop() {
  const auto t = this->time_->now();
  if (!t.is_valid()) {
    // Time is not synced yet
    return;
  }

  const auto total = this->total_->get_state();
  if (std::isnan(total)) {
    // Total is not published yet
    return;
  }

  // Check if the day has changed
  if (t.day_of_year != this->energy_.current_day_of_year) {
    // Save the current day's data
    this->energy_.start_yesterday = this->energy_.start_today;
    this->energy_.start_today = total;
    this->energy_.current_day_of_year = t.day_of_year;
  }

  // Detect start of a new calendar week
  if (t.day_of_week == this->energy_week_start_day_ && t.hour == 0 && t.minute == 0) {
    this->energy_.start_week = total;
    this->energy_.full_week_started = true;
  }

  // Detect start of a new calendar month
  if (t.day_of_month == this->energy_month_start_day_ && t.hour == 0 && t.minute == 0) {
    this->energy_.start_month = total;
    this->energy_.full_month_started = true;
  }

  // Detect start of a new calendar year
  if (t.day_of_year == 1 && t.hour == 0 && t.minute == 0) {
    this->energy_.start_year = total;
    this->energy_.full_year_started = true;
  }

  // Process current totals
  this->process_(total);
}



void EnergyStatistics::process_(float total) {
  // Publish daily energy
  if (this->energy_today_ && !std::isnan(this->energy_.start_today)) {
    this->energy_today_->publish_state(total - this->energy_.start_today);
  }

  // Publish yesterday energy
  if (this->energy_yesterday_ && !std::isnan(this->energy_.start_yesterday)) {
    this->energy_yesterday_->publish_state(this->energy_.start_today - this->energy_.start_yesterday);
  }

  // Publish week energy
  if (this->energy_week_ && !std::isnan(this->energy_.start_week)) {
    if (this->energy_.full_week_started) {
      this->energy_week_->publish_state(total - this->energy_.start_week);
    } else {
      this->energy_week_->publish_state(total - this->energy_.start_week);  // Partial week
    }
  }

  // Publish month energy
  if (this->energy_month_ && !std::isnan(this->energy_.start_month)) {
    if (this->energy_.full_month_started) {
      this->energy_month_->publish_state(total - this->energy_.start_month);
    } else {
      this->energy_month_->publish_state(total - this->energy_.start_month);  // Partial month
    }
  }

  // Publish year energy
  if (this->energy_year_ && !std::isnan(this->energy_.start_year)) {
    if (this->energy_.full_year_started) {
      this->energy_year_->publish_state(total - this->energy_.start_year);
    } else {
      this->energy_year_->publish_state(total - this->energy_.start_year);  // Partial year
    }
  }
  
  this->save_();
}

void EnergyStatistics::save_() { this->pref_.save(&(this->energy_)); }  // Save to flash memory

}  // namespace energy_statistics
}  // namespace esphome
