#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>


#define interval_ms 20
#define duration_ms 20000  // 20 seconds
#define SAMP_SIZE 4
#define RISE_THRESHOLD 4
#define RECORDING_TIME 10000  // Time in milliseconds (10 seconds)
// Access Point credentials
const char* apSSID = "ESP32_Setup";
const char* apPassword = "12345678";

// Global Variables
String wifiSSID = "";
String wifiPassword = "";
String patientId = "";
String serverUrl = "http://192.168.163.162:3000"; // Replace with your backend URL
bool isRecordingHR = false;
bool isRecordingTemp = false;

int sensorPin = 32;
unsigned long startTime = 0;    // Start time for measurement
const unsigned long duration = 10000; // 10 seconds in milliseconds
const int calibrationTime = 5000; // Time to calibrate the baseline (3 seconds)
int baseline = 0;               // Baseline value
int threshold = 0;  
int upwardFluctuations = 0;     // Count of upward fluctuations
bool isCalibrated = false;      // Flag to indicate calibration completion

// Temperature Sensor
#define ONE_WIRE_BUS 33
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float lastTemperature = 0.0;

// Heart Rate Sensor Pin
const int heartRateSensorPin = 32;

// Server
AsyncWebServer server(80);
bool credentialsReceived = false;

void setup() {
  Serial.begin(115200);

  // Start Access Point
  WiFi.softAP(apSSID, apPassword);
  Serial.println("Access Point started");
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Initialize temperature sensor
  sensors.begin();

  // Serve Captive Portal
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html",
    "<!DOCTYPE html>"
    "<html>"
    "<head><title>ESP32 Setup</title></head>"
    "<body>"
    "<h1>Enter Wi-Fi and Patient Info</h1>"
    "<form action=\"/setup\" method=\"POST\">"
    "Wi-Fi SSID: <input type=\"text\" name=\"ssid\"><br>"
    "Password: <input type=\"password\" name=\"password\"><br>"
    "Patient ID: <input type=\"text\" name=\"patientId\"><br>"
    "Server URL: <input type=\"text\" name=\"serverUrl\" value=\"http://192.168.163.162:3000\"><br>"
    "<input type=\"submit\" value=\"Submit\">"
    "</form>"
    "</body>"
    "</html>");
  });

  // Handle Form Submission
  server.on("/setup", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("ssid", true)) {
      wifiSSID = request->getParam("ssid", true)->value();
    }
    if (request->hasParam("password", true)) {
      wifiPassword = request->getParam("password", true)->value();
    }
    if (request->hasParam("patientId", true)) {
      patientId = request->getParam("patientId", true)->value();
    }
    if (request->hasParam("serverUrl", true)) {
      serverUrl = request->getParam("serverUrl", true)->value();
    }

    credentialsReceived = true;

    request->send(200, "text/html",
    "<!DOCTYPE html>"
    "<html>"
    "<head><title>Setup Complete</title></head>"
    "<body>"
    "<h1>Setup Complete!</h1>"
    "<p>Connecting to Wi-Fi...</p>"
    "</body>"
    "</html>");
  });

  // Start the server
  server.begin();
}

void loop() {
  if (credentialsReceived) {
    credentialsReceived = false;

    // Connect to Wi-Fi
    connectToWiFi();

    if (WiFi.status() == WL_CONNECTED) {
      while (true) {
        pollRecordingStatusHR();
        pollRecordingStatusTemp();

        if (isRecordingTemp) {
          // Read temperature
          float temperature = getTemperature();
          if (temperature != DEVICE_DISCONNECTED_C) {
            sendTemperature(temperature);
          }

          delay(30000); // Send data every 30 seconds
        }
        if (isRecordingHR) {
          // Read heart rate
          int heartRate = readHeartRate();
          if (heartRate > 0) {
            sendHeartRate(heartRate);
          }
        }

        delay(5000); // Poll the backend every 5 seconds
      }
    } else {
      Serial.println("Failed to connect to Wi-Fi. Restarting AP...");
      WiFi.softAP(apSSID, apPassword); // Restart Access Point
    }
  }
}

void connectToWiFi() {
  WiFi.softAPdisconnect(true); // Turn off AP mode
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  Serial.println("Connecting to Wi-Fi...");
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Wi-Fi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWi-Fi connection failed.");
  }
}

void pollRecordingStatusHR() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(serverUrl) + "/api/heartrate/recordingStatusESP";
    http.begin(url);
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["patientId"] = patientId;
    String jsonData;
    serializeJson(jsonDoc, jsonData);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonData);
    if (httpResponseCode > 0) {
      String response = http.getString();

      // Parse JSON response
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, response);
      if (!error) {
        isRecordingHR = doc["isRecordingHR"];
        Serial.println(isRecordingHR ? "Recording is ON" : "Recording is OFF");
      } else {
        Serial.println("Failed to parse JSON response");
      }
    } else {
      Serial.print("Error checking recording status: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Wi-Fi not connected. Attempting to reconnect...");
    connectToWiFi();
  }
}

void pollRecordingStatusTemp() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(serverUrl) + "/api/temperature/recordingStatusESP";
    http.begin(url);
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["patientId"] = patientId;
    String jsonData;
    serializeJson(jsonDoc, jsonData);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonData);
    if (httpResponseCode > 0) {
      String response = http.getString();

      // Parse JSON response
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, response);
      if (!error) {
        isRecordingTemp = doc["isRecordingTemp"];
        Serial.println(isRecordingTemp ? "Recording is ON" : "Recording is OFF");
      } else {
        Serial.println("Failed to parse JSON response");
      }
    } else {
      Serial.print("Error checking recording status: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Wi-Fi not connected. Attempting to reconnect...");
    connectToWiFi();
  }
}

float getTemperature() {
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  if (temperature == DEVICE_DISCONNECTED_C) {
    Serial.println("Failed to read from DS18B20 sensor!");
  } else {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
  }
  return temperature;
}

void sendTemperature(float temperature) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(serverUrl) + "/api/temperature/temperature";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // Construct JSON data
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["temperature"] = temperature;
    jsonDoc["patientId"] = patientId;
    String jsonData;
    serializeJson(jsonDoc, jsonData);

    int httpResponseCode = http.POST(jsonData);
    if (httpResponseCode > 0) {
      Serial.println("Temperature data sent successfully!");
      Serial.println("Response: " + http.getString());
    } else {
      Serial.print("Error sending temperature data: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

int readHeartRate() {
  // add heart rate reading
  if (!isCalibrated) {
      calibrateBaseline(); // Determine the baseline value
    }
  unsigned long startTime = millis(); // Record the start time
  unsigned long duration = 10000;    // 10 seconds in milliseconds
  upwardFluctuations = 0;
  while (millis() - startTime < duration) {
    // Read the current sensor value
  int currentValue = analogRead(sensorPin);

  // Normalize the value (optional) to reduce raw data range
  currentValue = map(currentValue, 0, 4095, 0, 255);
  // Serial.println(currentValue);
  // Check if the current value is greater than the baseline
  if (currentValue - baseline>=threshold) {
    upwardFluctuations++;
    Serial.print("Upward fluctuation detected! Current count: ");
    Serial.println(upwardFluctuations);
    delay(100); // Small delay to prevent duplicate counts from noise
  }


    delay(10); // Short delay for stability
  }
  return upwardFluctuations;
}

void sendHeartRate(int heartRate) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(serverUrl) + "/api/heartrate/heartRate";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // Construct JSON data
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["heartRate"] = heartRate;
    jsonDoc["patientId"] = patientId;
    String jsonData;
    serializeJson(jsonDoc, jsonData);

    int httpResponseCode = http.POST(jsonData);
    if (httpResponseCode > 0) {
      Serial.println("Heart rate data sent successfully!");
      Serial.println("Response: " + http.getString());
    } else {
      Serial.print("Error sending heart rate data: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

void calibrateBaseline() {
  static int total = 0;
  static int count = 0;
  static int minValue = 255;  // Minimum value observed during calibration
  static int maxValue = 0;    // Maximum value observed during calibration
  unsigned long startTime = millis(); // Record the start time
  unsigned long duration = 5000;    // 10 seconds in milliseconds
  while (millis() - startTime < duration) {
    int value = analogRead(sensorPin);
    value = map(value, 0, 4095, 0, 255); // Normalize for simpler range
    total += value;
    count++;
    if (value < minValue) minValue = value;
    if (value > maxValue) maxValue = value;
  }
  // Accumulate readings for calibration time
  
    baseline = total / count;

    // Determine threshold as a fraction of the peak-to-peak range
    int peakToPeak = maxValue - minValue;
    threshold = peakToPeak / 6; // Adjust this factor for sensitivity

    isCalibrated = true;
    startTime = millis(); // Start the 10-second measurement

    Serial.print("Baseline value determined: ");
    Serial.println(baseline);
    Serial.print("Peak-to-peak range: ");
    Serial.println(peakToPeak);
    Serial.print("Adaptive threshold set to: ");
    Serial.println(threshold);
    Serial.println("Starting fluctuation measurement...");
  
}
