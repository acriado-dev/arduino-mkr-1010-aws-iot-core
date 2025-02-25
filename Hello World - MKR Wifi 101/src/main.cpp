/*
  AWS IoT WiFi

  This sketch securely connects to an AWS IoT using MQTT over WiFi.
  It uses a private key stored in the ATECC508A and a public
  certificate for SSL/TLS authetication.

  It publishes a message every 5 seconds to arduino/outgoing
  topic and subscribes to messages on the arduino/incoming
  topic.

  The circuit:
  - Arduino MKR WiFi 1010 or MKR1000

*/
#include <Arduino.h>
#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h> // change to #include <WiFi101.h> for MKR1000

#include "arduino_secrets.h"

/////// Enter your sensitive data in arduino_secrets.h
const char ssid[]        = SECRET_SSID;
const char pass[]        = SECRET_PASS;
const char broker[]      = SECRET_BROKER;
const char* certificate  = SECRET_CERTIFICATE;

WiFiClient    wifiClient;            // Used for the TCP socket connection
BearSSLClient sslClient(wifiClient); // Used for SSL/TLS connection, integrates with ECC508
MqttClient    mqttClient(sslClient);

unsigned long lastMillis = 0;

unsigned long getTime() {
  // get the current time from the WiFi module  
  return WiFi.getTime();
}



void connectWiFi() {
  Serial.print("Attempting to connect to the SSID: ");
  Serial.print(ssid);
  Serial.print(" ");

bool connected = false;
  int retryCount = 0;
  const int maxRetries = 10; 


while (!connected && retryCount< maxRetries) {
  if (WiFi.begin(ssid, pass) == WL_CONNECTED) {
    connected = true;
  } else {
        // failed, retry
    Serial.print(".");
    delay(1000);
    retryCount++;
  }
}
  
  if (connected) {
    Serial.println();
    Serial.println("You're connected to the network");
    Serial.println();
  } else {
    Serial.println();
    Serial.println("Failed to connect to the network after multiple attempts.");
  }


}

void connectMQTT() {
  Serial.print("Attempting to MQTT broker: ");
  Serial.print(broker);
  Serial.println(" ");

  while (!mqttClient.connect(broker, 8883)) {
    // failed, retry
    Serial.print(".");
    delay(3000);
  }
  Serial.println();

  Serial.println("You're connected to the MQTT broker");
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe("arduino/incoming");
}

void publishMessage() {
  Serial.println("Publishing message");

  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("arduino/outgoing");
  mqttClient.print("{\"deviceModel\": \"MKR 1010\" ");
  mqttClient.print(",\"temperature\": "  );
  mqttClient.print(random(15, 30));
  mqttClient.print(",\"humidity\": ");
  mqttClient.print(random(1, 500));
  mqttClient.print(",\"vehicleId\": \"MKR1010-1");
  mqttClient.print("\"}");
  mqttClient.endMessage();
}



void onMessageReceived(int messageSize) {
  // we received a message, print out the topic and contents
  Serial.print("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    Serial.print((char)mqttClient.read());
  }
  Serial.println();

  Serial.println();
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    while (1);
  }

  // Set a callback to get the current time
  // used to validate the servers certificate
  ArduinoBearSSL.onGetTime(getTime);

  // Set the ECCX08 slot to use for the private key
  // and the accompanying public certificate for it
  sslClient.setEccSlot(0, certificate);

  // Optional, set the client id used for MQTT,
  // each device that is connected to the broker
  // must have a unique client id. The MQTTClient will generate
  // a client id for you based on the millis() value if not set
  //
  // mqttClient.setId("clientId");
       
  // Set the message callback, this function is
  // called when the MQTTClient receives a message
  mqttClient.onMessage(onMessageReceived);
}

void loop() {

  

  if (WiFi.status() != WL_CONNECTED) {
    int numNetworks = WiFi.scanNetworks();
    Serial.println("Discovered " + String(numNetworks) + " Networks");
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    // MQTT client is disconnected, connect
    connectMQTT();
  }

  // poll for new MQTT messages and send keep alives
  mqttClient.poll();

  // publish a message roughly every 1 seconds.
  if (millis() - lastMillis > 1000) {
    lastMillis = millis();

    publishMessage();
  }
}

