#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include <map>
#include <vector>
#include <string>

#ifdef USE_API
#include "esphome/components/api/custom_api_device.h"
#endif

namespace esphome {
namespace energy_tariffs {

using sensor::Sensor;

// Time range.
// In this customization, we assume that time is in minutes counted from the start of the day,
// calculated by the formula: hour * 60 + minute.
struct time_range_t {
  uint16_t min;
  uint16_t max;
  float rate;  // Added rate field for tariffs
};

class EnergyTariff : public Sensor, public Component
#ifdef USE_API
    , public api::CustomAPIDevice
#endif
{
 public:
  void dump_config() override;
  void setup() override;

  void add_time(uint16_t min, uint16_t max, float rate) {
    this->time_.push_back({
        .min = min,
        .max = max,
        .rate = rate,
    });
  }

  void set_service(const std::string &service) { this->service_ = service; }

  bool is_default() const { return this->time_.empty(); }

  bool time_in_range(const ESPTime &time) const {
    for (auto const &range : this->time_) {
      if (time_in_range_(range.min, range.max, time)) {
        return true;
      }
    }
    return false;
  }

  void add(float x) { this->publish_state_and_save(x + this->state); }

  void publish_state_and_save(float state);

 protected:
  ESPPreferenceObject rtc_;
  std::vector<time_range_t> time_;
  std::string service_;

  // Map to hold tariffs for each day of the week
  std::map<std::string, std::vector<time_range_t>> day_tariffs_;

  // Return true if time is in the range [min, max]
  static bool time_in_range_(uint16_t min, uint16_t max, const ESPTime &time) {
    auto x = time.hour * 60 + time.minute;
    return time_in_range_(min, max, x);
  }

  // Return true if x is in the range [min, max].
  static bool time_in_range_(uint16_t min, uint16_t max, uint16_t x) {
    if (min <= max) {
      return min <= x && x < max;
    }
    return min <= x || x < max;
  }

  // Method to get the current day of the week
  std::string get_current_day();

  // Method to apply the correct tariff based on the current time and day
  void apply_tariff_based_on_time();

  // Helper function to check if the current time is within a specific range
  bool is_within_time_range(int start, int end);
};

}  // namespace energy_tariffs
}  // namespace esphome
