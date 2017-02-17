#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "Wisdom"
#define WLAN_PASS       "asoccinnr3316"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "192.168.1.22"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "guest"
#define AIO_KEY         "owntracks"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Store the MQTT server, client ID, username, and password in flash memory.
const char MQTT_SERVER[] PROGMEM = AIO_SERVER;

// Set a unique MQTT client ID using the AIO key + the date and time the sketch
// was compiled (so this should be unique across multiple devices for a user,
// alternatively you can manually set this to a GUID or other random value).
const char MQTT_CLIENTID[] PROGMEM = AIO_KEY __DATE__ __TIME__;
const char MQTT_USERNAME[] PROGMEM = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);/****************************** Feeds ***************************************/

                                                       // Setup feeds for temperature & humidity
const char DOG_FOOD_FEED[] PROGMEM = "/dog-monitor/dog-food";
Adafruit_MQTT_Publish dog_food = Adafruit_MQTT_Publish(&mqtt, DOG_FOOD_FEED);

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 7 & 8

RF24 radio(4,5);

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

const int min_payload_size = 4;
const int max_payload_size = 32;
const int payload_size_increments_by = 1;
int next_payload_size = min_payload_size;

char receive_payload[max_payload_size+1]; 

void setup() {
   //
  // Print preamble
  //

  Serial.begin(115200);
  
  Serial.println(F("ESP8266 Dog Food Signal Receivifier"));
  Serial.println(); Serial.println();
  delay(10);
  Serial.print(F("Connecting to "));
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(F("."));
  }
  Serial.println();

  Serial.println(F("WiFi connected"));

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // connect to adafruit io
  connect();
  
  //
  // Setup and configure rf radio
  //

  radio.begin();

  // enable dynamic payloads
  radio.enableDynamicPayloads();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(5,15);
  radio.setChannel(0x76);

  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);

  radio.startListening();
  radio.printDetails();

  Serial.println("Radio details printed.");

}

void loop() {
  // ping adafruit io a few times to make sure we remain connected
  if (!mqtt.ping(3)) {
    // reconnect to adafruit io
    if (!mqtt.connected()) {
      Serial.println(F("Reconnecting to Adafruit IO"));
      connect();
    }
  }
  // if there is data ready
    while ( radio.available() )
    {

      // Fetch the payload, and see if this was the last one.
      uint8_t len = radio.getDynamicPayloadSize();
      
      // If a corrupt dynamic payload is received, it will be flushed
      if(!len){
        continue; 
      }
      
      radio.read( receive_payload, len );

      // Put a zero at the end for easy printing
      receive_payload[len] = 0;

      // Spew it
      Serial.print(F("Got response size="));
      Serial.print(len);
      Serial.print(F(" value="));
      Serial.println(receive_payload);

      // First, stop listening so we can talk
      radio.stopListening();

      // Send the final one back.
      radio.write( receive_payload, len );
      Serial.println(F("Sent response."));

      dog_food.publish(receive_payload);

      // Now, resume listening so we catch the next packets.
      radio.startListening();
    }
}

// connect to adafruit io via MQTT
void connect() {

  Serial.println(F("Connecting to Adafruit IO... "));

  int8_t ret;

  while ((ret = mqtt.connect()) != 0) {

    switch (ret) {
    case 1: Serial.println(F("Wrong protocol")); break;
    case 2: Serial.println(F("ID rejected")); break;
    case 3: Serial.println(F("Server unavail")); break;
    case 4: Serial.println(F("Bad user/pass")); break;
    case 5: Serial.println(F("Not authed")); break;
    case 6: Serial.println(F("Failed to subscribe")); break;
    default: Serial.println(F("Connection failed")); break;
    }

    if (ret >= 0)
      mqtt.disconnect();

    Serial.println(F("Retrying connection..."));
    delay(5000);

  }

  Serial.println(F("Adafruit IO Connected!"));

}
