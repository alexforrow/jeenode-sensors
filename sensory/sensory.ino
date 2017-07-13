#include <OneWire.h>
#include <DallasTemperature.h>
#include <JeeLib.h>
#include "DHT.h"

// Require for more efficient sleeping
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// Customize node information
#define NODE_ID 3
#define NODE_NAME "my_first_sensor"
#define NETWORK_ID 100
#define INTERVAL_MIN 10
#define INFO_FREQUENCY 50

// CONFIG - ONE WIRE
//#define ONE_WIRE_ENABLED
#define ONE_WIRE_BUS 5
#define ONE_WIRE_POWER A1

// CONFIG - DHT22
#define DHT_ENABLED
#define DHT_PIN 4
#define DHT_POWER A0
#define DHT_TYPE DHT22   // DHT 22  (AM2302)
#define DHT_TAG "dht"

// CONFIG - Capacitive moisture sensor https://www.dfrobot.com/wiki/index.php/Capacitive_Soil_Moisture_Sensor_SKU:SEN0193
//#define MOISTURE_ENABLED
#define MOISTURE_PIN A2
#define MOISTURE_POWER 6

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs) and tie in Dallas
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int numSensors;

// Setup DHT
DHT dht(DHT_PIN, DHT_TYPE);
boolean dhtEnabled;
boolean moistureEnabled = false;

unsigned int sequence = 0;

void setup(void) {
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
  pinMode(DHT_POWER, OUTPUT); // set power pin for DHT to output
  digitalWrite(DHT_POWER, HIGH); // turn DHT sensor on
  dht.begin();
  delay(1000);
  dhtEnabled = !isnan(dht.readHumidity());
  
  Serial.print("DHT enabled: ");
  Serial.println(dhtEnabled ? "Yes" : "No");

  Serial.print("Moisture reading: ");
  #ifdef MOISTURE_ENABLED
  Serial.println("Enabled");
  moistureEnabled = true;
  #endif
  #ifndef MOISTURE_ENABLED
  Serial.println("Disabled");
  #endif
  
  // sleep
  rf12_sleep(0);                          // Put the RFM12 to sleep
}

void loop(void) {
  char sensorId[32];
  
  // wakeup
  rf12_sleep(-1);     //wake up RF module
  
  if (sequence % INFO_FREQUENCY == 0) {
      transmitInfo();
  }
  
  if (numSensors > 0) {
    pinMode(ONE_WIRE_POWER, OUTPUT); // set power pin for DS18B20 to output
    digitalWrite(ONE_WIRE_POWER, HIGH); // turn DS18B20 sensor on
  
    Sleepy::loseSomeTime(500); // Allow 10ms for the sensor to be ready
    
    sensors.requestTemperatures(); // Send the command to get temperatures
    for (int count=0;count < numSensors;count++) {
      sprintf(sensorId, "%d", count);
      transmitValue('t', sensorId, sensors.getTempCByIndex(count));
    }
    
    digitalWrite(ONE_WIRE_POWER, LOW); // turn DS18B20 sensor off
    pinMode(ONE_WIRE_POWER, INPUT); // set power pin for DS18B20 to input before sleeping, saves power
  }
  
  if (dhtEnabled) {
    pinMode(DHT_POWER, OUTPUT); // set power pin for DHT to output
    digitalWrite(DHT_POWER, HIGH); // turn DHT sensor on
    dht.begin();
    delay(1000);

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT");
    } else {
      transmitValue('t', "dht", temperature);
      transmitValue('h', "dht", humidity);
    }

    digitalWrite(DHT_POWER, LOW); // turn DHT sensor off
    pinMode(DHT_POWER, INPUT); // set power pin for DHT to input before sleeping, saves power
  }

  #ifdef MOISTURE_ENABLED
  pinMode(MOISTURE_POWER, OUTPUT);
  digitalWrite(MOISTURE_POWER, HIGH);

  delay(200);

  int moisture = analogRead(MOISTURE_PIN);
  transmitValue('m', "0", moisture);

  digitalWrite(MOISTURE_POWER, LOW);
  pinMode(MOISTURE_POWER, INPUT);
  #endif
  
  // go to sleep
  rf12_sleep(0);     // sleep RF module
  
  for (byte i = 0; i < INTERVAL_MIN; ++i)
    Sleepy::loseSomeTime(60000);
  
  sequence++;
}

void transmitValue(char cmd, char* sensorId, float value) {
  char buffer[32];

  // quick and nasty way of converting float to string
  int decimal = (value - (int)value) * 100;
  if (decimal < 0) {
    decimal = -decimal;
  }
  
  // build string
  // e.g. kitchen t dht 21.3 (kitchen, temperature, dht sensor, 21.3oC)
  sprintf(buffer, "%s %0d.%d", sensorId, (int)value, decimal);
  transmitString(cmd, buffer);
}

void transmitString(char cmd, char* data) {
  char sendBuffer[64];
  
  sprintf(sendBuffer, "%s %u %c %s", NODE_NAME, sequence, cmd, data);
  Serial.print("Transmitting: ");
  Serial.println(sendBuffer);
  
  rf12_sendNow(0, sendBuffer, strlen(sendBuffer));
  
  Serial.flush();
  rf12_sendWait(2);
  
  // this seems to be needed, otherwise messages get lost
  delay(500);
}

void transmitInfo() {
  char buffer[32];
  sprintf(buffer, "%d %d %d %d", NODE_ID, numSensors, dhtEnabled, moistureEnabled);
  transmitString('i', buffer);
}
