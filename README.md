# ESP32 Health Monitoring System

## Overview

This project is a health monitoring system based on the ESP32 microcontroller. It captures temperature and heart rate data from sensors and sends them to a backend server via HTTP. The system allows dynamic configuration through a captive portal, enabling users to input Wi-Fi credentials, a patient ID, and a server URL.

## Features

1. **Captive Portal**:

   - ESP32 hosts a Wi-Fi access point.
   - Provides a web interface for users to configure Wi-Fi credentials, patient ID, and server URL.

2. **Heart Rate Monitoring**:

   - Uses an analog sensor for heart rate detection.
   - Baseline calibration to adapt to varying environmental and individual conditions.
   - Counts upward fluctuations in signal as an indicator of heart rate.

3. **Temperature Monitoring**:

   - Utilizes a DS18B20 temperature sensor.
   - Periodically sends temperature data to the server.

4. **Data Transmission**:

   - Sends heart rate and temperature data to a specified backend server via HTTP POST requests.
   - Supports dynamic recording toggling based on backend responses.

5. **Wi-Fi Connectivity**:
   - Dynamically connects to Wi-Fi using user-provided credentials.
   - Reconnects automatically if the connection is lost.

## Components Used

1. **ESP32 Microcontroller**: For Wi-Fi connectivity and sensor data processing.
2. **DS18B20 Temperature Sensor**: For capturing body temperature.
3. **Analog Heart Rate Sensor**: For heart rate measurements.
4. **Backend Server**: Receives and stores temperature and heart rate data (replace `serverUrl` with your backend URL).

## Circuit Connections

1. **Temperature Sensor**:

   - Data pin connected to GPIO 33.
   - Pull-up resistor (4.7kΩ) between data and VCC pins.

2. **Heart Rate Sensor**:
   - Signal pin connected to GPIO 32.

## Libraries Used

1. **WiFi.h**: For Wi-Fi management.
2. **ESPAsyncWebServer.h**: For hosting the captive portal.
3. **HTTPClient.h**: For HTTP communication with the backend.
4. **ArduinoJson.h**: For handling JSON data.
5. **OneWire.h**: For interfacing with the DS18B20 temperature sensor.
6. **DallasTemperature.h**: High-level interface for the DS18B20 sensor.

## Workflow

### Setup Phase

1. The ESP32 starts as an Access Point (AP).
2. A web interface is provided for configuration:
   - Wi-Fi SSID and password.
   - Patient ID.
   - Backend server URL.
3. Credentials are saved globally, and the ESP32 attempts to connect to the provided Wi-Fi.

### Main Loop

1. **Wi-Fi Connection**:

   - If Wi-Fi credentials are valid, the ESP32 connects to the network.
   - If the connection fails, the device restarts the AP.

2. **Data Monitoring**:

   - Regularly polls the backend server to check if data recording for heart rate or temperature is active.
   - If active, retrieves and sends the respective sensor data.

3. **Heart Rate Measurement**:

   - Calibrates the baseline for environmental conditions.
   - Counts upward fluctuations over a 10-second period.
   - Sends the calculated heart rate to the server.

4. **Temperature Measurement**:
   - Reads temperature from the DS18B20 sensor.
   - Sends the temperature to the server every 30 seconds.

### Functions

#### Captive Portal

- Displays a web form for users to input configuration details.

#### Wi-Fi Management

- Establishes and maintains a Wi-Fi connection.

#### Sensor Handling

- **Temperature**:
  - Reads data from the DS18B20.
  - Sends temperature data to the backend server.
- **Heart Rate**:
  - Reads heart rate data using analog input.
  - Performs baseline calibration and fluctuation measurement.
  - Sends heart rate data to the backend server.

#### Backend Interaction

- Periodically checks the backend server for recording status.
- Sends sensor data to the backend when recording is active.

## Example Usage

1. Power up the ESP32.
2. Connect to the Wi-Fi network named `ESP32_Setup` using the password `12345678`.
3. Open a web browser and navigate to `192.168.4.1`.
4. Fill in the Wi-Fi credentials, patient ID, and server URL in the form and submit.
5. The ESP32 will connect to the specified Wi-Fi network.
6. Monitor heart rate and temperature data sent to the backend server.

## Customization

- Update the `serverUrl` variable with the URL of your backend server.
- Adjust calibration time (`calibrationTime`) or sampling intervals as needed.
- Modify thresholds for heart rate fluctuation detection (`threshold`).

## Potential Enhancements

1. Add support for other sensors or data points.
2. Implement secure communication (e.g., HTTPS).
3. Use a non-blocking architecture to allow simultaneous tasks.
4. Store configuration in non-volatile memory to persist across reboots.

## Troubleshooting

1. **Wi-Fi Connection Fails**:
   - Check SSID and password.
   - Ensure the backend server is reachable.
2. **Sensor Issues**:
   - Verify sensor connections and pins.
   - Check for pull-up resistor on the DS18B20 data line.
3. **Data Not Sent**:
   - Verify the backend URL.
   - Check network connectivity and power supply.
