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
  }

  auto total = this->total_->get_state();
  if (!std::isnan(total)) {
    // Initialize start points for partial periods
    if (std::isnan(this->energy_.start_week)) {
      this->energy_.start_week = total;
      ESP_LOGD(TAG, "Initialized start_week for partial week: %.5f", total);
    }
    if (std::isnan(this->energy_.start_month)) {
      this->energy_.start_month = total;
      ESP_LOGD(TAG, "Initialized start_month for partial month: %.5f", total);
    }
    if (std::isnan(this->energy_.start_year)) {
      this->energy_.start_year = total;
      ESP_LOGD(TAG, "Initialized start_year for partial year: %.5f", total);
    }
  } else {
    ESP_LOGW(TAG, "Total energy sensor state is NaN during setup.");
  }
}


void EnergyStatistics::loop() {
  const auto t = this->time_->now();
  if (!t.is_valid()) {
    ESP_LOGW(TAG, "Time is not valid yet.");
    return;
  }

  ESP_LOGD(TAG, "Current time: %02d:%02d:%02d", t.hour, t.minute, t.second);
  ESP_LOGD(TAG, "Day of week: %d, Day of month: %d, Day of year: %d",
           t.day_of_week, t.day_of_month, t.day_of_year);

  const auto total = this->total_->get_state();
  if (std::isnan(total)) {
    ESP_LOGW(TAG, "Total energy sensor state is NaN.");
    return;
  }

  if (t.day_of_year != this->energy_.current_day_of_year) {
    ESP_LOGD(TAG, "New day detected: %d", t.day_of_year);

    this->energy_.start_yesterday = this->energy_.start_today;
    this->energy_.start_today = total;
    this->energy_.current_day_of_year = t.day_of_year;

    // Reset for new week
    if (t.day_of_week == this->energy_week_start_day_) {
      this->energy_.start_week = total;
      ESP_LOGD(TAG, "Weekly reset: start_week set to %.5f", total);
    }

    // Reset for new month
    if (t.day_of_month == this->energy_month_start_day_) {
      this->energy_.start_month = total;
      ESP_LOGD(TAG, "Monthly reset: start_month set to %.5f", total);
    }

    // Reset for new year
    if (t.day_of_year == this->energy_year_start_day_) {
      this->energy_.start_year = total;
      ESP_LOGD(TAG, "Yearly reset: start_year set to %.5f", total);
    }
  }

  this->process_(total);
}


void EnergyStatistics::process_(float total) {
  if (this->energy_today_ && !std::isnan(this->energy_.start_today)) {
    this->energy_today_->publish_state(total - this->energy_.start_today);
  }

  if (this->energy_week_ && !std::isnan(this->energy_.start_week)) {
    this->energy_week_->publish_state(total - this->energy_.start_week);
    ESP_LOGD(TAG, "Updated energy_week: %.5f", total - this->energy_.start_week);
  }

  if (this->energy_month_ && !std::isnan(this->energy_.start_month)) {
    this->energy_month_->publish_state(total - this->energy_.start_month);
    ESP_LOGD(TAG, "Updated energy_month: %.5f", total - this->energy_.start_month);
  }

  if (this->energy_year_ && !std::isnan(this->energy_.start_year)) {
    this->energy_year_->publish_state(total - this->energy_.start_year);
    ESP_LOGD(TAG, "Updated energy_year: %.5f", total - this->energy_.start_year);
  }

  this->save_();
}

void EnergyStatistics::save_() {
  this->pref_.save(&(this->energy_));
  ESP_LOGD(TAG, "Energy data saved.");
}

}  // namespace energy_statistics
}  // namespace esphome
