#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "water_statistics.h"

namespace esphome {
namespace water_statistics {

static const char *const TAG = "water_statistics";

// Bumped to v3, and the key is no longer a single fixed string. v2 (and
// earlier) used a hardcoded "water_statistics_v2" key shared by *every*
// instance of this component - fine while only one instance existed (Town
// Water), but a second instance (e.g. Tank Water, once its flow meter is
// installed) would silently read/write the exact same NVS slot, corrupting
// whichever instance's setup() ran second. v3 derives the key from the
// instance's own `total:` sensor object_id, so each instance gets its own
// storage automatically - no YAML-side configuration needed.
//
// v3 also grew two fields (start_quarter, quarter_start_day_of_year/year)
// for the new manual quarter-tracking support, and fixes the same
// "fresh NVS defaults to 0.0f instead of NAN" bug that gas_statistics had:
// previously, a freshly-flashed device would get every start_* baseline
// pinned to 0.0f, and since the "backfill NaN baselines" step only fires
// for baselines that are still NaN, Week/Month/Year got stuck reading the
// full lifetime total until their real calendar boundary next arrived.
static const char *const PREF_PREFIX = "water_statistics_v3_";

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
  if (this->water_quarter_) {
    LOG_SENSOR("  ", "Water (L) Quarter", this->water_quarter_);
  }
}

void WaterStatistics::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });

  // Per-instance key: e.g. "water_statistics_v3_town_water_total". Falls
  // back to just the prefix (matching pre-v3 shared behaviour) only in the
  // unlikely case the total sensor has no object_id yet.
  std::string pref_key = std::string(PREF_PREFIX) + this->total_->get_object_id();
  this->pref_ = global_preferences->make_preference<water_data_t>(fnv1_hash(pref_key));
  bool loaded = this->pref_.load(&this->water_);
  if (loaded) {
    ESP_LOGI(TAG, "Loaded Water NVS (%s): today=%f, yesterday=%f, week=%f, month=%f, year=%f, quarter=%f",
             pref_key.c_str(), this->water_.start_today, this->water_.start_yesterday, this->water_.start_week,
             this->water_.start_month, this->water_.start_year, this->water_.start_quarter);
    this->initial_total_retries_ = 40; // Try for 5 seconds to get valid total
    this->has_loaded_nvs_ = true;
    // Process stored values for initial restoration
    float total = this->total_->state;
    if (std::isnan(total)) {
      total = this->water_.start_today; // Fallback to stored start_today
    }
    this->process_(total, true); // Initial restore
  } else {
    ESP_LOGW(TAG, "No Water NVS data loaded (%s), starting fresh", pref_key.c_str());
    // Initialize defaults to NAN (matching the struct's own member
    // defaults) rather than 0.0f, so loop() can tell "never initialized"
    // apart from "genuinely zero" and correctly snaps every baseline to
    // the current total on the very first run below.
    this->water_.start_today = NAN;
    this->water_.start_yesterday = NAN;
    this->water_.start_week = NAN;
    this->water_.start_month = NAN;
    this->water_.start_year = NAN;
    this->water_.start_quarter = NAN;
    this->water_.quarter_start_day_of_year = 0;
    this->water_.quarter_start_year = 0;
    this->pref_.save(&this->water_);
    ESP_LOGD(TAG, "Saved initial Water NVS: today=%f, yesterday=%f, week=%f, month=%f, year=%f, quarter=%f",
             this->water_.start_today, this->water_.start_yesterday, this->water_.start_week,
             this->water_.start_month, this->water_.start_year, this->water_.start_quarter);
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

  // Periodic NVS save every 5 minutes if values changed
  this->set_interval(300000, [this]() {
    if (this->has_value_changed_) {
      this->pref_.save(&this->water_);
      ESP_LOGD(TAG, "Saved Water NVS after 5min interval (value changed): today=%f, yesterday=%f, week=%f, month=%f, year=%f, quarter=%f",
               this->water_.start_today, this->water_.start_yesterday, this->water_.start_week,
               this->water_.start_month, this->water_.start_year, this->water_.start_quarter);
      this->has_value_changed_ = false;
    } else {
      ESP_LOGV(TAG, "Skipped Water NVS save after 5min interval (no value change)");
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
  ESP_LOGD(TAG, "Saved Water NVS on shutdown: today=%f, yesterday=%f, week=%f, month=%f, year=%f, quarter=%f",
           this->water_.start_today, this->water_.start_yesterday, this->water_.start_week,
           this->water_.start_month, this->water_.start_year, this->water_.start_quarter);
}

void WaterStatistics::reset_quarter(float already_consumed) {
  float current_total = this->total_->get_state();
  if (std::isnan(current_total)) {
    ESP_LOGW(TAG, "Water reset_quarter called but total not yet available, ignoring");
    return;
  }
  this->water_.start_quarter = current_total - already_consumed;
  const auto t = this->time_->now();
  if (t.is_valid()) {
    this->water_.quarter_start_day_of_year = t.day_of_year;
    this->water_.quarter_start_year = t.year;
  }
  // Force process_() to republish even if the numeric value happens to
  // match what was last published (e.g. resetting to the same figure twice).
  this->last_quarter_ = NAN;
  this->pref_.save(&this->water_);
  this->process_(current_total);
  ESP_LOGI(TAG, "Water quarter manually (re)started: total=%f, already_consumed=%f, baseline=%f", current_total,
           already_consumed, this->water_.start_quarter);
}

void WaterStatistics::calibrate_total(float new_total) {
  float old_total = this->total_->get_state();
  if (std::isnan(old_total)) {
    old_total = new_total;
  }
  float delta = new_total - old_total;
  if (!std::isnan(this->water_.start_today)) this->water_.start_today += delta;
  if (!std::isnan(this->water_.start_yesterday)) this->water_.start_yesterday += delta;
  if (!std::isnan(this->water_.start_week)) this->water_.start_week += delta;
  if (!std::isnan(this->water_.start_month)) this->water_.start_month += delta;
  if (!std::isnan(this->water_.start_year)) this->water_.start_year += delta;
  // start_quarter intentionally left alone here - see header comment.
  this->last_today_ = NAN;
  this->last_yesterday_ = NAN;
  this->last_week_ = NAN;
  this->last_month_ = NAN;
  this->last_year_ = NAN;
  this->pref_.save(&this->water_);
  this->process_(new_total);
  ESP_LOGI(TAG, "Water total calibrated: old=%f, new=%f, delta=%f applied to today/week/month/year baselines",
           old_total, new_total, delta);
}

int WaterStatistics::days_into_quarter() {
  if (this->water_.quarter_start_day_of_year == 0) {
    return 0;  // reset_quarter() has never been called on this instance
  }
  const auto t = this->time_->now();
  if (!t.is_valid()) {
    return 0;
  }
  int days;
  if (t.year == this->water_.quarter_start_year) {
    days = (int) t.day_of_year - (int) this->water_.quarter_start_day_of_year + 1;
  } else {
    // Quarter start was in a previous calendar year - water billing periods
    // regularly straddle the New Year boundary. Approximate using the
    // quarter-start year's leap status; good enough for a period that spans
    // at most one New Year.
    bool leap = (this->water_.quarter_start_year % 4 == 0 &&
                 (this->water_.quarter_start_year % 100 != 0 || this->water_.quarter_start_year % 400 == 0));
    int days_in_start_year = leap ? 366 : 365;
    days = (days_in_start_year - (int) this->water_.quarter_start_day_of_year + 1) + (int) t.day_of_year;
  }
  return days < 1 ? 1 : days;
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
      ESP_LOGD(TAG, "Processing Water restored total: %f", total);
      this->process_(total);
      this->initial_total_retries_ = 0; // Done
      this->has_loaded_nvs_ = false;
    } else {
      ESP_LOGD(TAG, "Waiting for valid Water total: %f, retries: %d", total, this->initial_total_retries_);
      this->initial_total_retries_--;
      if (this->initial_total_retries_ == 0) {
        ESP_LOGW(TAG, "Total Water invalid after 5s: %f, retaining prior stats", total);
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

  // A day change (current_day_of_year != 0 and different from t.day_of_year)
  // means a real calendar day boundary was crossed. current_day_of_year == 0
  // means this is the very first run against a fresh/reset baseline, with no
  // prior data to compare against - every period is force-started here too,
  // so Today/Week/Month/Year all correctly read 0 immediately after a reset
  // instead of getting stuck reporting the full lifetime total until their
  // real boundary eventually arrives. (Quarter is deliberately NOT included
  // here - it only ever starts via an explicit reset_quarter() call, since
  // there's no automatic day/month pattern for water's irregular periods.)
  bool is_first_run = (this->water_.current_day_of_year == 0);

  // Save the current day's data
  this->water_.start_yesterday = this->water_.start_today;
  this->water_.start_today = total;

  // At specified day of week, start a new week calculation
  if (is_first_run || t.day_of_week == this->water_week_start_day_) {
    this->water_.start_week = total;
  }
  // At first day of month, start a new month calculation
  if (is_first_run || t.day_of_month == 1) {
    this->water_.start_month = total;
  }
  // At first day of year, start a new year calculation
  if (is_first_run || t.day_of_year == 1) {
    this->water_.start_year = total;
  }

  // Defensive backfill: if any baseline is still NaN for some other reason
  // (e.g. a sensor was newly added to the YAML after initial setup), fall
  // back to yesterday's starting value rather than leaving it unset.
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
  ESP_LOGD(TAG, "Saved Water NVS on day change: today=%f, yesterday=%f, week=%f, month=%f, year=%f, quarter=%f",
           this->water_.start_today, this->water_.start_yesterday, this->water_.start_week,
           this->water_.start_month, this->water_.start_year, this->water_.start_quarter);
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
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Water Today value changed: %f", value);
    }
  } else if (this->water_today_) {
    if (std::isnan(this->last_today_) || fabs(0.0f - this->last_today_) > 0.001f) {
      this->water_today_->publish_state(0);
      this->last_today_ = 0.0f;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Water Today value changed to zero");
    }
  }

  // Calculate and publish yesterday's water
  if (this->water_yesterday_ && !std::isnan(this->water_.start_yesterday)) {
    float value = this->water_.start_today - this->water_.start_yesterday;
    if (std::isnan(this->last_yesterday_) || fabs(value - this->last_yesterday_) > 0.001f) {
      this->water_yesterday_->publish_state(value);
      this->last_yesterday_ = value;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Water Yesterday value changed: %f", value);
    }
  } else if (this->water_yesterday_) {
    if (std::isnan(this->last_yesterday_) || fabs(0.0f - this->last_yesterday_) > 0.001f) {
      this->water_yesterday_->publish_state(0);
      this->last_yesterday_ = 0.0f;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Water Yesterday value changed to zero");
    }
  }

  // Calculate and publish weekly water
  if (this->water_week_ && !std::isnan(this->water_.start_week)) {
    float value = total - this->water_.start_week;
    if (std::isnan(this->last_week_) || fabs(value - this->last_week_) > 0.001f) {
      this->water_week_->publish_state(value);
      this->last_week_ = value;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Water Week value changed: %f", value);
    }
  } else if (this->water_week_) {
    if (std::isnan(this->last_week_) || fabs(0.0f - this->last_week_) > 0.001f) {
      this->water_week_->publish_state(0);
      this->last_week_ = 0.0f;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Water Week value changed to zero");
    }
  }

  // Calculate and publish monthly water
  if (this->water_month_ && !std::isnan(this->water_.start_month)) {
    float value = total - this->water_.start_month;
    if (std::isnan(this->last_month_) || fabs(value - this->last_month_) > 0.001f) {
      this->water_month_->publish_state(value);
      this->last_month_ = value;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Water Month value changed: %f", value);
    }
  } else if (this->water_month_) {
    if (std::isnan(this->last_month_) || fabs(0.0f - this->last_month_) > 0.001f) {
      this->water_month_->publish_state(0);
      this->last_month_ = 0.0f;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Water Month value changed to zero");
    }
  }

  // Calculate and publish yearly water
  if (this->water_year_ && !std::isnan(this->water_.start_year)) {
    float value = total - this->water_.start_year;
    if (std::isnan(this->last_year_) || fabs(value - this->last_year_) > 0.001f) {
      this->water_year_->publish_state(value);
      this->last_year_ = value;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Water Year value changed: %f", value);
    }
  } else if (this->water_year_) {
    if (std::isnan(this->last_year_) || fabs(0.0f - this->last_year_) > 0.001f) {
      this->water_year_->publish_state(0);
      this->last_year_ = 0.0f;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Water Year value changed to zero");
    }
  }

  // Calculate and publish quarterly water (only ever set via reset_quarter())
  if (this->water_quarter_ && !std::isnan(this->water_.start_quarter)) {
    float value = total - this->water_.start_quarter;
    if (std::isnan(this->last_quarter_) || fabs(value - this->last_quarter_) > 0.001f) {
      this->water_quarter_->publish_state(value);
      this->last_quarter_ = value;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Water Quarter value changed: %f", value);
    }
  } else if (this->water_quarter_) {
    if (std::isnan(this->last_quarter_) || fabs(0.0f - this->last_quarter_) > 0.001f) {
      this->water_quarter_->publish_state(0);
      this->last_quarter_ = 0.0f;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Water Quarter value changed to zero");
    }
  }

  // Save to NVS on initial restore
  if (is_initial_restore) {
    this->pref_.save(&this->water_);
    ESP_LOGD(TAG, "Saved Water NVS on initial restore: today=%f, yesterday=%f, week=%f, month=%f, year=%f, quarter=%f",
             this->water_.start_today, this->water_.start_yesterday, this->water_.start_week,
             this->water_.start_month, this->water_.start_year, this->water_.start_quarter);
    this->has_value_changed_ = false;
  }
}

}  // namespace water_statistics
}  // namespace esphome
