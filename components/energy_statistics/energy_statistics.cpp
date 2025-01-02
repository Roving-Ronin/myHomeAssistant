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
    if (std::isnan(this->energy_.start_week)) {
      this->energy_.start_week = total;
      ESP_LOGD(TAG, "Initialized start_week: %.5f", total);
    }
    if (std::isnan(this->energy_.start_month)) {
      this->energy_.start_month = total;
      ESP_LOGD(TAG, "Initialized start_month: %.5f", total);
    }
    if (std::isnan(this->energy_.start_year)) {
      this->energy_.start_year = total;
      ESP_LOGD(TAG, "Initialized start_year: %.5f", total);
    }
  }
}

void EnergyStatistics::loop() {
  const auto t = this->time_->now();
  if (!t.is_valid()) {
    ESP_LOGW(TAG, "Time is not valid yet.");
    return;
  }

  ESP_LOGD(TAG, "Current day of year: %d", t.day_of_year);
  ESP_LOGD(TAG, "Current day of week: %d", t.day_of_week);
  ESP_LOGD(TAG, "Current day of month: %d", t.day_of_month);

  const auto total = this->total_->get_state();
  if (std::isnan(total)) {
    ESP_LOGW(TAG, "Total energy state is NaN.");
    return;
  }

  if (t.day_of_year != this->energy_.current_day_of_year) {
    ESP_LOGD(TAG, "New day detected: %d", t.day_of_year);

    this->energy_.start_yesterday = this->energy_.start_today;
    this->energy_.start_today = total;
    this->energy_.current_day_of_year = t.day_of_year;

    if (t.day_of_week == this->energy_week_start_day_) {
      this->energy_.start_week = total;
      ESP_LOGD(TAG, "Weekly reset: start_week set to %.5f", total);
    }

    if (t.day_of_month == this->energy_month_start_day_) {
      this->energy_.start_month = total;
      ESP_LOGD(TAG, "Monthly reset: start_month set to %.5f", total);
    }

    if (t.day_of_year == this->energy_year_start_day_) {
      this->energy_.start_year = total;
      ESP_LOGD(TAG, "Yearly reset: start_year set to %.5f", total);
    }
  }

  this->process_(total);
}


void EnergyStatistics::process_(float total) {
  if (this->energy_today_ && !std::isnan(this->energy_.start_today)) {
    float value = total - this->energy_.start_today;
    this->energy_today_->publish_state(value);
    ESP_LOGD(TAG, "Updated energy_today: %.5f", value);
  }

  if (this->energy_yesterday_ && !std::isnan(this->energy_.start_yesterday)) {
    float value = this->energy_.start_today - this->energy_.start_yesterday;
    this->energy_yesterday_->publish_state(value);
    ESP_LOGD(TAG, "Updated energy_yesterday: %.5f", value);
  }

  if (this->energy_week_ && !std::isnan(this->energy_.start_week)) {
    float value = total - this->energy_.start_week;
    this->energy_week_->publish_state(value);
    ESP_LOGD(TAG, "Updated energy_week: %.5f", value);
  }

  if (this->energy_month_ && !std::isnan(this->energy_.start_month)) {
    float value = total - this->energy_.start_month;
    this->energy_month_->publish_state(value);
    ESP_LOGD(TAG, "Updated energy_month: %.5f", value);
  }

  if (this->energy_year_ && !std::isnan(this->energy_.start_year)) {
    float value = total - this->energy_.start_year;
    this->energy_year_->publish_state(value);
    ESP_LOGD(TAG, "Updated energy_year: %.5f", value);
  }

  this->save_();
}

void EnergyStatistics::save_() {
  this->pref_.save(&(this->energy_));
  ESP_LOGD(TAG, "Energy data saved.");
}

}  // namespace energy_statistics
}  // namespace esphome
