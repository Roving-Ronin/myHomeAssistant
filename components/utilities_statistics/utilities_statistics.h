// utilities_statistics.h

#ifndef UTILITIES_STATISTICS_H
#define UTILITIES_STATISTICS_H

struct UtilitiesData {
    float gas_m3;  // Gas in cubic meters
    float gas_mj;  // Gas in Megajoules
    float water;   // Water consumption in liters or cubic meters
};

class UtilitiesComponent {
public:
    UtilitiesComponent();

    void updateGasM3(float newGasM3);
    void updateGasMJ(float newGasMJ);
    void updateWater(float newWater);

    void saveToNVS();  // Save to non-volatile storage
    void loadFromNVS(); // Load values from NVS during initialization

private:
    UtilitiesData data;

    void writeNVS(const char* key, float value);  // Helper function to write to NVS
    float readNVS(const char* key, float default_value);  // Helper function to read from NVS
};

#endif // UTILITIES_STATISTICS_H
