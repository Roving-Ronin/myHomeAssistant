#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "energy_statistics.h"

namespace esphome {
namespace energy_statistics {

static const char *const TAG = "energy_statistics";
static const char *const GAP = "  ";

void EnergyStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Energy Statistics - Sensors");
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
    // time is not sync yet
    return;
  }

  const auto total = this->total_->get_state();
  if (std::isnan(total)) {
    // total is not published yet
    return;
  }

  if (t.day_of_year == this->energy_.current_day_of_year) {
    // nothing to do
    return;
  }

    // Save the current day's data
  this->energy_.start_yesterday = this->energy_.start_today;
  this->energy_.start_today = total;

  if (this->energy_.current_day_of_year != 0) {
    // at specified day of week we start a new week calculation
    if (t.day_of_week == this->energy_week_start_day_) {
      this->energy_.start_week = total;
    }
    // at first day of month we start a new month calculation
    if (t.day_of_month == 1) {
      this->energy_.start_month = total;
    }
    // at first day of year we start a new year calculation
    if (t.day_of_year == 1) {
      this->energy_.start_year = total;
    }
  }

  this->energy_.current_day_of_year = t.day_of_year;

  this->process_(total);
}



void EnergyStatistics::process_(float total) {
  // Publish today's energy
  if (this->energy_today_ && !std::isnan(this->energy_.start_today)) {
    this->energy_today_->publish_state(total - this->energy_.start_today);
  } else if (std::isnan(this->energy_.start_today)) {
    // Initialize today's start value
    this->energy_.start_today = total;
    if (this->energy_today_) {
      this->energy_today_->publish_state(0);  // Publish initial value as 0
    }
  }

  // Publish yesterday's energy
  if (this->energy_yesterday_ && !std::isnan(this->energy_.start_yesterday)) {
    this->energy_yesterday_->publish_state(this->energy_.start_today - this->energy_.start_yesterday);
  }

  // Publish weekly energy (partial or full)
  if (this->energy_week_ && !std::isnan(this->energy_.start_week)) {
    if (this->energy_.full_week_started) {
      // Publish full calendar week value
      this->energy_week_->publish_state(total - this->energy_.start_week);
    } else {
      // Publish partial week value
      this->energy_week_->publish_state(total - this->energy_.start_week);
    }
  } else if (std::isnan(this->energy_.start_week)) {
    // Initialize start_week for the first time
    this->energy_.start_week = total;
    if (this->energy_week_) {
      this->energy_week_->publish_state(0);  // Publish initial value as 0
    }
  }

  // Publish monthly energy (partial or full)
  if (this->energy_month_ && !std::isnan(this->energy_.start_month)) {
    if (this->energy_.full_month_started) {
      // Publish full calendar month value
      this->energy_month_->publish_state(total - this->energy_.start_month);
    } else {
      // Publish partial month value
      this->energy_month_->publish_state(total - this->energy_.start_month);
    }
  } else if (std::isnan(this->energy_.start_month)) {
    // Initialize start_month for the first time
    this->energy_.start_month = total;
    if (this->energy_month_) {
      this->energy_month_->publish_state(0);  // Publish initial value as 0
    }
  }

  // Publish yearly energy (partial or full)
  if (this->energy_year_ && !std::isnan(this->energy_.start_year)) {
    if (this->energy_.full_year_started) {
      // Publish full calendar year value
      this->energy_year_->publish_state(total - this->energy_.start_year);
    } else {
      // Publish partial year value
      this->energy_year_->publish_state(total - this->energy_.start_year);
    }
  } else if (std::isnan(this->energy_.start_year)) {
    // Initialize start_year for the first time
    this->energy_.start_year = total;
    if (this->energy_year_) {
      this->energy_year_->publish_state(0);  // Publish initial value as 0
    }
  }

  // Save the current state
  this->save_();
}


void EnergyStatistics::save_() { this->pref_.save(&(this->energy_)); }  // Save to flash memory

}  // namespace energy_statistics
}  // namespace esphome
