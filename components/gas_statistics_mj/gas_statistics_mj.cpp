#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "gas_statistics_mj.h"

namespace esphome {
namespace gas_statistics_mj {

static const char *const TAG = "gas_statistics_mj";

static const char *const PREF_V2 = "gas_statistics_mj_v2";

void GasStatisticsMJ::dump_config() {
  ESP_LOGCONFIG(TAG, "Gas Statistics (MJ) - Sensors");
  if (this->gas_today_) {
    LOG_SENSOR("  ", "Gas (MJ) Today", this->gas_today_);
  }
  if (this->gas_yesterday_) {
    LOG_SENSOR("  ", "Gas (MJ) Yesterday", this->gas_yesterday_);
  }
  if (this->gas_week_) {
    LOG_SENSOR("  ", "Gas (MJ) Week", this->gas_week_);
  }
  if (this->gas_month_) {
    LOG_SENSOR("  ", "Gas (MJ) Month", this->gas_month_);
  }
  if (this->gas_year_) {
    LOG_SENSOR("  ", "Gas (MJ) Year", this->gas_year_);
  }
}

void GasStatisticsMJ::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });

  this->pref_ = global_preferences->make_preference<gas_mj_data_t>(fnv1_hash(PREF_V2));
  bool loaded = this->pref_.load(&this->gas_);
  if (loaded) {
    ESP_LOGI(TAG, "Gas (MJ) successfully loaded NVS data: start_today=%f, start_yesterday=%f",
             this->gas_.start_today, this->gas_.start_yesterday);
    this->initial_total_retries_ = 40; // Try for 5 seconds to get valid total
    this->has_loaded_nvs_ = true;
    // Process stored values immediately for initial restoration
    float total = this->total_->state;
    if (std::isnan(total)) {
      total = this->gas_.start_today; // Fallback to stored start_today
    }
    this->process_(total, true); // Initial restore
  } else {
    ESP_LOGW(TAG, "Gas (MJ) no previous data loaded from NVS, starting fresh");
    // Initialize defaults to avoid NaN
    this->gas_.start_today = 0.0f;
    this->gas_.start_yesterday = 0.0f;
    this->gas_.start_week = 0.0f;
    this->gas_.start_month = 0.0f;
    this->gas_.start_year = 0.0f;
    this->pref_.save(&this->gas_);
    this->process_(0.0f, true); // Initial restore with zero
  }
}

void GasStatisticsMJ::loop() {
  // Handle initial total check non-blocking
  if (this->has_loaded_nvs_ && this->initial_total_retries_ > 0) {
    float total = this->total_->state;
    if (!std::isnan(total) && total >= 0.0f) {
      ESP_LOGD(TAG, "Gas (MJ) processing restored total: %f", total);
      this->process_(total);
      this->initial_total_retries_ = 0; // Done
      this->has_loaded_nvs_ = false;
    } else {
      ESP_LOGD(TAG, "Gas (MJ) waiting for valid total: %f, retries: %d", total, this->initial_total_retries_);
      this->initial_total_retries_--;
      if (this->initial_total_retries_ == 0) {
        ESP_LOGW(TAG, "Gas (MJ) total invalid after 5s: %f, retaining prior stats", total);
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
  if (t.day_of_year == this->gas_.current_day_of_year) {
    // Nothing to do
    return;
  }

  // Save the current day's data
  this->gas_.start_yesterday = this->gas_.start_today;
  this->gas_.start_today = total;

  if (this->gas_.current_day_of_year != 0) {
    // At specified day of week, start a new week calculation
    if (t.day_of_week == this->gas_week_start_day_) {
      this->gas_.start_week = total;
    }
    // At first day of month, start a new month calculation
    if (t.day_of_month == 1) {
      this->gas_.start_month = total;
    }
    // At first day of year, start a new year calculation
    if (t.day_of_year == 1) {
      this->gas_.start_year = total;
    }
  }

  // Initialize all sensors (fix for issue #65)
  if (this->gas_week_ && std::isnan(this->gas_.start_week)) {
    this->gas_.start_week = this->gas_.start_yesterday;
  }
  if (this->gas_month_ && std::isnan(this->gas_.start_month)) {
    this->gas_.start_month = this->gas_.start_yesterday;
  }
  if (this->gas_year_ && std::isnan(this->gas_.start_year)) {
    this->gas_.start_year = this->gas_.start_yesterday;
  }

  this->gas_.current_day_of_year = t.day_of_year;

  this->process_(total);
}

void GasStatisticsMJ::process_(float total, bool is_initial_restore) {
  bool data_changed = false;

  // Use stored start_today as fallback for initial restore if total is invalid
  if (is_initial_restore && std::isnan(total)) {
    total = this->gas_.start_today;
    if (std::isnan(total)) {
      total = 0.0f; // Ultimate fallback
    }
  }

  // Calculate and publish today's gas
  if (this->gas_today_ && !std::isnan(this->gas_.start_today)) {
    float value = total - this->gas_.start_today;
    if (std::isnan(this->last_today_) || fabs(value - this->last_today_) > 0.001f) {
      this->gas_today_->publish_state(value);
      this->last_today_ = value;
    }
  } else if (this->gas_today_) {
    if (std::isnan(this->last_today_) || fabs(0.0f - this->last_today_) > 0.001f) {
      this->gas_today_->publish_state(0);
      this->last_today_ = 0.0f;
    }
  }

  // Calculate and publish yesterday's gas
  if (this->gas_yesterday_ && !std::isnan(this->gas_.start_yesterday)) {
    float value = this->gas_.start_today - this->gas_.start_yesterday;
    if (std::isnan(this->last_yesterday_) || fabs(value - this->last_yesterday_) > 0.001f) {
      this->gas_yesterday_->publish_state(value);
      this->last_yesterday_ = value;
    }
  } else if (this->gas_yesterday_) {
    if (std::isnan(this->last_yesterday_) || fabs(0.0f - this->last_yesterday_) > 0.001f) {
      this->gas_yesterday_->publish_state(0);
      this->last_yesterday_ = 0.0f;
    }
  }

  // Calculate and publish weekly gas
  if (this->gas_week_ && !std::isnan(this->gas_.start_week)) {
    float value = total - this->gas_.start_week;
    if (std::isnan(this->last_week_) || fabs(value - this->last_week_) > 0.001f) {
      this->gas_week_->publish_state(value);
      this->last_week_ = value;
    }
  } else if (this->gas_week_) {
    if (std::isnan(this->last_week_) || fabs(0.0f - this->last_week_) > 0.001f) {
      this->gas_week_->publish_state(0);
      this->last_week_ = 0.0f;
    }
  }

  // Calculate and publish monthly gas
  if (this->gas_month_ && !std::isnan(this->gas_.start_month)) {
    float value = total - this->gas_.start_month;
    if (std::isnan(this->last_month_) || fabs(value - this->last_month_) > 0.001f) {
      this->gas_month_->publish_state(value);
      this->last_month_ = value;
    }
  } else if (this->gas_month_) {
    if (std::isnan(this->last_month_) || fabs(0.0f - this->last_month_) > 0.001f) {
      this->gas_month_->publish_state(0);
      this->last_month_ = 0.0f;
    }
  }

  // Calculate and publish yearly gas
  if (this->gas_year_ && !std::isnan(this->gas_.start_year)) {
    float value = total - this->gas_.start_year;
    if (std::isnan(this->last_year_) || fabs(value - this->last_year_) > 0.001f) {
      this->gas_year_->publish_state(value);
      this->last_year_ = value;
    }
  } else if (this->gas_year_) {
    if (std::isnan(this->last_year_) || fabs(0.0f - this->last_year_) > 0.001f) {
      this->gas_year_->publish_state(0);
      this->last_year_ = 0.0f;
    }
  }

  // Save to NVS only if data has changed (e.g., new day or initial restore)
  if (this->gas_.current_day_of_year != this->time_->now().day_of_year ||
      is_initial_restore || this->has_loaded_nvs_) {
    this->pref_.save(&this->gas_);
    ESP_LOGD(TAG, "Gas (MJ) saved NVS data: start_today=%f, start_yesterday=%f",
             this->gas_.start_today, this->gas_.start_yesterday);
  }
}

}  // namespace gas_statistics_mj
}  // namespace esphome
