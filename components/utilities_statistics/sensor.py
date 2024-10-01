# sensor.py

from homeassistant.components.sensor import SensorEntity

class UtilitiesSensor(SensorEntity):
    """Representation of a sensor for the Utilities Component."""

    def __init__(self, utilities_component, sensor_type):
        """Initialize the sensor."""
        self._utilities_component = utilities_component
        self._sensor_type = sensor_type
        self._state = None

    @property
    def name(self):
        """Return the name of the sensor."""
        if self._sensor_type == "gas_m3":
            return "Gas Usage (m3)"
        elif self._sensor_type == "gas_mj":
            return "Gas Usage (MJ)"
        elif self._sensor_type == "water":
            return "Water Usage"
    
    @property
    def state(self):
        """Return the state of the sensor."""
        if self._sensor_type == "gas_m3":
            return self._utilities_component.data.gas_m3
        elif self._sensor_type == "gas_mj":
            return self._utilities_component.data.gas_mj
        elif self._sensor_type == "water":
            return self._utilities_component.data.water

    def update(self):
        """Fetch new state data for the sensor."""
        self._utilities_component.loadFromNVS()  # Load latest values from NVS
