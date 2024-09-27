#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace gas_statistics_mj {

using sensor::Sensor;

class GasStatisticsMJ : public Component {
 public:
  float get_setup_priority() const override { return setup_priority::DATA; }
  void dump_config() override;
  void setup() override;
  void loop() override;

  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_total(Sensor *sensor) { this->total_ = sensor; }

  void set_gas_today(Sensor *sensor) { this->gas_today_ = sensor; }
  void set_gas_yesterday(Sensor *sensor) { this->gas_yesterday_ = sensor; }
  void set_gas_week(Sensor *sensor) { this->gas_week_ = sensor; }
  void set_gas_month(Sensor *sensor) { this->gas_month_ = sensor; }
  void set_gas_year(Sensor *sensor) { this->gas_year_ = sensor; }

 protected:
  ESPPreferenceObject pref_;
  time::RealTimeClock *time_;

  // input sensors
  Sensor *total_{nullptr};

  // exposed sensors
  Sensor *gas_today_{nullptr};
  Sensor *gas_yesterday_{nullptr};
  Sensor *gas_week_{nullptr};
  Sensor *gas_month_{nullptr};
  Sensor *gas_year_{nullptr};

  // start day of week configuration
  int gas_week_start_day_{2};
  // start day of month configuration
  int gas_month_start_day_{1};
  int gas_year_start_day_{1};

  // Structure for storing gas statistics in MJ
  struct gas_data_mj_t {
    uint16_t current_day_of_year{0};    // Track the current day of year
    float start_today{NAN};             // MJ usage at the start of today
    float start_yesterday{NAN};         // MJ usage at the start of yesterday
    float start_week{NAN};              // MJ usage at the start of the current week
    float start_month{NAN};             // MJ usage at the start of the current month
    float start_year{NAN};              // MJ usage at the start of the current year

    // Store MJ sensor values
    float gas_today{NAN};               // Stored MJ usage today
    float gas_yesterday{NAN};           // Stored MJ usage yesterday
    float gas_week{NAN};                // Stored MJ usage this week
    float gas_month{NAN};               // Stored MJ usage this month
    float gas_year{NAN};                // Stored MJ usage this year
  } gas_;

  void process_(float total);
  void save_();
};  // class GasStatisticsMJ

}  // namespace gas_statistics_mj
}  // namespace esphome
