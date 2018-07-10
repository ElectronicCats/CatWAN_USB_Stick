#include <SPI.h>
#include <LoRa.h>
bool flag_rx=0;
String inputString = "";         
boolean flagSerial = false;  


void ISR_RX(){
  flag_rx=1;
  }


void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') {
      flagSerial = true;
    }
  }
}


void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  LoRa.setPins(10, 9, 2);
  
  //Serial.println("LoRa Receiver");

  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  attachInterrupt(digitalPinToInterrupt(2), ISR_RX, RISING);
  inputString.reserve(200);
}


void loop() {
  //if(flag_rx){
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      // received a packet
      Serial.print("Received packet '");
      while (LoRa.available()) {
        Serial.print((char)LoRa.read());
      }

      Serial.print("' with RSSI ");
      Serial.println(LoRa.packetRssi());
    }
   flag_rx=0;
  //}
  
  if(flagSerial){
  Serial.println(inputString);
  if(inputString.startsWith("1")){
      char inputChar[inputString.length()];
      inputString.toCharArray(inputChar,inputString.length());  
      Serial.print("Frecuencia= ");
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
    flagSerial=0;
    //
  }

