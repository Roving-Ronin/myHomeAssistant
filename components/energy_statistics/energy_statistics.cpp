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

  ESP_LOGCONFIG(TAG, "Restored Energy Today: %.3f", this->energy_.energy_today);
  ESP_LOGCONFIG(TAG, "Restored Energy Yesterday: %.3f", this->energy_.energy_yesterday);
  ESP_LOGCONFIG(TAG, "Restored Energy Week: %.3f", this->energy_.energy_week);
  ESP_LOGCONFIG(TAG, "Restored Energy Month: %.3f", this->energy_.energy_month);
  ESP_LOGCONFIG(TAG, "Restored Energy Year: %.3f", this->energy_.energy_year);
}

void EnergyStatistics::setup() {
  this->pref_ = global_preferences->make_preference<energy_data_t>(fnv1_hash(TAG));

  energy_data_t loaded{};
  if (this->pref_.load(&loaded) && !this->is_resetting_) {
    this->energy_ = loaded;

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
  } else {
    reset_statistics(); // Ensure reset if resetting
  }
}

void EnergyStatistics::loop() {
  const auto t = this->time_->now();
  if (!t.is_valid()) {
    return;
  }

  const auto total = this->total_->get_state();
  if (std::isnan(total)) {
    return;
  }

  if (t.day_of_year == this->energy_.current_day_of_year) {
    return;
  }

  this->energy_.start_yesterday = this->energy_.start_today;
  this->energy_.start_today = total;

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

  this->energy_.current_day_of_year = t.day_of_year;

  this->process_(total);
}

void EnergyStatistics::process_(float total) {
  ESP_LOGD(TAG, "Entered process_() function.");

  if (this->energy_today_ && !std::isnan(this->energy_.start_today)) {
    this->energy_.energy_today = total - this->energy_.start_today;
    this->energy_today_->publish_state(this->energy_.energy_today);
  } else if (this->energy_today_) {
    this->energy_today_->publish_state(0.0);
  }

  if (this->energy_yesterday_ && !std::isnan(this->energy_.start_yesterday)) {
    this->energy_.energy_yesterday = this->energy_.start_today - this->energy_.start_yesterday;
    this->energy_yesterday_->publish_state(this->energy_.energy_yesterday);
  } else if (this->energy_yesterday_) {
    this->energy_yesterday_->publish_state(0.0);
  }

  if (this->energy_week_ && !std::isnan(this->energy_.start_week)) {
    this->energy_.energy_week = total - this->energy_.start_week;
    this->energy_week_->publish_state(this->energy_.energy_week);
  } else if (this->energy_week_) {
    this->energy_week_->publish_state(0.0);
  }

  if (this->energy_month_ && !std::isnan(this->energy_.start_month)) {
    this->energy_.energy_month = total - this->energy_.start_month;
    this->energy_month_->publish_state(this->energy_.energy_month);
  } else if (this->energy_month_) {
    this->energy_month_->publish_state(0.0);
  }

  if (this->energy_year_ && !std::isnan(this->energy_.start_year)) {
    this->energy_.energy_year = total - this->energy_.start_year;
    this->energy_year_->publish_state(this->energy_.energy_year);
  } else if (this->energy_year_) {
    this->energy_year_->publish_state(0.0);
  }

  ESP_LOGD(TAG, "Calling save_() from process_().");
  this->save_();
}

void EnergyStatistics::save_() {
  static uint32_t last_save_time_ = 0;
  const uint32_t save_interval_ = 60 * 1000; // Save every 60 seconds (1 minute)

  uint32_t current_time = millis();

  // Add log at the start of the function to see if it's entered
  ESP_LOGD(TAG, "Entered save_() method to check if save is needed.");

  // Ensure proper time comparison, including millis() wrap-around handling
  if ((current_time - last_save_time_) >= save_interval_ || last_save_time_ == 0) {
    ESP_LOGI(TAG, "Attempting to save energy statistics to flash memory.");

    // Save energy statistics to flash
    if (this->pref_.save(&(this->energy_))) {
      ESP_LOGI(TAG, "Energy Statistics - Successfully saved to flash memory.");
    } else {
      ESP_LOGW(TAG, "Energy Statistics - Failed to save to flash memory.");
    }

    // Update the last save time
    last_save_time_ = current_time;
  }
}


void EnergyStatistics::reset_statistics() {
  ESP_LOGI(TAG, "Resetting Energy Statistics to 0.0");

  this->energy_.energy_today = 0.0;
  this->energy_.energy_yesterday = 0.0;
  this->energy_.energy_week = 0.0;
  this->energy_.energy_month = 0.0;
  this->energy_.energy_year = 0.0;

  if (this->energy_today_) this->energy_today_->publish_state(0.0);
  if (this->energy_yesterday_) this->energy_yesterday_->publish_state(0.0);
  if (this->energy_week_) this->energy_week_->publish_state(0.0);
  if (this->energy_month_) this->energy_month_->publish_state(0.0);
  if (this->energy_year_) this->energy_year_->publish_state(0.0);
}

}  // namespace energy_statistics
}  // namespace esphome
