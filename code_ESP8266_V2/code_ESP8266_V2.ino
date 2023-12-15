#include <ArduinoWiFiServer.h>
#include <BearSSLHelpers.h>
#include <CertStoreBearSSL.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiGratuitous.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiServer.h>
#include <WiFiServerSecure.h>
#include <WiFiServerSecureBearSSL.h>
#include <WiFiUdp.h>

#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>


#define WIFI_ENABLE 1

String serverName = "http://172.20.10.7:3000/irrigation/";


const char* ssid = "Vân Vân";
const char* password = "hongnhung";
unsigned long last;
unsigned long last2;

float humidity, temperature, fahrenheit, soil_moisture;
boolean pump_status = true;  // FALSE = OFF as default
boolean pump_control = false;
int set_mode = 0; // automatic (0); manual (1)

StaticJsonBuffer<1024> doc;

void setup() {
  // Initialize Serial port
  Serial.begin(115200);

  // khởi tạo giá trị biến time là giá trị hiện tại của hàm millis();
  last = millis();
  last2 = millis();
  //WIFI setup here
  WiFi.begin(ssid, password);
}

static int parseData(JsonObject& data) {
  if (data == JsonObject::invalid()) {
    return -1;
  }

  humidity = data["humidity"];
  temperature = data["temperature"];
  fahrenheit = data["fahrenheit"];
  soil_moisture = data["soil_moisture"];
  pump_status = data["pump_status"];
  return 0;
}
void httpGET() {
  WiFiClient client;
  HTTPClient http;

  http.begin(client, serverName.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
   Serial.println(httpResponseCode);
    String payload = http.getString();
    //Serial.println(payload);

    if (httpResponseCode == 200) {
      // String payload = http.getString();
    //Serial.println(payload);
      JsonObject& data = doc.parseObject(payload);
      doc.clear();
      if (data == JsonObject::invalid()) {
        // data.prettyPrintTo(Serial); // send to arduino
        return;
      }

      data["data"].prettyPrintTo(Serial);
      // Check setMode 
      set_mode = data["data"]["setMode"];
      pump_control = data["data"]["setPump"];

      if(set_mode == 0){ //Automatic
        Serial.print("MODE_AUTO\r\n"); //Send the pump mode to arduino
      }else if(set_mode == 1 && pump_control == true && pump_control != pump_status){ //Manual and on
        Serial.print("RELAY_ON\r\n");
      }else if(set_mode == 1 && pump_control == false && pump_control != pump_status){ // manual and off
        Serial.print("RELAY_OFF\r\n");
      }
    }

  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}
void httpPOST() {

  WiFiClient client;
  HTTPClient http;

  http.begin(client, serverName.c_str());
  // Specify content-type header
  http.addHeader("Content-Type", "application/json");
  // Data to send with HTTP POST
  String httpRequestData = "{\"humidity\":" + String(humidity)
                           + ", \"temperature\":" + String(temperature)
                           + ", \"fahrenheit\":" + String(fahrenheit)
                           + ", \"soilHumidity\":" + String(soil_moisture)
                           + ", \"pumpStatus\":" + (pump_status ? "true" : "false") + "}";

  // Send HTTP POST request
  int httpResponseCode = http.POST(httpRequestData);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}



void loop() {

  if ((millis() - last) > 500) {
    StaticJsonBuffer<1000> jsonBuffer;
    if (Serial.available()) {
      JsonObject& data = jsonBuffer.parseObject(Serial);

      if (data == JsonObject::invalid()) {
        // Serial.println("Invalid Json Object");
        jsonBuffer.clear();
        return;
      }

      Serial.println("JSON Object Recieved");
      data.prettyPrintTo(Serial);

      // Parse data from Uno

      if (parseData(data) != 0) {
        Serial.print("Invalid Json data!\r\n");
      }
    }

    last = millis();
  }

  if ((millis() - last2) > 500) {
    if (WiFi.status() == WL_CONNECTED) {
      httpGET();
      httpPOST();
    } else {
      Serial.println("WiFi Disconnected");
    }
    last2 = millis();
  }
}
