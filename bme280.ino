// Retrieve and send to thingspeak temperature and humidity using BME280 sensor
// May 2019
// Trunk
// Author: Daniel Sánchez

#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>
#include <command.h>

// replace with your channel’s thingspeak API key,
String apiKey = "98M2B9CAJ9XRID6L";
const char* ssid = "11AA3BEFX";
const char* password = "DSRMCVCRFNCOVI";
const char* ssidAP = "ConfigureStation";
const char* passAP = "1234567890";

const char* apiServer = "api.thingspeak.com";

boolean result;
int att;

Adafruit_BME280 bme;
WiFiClient client;
WiFiServer server(666);

void setup() {

  Serial.begin(115200);
  delay(10);
  att = 0;

  Wire.begin(12,14);
  Wire.setClock(100000);
  if(!bme.begin(0x76)){
    Serial.println("Could not find a valid BME280");
  }
  
  if (ssid && password) {
    WiFi.begin(ssid, password);
    Serial.println();
    Serial.println();
    Serial.print("Trying to connect to ");
    Serial.println(ssid);
  } else {
    startSoftAP();
  }

  while (WiFi.status() != WL_CONNECTED && att < 10) {
    delay(10000);
    Serial.print("Intento: ");
    Serial.print(att);
    Serial.println(" ; ");
    att++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
  } else {
    startSoftAP();
  }
}

void loop() {
  readPostTempSensor();
}

void startSoftAP() {

  result = WiFi.softAP(ssidAP);

  if (!result) {
    ESP.reset();
  }
  else {
    server.begin();
    att = 0;
    while (att < 120) {
      client = server.available();
      if (client) {
        client.flush();
        Serial.println("We have a new client!");
        client.println("You are a new client!");
        delay(1000);
      }
      att++;
      delay(100);
    }
  }
}

void readPostTempSensor() {
  float h = bme.readHumidity();
  float t = bme.readTemperature();
  float p = bme.readPressure();
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from BME sensor!");
    return;
  }
  Serial.println("Trying to connect to API server");
  int result = client.connect(apiServer, 80);
  if (result == 1) { // "184.106.153.149" or api.thingspeak.com
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(t);
    postStr += "&field2=";
    postStr += String(h);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);

    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" degrees Celcius. Humidity: ");
    Serial.print(h);
    Serial.print(" %. Pressure: ");
    Serial.print(p);
    Serial.println(" hPa send to Thingspeak");
  }
  else if (result == -1) {
    Serial.println("TIMED OUT");
  }
  else if (result == -2) {
    Serial.println("INVALID SERVER");
  }
  else if (result == -3) {
    Serial.println("TRUNCATED");
  }
  else if (result == -4) {
    Serial.println("INVALID RESPONSE");
  }
  else {
    Serial.print("Something bad happened, connection result: ");
    Serial.print(result);
    Serial.print("\n");
  }

  delay(10000);
  
  client.stop();

  Serial.println("Waiting next reading");
  // thingspeak needs minimum 15 sec delay between updates
  delay(900000);
}
