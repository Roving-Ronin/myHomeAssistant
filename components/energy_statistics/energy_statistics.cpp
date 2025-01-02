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
    } else {
      ESP_LOGD(TAG, "No weekly reset (day_of_week: %d, expected: %d)",
               t.day_of_week, this->energy_week_start_day_);
    }

    if (t.day_of_month == this->energy_month_start_day_) {
      this->energy_.start_month = total;
      ESP_LOGD(TAG, "Monthly reset: start_month set to %.5f", total);
    } else {
      ESP_LOGD(TAG, "No monthly reset (day_of_month: %d, expected: %d)",
               t.day_of_month, this->energy_month_start_day_);
    }

    if (t.day_of_year == this->energy_year_start_day_) {
      this->energy_.start_year = total;
      ESP_LOGD(TAG, "Yearly reset: start_year set to %.5f", total);
    } else {
      ESP_LOGD(TAG, "No yearly reset (day_of_year: %d, expected: %d)",
               t.day_of_year, this->energy_year_start_day_);
    }
  }

  this->process_(total);
}
