#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/components/ota/ota_component.h"
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
    ESP_LOGI(TAG, "Loaded Gas (MJ) NVS: today=%f, yesterday=%f, week=%f, month=%f, year=%f",
             this->gas_.start_today, this->gas_.start_yesterday, this->gas_.start_week,
             this->gas_.start_month, this->gas_.start_year);
    this->initial_total_retries_ = 40; // Try for 5 seconds to get valid total
    this->has_loaded_nvs_ = true;
    // Process stored values for initial restoration
    float total = this->total_->state;
    if (std::isnan(total)) {
      total = this->gas_.start_today; // Fallback to stored start_today
    }
    this->process_(total, true); // Initial restore
  } else {
    ESP_LOGW(TAG, "No Gas (MJ) NVS data loaded, starting fresh");
    // Initialize defaults
    this->gas_.start_today = 0.0f;
    this->gas_.start_yesterday = 0.0f;
    this->gas_.start_week = 0.0f;
    this->gas_.start_month = 0.0f;
    this->gas_.start_year = 0.0f;
    this->pref_.save(&this->gas_);
    ESP_LOGD(TAG, "Saved initial Gas (MJ) NVS: today=%f, yesterday=%f, week=%f, month=%f, year=%f",
             this->gas_.start_today, this->gas_.start_yesterday, this->gas_.start_week,
             this->gas_.start_month, this->gas_.start_year);
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

  // Register OTA callback to save NVS before OTA
  ota::global_ota_component->add_on_state_callback([this](ota::OTAState state, float progress, uint8_t error) {
    if (state == ota::OTA_STARTED) {
      this->pref_.save(&this->gas_);
      ESP_LOGD(TAG, "Saved Gas (MJ) NVS before OTA: today=%f, yesterday=%f, week=%f, month=%f, year=%f",
               this->gas_.start_today, this->gas_.start_yesterday, this->gas_.start_week,
               this->gas_.start_month, this->gas_.start_year);
    }
  });
}

void GasStatisticsMJ::retry_sntp_sync_() {
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

void GasStatisticsMJ::on_shutdown() {
  this->pref_.save(&this->gas_);
  ESP_LOGD(TAG, "Saved Gas (MJ) NVS on shutdown: today=%f, yesterday=%f, week=%f, month=%f, year=%f",
           this->gas_.start_today, this->gas_.start_yesterday, this->gas_.start_week,
           this->gas_.start_month, this->gas_.start_year);
}

void GasStatisticsMJ::loop() {
  // Skip processing until SNTP sync delay
  if (!this->initial_processing_started_) {
    return;
  }

  // Handle initial total check non-blocking
  if (this->has_loaded_nvs_ && this->initial_total_retries_ > 0) {
    float total = this->total_->state;
    if (!std::isnan(total) && total >= 0.0f) {
      ESP_LOGD(TAG, "Processing restored Gas (MJ) total: %f", total);
      this->process_(total);
      this->initial_total_retries_ = 0; // Done
      this->has_loaded_nvs_ = false;
    } else {
      ESP_LOGD(TAG, "Waiting for valid Gas (MJ) total: %f, retries: %d", total, this->initial_total_retries_);
      this->initial_total_retries_--;
      if (this->initial_total_retries_ == 0) {
        ESP_LOGW(TAG, "Total invalid after 5s: %f, retaining prior stats", total);
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
    ESP_LOGD(TAG, "Total Gas (MJ) not published yet, skipping");
    return;
  }

  // Update stats on first run or when day changes
  if (t.day_of_year == this->gas_.current_day_of_year && this->gas_.current_day_of_year != 0) {
    return; // No day change, skip
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

  // Initialize all sensors
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
  this->pref_.save(&this->gas_);
  ESP_LOGD(TAG, "Saved Gas (MJ) NVS on day change: today=%f, yesterday=%f, week=%f, month=%f, year=%f",
           this->gas_.start_today, this->gas_.start_yesterday, this->gas_.start_week,
           this->gas_.start_month, this->gas_.start_year);
}

void GasStatisticsMJ::process_(float total, bool is_initial_restore) {
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

  // Save to NVS on initial restore or state change
  if (is_initial_restore || this->has_loaded_nvs_ || this->gas_.current_day_of_year != this->time_->now().day_of_year) {
    this->pref_.save(&this->gas_);
    ESP_LOGD(TAG, "Saved Gas (MJ) NVS: today=%f, yesterday=%f, week=%f, month=%f, year=%f",
             this->gas_.start_today, this->gas_.start_yesterday, this->gas_.start_week,
             this->gas_.start_month, this->gas_.start_year);
  }
}

}  // namespace gas_statistics_mj
}  // namespace esphome
