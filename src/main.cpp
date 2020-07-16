/*
ESP8266 MQTT with OTA
This skech is in charge to verify and maintain the MQTT connection with the server

Ricardo Mena C.
https://wfx.ddns.net

License
**********************************************************************************
This program is free software; you can redistribute it
and/or modify it under the terms of the GNU General
Public License as published by the Free Software
Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public
License for more details.

You should have received a copy of the GNU General
Public License along with this program.
If not, see <http://www.gnu.org/licenses/>.

Licence can be viewed at
http://www.gnu.org/licenses/gpl-3.0.txt

Please maintain this license information along with authorship
and copyright notices in any redistribution of this code
**********************************************************************************
*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "configs.h" //MUST: Rename configs.h.default to configs.h and fill it with your personal data

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_PSSWD);

Adafruit_MQTT_Publish status = Adafruit_MQTT_Publish(&mqtt, TOPIC_N1);//Test topic

uint32_t previousMillis = 0;        // will store last time LED was updated
uint32_t prevMillis = 0;
#define LED 0

void blink(uint32_t timer, uint32_t interval);
void MQTT_connect();
void stats(uint32_t timer, uint32_t interval);

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  //Wifi settings
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PSSWD);

  //Wifi connection
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }//end if

  //Led indicador config
  pinMode(LED, OUTPUT);

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(HOSTNAME);

  // Authentication by OTA_PSSWD
  ArduinoOTA.setPassword(OTA_PSSWD);

  //Gamma functions for events
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  //Start OTA service
  ArduinoOTA.begin();

  //Wifi information
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}//end setup

void loop() {
  ArduinoOTA.handle();//Wifi service handler
  blink(millis(), 1000);//Blink LED every 1s
  stats(millis(), 10000);//Send analog reads every 10s to MQTT Server
  MQTT_connect();//Stablish connection with mqtt server
  if(WiFi.status() != WL_CONNECTED){//Continuously check wifi status
    ESP.restart();
  }//end if
}//end loop

void blink(uint32_t timer, uint32_t interval){
  if(timer - previousMillis > interval) {
    // save the last time you blinked the LED
    if(!digitalRead(LED)){
      digitalWrite(LED, HIGH);
    }else{
      digitalWrite(LED, LOW);
    }//end if
    previousMillis = timer;
  }//end if
}//end temporizer

void stats(uint32_t timer, uint32_t interval){
  if(timer - prevMillis > interval){
    Serial.println("Alive");
    //=====Temporized code=====
    float a0_read = analogRead(A0);//Read A0 pin
    if (! status.publish(a0_read)) {//Try to publish previous read
      Serial.println(F("Failed"));
    } else {
      Serial.println(F("OK!"));
    }//end if
    //=========================
    prevMillis = timer;
  }//end if
}//end temporizer

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()){
    return;
  }//end if

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      ESP.restart();//Restart ESP in case of MQTT doesn't response
    }//end if
  }//end while
  Serial.println("MQTT Connected!");
}//end MQTT_connect
