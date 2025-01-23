#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "energy_statistics.h"

namespace esphome {
namespace energy_statistics {

static const char *const TAG = "energy_statistics";

static const char *const PREF_V1 = "energy_statistics";
static const char *const PREF_V2 = "energy_statistics_v2";

void EnergyStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Energy statistics sensors");
  if (this->energy_today_) {
    LOG_SENSOR("  ", "Energy Today", this->energy_today_);
  }
  if (this->energy_yesterday_) {
    LOG_SENSOR("  ", "Energy Yesterday", this->energy_yesterday_);
  }
  if (this->energy_week_) {
    LOG_SENSOR("  ", "Energy Week", this->energy_week_);
  }
  if (this->energy_month_) {
    LOG_SENSOR("  ", "Energy Month", this->energy_month_);
  }
  if (this->energy_year_) {
    LOG_SENSOR("  ", "Energy Year", this->energy_year_);
  }
}

void EnergyStatistics::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });

  this->pref_ = global_preferences->make_preference<energy_data_t>(fnv1_hash(PREF_V2));
  bool loaded = this->pref_.load(&this->energy_);
  // Check for prior data or perform migration from older formats
  if (!loaded) {
    // Attempt to load data from older version (v1)
    loaded = global_preferences->make_preference<energy_data_v1_t>(fnv1_hash(PREF_V1)).load(&this->energy_);
    if (loaded) {
      // Migrate v1 data to v2 format
      this->energy_.start_year = this->energy_.start_month;
      this->pref_.save(&this->energy_);
      global_preferences->sync();
    }
  }

  // Fallback initialization if no prior data exists
  if (!loaded) {
    auto total = this->total_->get_state();
    if (!std::isnan(total)) {
      this->energy_.start_today = total;
      this->energy_.start_week = total;
      this->energy_.start_month = total;
      this->energy_.start_year = total;
    } else {
      // Initialize values to NAN if the total sensor hasn't published yet
      this->energy_.start_today = NAN;
      this->energy_.start_week = NAN;
      this->energy_.start_month = NAN;
      this->energy_.start_year = NAN;
    }
  }  
  
  // Process the current total to initialize sensors if data is available
  auto total = this->total_->get_state();
  if (!std::isnan(total)) {
    this->process_(total);
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
    // Total energy value is not published yet
    return;
  }

  // Check if a new day has started
  if (t.day_of_year != this->energy_.current_day_of_year) {
    // Shift yesterday's start value and update today
    this->energy_.start_yesterday = this->energy_.start_today;
    this->energy_.start_today = total;

    // Handle weekly, monthly, and yearly resets
    if (this->energy_.current_day_of_year != 0) {
      if (t.day_of_week == this->energy_week_start_day_) {
        this->energy_.start_week = total;
      }
      if (t.day_of_month == 1) {
        this->energy_.start_month = total;
      }
      if (t.day_of_year == 1) {
        this->energy_.start_year = total;
      }
    }

    // Ensure initialization of all sensors
    if (this->energy_week_ && std::isnan(this->energy_.start_week)) {
      this->energy_.start_week = this->energy_.start_yesterday;
    }
    if (this->energy_month_ && std::isnan(this->energy_.start_month)) {
      this->energy_.start_month = this->energy_.start_yesterday;
    }
    if (this->energy_year_ && std::isnan(this->energy_.start_year)) {
      this->energy_.start_year = this->energy_.start_yesterday;
    }

    this->energy_.current_day_of_year = t.day_of_year;
  }

  // Process the latest total energy value
  this->process_(total);
}


void EnergyStatistics::process_(float total) {
  // Calculate and publish daily, weekly, monthly, and yearly energy values
  if (this->energy_today_ && !std::isnan(this->energy_.start_today)) {
    this->energy_today_->publish_state(total - this->energy_.start_today);
  }

  if (this->energy_yesterday_ && !std::isnan(this->energy_.start_yesterday)) {
    this->energy_yesterday_->publish_state(this->energy_.start_today - this->energy_.start_yesterday);
  }

  if (this->energy_week_ && !std::isnan(this->energy_.start_week)) {
    this->energy_week_->publish_state(total - this->energy_.start_week);
  }

  if (this->energy_month_ && !std::isnan(this->energy_.start_month)) {
    this->energy_month_->publish_state(total - this->energy_.start_month);
  }
  
  if (this->energy_year_ && !std::isnan(this->energy_.start_year)) {
    this->energy_year_->publish_state(total - this->energy_.start_year);
  }
  
  this->pref_.save(&this->energy_);
}

}  // namespace energy_statistics
}  // namespace esphome
