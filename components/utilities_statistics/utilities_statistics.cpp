// utilities_statistics.cpp

#include "utilities_statistics.h"
#include "nvs_flash.h"  // Include for NVS functions

UtilitiesComponent::UtilitiesComponent() {
    // Initialize data from NVS
    this->loadFromNVS();
}

void UtilitiesComponent::updateGasM3(float newGasM3) {
    this->data.gas_m3 = newGasM3;
    this->saveToNVS();
}

void UtilitiesComponent::updateGasMJ(float newGasMJ) {
    this->data.gas_mj = newGasMJ;
    this->saveToNVS();
}

void UtilitiesComponent::updateWater(float newWater) {
    this->data.water = newWater;
    this->saveToNVS();
}

void UtilitiesComponent::saveToNVS() {
    // Save all the utility data to NVS at once
    this->writeNVS("gas_m3", this->data.gas_m3);
    this->writeNVS("gas_mj", this->data.gas_mj);
    this->writeNVS("water", this->data.water);
}

void UtilitiesComponent::loadFromNVS() {
    // Load values from NVS, with default values set to 0
    this->data.gas_m3 = this->readNVS("gas_m3", 0.0f);
    this->data.gas_mj = this->readNVS("gas_mj", 0.0f);
    this->data.water = this->readNVS("water", 0.0f);
}

void UtilitiesComponent::writeNVS(const char* key, float value) {
    // Helper method to write a float value to NVS
    nvs_handle_t my_handle;
    nvs_open("storage", NVS_READWRITE, &my_handle);
    nvs_set_float(my_handle, key, value);
    nvs_commit(my_handle);
    nvs_close(my_handle);
}

float UtilitiesComponent::readNVS(const char* key, float default_value) {
    // Helper method to read a float value from NVS
    nvs_handle_t my_handle;
    nvs_open("storage", NVS_READWRITE, &my_handle);
    float value = default_value;
    nvs_get_float(my_handle, key, &value);
    nvs_close(my_handle);
    return value;
}
