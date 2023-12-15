#include <DHT.h>

#include "HardwareSerial.h"
#include <ArduinoJson.h>
#include "string.h"

#define SoilSensor_pin A0

// #define SERIAL_RX_BUFFER_SIZE 1024
// #define SERIAL_TX_BUFFER_SIZE 1024

//Initialisation of DHT11 Sensor
#define DHTPIN 4
DHT dht(DHTPIN, DHT11);

#define SOIL_HUMIDITY_LOWER_THRESHOLD 30
#define SOIL_HUMIDITY_UPPER_THRESHOLD 60

enum {
  RELAY_ON,
  RELAY_OFF,
  MODE_AUTO,
  INVALID_CMD
};

int relay_pin = 7;

boolean pump_status = false;  // FALSE = OFF as default

float temp, hum, temp_f, soil_moisture;

unsigned long _timeout, _cmdInterval;

// char rxBuffer[1024];

uint8_t cmd;
uint16_t byte_received;

boolean manualPumpControl = false;  //default in automatic

void setup() {
  Serial.begin(115200);
  // khởi tạo giá trị biến time là giá trị hiện tại của hàm millis();
  _timeout = millis();
  _cmdInterval = millis();

  dht.begin();
  pinMode(SoilSensor_pin, INPUT);
  // set the digital pin as output:
  pinMode(relay_pin, OUTPUT);
  // sensors.begin(); // Initialize DS18B20 sensor
 
}

static uint8_t get_command(char *buffer) {
  if (buffer == NULL) {
    return INVALID_CMD;
  }
  if (strstr(buffer, "RELAY_ON") != NULL) {
    return RELAY_ON;
  } else if (strstr(buffer, "RELAY_OFF") != NULL) {
    return RELAY_OFF;
  } else if (strstr(buffer, "MODE_AUTO") != NULL) {
    return MODE_AUTO;
  }
  return INVALID_CMD;
}

void loop() {
  if ((unsigned long)(millis() - _timeout) > 1000) {
    //StaticJsonBuffer<1000> jsonBuffer;
    //JsonObject& data = jsonBuffer.createObject();

    //Obtain Temp and Hum data
    Read_DHT();
    Read_SoilSensor();

    //Assign collected data to JSON Object
    //data["humidity"] = hum;
    //data["temperature"] = temp;
    //data["fahrenheit"] = temp_f;
    //data["soil_moisture"] = soil_moisture;
    //data["pump_status"] = pump_status;

    String data = "{\"humidity\":" + String(hum)
                  + ", \"temperature\":" + String(temp)
                  + ", \"fahrenheit\":" + String(temp_f)
                  + ", \"soil_moisture\":" + String(soil_moisture)
                  + ", \"pump_status\":" + (pump_status ? "true" : "false") + "}";

    Serial.println(data);  //send sensor value to esp
    _timeout = millis();
  }

  if ((unsigned long)millis() - _cmdInterval > 1000)  // Check the received data every 1000ms
  {
    _cmdInterval = millis();
    if (Serial.available()) {
      // Read data from esp
      String receviedStr = Serial.readString();
      cmd = get_command(receviedStr.c_str());

      switch (cmd) {
        // If you receive RELAY on and off, it is always in manual mode 
        case RELAY_ON: //manual
            // Turn the relay ON here;
            digitalWrite(relay_pin, HIGH);
            manualPumpControl = true;
            pump_status = true;
      
          break;
        case RELAY_OFF: //manual
            // Turn the relay off here
            digitalWrite(relay_pin, LOW);
            manualPumpControl = true;
            pump_status = false;
          break;
        case MODE_AUTO:
          manualPumpControl = false;
          break;
        default:
          break;
      }
    }
  }
}

void Read_DHT() {

  hum = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temp = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  temp_f = dht.readTemperature(true);
  // // Check if any reads failed and exit early (to try again).
}

void Read_SoilSensor() {
  soil_moisture = map(analogRead(SoilSensor_pin), 411, 1021, 100, 0);
  if(soil_moisture < 0) soil_moisture = 0;
  if(soil_moisture > 100) soil_moisture = 100;
  // int soil_moisture = analogRead(SoilSensor_pin);
  Serial.println();
  Serial.print("Data: ");
  Serial.print(soil_moisture);
  Serial.println();
  if (manualPumpControl == false) {  //auto
     if (soil_moisture <= SOIL_HUMIDITY_LOWER_THRESHOLD) {
      digitalWrite(relay_pin, HIGH);
      pump_status = true;
    } else if (soil_moisture >= SOIL_HUMIDITY_UPPER_THRESHOLD) {
      digitalWrite(relay_pin, LOW);
      pump_status = false;
    } 
  }


}