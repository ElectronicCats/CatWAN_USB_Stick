#define SERIALCOMMAND_HARDWAREONLY
 
#include <SerialCommand.h>
#include <SPI.h>
#include <LoRa.h>

SerialCommand SCmd;   // The demo SerialCommand object

float fwVersion= 0.1;

float frequency=915;
int spreadFactor=7;
int bwReference=7;
bool rx_status=false;

void setup(){  
  pinMode(LED_BUILTIN,OUTPUT);      // Configure the onboard LED for output
  digitalWrite(LED_BUILTIN,LOW);    // default to LED off

  Serial.begin(115200); 
  while (!Serial);
  Serial.println("Welcome to the LoRa Sniffer CLI " + String(fwVersion,1) + "v\n");
  Serial.println("With this sketch you can scan the LoRa spectrum");
  Serial.println("Changing the Frequency, Spreading Factor, BandWidth or the IQ signals of the radio.");
  Serial.println("Type help to get the available commands.");
  Serial.println("Electronic Cats ® 2019");
  // Setup callbacks for SerialCommand commands 
  /*
  TODO: 
    set_chann -> Put LoRaWAN frequencies
    set_tx_acii -> Send Ascii data over LoRa
  */
  SCmd.addCommand("set_rx",set_rx);
  SCmd.addCommand("set_tx",set_tx1);
  SCmd.addCommand("set_tx_ascii",set_tx2);
  SCmd.addCommand("set_freq",set_freq);
  SCmd.addCommand("set_sf",set_sf);
  SCmd.addCommand("set_bw",set_bw);

  SCmd.addCommand("get_config",get_config);
  SCmd.addCommand("get_freq",get_freq);
  SCmd.addCommand("get_sf",get_sf);
  SCmd.addCommand("get_bw",get_bw);

  SCmd.addCommand("help",help);        
  SCmd.addDefaultHandler(unrecognized);  // Handler for command that isn't matched  (says "What?") 

  LoRa.setPins(SS, RFM_RST, RFM_DIO0);

  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.onReceive(onReceive);
}

void loop()
{  
  SCmd.readSerial();     // We don't do much, just process serial commands
}

/**********Set configuration**************/
void set_freq(){
  char *arg;  
  arg = SCmd.next();
  frequency=atof(arg);
  if (arg != NULL){
    if(frequency>902&&frequency<923){
      long freq =frequency*1000000;
      LoRa.setFrequency(freq);
      Serial.println("Frequency set to "+String(frequency)+" Mhz");
      rx_status=false;
    }
    else{
      Serial.println("Error setting the frequency");
      Serial.println("Value must be between 902 Mhz and 923");
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
    spreadFactor=atoi(arg);
    if(spreadFactor<6||spreadFactor>12){
      Serial.println("Error setting the Spreading factor");
      Serial.println("Value must be between 6 and 12");
      return;
    }
    else{
      LoRa.setSpreadingFactor(spreadFactor);
      Serial.println("Spreading factor set to "+String(spreadFactor));
      rx_status=false;
    }

  } 
  else {
    Serial.println("No argument"); 
  }
}

void set_bw(){
  char *arg;  
  arg = SCmd.next();    // Get the next argument from the SerialCommand object buffer
  int bwRefResp=bwReference; //save the previous data 
  bwReference=atoi(arg);
  if (arg != NULL){
      switch (bwReference){
        case 0:
        LoRa.setSignalBandwidth(7.8E3);
        rx_status=false;
        Serial.println("Bandwidth set to 7.8 Khz");
          break;
        case 1:
        LoRa.setSignalBandwidth(10.4E3);
        rx_status=false;
        Serial.println("Bandwidth set to 10.4 Khz");
          break;
        case 2:
        LoRa.setSignalBandwidth(15.6E3);
        rx_status=false;
        Serial.println("Bandwidth set to 15.6 Khz");
          break;
        case 3:
        LoRa.setSignalBandwidth(20.8E3);
        rx_status=false;
        Serial.println("Bandwidth set to 20.8 Khz");
          break;
        case 4:
        LoRa.setSignalBandwidth(31.25E3);
        rx_status=false;
        Serial.println("Bandwidth set to 31.25 Khz");
          break;
        case 5:
        LoRa.setSignalBandwidth(41.7E3);
        rx_status=false;
        Serial.println("Bandwidth set to 41.7 Khz");
          break;
        case 6:
        LoRa.setSignalBandwidth(62.5E3);
        rx_status=false;
        Serial.println("Bandwidth set to 62.5 Khz");
          break;
        case 7:
        LoRa.setSignalBandwidth(125E3);
        rx_status=false;
        Serial.println("Bandwidth set to 125 Khz");
          break;
        case 8:
        LoRa.setSignalBandwidth(250E3);
        rx_status=false;
        Serial.println("Bandwidth set to 250 Khz");
          break;

        default:
        Serial.println("Error setting the bandwidth value must be between 0-8");
        bwReference=bwRefResp; //if there's no valid data restore previous value
          break;
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
  rx_status=false;
  } 
  else {
    Serial.println("No argument"); 
  }
}

void set_tx2(){
  char *arg;  
  arg = SCmd.next();    // Get the next argument from the SerialCommand object buffer
  if (arg != NULL){
  rx_status=false;
  } 
  else {
    Serial.println("No argument"); 
  }
}

void set_rx(){
  char *arg;  
  arg = SCmd.next(); 
  if (arg != NULL){
    frequency=atof(arg);
    if(frequency>902&&frequency<923){
      long freq =frequency*1000000;
      LoRa.setFrequency(freq);
      Serial.println("LoRa radio receiving at "+String(frequency)+" Mhz"); 
      rx_status=true;
    }
    else{
      Serial.println("Error setting the frequency");
      Serial.println("Value must be between 902 Mhz and 923");
    }
  } 
  else {
    Serial.println("LoRa radio receiving at "+String(frequency)+" Mhz");
    rx_status =true;
  }
}

/**********Get information**************/
void get_freq(){
  Serial.println(frequency);
}

void get_sf(){
  Serial.println(spreadFactor);
}

void get_bw(){
  switch (bwReference){
    case 0:
    Serial.println("7.8 Khz");
      break;
    case 1:
    Serial.println("10.4 Khz");
      break;
    case 2:
    Serial.println("15.6 Khz");
      break;
    case 3:
    Serial.println("20.8 Khz");
      break;
    case 4:
    Serial.println("31.25 Khz");
      break;
    case 5:
    Serial.println("41.7 Khz");
      break;
    case 6:
    Serial.println("62.5 Khz");
      break;
    case 7:
    Serial.println("125 Khz");
      break;
    case 8:
    Serial.println("250 Khz");
      break;

    default:
    Serial.println("Error setting the bandwidth value must be between 0-8");
      break;
  } 

}

void get_config(){
  Serial.println("Radio configurations: ");
  Serial.println("Frequency = "+String(frequency)+" Mhz");
  Serial.println("Spreading Factor = "+String(spreadFactor));
  Serial.print("Bandwidth = ");
  switch (bwReference){
    case 0:
    Serial.println("7.8 Khz");
      break;
    case 1:
    Serial.println("10.4 Khz");
      break;
    case 2:
    Serial.println("15.6 Khz");
      break;
    case 3:
    Serial.println("20.8 Khz");
      break;
    case 4:
    Serial.println("31.25 Khz");
      break;
    case 5:
    Serial.println("41.7 Khz");
      break;
    case 6:
    Serial.println("62.5 Khz");
      break;
    case 7:
    Serial.println("125 Khz");
      break;
    case 8:
    Serial.println("250 Khz");
      break;
  }
  Serial.println("Rx active = "+String(rx_status));

}

void help(){
  Serial.println("Fw version: "+String(fwVersion,1)+"v");
  Serial.println("\tConfiguration commands:");
  Serial.println("\tset_rx");
  Serial.println("\tset_tx");
  Serial.println("\tset_tx_ascii");
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
// This gets set as the default handler, and gets called when no other command matches. 
void unrecognized(){
  Serial.println("Command not found, type help to get the valid commands"); 
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
