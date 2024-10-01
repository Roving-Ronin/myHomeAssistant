#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace utility_statistics {

using sensor::Sensor;

class UtilityStatistics : public Component {
 public:
  float get_setup_priority() const override { return setup_priority::DATA; }
  void dump_config() override;
  void setup() override;
  void loop() override;

  // Reset statistics for gas_m3, gas_mj, and water
  void reset_gas_m3_statistics();
  void reset_gas_mj_statistics();
  void reset_water_statistics();

  // Setters for time and sensors
  void set_time(time::RealTimeClock *time) { this->time_ = time; }

  // Gas (m³) statistics setters
  void set_gas_m3_total(Sensor *sensor) { this->gas_m3_total_ = sensor; }
  void set_gas_m3_today(Sensor *sensor) { this->gas_m3_today_ = sensor; }
  void set_gas_m3_yesterday(Sensor *sensor) { this->gas_m3_yesterday_ = sensor; }
  void set_gas_m3_week(Sensor *sensor) { this->gas_m3_week_ = sensor; }
  void set_gas_m3_month(Sensor *sensor) { this->gas_m3_month_ = sensor; }
  void set_gas_m3_year(Sensor *sensor) { this->gas_m3_year_ = sensor; }

  // Gas MJ statistics setters
  void set_gas_mj_total(Sensor *sensor) { this->gas_mj_total_ = sensor; }
  void set_gas_mj_today(Sensor *sensor) { this->gas_mj_today_ = sensor; }
  void set_gas_mj_yesterday(Sensor *sensor) { this->gas_mj_yesterday_ = sensor; }
  void set_gas_mj_week(Sensor *sensor) { this->gas_mj_week_ = sensor; }
  void set_gas_mj_month(Sensor *sensor) { this->gas_mj_month_ = sensor; }
  void set_gas_mj_year(Sensor *sensor) { this->gas_mj_year_ = sensor; }

  // Water statistics setters
  void set_water_total(Sensor *sensor) { this->water_total_ = sensor; }
  void set_water_today(Sensor *sensor) { this->water_today_ = sensor; }
  void set_water_yesterday(Sensor *sensor) { this->water_yesterday_ = sensor; }
  void set_water_week(Sensor *sensor) { this->water_week_ = sensor; }
  void set_water_month(Sensor *sensor) { this->water_month_ = sensor; }
  void set_water_year(Sensor *sensor) { this->water_year_ = sensor; }

 protected:
  uint32_t save_interval_{300};  // Save every 5 minutes
  uint32_t last_save_time_{0};   // Timestamp of the last save
  uint32_t last_warning_time_{0}; // Timestamp of the last warning log
  bool data_changed{false};  // Flag to track if data has changed

  ESPPreferenceObject pref_;
  time::RealTimeClock *time_;

  // Gas m3 sensor inputs
  Sensor *gas_m3_total_{nullptr};

  // Gas MJ sensor inputs
  Sensor *gas_mj_total_{nullptr};

  // Water sensor inputs
  Sensor *water_total_{nullptr};

  // Gas m3 exposed sensors
  Sensor *gas_m3_today_{nullptr};
  Sensor *gas_m3_yesterday_{nullptr};
  Sensor *gas_m3_week_{nullptr};
  Sensor *gas_m3_month_{nullptr};
  Sensor *gas_m3_year_{nullptr};

  // Gas MJ exposed sensors
  Sensor *gas_mj_today_{nullptr};
  Sensor *gas_mj_yesterday_{nullptr};
  Sensor *gas_mj_week_{nullptr};
  Sensor *gas_mj_month_{nullptr};
  Sensor *gas_mj_year_{nullptr};

  // Water exposed sensors
  Sensor *water_today_{nullptr};
  Sensor *water_yesterday_{nullptr};
  Sensor *water_week_{nullptr};
  Sensor *water_month_{nullptr};
  Sensor *water_year_{nullptr};

  // Resetting state flag
  bool is_resetting_{false};
  // To prevent sensor updates
  bool prevent_sensor_update_{false};
  // Flag to wait for a valid sensor reading after reset
  bool waiting_for_sensor_read_{false};

  // start day of week configuration
  int gas_m3_week_start_day_{2};
  int gas_mj_week_start_day_{2};  // New line for gas MJ week start day
  int water_week_start_day_{2};   // New line for water week start day

  // start day of month configuration
  int gas_m3_month_start_day_{1};
  int gas_mj_month_start_day_{1};  // New line for gas MJ month start day
  int water_month_start_day_{1};   // New line for water month start day

  // start day of year configuration
  int gas_m3_year_start_day_{1};
  int gas_mj_year_start_day_{1};   // New line for gas MJ year start day
  int water_year_start_day_{1};    // New line for water year start day

  // Gas m3 statistics data
  struct gas_m3_data_t {
    uint16_t current_day_of_year{0};
    float start_today{NAN};
    float start_yesterday{NAN};
    float start_week{NAN};
    float start_month{NAN};
    float start_year{NAN};

    float gas_m3_today{NAN};
    float gas_m3_yesterday{NAN};
    float gas_m3_week{NAN};
    float gas_m3_month{NAN};
    float gas_m3_year{NAN};
  } gas_m3_;

  // Gas MJ statistics data
  struct gas_mj_data_t {
    uint16_t current_day_of_year{0};
    float start_today{NAN};
    float start_yesterday{NAN};
    float start_week{NAN};
    float start_month{NAN};
    float start_year{NAN};

    float gas_mj_today{NAN};
    float gas_mj_yesterday{NAN};
    float gas_mj_week{NAN};
    float gas_mj_month{NAN};
    float gas_mj_year{NAN};
  } gas_mj_;

  // Water statistics data
  struct water_data_t {
    uint16_t current_day_of_year{0};
    float start_today{NAN};
    float start_yesterday{NAN};
    float start_week{NAN};
    float start_month{NAN};
    float start_year{NAN};

    float water_today{NAN};
    float water_yesterday{NAN};
    float water_week{NAN};
    float water_month{NAN};
    float water_year{NAN};
  } water_;

  // Internal helper methods
  void process_gas_m3_(float total, const esphome::time::ESPTime &t);  // Updated
  void process_gas_mj_(const esphome::time::ESPTime &t);               // Updated
  void process_water_(float total, const esphome::time::ESPTime &t);    // Updated
  void save_();
};  // class UtilityStatistics

}  // namespace utility_statistics
}  // namespace esphome
