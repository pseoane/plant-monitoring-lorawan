# Plant monitoring system
Plant monitoring system for B-L072Z-LRWAN1 ARM platform

## System requirements
- The system should measure
  - Temperature
  - Soil moisture
  - Humidity
  - Light
  - Dominant color
  - Location
  - Current date and time
  - Acceleration in X and Z axis
- All aforementioned values should be sent to a LoRaWan Gateway, in order to be visualized. 

# Sensors used
- **GPS**: Mbed adafruit TCS34725
- **Temperature and humidity sensor**: Adafruit Si7021 
- **Soil moisture sensor**
- **HW5P-1 Photo Transistor Light sensor**
- **RGB color sensor**: Adafruit TCS34725
- **Accelerometer**: MMA8451Q
