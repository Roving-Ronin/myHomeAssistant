#include "esphome.h"

class WaterUsageSensor : public PollingComponent, public Sensor {
 public:
  Sensor *daily_sensor = new Sensor();
  Sensor *weekly_sensor = new Sensor();
  Sensor *monthly_sensor = new Sensor();
  Sensor *quarterly_sensor = new Sensor();
  Sensor *yearly_sensor = new Sensor();

  float total_water_usage = 0;
  float daily_usage = 0;
  float weekly_usage = 0;
  float monthly_usage = 0;
  float quarterly_usage = 0;
  float yearly_usage = 0;

  WaterUsageSensor() : PollingComponent(60000) { }  // Polling every 1 minute

  void setup() override {
    // Initialize the total water usage from the pulse_meter sensor
    total_water_usage = id(total_water_usage).state;
  }

  void update() override {
    // Update the total water usage from the pulse_meter sensor
    total_water_usage = id(total_water_usage).state;

    // Calculate the daily, weekly, monthly, quarterly, and yearly usage
    calculate_usage();

    // Publish the usage values
    daily_sensor->publish_state(daily_usage);
    weekly_sensor->publish_state(weekly_usage);
    monthly_sensor->publish_state(monthly_usage);
    quarterly_sensor->publish_state(quarterly_usage);
    yearly_sensor->publish_state(yearly_usage);
  }

  void calculate_usage() {
    // Reset usage at midnight
    if (is_midnight()) {
      daily_usage = 0;
    }

    // Reset usage at the start of the week
    if (is_start_of_week()) {
      weekly_usage = 0;
    }

    // Reset usage at the start of the month
    if (is_start_of_month()) {
      monthly_usage = 0;
    }

    // Reset usage at the start of the quarter
    if (is_start_of_quarter()) {
      quarterly_usage = 0;
    }

    // Reset usage at the start of the year
    if (is_start_of_year()) {
      yearly_usage = 0;
    }

    // Increment usage counters
    daily_usage += total_water_usage;
    weekly_usage += total_water_usage;
    monthly_usage += total_water_usage;
    quarterly_usage += total_water_usage;
    yearly_usage += total_water_usage;
  }

  bool is_midnight() {
    auto time = id(homeassistant_time).now();
    return time.hour == 0 && time.minute == 0;
  }

  bool is_start_of_week() {
    auto time = id(homeassistant_time).now();
    return time.weekday == 1 && time.hour == 0 && time.minute == 0;
  }

  bool is_start_of_month() {
    auto time = id(homeassistant_time).now();
    return time.day_of_month == 1 && time.hour == 0 && time.minute == 0;
  }

  bool is_start_of_quarter() {
    auto time = id(homeassistant_time).now();
    return (time.month == 1 || time.month == 4 || time.month == 7 || time.month == 10) && time.day_of_month == 1 && time.hour == 0 && time.minute == 0;
  }

  bool is_start_of_year() {
    auto time = id(homeassistant_time).now();
    return time.month == 1 && time.day_of_month == 1 && time.hour == 0 && time.minute == 0;
  }
};
