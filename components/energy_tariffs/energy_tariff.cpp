#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "energy_tariff.h"
#include "energy_tariffs.h"
#include <map>
#include <vector>

namespace esphome {
namespace energy_tariffs {

static const char *const TAG = "energy_tariff";
static const char *const GAP = "  ";

void EnergyTariff::dump_config() {
  LOG_SENSOR(GAP, "Energy Tariff", this);
  ESP_LOGCONFIG(TAG, "    Default: %s", this->is_default() ? "true" : "false");

  for (auto const &day_tariff : this->day_tariffs_) {
    ESP_LOGCONFIG(TAG, "  Day: %s", day_tariff.first.c_str());
    for (auto const &t : day_tariff.second) {
      ESP_LOGCONFIG(TAG, "    Time: %02d:%02d-%02d:%02d", t.min / 60, t.min % 60, t.max / 60, t.max % 60);
    }
  }

  ESP_LOGCONFIG(TAG, "    State: %.2f", this->state);
}

void EnergyTariff::setup() {
  this->rtc_ = global_preferences->make_preference<float>(this->get_object_id_hash());

  float loaded;
  if (this->rtc_.load(&loaded)) {
    this->publish_state_and_save(loaded);
  } else {
    this->publish_state_and_save(0.0f);
  }

#ifdef USE_API
  if (!this->service_.empty()) {
    this->register_service(&EnergyTariff::publish_state_and_save, this->service_, {"value"});
  }
#endif
}

void EnergyTariff::publish_state_and_save(float state) {
  ESP_LOGD(TAG, "'%s': Setting new state to %.2f", this->get_name().c_str(), state);
  this->publish_state(state);
  this->rtc_.save(&state);
}

std::string EnergyTariff::get_current_day() {
  time_t now = time(nullptr);
  struct tm *tm_info = localtime(&now);
  char day[10];
  strftime(day, sizeof(day), "%A", tm_info);
  return std::string(day);
}

void EnergyTariff::apply_tariff_based_on_time() {
  std::string current_day = this->get_current_day();

  if (this->day_tariffs_.find(current_day) != this->day_tariffs_.end()) {
    for (auto const &t : this->day_tariffs_[current_day]) {
      if (this->is_within_time_range(t.min, t.max)) {
        this->publish_state_and_save(t.rate);
        return;
      }
    }
  }

  // If no tariff matches, you might want to apply a default rate or take no action.
  this->publish_state_and_save(this->default_rate_);
}

bool EnergyTariff::is_within_time_range(int start, int end) {
  time_t now = time(nullptr);
  struct tm *tm_info = localtime(&now);
  int current_min = tm_info->tm_hour * 60 + tm_info->tm_min;

  return current_min >= start && current_min < end;
}

}  // namespace energy_tariffs
}  // namespace esphome
