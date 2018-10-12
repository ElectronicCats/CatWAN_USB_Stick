#include <SPI.h>
#include <LoRa.h>
bool flag_rx=0;
String inputString = "";         
boolean flagSerial = false;  


void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  LoRa.setPins(17, 16, 4);
  
  Serial.println("LoRa Receiver");

  if (!LoRa.begin(904300000)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.onReceive(onReceive);
  LoRa.receive();
  inputString.reserve(200);
}


void loop() {
  
  if(flagSerial){
  Serial.println(inputString);
  if(inputString.startsWith("1")){
      char inputChar[inputString.length()];
      inputString.toCharArray(inputChar,inputString.length());  
      Serial.print("Frequency= ");
        for(int i=2;i<inputString.length();i++){
          Serial.print(inputChar[i]);
        } 
       Serial.println();
       char freq_char[100];
        int i1=0;
    
      for(int i=2;i<inputString.length()+1;i++){
        freq_char[i1]=inputChar[i];
        i1++;
        }
      uint32_t freq = atof(freq_char);
      LoRa.setFrequency(freq);
  }
  inputString = "";  
      }
   // flagSerial=0;
  }



void onReceive(int packetSize) {
  // received a packet
  Serial.print("Received packet '");

  // read packet
  for (int i = 0; i < packetSize; i++) {
    Serial.print((char)LoRa.read());
  }

  // print RSSI of packet
  Serial.print("' with RSSI ");
  Serial.println(LoRa.packetRssi());
}
 

