#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "energy_statistics.h"

namespace esphome {
namespace energy_statistics {

static const char *const TAG = "energy_statistics";

static const char *const PREF_V2 = "energy_statistics_v2";

void EnergyStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Energy Statistics - Sensors");
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
  if (loaded) {
    ESP_LOGI(TAG, "Energy successfully loaded NVS data: start_today=%f, start_yesterday=%f",
             this->energy_.start_today, this->energy_.start_yesterday);
    this->initial_total_retries_ = 40; // Try for 5 seconds to get valid total
    this->has_loaded_nvs_ = true;
    // Process stored values immediately for initial restoration
    float total = this->total_->state;
    if (std::isnan(total)) {
      total = this->energy_.start_today; // Fallback to stored start_today
    }
    this->process_(total, true); // Initial restore
  } else {
    ESP_LOGW(TAG, "No previous data loaded from NVS, starting fresh");
    // Initialize defaults to avoid NaN
    this->energy_.start_today = 0.0f;
    this->energy_.start_yesterday = 0.0f;
    this->energy_.start_week = 0.0f;
    this->energy_.start_month = 0.0f;
    this->energy_.start_year = 0.0f;
    this->pref_.save(&this->energy_);
    this->process_(0.0f, true); // Initial restore with zero
  }
}

void EnergyStatistics::loop() {
  // Handle initial total check non-blocking
  if (this->has_loaded_nvs_ && this->initial_total_retries_ > 0) {
    float total = this->total_->state;
    if (!std::isnan(total) && total >= 0.0f) {
      ESP_LOGD(TAG, "Energy processing restored total: %f", total);
      this->process_(total);
      this->initial_total_retries_ = 0; // Done
      this->has_loaded_nvs_ = false;
    } else {
      ESP_LOGD(TAG, "Energy waiting for valid total: %f, retries: %d", total, this->initial_total_retries_);
      this->initial_total_retries_--;
      if (this->initial_total_retries_ == 0) {
        ESP_LOGW(TAG, "Energy total invalid after 5s: %f, retaining prior stats", total);
        this->has_loaded_nvs_ = false;
      }
      return; // Yield to avoid blocking
    }
  }

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

  // Update stats on first run or when day changes
  if (t.day_of_year == this->energy_.current_day_of_year) {
    // Nothing to do
    return;
  }

  // Save the current day's data
  this->energy_.start_yesterday = this->energy_.start_today;
  this->energy_.start_today = total;

  if (this->energy_.current_day_of_year != 0) {
    // At specified day of week, start a new week calculation
    if (t.day_of_week == this->energy_week_start_day_) {
      this->energy_.start_week = total;
    }
    // At first day of month, start a new month calculation
    if (t.day_of_month == 1) {
      this->energy_.start_month = total;
    }
    // At first day of year, start a new year calculation
    if (t.day_of_year == 1) {
      this->energy_.start_year = total;
    }
  }

  // Initialize all sensors (fix for issue #65)
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

  this->process_(total);
}

void EnergyStatistics::process_(float total, bool is_initial_restore) {
  bool data_changed = false;

  // Use stored start_today as fallback for initial restore if total is invalid
  if (is_initial_restore && std::isnan(total)) {
    total = this->energy_.start_today;
    if (std::isnan(total)) {
      total = 0.0f; // Ultimate fallback
    }
  }

  // Calculate and publish today's energy
  if (this->energy_today_ && !std::isnan(this->energy_.start_today)) {
    float value = total - this->energy_.start_today;
    if (std::isnan(this->last_today_) || fabs(value - this->last_today_) > 0.001f) {
      this->energy_today_->publish_state(value);
      this->last_today_ = value;
    }
  } else if (this->energy_today_) {
    if (std::isnan(this->last_today_) || fabs(0.0f - this->last_today_) > 0.001f) {
      this->energy_today_->publish_state(0);
      this->last_today_ = 0.0f;
    }
  }

  // Calculate and publish yesterday's energy
  if (this->energy_yesterday_ && !std::isnan(this->energy_.start_yesterday)) {
    float value = this->energy_.start_today - this->energy_.start_yesterday;
    if (std::isnan(this->last_yesterday_) || fabs(value - this->last_yesterday_) > 0.001f) {
      this->energy_yesterday_->publish_state(value);
      this->last_yesterday_ = value;
    }
  } else if (this->energy_yesterday_) {
    if (std::isnan(this->last_yesterday_) || fabs(0.0f - this->last_yesterday_) > 0.001f) {
      this->energy_yesterday_->publish_state(0);
      this->last_yesterday_ = 0.0f;
    }
  }

  // Calculate and publish weekly energy
  if (this->energy_week_ && !std::isnan(this->energy_.start_week)) {
    float value = total - this->energy_.start_week;
    if (std::isnan(this->last_week_) || fabs(value - this->last_week_) > 0.001f) {
      this->energy_week_->publish_state(value);
      this->last_week_ = value;
    }
  } else if (this->energy_week_) {
    if (std::isnan(this->last_week_) || fabs(0.0f - this->last_week_) > 0.001f) {
      this->energy_week_->publish_state(0);
      this->last_week_ = 0.0f;
    }
  }

  // Calculate and publish monthly energy
  if (this->energy_month_ && !std::isnan(this->energy_.start_month)) {
    float value = total - this->energy_.start_month;
    if (std::isnan(this->last_month_) || fabs(value - this->last_month_) > 0.001f) {
      this->energy_month_->publish_state(value);
      this->last_month_ = value;
    }
  } else if (this->energy_month_) {
    if (std::isnan(this->last_month_) || fabs(0.0f - this->last_month_) > 0.001f) {
      this->energy_month_->publish_state(0);
      this->last_month_ = 0.0f;
    }
  }

  // Calculate and publish yearly energy
  if (this->energy_year_ && !std::isnan(this->energy_.start_year)) {
    float value = total - this->energy_.start_year;
    if (std::isnan(this->last_year_) || fabs(value - this->last_year_) > 0.001f) {
      this->energy_year_->publish_state(value);
      this->last_year_ = value;
    }
  } else if (this->energy_year_) {
    if (std::isnan(this->last_year_) || fabs(0.0f - this->last_year_) > 0.001f) {
      this->energy_year_->publish_state(0);
      this->last_year_ = 0.0f;
    }
  }

  // Save to NVS only if data has changed (e.g., new day or initial restore)
  if (this->energy_.current_day_of_year != this->time_->now().day_of_year ||
      is_initial_restore || this->has_loaded_nvs_) {
    this->pref_.save(&this->energy_);
    ESP_LOGD(TAG, "Energy saved NVS data: start_today=%f, start_yesterday=%f",
             this->energy_.start_today, this->energy_.start_yesterday);
  }
}

}  // namespace energy_statistics
}  // namespace esphome
