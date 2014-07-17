#include <OneWire.h>
#include <DallasTemperature.h>
#include <JeeLib.h>
#include "DHT.h"

// Require for more efficient sleeping
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// Customize node information
#define NODE_ID 2
#define NODE_NAME "skeleton"
#define NETWORK_ID 100
#define INTERVAL 30000

// CONFIG - ONE WIRE
//#define ONE_WIRE_ENABLED
#define ONE_WIRE_BUS 5
#define ONE_WIRE_POWER A1

// CONFIG - DHT22
#define DHT_ENABLED
#define DHT_PIN 4
#define DHT_TYPE DHT22   // DHT 22  (AM2302)
#define DHT_TAG "dht"

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs) and tie in Dallas
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int numSensors;

// Setup DHT
DHT dht(DHT_PIN, DHT_TYPE);
boolean dhtEnabled;

void setup(void)
{
  // start serial port
  Serial.begin(57600);
  Serial.println("AF jeenode sensory node");
  Serial.print("Node: ");
  Serial.print(NODE_NAME);
  Serial.print(" - ");
  Serial.println(NODE_ID);
  Serial.flush();
  
  rf12_initialize(NODE_ID, RF12_868MHZ, NETWORK_ID);
  
  // init for One-wire
  pinMode(ONE_WIRE_POWER, OUTPUT); // set power pin for DS18B20 to output
  digitalWrite(ONE_WIRE_POWER, HIGH); // turn DS18B20 sensor on
  Sleepy::loseSomeTime(50); // Allow 10ms for the sensor to be ready

  sensors.begin();
  numSensors=sensors.getDeviceCount();
  
  Serial.print("One-wire sensors: ");
  Serial.println(numSensors);
  
  // init for DHT
  dht.begin();
  dhtEnabled = !isnan(dht.readHumidity());
  
  Serial.print("DHT enabled: ");
  Serial.println(dhtEnabled ? "Yes" : "No");
 
  // end inits
  
  // send HELO string
  char sendBuffer[32];
  // INIT
  sprintf(sendBuffer, "%s i %d %d %d", NODE_NAME, NODE_ID, numSensors, dhtEnabled);
  transmitString(sendBuffer);
  
  // sleep
  rf12_sleep(0);                          // Put the RFM12 to sleep
}

void loop(void)
{
  char sensorId[32];
  
  // wakeup
  rf12_sleep(-1);     //wake up RF module
  
  if (numSensors > 0) {
    pinMode(ONE_WIRE_POWER, OUTPUT); // set power pin for DS18B20 to output
    digitalWrite(ONE_WIRE_POWER, HIGH); // turn DS18B20 sensor on
  
    Sleepy::loseSomeTime(500); // Allow 10ms for the sensor to be ready
    
    sensors.requestTemperatures(); // Send the command to get temperatures
    for (int count=0;count < numSensors;count++) {
      sprintf(sensorId, "%d", count);
      transmitValue('t', sensorId, sensors.getTempCByIndex(count));
      delay(1000);
    }
    
    digitalWrite(ONE_WIRE_POWER, LOW); // turn DS18B20 sensor off
    pinMode(ONE_WIRE_POWER, INPUT); // set power pin for DS18B20 to input before sleeping, saves power
  }
  
  if (dhtEnabled) {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT");
    } else {
      transmitValue('t', "dht", temperature);
      transmitValue('h', "dht", humidity);
    }
  }
  
  // go to sleep
  rf12_sleep(0);     // sleep RF module
  
  Sleepy::loseSomeTime(INTERVAL);
}

void transmitValue(char cmd, char* sensorId, float value)
{
  char sendBuffer[32];
  int decimal = (value - (int)value) * 100;
  
  // build string
  // e.g. kitchen t dht 21.3 (kitchen, temperature, dht sensor, 21.3oC)
  sprintf(sendBuffer, "%s %c %s %0d.%d", NODE_NAME, cmd, sensorId, (int)value, decimal);
  transmitString(sendBuffer);
}

void transmitString(char* data)
{
  Serial.print("Transmitting: ");
  Serial.println(data);
  
  rf12_sendNow(0, data, strlen(data));
  
  Serial.flush();
  rf12_sendWait(2);
  delay(500);
}
