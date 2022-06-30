/*
  Created by Eduardo Contreras @ Electronic Cats 2020

  PLEASE REFER TO THESE LIBRARIES:
    https://github.com/kroimon/Arduino-SerialCommand
    https://github.com/sandeepmistry/arduino-LoRa

  This example code works as a CLI to control your CatWAN USB-Stick
  As a LoRa Sniffer to catch any LoRa Packet
  
  This code is beerware; if you see me (or any other Electronic Cats
  member) at the local, and you've found our code helpful,
  please buy us a round!
  Distributed as-is; no warranty is given.
*/

#define SERIALCOMMAND_HARDWAREONLY
 
#include <SerialCommand.h>
#include <SPI.h>
#include <LoRa.h>

SerialCommand SCmd;

float fwVersion= 0.2;

float frequency = 915;
int spreadFactor = 7;
int bwReference = 7;
int channel = 0;
bool rx_status = false;

void setup(){  
  pinMode(LED_BUILTIN,OUTPUT);      // Configure the onboard LED for output
  digitalWrite(LED_BUILTIN,LOW);    // default to LED off
  pinMode(RFM_DIO5,INPUT);
  Serial.begin(115200); 
  while (!Serial);
  Serial.println("Welcome to the LoRa Sniffer CLI " + String(fwVersion,1) + "v\n");
  Serial.println("With this sketch you can scan the LoRa spectrum");
  Serial.println("Changing the Frequency, Spreading Factor, BandWidth or the IQ signals of the radio.");
  Serial.println("Type help to get the available commands.");
  Serial.println("Electronic Cats ® 2020");
  
  // Setup callbacks for SerialCommand commands 
  SCmd.addCommand("help",help); 
  SCmd.addCommand("set_rx",set_rx);
  SCmd.addCommand("set_tx",set_tx0);
  SCmd.addCommand("set_tx_hex",set_tx1);
  SCmd.addCommand("set_tx_ascii",set_tx2);
  SCmd.addCommand("set_freq",set_freq);
  SCmd.addCommand("set_sf",set_sf);
  SCmd.addCommand("set_bw",set_bw);
  SCmd.addCommand("set_chann",set_chann);

  SCmd.addCommand("get_config",get_config);
  SCmd.addCommand("get_freq",get_freq);
  SCmd.addCommand("get_sf",get_sf);  
  SCmd.addCommand("get_bw",get_bw);

  SCmd.setDefaultHandler(unrecognized);  // Handler for command that isn't matched  (says "What?") 

  LoRa.setPins(SS, RFM_RST, RFM_DIO0);

  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  //LoRa.setFrequency(915E6);
  //LoRa.setSpreadingFactor(spreadFactor);
  //LoRa.setSignalBandwidth(125E3);
  rx_status = false;

  LoRa.onReceive(onReceive);
}

void loop()
{  
  SCmd.readSerial();     // We don't do much, just process serial commands
}


void help(){
  Serial.println("Fw version: " + String(fwVersion,1)+"v");
  Serial.println("\tConfiguration commands:");
  Serial.println("\tset_rx");
  Serial.println("\tset_tx");
  Serial.println("\tset_tx_hex");
  Serial.println("\tset_tx_ascii");
  Serial.println("\tset_chann");
  Serial.println("\tset_freq");
  Serial.println("\tset_sf");
  Serial.println("\tset_bw");

  Serial.println("Monitor commands:");
  Serial.println("\tget_freq");
  Serial.println("\tget_sf");
  Serial.println("\tget_bw");
  Serial.println("\tget_config");

  Serial.println("..help");

}

/**********Set configuration**************/
void set_freq(){
  char *arg;  
  arg = SCmd.next();
  frequency = atof(arg);
  if (arg != NULL){
    if(frequency > 902 && frequency < 923){
      long freq = frequency*1000000;
      LoRa.setFrequency(freq);
      Serial.println("Frequency set to " + String(frequency) + " MHz");
      rx_status = false;
    }
    else{
      Serial.println("Error setting the frequency");
      Serial.println("Value must be between 902 MHz and 923 MHz");
    }
  } 
  else {
    Serial.println("No argument"); 
  }
}

void set_sf(){
  char *arg;  
  arg = SCmd.next();  
  if (arg != NULL){
    spreadFactor = atoi(arg);
    if(spreadFactor < 6 || spreadFactor > 12){
      Serial.println("Error setting the Spreading factor");
      Serial.println("Value must be between 6 and 12");
      return;
    }
    else{
      LoRa.setSpreadingFactor(spreadFactor);
      Serial.println("Spreading factor set to " + String(spreadFactor));
      rx_status = false;
    }

  } 
  else {
    Serial.println("No argument"); 
  }
}

void set_bw(){
  char *arg;  
  arg = SCmd.next();    // Get the next argument from the SerialCommand object buffer
  int bwRefResp = bwReference; //save the previous data 
  bwReference = atoi(arg);
  if (arg != NULL){
      switch (bwReference){
        case 0:
          LoRa.setSignalBandwidth(7.8E3);
          rx_status = false;
          Serial.println("Bandwidth set to 7.8 kHz");
          break;
        case 1:
          LoRa.setSignalBandwidth(10.4E3);
          rx_status = false;
          Serial.println("Bandwidth set to 10.4 kHz");
          break;
        case 2:
          LoRa.setSignalBandwidth(15.6E3);
          rx_status = false;
          Serial.println("Bandwidth set to 15.6 kHz");
          break;
        case 3:
          LoRa.setSignalBandwidth(20.8E3);
          rx_status = false;
          Serial.println("Bandwidth set to 20.8 kHz");
          break;
        case 4:
          LoRa.setSignalBandwidth(31.25E3);
          rx_status = false;
          Serial.println("Bandwidth set to 31.25 kHz");
          break;
        case 5:
          LoRa.setSignalBandwidth(41.7E3);
          rx_status = false;
          Serial.println("Bandwidth set to 41.7 kHz");
          break;
        case 6:
          LoRa.setSignalBandwidth(62.5E3);
          rx_status = false;
          Serial.println("Bandwidth set to 62.5 kHz");
          break;
        case 7:
          LoRa.setSignalBandwidth(125E3);
          rx_status = false;
          Serial.println("Bandwidth set to 125 kHz");
          break;
        case 8:
          LoRa.setSignalBandwidth(250E3);
          rx_status = false;
          Serial.println("Bandwidth set to 250 kHz");
          break;

        default:
          Serial.println("Error setting the bandwidth value must be between 0-8");
          bwReference = bwRefResp; //if there's no valid data restore previous value
          break;
      }
  } 
  else {
    Serial.println("No argument"); 
  }
}

byte nibble(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return 0;  // Not a valid hexadecimal character
}

void set_tx0(){
  char *arg;  
  arg = SCmd.next();    // Get the next argument from the SerialCommand object buffer
  if (arg != NULL){
      if((arg[0] > 47 && arg[0]< 58) && (arg[1] > 47 && arg[1]< 58) && (arg[2] > 47 && arg[2]< 58) && arg[3] == 0){
              
        byte data = (byte)strtoul(arg, NULL, 10);
  
        Serial.println(data,BIN);
        
        LoRa.beginPacket();                   // start packet                 // add payload
        LoRa.write(data);
        LoRa.endPacket(true);                 // finish packet and send it
  
        Serial.println("Byte sent"); 
  
        rx_status = false;
        
      }
    else {
      Serial.println("Use xxx. The value xxx represents a 3-digit number. ");
    }
  } 
  else {
    Serial.println("No argument"); 
  }
}

void set_tx1(){
  char *arg;  
  arg = SCmd.next();    // Get the next argument from the SerialCommand object buffer
  if (arg != NULL){
    if(arg[0] == '0' && arg[1] == 'x'&& arg[4] == 0){
      if((arg[2] > 64 && arg[2]< 71 || arg[2] > 47 && arg[2]< 58) && (arg[3] > 64 && arg[3]< 71 || arg[3] > 47 && arg[3]< 58)){

        byte data = 0;
        data = nibble(*(arg + 2))<<4;
        data = data|nibble(*(arg + 3));
  
        Serial.println(data,BIN);
        
        LoRa.beginPacket();                   // start packet                 // add payload
        LoRa.write(data);
        LoRa.endPacket(true);                 // finish packet and send it
  
        Serial.println("Byte sent"); 
  
        rx_status = false;
        
      }
    }
    else {
      Serial.println("Use 0xyy. The value yy represents any pair of hexadecimal digits. ");
    }
  } 
  else {
    Serial.println("No argument"); 
  }
}

void set_tx2(){
  char *arg;  
  arg = SCmd.next();    // Get the next argument from the SerialCommand object buffer
  if (arg != NULL){
      
      LoRa.beginPacket();                   // start packet
      
      for(int i = 0;;i++){
        if(arg[i] == 0)
          break;
        Serial.print(arg[i]);
        LoRa.write(arg[i]);             // add payload
      }
      
      LoRa.endPacket(true);                 // finish packet and send it

      Serial.println(" ASCII message sent"); 

      rx_status = false;
  } 
  else {
    Serial.println("No argument"); 
  }
}

void set_chann(){
  char *arg;  
  arg = SCmd.next();    // Get the next argument from the SerialCommand object buffer
  channel = atoi(arg);
  if (arg != NULL){
    if(channel > -1 && channel < 64){
    long freq = 902300000 + channel*125000;
    frequency = (float)freq/1000000;
    LoRa.setFrequency(freq);
    Serial.println("Uplink channel set to " + String(channel));
    rx_status = false;
    }
    else if(channel > 63 && channel < 72){
    long freq = 903000000 + (channel - 64)*500000;
    frequency = (float)freq/1000000;
    LoRa.setFrequency(freq);
    Serial.println("Uplink channel set to " + String(channel));
    rx_status = false;
    }
    else{
      Serial.println("Error setting the channel");
      Serial.println("Value must be between 0 and 63");
    }
  }  
  else {
    Serial.println("No argument"); 
  }
}

void set_rx(){
  char *arg;  
  arg = SCmd.next(); 
  if (arg != NULL){
    frequency = atof(arg);
    if(frequency > 902 && frequency < 923){
      long freq = frequency*1000000;
      LoRa.setFrequency(freq);
      Serial.println("LoRa radio receiving at " + String(frequency) + " MHz");
      while (digitalRead(RFM_DIO5) == LOW){
          Serial.print(".");
        }
      LoRa.receive(); 
      rx_status = true;
    }
    else{
      Serial.println("Error setting the frequency");
      Serial.println("Value must be between 902 MHz and 923 MHz");
    }
  } 
  else {
    Serial.println("LoRa radio receiving at " + String(frequency) + " MHz");
    LoRa.receive();
    rx_status = true;
  }
}

/**********Get information**************/
void get_freq(){
  Serial.print("Frequency = ");
  Serial.println(frequency);
}

void get_sf(){
  Serial.print("Spreading factor = ");
  Serial.println(spreadFactor);
}

void get_bw(){
  Serial.println("Bandwidth = ");
  switch (bwReference){
    case 0:
      Serial.println("7.8 kHz");
      break;
    case 1:
      Serial.println("10.4 kHz");
      break;
    case 2:
      Serial.println("15.6 kHz");
      break;
    case 3:
      Serial.println("20.8 kHz");
      break;
    case 4:
      Serial.println("31.25 kHz");
      break;
    case 5:
      Serial.println("41.7 kHz");
      break;
    case 6:
      Serial.println("62.5 kHz");
      break;
    case 7:
      Serial.println("125 kHz");
      break;
    case 8:
      Serial.println("250 kHz");
      break;
    default:
      Serial.println("Error setting the bandwidth value must be between 0-8");
      break;
  } 

}



void get_config(){
  Serial.println("Radio configurations: ");
  Serial.println("Frequency = " + String(frequency) + " MHz");
  Serial.println("Spreading Factor = " + String(spreadFactor));
  Serial.print("Bandwidth = ");
  switch (bwReference){
    case 0:
      Serial.println("7.8 kHz");
      break;
    case 1:
      Serial.println("10.4 kHz");
      break;
    case 2:
      Serial.println("15.6 kHz");
      break;
    case 3:
      Serial.println("20.8 kHz");
      break;
    case 4:
      Serial.println("31.25 kHz");
      break;
    case 5:
      Serial.println("41.7 kHz");
      break;
    case 6:
      Serial.println("62.5 kHz");
      break;
    case 7:
      Serial.println("125 kHz");
      break;
    case 8:
      Serial.println("250 kHz");
      break;
  }
  Serial.println("Rx active = " + String(rx_status));
}


// This gets set as the default handler, and gets called when no other command matches. 
void unrecognized(const char *command) {
  Serial.println("Command not found, type help to get the valid commands"); 
}

void onReceive(int packetSize) {
  char buf[256];
  int i;
  
  // received a packet
  Serial.print("Received packet '");

  Serial.print(packetSize);
  Serial.print(" bytes '");
  
  // read packet
  for (i = 0; i < packetSize; i++) {
    buf[i] = LoRa.read();
    Serial.print("<0x");
    Serial.print(buf[i], HEX);
    Serial.print(">");
  }
  buf[i] = 0;
  Serial.print(" ");
  Serial.print(buf);

  // print RSSI of packet
  Serial.print("' with RSSI ");
  Serial.println(LoRa.packetRssi());
  LoRa.receive();
}
