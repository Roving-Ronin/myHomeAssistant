#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/sensor/sensor.h"
#include "energy_tariffs.h"

namespace esphome {
namespace energy_tariffs {

using sensor::Sensor;

// TODO use Parented<EnergyStatistics>

class TariffChangeTrigger : public Trigger<Sensor *> {
 public:
  explicit TariffChangeTrigger(EnergyTariffs *parent) {
    parent->add_on_tariff_callback([this](Sensor *value) { 
      if (this->is_relevant_day()) {  // Check if the current day is relevant
        this->trigger(value); 
      }
    });
  }

 protected:
  // Method to check if the tariff change is relevant for the current day
  bool is_relevant_day() {
    // Retrieve the current day of the week from the EnergyTariffs component
    auto current_day = this->parent_->get_current_day();
    // Logic to determine if the current day's tariff should trigger this
    // (You would need to implement this logic based on how tariffs are configured)
    return true;  // Placeholder, implement actual day-check logic here
  }
};

class BeforeTariffChangeTrigger : public Trigger<> {
 public:
  explicit BeforeTariffChangeTrigger(EnergyTariffs *parent) {
    parent->add_on_before_tariff_callback([this] { 
      if (this->is_relevant_day()) {  // Check if the current day is relevant
        this->trigger(); 
      }
    });
  }

 protected:
  // Method to check if the tariff change is relevant for the current day
  bool is_relevant_day() {
    // Retrieve the current day of the week from the EnergyTariffs component
    auto current_day = this->parent_->get_current_day();
    // Logic to determine if the current day's tariff should trigger this
    // (You would need to implement this logic based on how tariffs are configured)
    return true;  // Placeholder, implement actual day-check logic here
  }
};

}  // namespace energy_tariffs
}  // namespace esphome
