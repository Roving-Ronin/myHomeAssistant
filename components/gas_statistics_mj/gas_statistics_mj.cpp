#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "gas_statistics_mj.h"

namespace esphome {
namespace gas_statistics_mj {

static const char *const TAG = "gas_statistics_mj";

// Bumped to v4. v3 grew a field (start_quarter) but the "no NVS" fresh-start
// path set every start_* baseline to 0.0f instead of NAN, and the "backfill
// NaN baselines" step further down only fires for baselines that are still
// NaN - so a fresh v3 struct got permanently stuck with start_week/month/
// quarter/year pinned at 0.0f until the real calendar boundary arrived,
// making Week/Month/Quarter/Year read as the full lifetime total in the
// meantime. v4 fixes that (see setup()/loop() below) and forces one more
// clean re-initialization so any device already stuck in the v3 state
// self-heals on next boot instead of waiting out the bug.
static const char *const PREF_V4 = "gas_statistics_mj_v4";

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
  if (this->gas_quarter_) {
    LOG_SENSOR("  ", "Gas (MJ) Quarter", this->gas_quarter_);
  }
  if (this->quarter_reset_day_) {
    ESP_LOGCONFIG(TAG, "  Quarter reset day source: select entity (state read each day change)");
  } else {
    ESP_LOGCONFIG(TAG, "  Quarter reset day source: fixed default (day %d)", this->gas_quarter_reset_day_default_);
  }
  if (this->quarter_start_month_) {
    ESP_LOGCONFIG(TAG, "  Quarter start month source: select entity (state read each day change)");
  } else {
    ESP_LOGCONFIG(TAG, "  Quarter start month source: fixed default (month %d)", this->gas_quarter_start_month_default_);
  }
}

void GasStatisticsMJ::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });

  this->pref_ = global_preferences->make_preference<gas_mj_data_t>(fnv1_hash(PREF_V4));
  bool loaded = this->pref_.load(&this->gas_);
  if (loaded) {
    ESP_LOGI(TAG, "Loaded Gas (MJ) NVS: today=%f, yesterday=%f, week=%f, month=%f, year=%f, quarter=%f",
             this->gas_.start_today, this->gas_.start_yesterday, this->gas_.start_week,
             this->gas_.start_month, this->gas_.start_year, this->gas_.start_quarter);
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
    // Initialize defaults to NAN (matching the struct's own member
    // defaults) rather than 0.0f, so loop() can tell "never initialized"
    // apart from "genuinely zero" and correctly snaps every baseline to
    // the current total on the very first run below.
    this->gas_.start_today = NAN;
    this->gas_.start_yesterday = NAN;
    this->gas_.start_week = NAN;
    this->gas_.start_month = NAN;
    this->gas_.start_year = NAN;
    this->gas_.start_quarter = NAN;
    this->pref_.save(&this->gas_);
    ESP_LOGD(TAG, "Saved initial Gas (MJ) NVS: today=%f, yesterday=%f, week=%f, month=%f, year=%f, quarter=%f",
             this->gas_.start_today, this->gas_.start_yesterday, this->gas_.start_week,
             this->gas_.start_month, this->gas_.start_year, this->gas_.start_quarter);
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
      this->pref_.save(&this->gas_);
      ESP_LOGD(TAG, "Saved Gas (MJ) NVS after 5min interval (value changed): today=%f, yesterday=%f, week=%f, month=%f, year=%f, quarter=%f",
               this->gas_.start_today, this->gas_.start_yesterday, this->gas_.start_week,
               this->gas_.start_month, this->gas_.start_year, this->gas_.start_quarter);
      this->has_value_changed_ = false;
    } else {
      ESP_LOGV(TAG, "Skipped Gas (MJ) NVS save after 5min interval (no value change)");
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
  ESP_LOGD(TAG, "Saved Gas (MJ) NVS on shutdown: today=%f, yesterday=%f, week=%f, month=%f, year=%f, quarter=%f",
           this->gas_.start_today, this->gas_.start_yesterday, this->gas_.start_week,
           this->gas_.start_month, this->gas_.start_year, this->gas_.start_quarter);
}

int GasStatisticsMJ::days_in_month_(int year, int month) {
  static const int dim[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) {
    return 31;
  }
  if (month == 2) {
    bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    return leap ? 29 : 28;
  }
  return dim[month - 1];
}

int GasStatisticsMJ::month_name_to_number_(const std::string &name) {
  static const char *const names[12] = {"January", "February", "March",     "April",   "May",      "June",
                                         "July",    "August",   "September", "October", "November", "December"};
  for (int i = 0; i < 12; i++) {
    if (name == names[i]) {
      return i + 1;
    }
  }
  return 0;  // No match
}

int GasStatisticsMJ::get_quarter_start_month_() {
  if (this->quarter_start_month_ != nullptr) {
    int parsed = GasStatisticsMJ::month_name_to_number_(this->quarter_start_month_->state);
    if (parsed >= 1 && parsed <= 12) {
      return parsed;
    }
  }
  return this->gas_quarter_start_month_default_;
}

int GasStatisticsMJ::get_quarter_reset_day_(int year, int month) {
  int configured_day = this->gas_quarter_reset_day_default_;
  if (this->quarter_reset_day_ != nullptr) {
    const std::string &state = this->quarter_reset_day_->state;
    if (!state.empty()) {
      int parsed = atoi(state.c_str());
      if (parsed >= 1 && parsed <= 31) {
        configured_day = parsed;
      }
    }
  }
  // Clamp to the actual number of days in this month, e.g. a configured
  // reset day of 31 falls back to the 28th/29th when the quarter-start
  // month is February.
  int max_day = this->days_in_month_(year, month);
  return configured_day > max_day ? max_day : configured_day;
}

bool GasStatisticsMJ::is_quarter_start_month_(int month) {
  int anchor = this->get_quarter_start_month_();
  int diff = month - anchor;
  if (diff < 0) {
    diff += 12;
  }
  return (diff % 3) == 0;
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
      ESP_LOGD(TAG, "Processing Gas (MJ) restored total: %f", total);
      this->process_(total);
      this->initial_total_retries_ = 0; // Done
      this->has_loaded_nvs_ = false;
    } else {
      ESP_LOGD(TAG, "Waiting for valid Gas (MJ) total: %f, retries: %d", total, this->initial_total_retries_);
      this->initial_total_retries_--;
      if (this->initial_total_retries_ == 0) {
        ESP_LOGW(TAG, "Total Gas (MJ) invalid after 5s: %f, retaining prior stats", total);
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

  // A day change (current_day_of_year != 0 and different from t.day_of_year)
  // means a real calendar day boundary was crossed. current_day_of_year == 0
  // means this is the very first run against a fresh/reset baseline, with no
  // prior data to compare against - every period is force-started here too,
  // so Today/Week/Month/Quarter/Year all correctly read 0 immediately after
  // a reset instead of some of them getting stuck reporting the full
  // lifetime total until their real boundary eventually arrives.
  bool is_first_run = (this->gas_.current_day_of_year == 0);

  // Save the current day's data
  this->gas_.start_yesterday = this->gas_.start_today;
  this->gas_.start_today = total;

  // At specified day of week, start a new week calculation
  if (is_first_run || t.day_of_week == this->gas_week_start_day_) {
    this->gas_.start_week = total;
  }
  // At first day of month, start a new month calculation
  if (is_first_run || t.day_of_month == 1) {
    this->gas_.start_month = total;
  }
  // At first day of year, start a new year calculation
  if (is_first_run || t.day_of_year == 1) {
    this->gas_.start_year = total;
  }
  // At the configured reset day, within a quarter-start month (derived
  // from the configurable start-month anchor), start a new quarter
  // calculation
  if (is_first_run ||
      (this->is_quarter_start_month_(t.month) && t.day_of_month == this->get_quarter_reset_day_(t.year, t.month))) {
    this->gas_.start_quarter = total;
    if (!is_first_run) {
      ESP_LOGI(TAG, "Gas (MJ) quarter reset triggered: month=%d, day=%d, baseline=%f", t.month, t.day_of_month, total);
    }
  }

  // Defensive backfill: if any baseline is still NaN for some other reason
  // (e.g. a sensor was newly added to the YAML after initial setup), fall
  // back to yesterday's starting value rather than leaving it unset.
  if (this->gas_week_ && std::isnan(this->gas_.start_week)) {
    this->gas_.start_week = this->gas_.start_yesterday;
  }
  if (this->gas_month_ && std::isnan(this->gas_.start_month)) {
    this->gas_.start_month = this->gas_.start_yesterday;
  }
  if (this->gas_year_ && std::isnan(this->gas_.start_year)) {
    this->gas_.start_year = this->gas_.start_yesterday;
  }
  if (this->gas_quarter_ && std::isnan(this->gas_.start_quarter)) {
    this->gas_.start_quarter = this->gas_.start_yesterday;
  }

  this->gas_.current_day_of_year = t.day_of_year;

  this->process_(total);
  this->pref_.save(&this->gas_);
  ESP_LOGD(TAG, "Saved Gas (MJ) NVS on day change: today=%f, yesterday=%f, week=%f, month=%f, year=%f, quarter=%f",
           this->gas_.start_today, this->gas_.start_yesterday, this->gas_.start_week,
           this->gas_.start_month, this->gas_.start_year, this->gas_.start_quarter);
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
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Gas (MJ) Today value changed: %f", value);
    }
  } else if (this->gas_today_) {
    if (std::isnan(this->last_today_) || fabs(0.0f - this->last_today_) > 0.001f) {
      this->gas_today_->publish_state(0);
      this->last_today_ = 0.0f;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Gas (MJ) Today value changed to zero");
    }
  }

  // Calculate and publish yesterday's gas
  if (this->gas_yesterday_ && !std::isnan(this->gas_.start_yesterday)) {
    float value = this->gas_.start_today - this->gas_.start_yesterday;
    if (std::isnan(this->last_yesterday_) || fabs(value - this->last_yesterday_) > 0.001f) {
      this->gas_yesterday_->publish_state(value);
      this->last_yesterday_ = value;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Gas (MJ) Yesterday value changed: %f", value);
    }
  } else if (this->gas_yesterday_) {
    if (std::isnan(this->last_yesterday_) || fabs(0.0f - this->last_yesterday_) > 0.001f) {
      this->gas_yesterday_->publish_state(0);
      this->last_yesterday_ = 0.0f;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Gas (MJ) Yesterday value changed to zero");
    }
  }

  // Calculate and publish weekly gas
  if (this->gas_week_ && !std::isnan(this->gas_.start_week)) {
    float value = total - this->gas_.start_week;
    if (std::isnan(this->last_week_) || fabs(value - this->last_week_) > 0.001f) {
      this->gas_week_->publish_state(value);
      this->last_week_ = value;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Gas (MJ) Week value changed: %f", value);
    }
  } else if (this->gas_week_) {
    if (std::isnan(this->last_week_) || fabs(0.0f - this->last_week_) > 0.001f) {
      this->gas_week_->publish_state(0);
      this->last_week_ = 0.0f;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Gas (MJ) Week value changed to zero");
    }
  }

  // Calculate and publish monthly gas
  if (this->gas_month_ && !std::isnan(this->gas_.start_month)) {
    float value = total - this->gas_.start_month;
    if (std::isnan(this->last_month_) || fabs(value - this->last_month_) > 0.001f) {
      this->gas_month_->publish_state(value);
      this->last_month_ = value;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Gas (MJ) Month value changed: %f", value);
    }
  } else if (this->gas_month_) {
    if (std::isnan(this->last_month_) || fabs(0.0f - this->last_month_) > 0.001f) {
      this->gas_month_->publish_state(0);
      this->last_month_ = 0.0f;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Gas (MJ) Month value changed to zero");
    }
  }

  // Calculate and publish yearly gas
  if (this->gas_year_ && !std::isnan(this->gas_.start_year)) {
    float value = total - this->gas_.start_year;
    if (std::isnan(this->last_year_) || fabs(value - this->last_year_) > 0.001f) {
      this->gas_year_->publish_state(value);
      this->last_year_ = value;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Gas (MJ) Year value changed: %f", value);
    }
  } else if (this->gas_year_) {
    if (std::isnan(this->last_year_) || fabs(0.0f - this->last_year_) > 0.001f) {
      this->gas_year_->publish_state(0);
      this->last_year_ = 0.0f;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Gas (MJ) Year value changed to zero");
    }
  }

  // Calculate and publish quarterly gas
  if (this->gas_quarter_ && !std::isnan(this->gas_.start_quarter)) {
    float value = total - this->gas_.start_quarter;
    if (std::isnan(this->last_quarter_) || fabs(value - this->last_quarter_) > 0.001f) {
      this->gas_quarter_->publish_state(value);
      this->last_quarter_ = value;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Gas (MJ) Quarter value changed: %f", value);
    }
  } else if (this->gas_quarter_) {
    if (std::isnan(this->last_quarter_) || fabs(0.0f - this->last_quarter_) > 0.001f) {
      this->gas_quarter_->publish_state(0);
      this->last_quarter_ = 0.0f;
      this->has_value_changed_ = true;
      ESP_LOGD(TAG, "Gas (MJ) Quarter value changed to zero");
    }
  }

  // Save to NVS on initial restore
  if (is_initial_restore) {
    this->pref_.save(&this->gas_);
    ESP_LOGD(TAG, "Saved Gas (MJ) NVS on initial restore: today=%f, yesterday=%f, week=%f, month=%f, year=%f, quarter=%f",
             this->gas_.start_today, this->gas_.start_yesterday, this->gas_.start_week,
             this->gas_.start_month, this->gas_.start_year, this->gas_.start_quarter);
    this->has_value_changed_ = false;
  }
}

}  // namespace gas_statistics_mj
}  // namespace esphome
