/**
 * Example of ABP device
 * Authors: 
 *        Ivan Moreno
 *        Eduardo Contreras
 *  June 2019
 * 
 * This code is beerware; if you see me (or any other collaborator 
 * member) at the local, and you've found our code helpful, 
 * please buy us a round!
 * Distributed as-is; no warranty is given.
 */
#include <lorawan.h>

//ABP Credentials 
const char *devAddr = "00000000";  //Here your Device Address
const char *nwkSKey = "00000000000000000000000000000000";  // Here your Network Session Key
const char *appSKey = "00000000000000000000000000000000";  //Here your App Session Key

const unsigned long interval = 10000;    // 10 s interval to send message
unsigned long previousMillis = 0;  // will store last time message sent
unsigned int counter = 0;     // message counter

char myStr[50];  //Array to send to the server 
char outStr[255];  //Array for incoming data from the server
int recvStatus = 0;  //Data read from LoRa server

//Enabling pins for RAK module
const sRFM_pins RFM_pins = {
  .CS = SS,
  .RST = RFM_RST,
  .DIO0 = RFM_DIO0,
  .DIO1 = RFM_DIO1,
  .DIO2 = RFM_DIO2,
  .DIO5 = RFM_DIO5,
  };

void setup() {
  // Setup loraID access
  Serial.begin(115200);
  while(!Serial);
  if(!lora.init()){
   Serial.println("RFM95 not detected");
   delay(5000);
    return;
   }

  // Set LoRaWAN Class change CLASS_A or CLASS_C
  lora.setDeviceClass(CLASS_A);

  // Set Data Rate
  lora.setDataRate(SF8BW125);

  // set channel to random
  lora.setChannel(MULTI);

  // Put ABP Key and DevAddress here
  lora.setNwkSKey(nwkSKey);
  lora.setAppSKey(appSKey);
  lora.setDevAddr(devAddr);
}

void loop() {
  // Check interval overflow
  if(millis() - previousMillis > interval) {
    previousMillis = millis(); 

    sprintf(myStr, "Hola-%d", counter); 
    Serial.print("Sending: ");
    Serial.println(myStr);
    lora.sendUplink(myStr, strlen(myStr), 0, 1);  //Send an Uplink to the LoRa Server (printing out array, lenght of our array, unconfirmed, Port 1)
    counter++;
  }

  recvStatus = lora.readData(outStr);
  if(recvStatus) {
    Serial.println(outStr);  //If data received from the server, print it on the Serial Monitor
  }
  
  // Check Lora RX
  lora.update();
}
