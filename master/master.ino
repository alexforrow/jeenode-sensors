#include <JeeLib.h>

// Customize node information
#define NODE_ID 10
#define NODE_NAME "master"
#define NETWORK_ID 100
#define INTERVAL 30000

void setup () {
  // start serial port
  Serial.begin(57600);
  Serial.println("AF jeenode master node - Simple ascii data relay");
  Serial.print("Node: ");
  Serial.print(NODE_NAME);
  Serial.print(" - ");
  Serial.println(NODE_ID);
  Serial.flush();

  rf12_initialize(NODE_ID, RF12_868MHZ, NETWORK_ID);
}

void loop () {    
    if (rf12_recvDone() && rf12_crc == 0) {
        Serial.print("OK ");
        byte buffer[3];
        for (byte i = 0; i < rf12_len; ++i) {
            Serial.print((char)rf12_data[i]);
        }
            
        //delay(100); // otherwise led blinking isn't visible
        Serial.println();
    }
}
