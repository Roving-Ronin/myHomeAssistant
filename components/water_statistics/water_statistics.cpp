#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "water_statistics.h"

namespace esphome {
namespace water_statistics {

static const char *const TAG = "water_statistics";

static const char *const PREF_V2 = "water_statistics_v2";

void WaterStatistics::dump_config() {
  ESP_LOGCONFIG(TAG, "Water Statistics (L) - Sensors");
  if (this->water_today_) {
    LOG_SENSOR("  ", "Water (L) Today", this->water_today_);
  }
  if (this->water_yesterday_) {
    LOG_SENSOR("  ", "Water (L) Yesterday", this->water_yesterday_);
  }
  if (this->water_week_) {
    LOG_SENSOR("  ", "Water (L) Week", this->water_week_);
  }
  if (this->water_month_) {
    LOG_SENSOR("  ", "Water (L) Month", this->water_month_);
  }
  if (this->water_year_) {
    LOG_SENSOR("  ", "Water (L) Year", this->water_year_);
  }
}

void WaterStatistics::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });

  this->pref_ = global_preferences->make_preference<water_data_t>(fnv1_hash(PREF_V2));
  bool loaded = this->pref_.load(&this->water_);
  if (loaded) {
    ESP_LOGI(TAG, "Loaded Water NVS: today=%f, yesterday=%f, week=%f, month=%f, year=%f",
             this->water_.start_today, this->water_.start_yesterday, this->water_.start_week,
             this->water_.start_month, this->water_.start_year);
    this->initial_total_retries_ = 40; // Try for 5 seconds to get valid total
    this->has_loaded_nvs_ = true;
    // Process stored values for initial restoration
    float total = this->total_->state;
    if (std::isnan(total)) {
      total = this->water_.start_today; // Fallback to stored start_today
    }
    this->process_(total, true); // Initial restore
  } else {
    ESP_LOGW(TAG, "No Water NVS data loaded, starting fresh");
    // Initialize defaults
    this->water_.start_today = 0.0f;
    this->water_.start_yesterday = 0.0f;
    this->water_.start_week = 0.0f;
    this->water_.start_month = 0.0f;
    this->water_.start_year = 0.0f;
    this->pref_.save(&this->water_);
    ESP_LOGD(TAG, "Saved initial Water NVS: today=%f, yesterday=%f, week=%f, month=%f, year=%f",
             this->water_.start_today, this->water_.start_yesterday, this->water_.start_week,
             this->water_.start_month, this->water_.start_year);
    this->process_(0.0f, true); // Initial restore with zero
  }

  // Delay processing until SNTP sync
  this->set_timeout(15000, [this]() {
    this->initial_processing_started_ = true;
    if (!this->time_->now().is_valid()) {
      ESP_LOGW(TAG, "SNTP not synced after 15s, scheduling retry");
      this->set_timeout(5000, [this]() { this->retry_sntp_sync_(); });
    }
  });
}

void WaterStatistics::retry_sntp_sync_() {
  if (this->time_->now().is_valid()) {
    ESP_LOGD(TAG, "SNTP synced on retry");
    this->initial_processing_started_ = true;
  } else if (this->sntp_retries_ < 3) {
    ESP_LOGW(TAG, "SNTP retry %d/3 failed, scheduling next retry", this->sntp_retries_ + 1);
    this->sntp_retries_++;
    this->set_timeout(5000, [this]() { this->retry_sntp_sync_(); });
  } else {
    ESP_LOGE(TAG, "SNTP sync failed after 3 retries, proceeding with caution");
    this->initial_processing_started_ = true;
  }
}

void WaterStatistics::on_shutdown() {
  this->pref_.save(&this->water_);
  ESP_LOGD(TAG, "Saved Water NVS on shutdown: today=%f, yesterday=%f, week=%f, month=%f, year=%f",
           this->water_.start_today, this->water_.start_yesterday, this->water_.start_week,
           this->water_.start_month, this->water_.start_year);
}

void WaterStatistics::loop() {
  // Skip processing until SNTP sync delay
  if (!this->initial_processing_started_) {
    return;
  }

  // Handle initial total check non-blocking
  if (this->has_loaded_nvs_ && this->initial_total_retries_ > 0) {
    float total = this->total_->state;
    if (!std::isnan(total) && total >= 0.0f) {
      ESP_LOGD(TAG, "Processing restored Water total: %f", total);
      this->process_(total);
      this->initial_total_retries_ = 0; // Done
      this->has_loaded_nvs_ = false;
    } else {
      ESP_LOGD(TAG, "Waiting for valid Water total: %f, retries: %d", total, this->initial_total_retries_);
      this->initial_total_retries_--;
      if (this->initial_total_retries_ == 0) {
        ESP_LOGW(TAG, "Water total invalid after 5s: %f, retaining prior stats", total);
        this->has_loaded_nvs_ = false;
      }
      return; // Yield to avoid blocking
    }
  }

  const auto t = this->time_->now();
  if (!t.is_valid()) {
    ESP_LOGW(TAG, "Time not synchronized, skipping update");
    return;
  }

  const auto total = this->total_->get_state();
  if (std::isnan(total)) {
    ESP_LOGD(TAG, "Total Water not published yet, skipping");
    return;
  }

  // Update stats on first run or when day changes
  if (t.day_of_year == this->water_.current_day_of_year && this->water_.current_day_of_year != 0) {
    return; // No day change, skip
  }

  // Save the current day's data
  this->water_.start_yesterday = this->water_.start_today;
  this->water_.start_today = total;

  if (this->water_.current_day_of_year != 0) {
    // At specified day of week, start a new week calculation
    if (t.day_of_week == this->water_week_start_day_) {
      this->water_.start_week = total;
    }
    // At first day of month, start a new month calculation
    if (t.day_of_month == 1) {
      this->water_.start_month = total;
    }
    // At first day of year, start a new year calculation
    if (t.day_of_year == 1) {
      this->water_.start_year = total;
    }
  }

  // Initialize all sensors
  if (this->water_week_ && std::isnan(this->water_.start_week)) {
    this->water_.start_week = this->water_.start_yesterday;
  }
  if (this->water_month_ && std::isnan(this->water_.start_month)) {
    this->water_.start_month = this->water_.start_yesterday;
  }
  if (this->water_year_ && std::isnan(this->water_.start_year)) {
    this->water_.start_year = this->water_.start_yesterday;
  }

  this->water_.current_day_of_year = t.day_of_year;

  this->process_(total);
  this->pref_.save(&this->water_);
  ESP_LOGD(TAG, "Saved Water NVS on day change: today=%f, yesterday=%f, week=%f, month=%f, year=%f",
           this->water_.start_today, this->water_.start_yesterday, this->water_.start_week,
           this->water_.start_month, this->water_.start_year);
}

void WaterStatistics::process_(float total, bool is_initial_restore) {
  // Use stored start_today as fallback for initial restore if total is invalid
  if (is_initial_restore && std::isnan(total)) {
    total = this->water_.start_today;
    if (std::isnan(total)) {
      total = 0.0f; // Ultimate fallback
    }
  }

  // Calculate and publish today's water
  if (this->water_today_ && !std::isnan(this->water_.start_today)) {
    float value = total - this->water_.start_today;
    if (std::isnan(this->last_today_) || fabs(value - this->last_today_) > 0.001f) {
      this->water_today_->publish_state(value);
      this->last_today_ = value;
      this->pref_.save(&this->water_);
      ESP_LOGD(TAG, "Saved Water NVS after today update: today=%f", this->water_.start_today);
    }
  } else if (this->water_today_) {
    if (std::isnan(this->last_today_) || fabs(0.0f - this->last_today_) > 0.001f) {
      this->water_today_->publish_state(0);
      this->last_today_ = 0.0f;
      this->pref_.save(&this->water_);
      ESP_LOGD(TAG, "Saved Water NVS after today zero: today=%f", this->water_.start_today);
    }
  }

  // Calculate and publish yesterday's water
  if (this->water_yesterday_ && !std::isnan(this->water_.start_yesterday)) {
    float value = this->water_.start_today - this->water_.start_yesterday;
    if (std::isnan(this->last_yesterday_) || fabs(value - this->last_yesterday_) > 0.001f) {
      this->water_yesterday_->publish_state(value);
      this->last_yesterday_ = value;
      this->pref_.save(&this->water_);
      ESP_LOGD(TAG, "Saved Water NVS after yesterday update: yesterday=%f", this->water_.start_yesterday);
    }
  } else if (this->water_yesterday_) {
    if (std::isnan(this->last_yesterday_) || fabs(0.0f - this->last_yesterday_) > 0.001f) {
      this->water_yesterday_->publish_state(0);
      this->last_yesterday_ = 0.0f;
      this->pref_.save(&this->water_);
      ESP_LOGD(TAG, "Saved Water NVS after yesterday zero: yesterday=%f", this->water_.start_yesterday);
    }
  }

  // Calculate and publish weekly water
  if (this->water_week_ && !std::isnan(this->water_.start_week)) {
    float value = total - this->water_.start_week;
    if (std::isnan(this->last_week_) || fabs(value - this->last_week_) > 0.001f) {
      this->water_week_->publish_state(value);
      this->last_week_ = value;
      this->pref_.save(&this->water_);
      ESP_LOGD(TAG, "Saved Water NVS after week update: week=%f", this->water_.start_week);
    }
  } else if (this->water_week_) {
    if (std::isnan(this->last_week_) || fabs(0.0f - this->last_week_) > 0.001f) {
      this->water_week_->publish_state(0);
      this->last_week_ = 0.0f;
      this->pref_.save(&this->water_);
      ESP_LOGD(TAG, "Saved Water NVS after week zero: week=%f", this->water_.start_week);
    }
  }

  // Calculate and publish monthly water
  if (this->water_month_ && !std::isnan(this->water_.start_month)) {
    float value = total - this->water_.start_month;
    if (std::isnan(this->last_month_) || fabs(value - this->last_month_) > 0.001f) {
      this->water_month_->publish_state(value);
      this->last_month_ = value;
      this->pref_.save(&this->water_);
      ESP_LOGD(TAG, "Saved Water NVS after month update: month=%f", this->water_.start_month);
    }
  } else if (this->water_month_) {
    if (std::isnan(this->last_month_) || fabs(0.0f - this->last_month_) > 0.001f) {
      this->water_month_->publish_state(0);
      this->last_month_ = 0.0f;
      this->pref_.save(&this->water_);
      ESP_LOGD(TAG, "Saved Water NVS after month zero: month=%f", this->water_.start_month);
    }
  }

  // Calculate and publish yearly water
  if (this->water_year_ && !std::isnan(this->water_.start_year)) {
    float value = total - this->water_.start_year;
    if (std::isnan(this->last_year_) || fabs(value - this->last_year_) > 0.001f) {
      this->water_year_->publish_state(value);
      this->last_year_ = value;
      this->pref_.save(&this->water_);
      ESP_LOGD(TAG, "Saved Water NVS after year update: year=%f", this->water_.start_year);
    }
  } else if (this->water_year_) {
    if (std::isnan(this->last_year_) || fabs(0.0f - this->last_year_) > 0.001f) {
      this->water_year_->publish_state(0);
      this->last_year_ = 0.0f;
      this->pref_.save(&this->water_);
      ESP_LOGD(TAG, "Saved Water NVS after year zero: year=%f", this->water_.start_year);
    }
  }

  // Save to NVS on initial restore or state change
  if (is_initial_restore || this->has_loaded_nvs_ || this->water_.current_day_of_year != this->time_->now().day_of_year) {
    this->pref_.save(&this->water_);
    ESP_LOGD(TAG, "Saved Water NVS: today=%f, yesterday=%f, week=%f, month=%f, year=%f",
             this->water_.start_today, this->water_.start_yesterday, this->water_.start_week,
             this->water_.start_month, this->water_.start_year);
  }
}

}  // namespace water_statistics
}  // namespace esphome
