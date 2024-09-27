#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/api/api.h"
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

  // Add logs for the loaded values
  ESP_LOGCONFIG(TAG, "Restored Energy Today: %.3f", this->energy_.energy_today);
  ESP_LOGCONFIG(TAG, "Restored Energy Yesterday: %.3f", this->energy_.energy_yesterday);
  ESP_LOGCONFIG(TAG, "Restored Energy Week: %.3f", this->energy_.energy_week);
  ESP_LOGCONFIG(TAG, "Restored Energy Month: %.3f", this->energy_.energy_month);
  ESP_LOGCONFIG(TAG, "Restored Energy Year: %.3f", this->energy_.energy_year);

  // Register the reset service
  this->add_custom_service("reset_energy_statistics", [this]() { this->on_reset_called(); });
}


void EnergyStatistics::on_reset_called() {
  this->reset();
}


void EnergyStatistics::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });

  this->pref_ = global_preferences->make_preference<energy_data_t>(fnv1_hash(TAG));

  energy_data_t loaded{};
  if (this->pref_.load(&loaded)) {
    this->energy_ = loaded;

    // Load the sensor values from preferences if available
    if (this->energy_today_ && !std::isnan(this->energy_.energy_today)) {
      this->energy_today_->publish_state(this->energy_.energy_today);
    }
    if (this->energy_yesterday_ && !std::isnan(this->energy_.energy_yesterday)) {
      this->energy_yesterday_->publish_state(this->energy_.energy_yesterday);
    }
    if (this->energy_week_ && !std::isnan(this->energy_.energy_week)) {
      this->energy_week_->publish_state(this->energy_.energy_week);
    }
    if (this->energy_month_ && !std::isnan(this->energy_.energy_month)) {
      this->energy_month_->publish_state(this->energy_.energy_month);
    }
    if (this->energy_year_ && !std::isnan(this->energy_.energy_year)) {
      this->energy_year_->publish_state(this->energy_.energy_year);
    }

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
  // Energy Today
  if (this->energy_today_ && !std::isnan(this->energy_.start_today)) {
    this->energy_.energy_today = total - this->energy_.start_today;
    this->energy_today_->publish_state(this->energy_.energy_today);
  } else if (this->energy_today_) {
    // Show 0 instead of NA if no valid data
    this->energy_today_->publish_state(0.0);
  }

  // Energy Yesterday
  if (this->energy_yesterday_ && !std::isnan(this->energy_.start_yesterday)) {
    this->energy_.energy_yesterday = this->energy_.start_today - this->energy_.start_yesterday;
    this->energy_yesterday_->publish_state(this->energy_.energy_yesterday);
  } else if (this->energy_yesterday_) {
    this->energy_yesterday_->publish_state(0.0);
  }

  // Energy Week
  if (this->energy_week_ && !std::isnan(this->energy_.start_week)) {
    this->energy_.energy_week = total - this->energy_.start_week;
    this->energy_week_->publish_state(this->energy_.energy_week);
  } else if (this->energy_week_) {
    this->energy_week_->publish_state(0.0);
  }

  // Energy Month
  if (this->energy_month_ && !std::isnan(this->energy_.start_month)) {
    this->energy_.energy_month = total - this->energy_.start_month;
    this->energy_month_->publish_state(this->energy_.energy_month);
  } else if (this->energy_month_) {
    this->energy_month_->publish_state(0.0);
  }

  // Energy Year
  if (this->energy_year_ && !std::isnan(this->energy_.start_year)) {
    this->energy_.energy_year = total - this->energy_.start_year;
    this->energy_year_->publish_state(this->energy_.energy_year);
  } else if (this->energy_year_) {
    this->energy_year_->publish_state(0.0);
  }

  // Save the updated values to preferences
  this->save_();
}


void EnergyStatistics::reset() {
  ESP_LOGI(TAG, "Resetting all energy statistics to zero.");

  // Reset all start points to zero
  this->energy_.start_today = 0.0f;
  this->energy_.start_yesterday = 0.0f;
  this->energy_.start_week = 0.0f;
  this->energy_.start_month = 0.0f;
  this->energy_.start_year = 0.0f;

  // Reset all sensor values to zero
  if (this->energy_today_) {
    this->energy_.energy_today = 0.0f;
    this->energy_today_->publish_state(0.0f);
  }
  if (this->energy_yesterday_) {
    this->energy_.energy_yesterday = 0.0f;
    this->energy_yesterday_->publish_state(0.0f);
  }
  if (this->energy_week_) {
    this->energy_.energy_week = 0.0f;
    this->energy_week_->publish_state(0.0f);
  }
  if (this->energy_month_) {
    this->energy_.energy_month = 0.0f;
    this->energy_month_->publish_state(0.0f);
  }
  if (this->energy_year_) {
    this->energy_.energy_year = 0.0f;
    this->energy_year_->publish_state(0.0f);
  }

  // Save the reset values to preferences
  this->save_();
}


void EnergyStatistics::save_() { 
  // Save the current energy statistics to preferences
  this->pref_.save(&(this->energy_));
}

}  // namespace energy_statistics
}  // namespace esphome
