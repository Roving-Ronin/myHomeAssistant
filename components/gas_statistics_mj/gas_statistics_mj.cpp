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
//
// Bumped to v5: struct grew quarter_start_day_of_year/quarter_start_year,
// used by days_into_quarter() to scale per-day MJ pricing thresholds by the
// actual number of days elapsed in the current quarter.
static const char *const PREF_V5 = "gas_statistics_mj_v5";

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
  ESP_LOGCONFIG(TAG, "  Quarter start: manual only (datetime entity / max-length fallback)");
}

void GasStatisticsMJ::setup() {
  this->total_->add_on_state_callback([this](float state) { this->process_(state); });

  this->pref_ = global_preferences->make_preference<gas_mj_data_t>(fnv1_hash(PREF_V5));
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
    this->gas_.quarter_start_day_of_year = 0;
    this->gas_.quarter_start_year = 0;
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

namespace {
// Converts a calendar date to a day-of-year (1-366), for backdating a
// quarter start to a specific date picked via the "Quarter Start Date"
// datetime entity, rather than always stamping "today".
uint16_t day_of_year_from_ymd(uint16_t year, uint8_t month, uint8_t day) {
  static const uint16_t cumulative_days[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  if (month < 1 || month > 12) {
    month = 1;
  }
  bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
  uint16_t doy = cumulative_days[month - 1] + day;
  if (leap && month > 2) {
    doy += 1;
  }
  return doy;
}
}  // namespace

void GasStatisticsMJ::reset_quarter(float already_consumed) {
  const auto t = this->time_->now();
  if (t.is_valid()) {
    this->set_quarter_baseline_(already_consumed, t.day_of_year, t.year);
  } else {
    // Time not synced yet - still move the usage baseline, but leave
    // whatever quarter start date is already stored in place rather than
    // writing a bogus one.
    this->set_quarter_baseline_(already_consumed, 0, 0);
  }
}

void GasStatisticsMJ::reset_quarter_from_date(float already_consumed, uint16_t year, uint8_t month, uint8_t day) {
  this->set_quarter_baseline_(already_consumed, day_of_year_from_ymd(year, month, day), year);
}

void GasStatisticsMJ::set_quarter_baseline_(float already_consumed, uint16_t start_day_of_year, uint16_t start_year) {
  float current_total = this->total_->get_state();
  if (std::isnan(current_total)) {
    ESP_LOGW(TAG, "Gas (MJ) quarter baseline set called but total not yet available, ignoring");
    return;
  }
  this->gas_.start_quarter = current_total - already_consumed;
  if (start_day_of_year != 0 && start_year != 0) {
    this->gas_.quarter_start_day_of_year = start_day_of_year;
    this->gas_.quarter_start_year = start_year;
  }
  // Force process_() to republish even if the numeric value happens to match
  // what was last published (e.g. resetting to the same figure twice).
  this->last_quarter_ = NAN;
  this->pref_.save(&this->gas_);
  this->process_(current_total);
  ESP_LOGI(TAG,
           "Gas (MJ) quarter (re)started: total=%f, already_consumed=%f, baseline=%f, start_day_of_year=%u, "
           "start_year=%u",
           current_total, already_consumed, this->gas_.start_quarter, (unsigned) this->gas_.quarter_start_day_of_year,
           (unsigned) this->gas_.quarter_start_year);
}

int GasStatisticsMJ::days_into_quarter() {
  if (this->gas_.quarter_start_day_of_year == 0) {
    return 0;  // Quarter start date not yet established
  }
  const auto t = this->time_->now();
  if (!t.is_valid()) {
    return 0;
  }
  int days;
  if (t.year == this->gas_.quarter_start_year) {
    days = (int) t.day_of_year - (int) this->gas_.quarter_start_day_of_year + 1;
  } else {
    // Quarter start was in a previous calendar year (e.g. a Nov-anchored
    // quarter still running in January). Approximate the year length using
    // the quarter-start year's leap status - good enough for a quarter that
    // spans at most one New Year boundary.
    bool leap = (this->gas_.quarter_start_year % 4 == 0 &&
                 (this->gas_.quarter_start_year % 100 != 0 || this->gas_.quarter_start_year % 400 == 0));
    int days_in_start_year = leap ? 366 : 365;
    days = (days_in_start_year - (int) this->gas_.quarter_start_day_of_year + 1) + (int) t.day_of_year;
  }
  return days < 1 ? 1 : days;
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
  // Quarter start is now driven entirely manually - either via the "Gas -
  // Quarter Start Date" datetime entity's on_value handler (which calls
  // reset_quarter_from_date()), or the max-quarter-length fallback in
  // gas.yaml (which calls the plain reset_quarter()). There's no more
  // automatic calendar day/month matching here - only the very first run
  // after a fresh/reset baseline forces start_quarter to snap to the
  // current total, so Quarter doesn't read the full lifetime total while
  // waiting for the user to set the real start date.
  if (is_first_run) {
    this->gas_.start_quarter = total;
    this->gas_.quarter_start_day_of_year = t.day_of_year;
    this->gas_.quarter_start_year = t.year;
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
