/*******************************************************************************
  Created by Eduardo Contreras @ Electronic Cats 2018
  Based on Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
  PLEASE REFER TO THIS LMIC LIBRARY https://github.com/things-nyc/arduino-lmic
  
  CatWAN USB-Stick
  Example
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

unsigned long previousMillis = 0;

                                        //30   ff   5c  1e    44  02   a0   88    64   66   6c   bb   63   68   f2   e8
static const PROGMEM u1_t NWKSKEY[16] ={0x30,0xFF,0x5C,0x1E,0x44,0x02,0xA0,0x88,0x64,0x66,0x6C,0xBB,0x63,0x68,0xF2,0xE8};
                                       // ac  ad    4a   55  6a    21  4c    f5   59   89   c0   df   ec   ba   31   e9
static const u1_t PROGMEM APPSKEY[16] ={0xAC,0xAD,0x4A,0x55,0x6A,0x21,0x4C,0xF5,0x59,0x89,0xC0,0xDF,0xEC,0xBA,0x31,0xE9};
static const u4_t DEVADDR =0x01a09cfb;  //01a09cfb


void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

// We will be using Cayenne Payload Format
// For one sensor,
// the general format is channel | type | payload
// payload size depends on type
// here we are using temperature

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 10;

// Pin mapping for RFM9X
const lmic_pinmap lmic_pins = {
    .nss = SS,
    .rxtx = RFM_SWITCH,
    .rst = RFM_RST,
    .dio = {RFM_DIO0, RFM_DIO1, RFM_DIO2},
};

void onEvent (ev_t ev) {
  switch (ev) {
    case EV_TXCOMPLETE:

      // indicating radio TX complete
      digitalWrite(LED_BUILTIN, LOW);

      Serial.println(F("[LMIC] Radio TX complete (included RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("[LMIC] Received ack"));
      if (LMIC.dataLen) {
        Serial.print(F("[LMIC] Received "));
        Serial.print(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));
        //Serial.write(LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
        //Serial.println(LMIC.frame + LMIC.dataBeg, HEX);
        Serial.write(LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
        Serial.println();

      }
      break;

    default:
      Serial.println(F("[LMIC] Unknown event"));
      break;
  }
}

void do_send(uint8_t *mydata1, uint16_t len) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("[LMIC] OP_TXRXPEND, not sending"));
  } else {
    // Prepare upstream data transmission at the next possible time
    LMIC_setTxData2(1, mydata1, len, 0);
  }
}



void setup() {
  pinMode(LED_BUILTIN,OUTPUT);
  // indicating radio TX complete
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  Serial.println(F("[INFO] LoRa Demo Node 1 Demonstration"));

  os_init();
  LMIC_reset();

  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));

  LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);

  for (int channel=0; channel<72; ++channel) {
      LMIC_disableChannel(channel);
    }

   //Beelan channels 
  LMIC_enableChannel(48);
  LMIC_enableChannel(49);
  LMIC_enableChannel(50);
  LMIC_enableChannel(51);
  LMIC_enableChannel(52);
  LMIC_enableChannel(53);
  LMIC_enableChannel(54);
  LMIC_enableChannel(55);
  LMIC_enableChannel(70);
  
  LMIC_setLinkCheckMode(0);
  LMIC_setAdrMode(false);
  LMIC_setDrTxpow(DR_SF7, 14); //SF7

  previousMillis = millis();

}

void loop() {

  if (millis() > previousMillis + TX_INTERVAL * 1000) { //Start Job at every TX_INTERVAL*1000

    getInfoAndSend();
    previousMillis = millis();
  }

  os_runloop_once();
}

void getInfoAndSend() {

  uint8_t len=4;   //Bug of len

  uint8_t mydata[4];
  uint8_t cnt = 0;
  uint8_t ch = 0;
  Serial.println(F("[INFO] Collecting info"));

  float temp =  37;
  Serial.print(F("[INFO] Temperature:")); Serial.println(temp);
  int val = round(temp * 10);
  mydata[cnt++] = ch;
  mydata[cnt++] = 0x67;
  mydata[cnt++] = highByte(val);
  mydata[cnt++] = lowByte(val);
  
    Serial.println(F("[LMIC] Start Radio TX"));
    // indicating start radio TX
    for(int i;i<sizeof(mydata);i++){
      Serial.print(mydata[i]);
    }
    Serial.println();
    digitalWrite(LED_BUILTIN, HIGH);
    do_send(mydata, sizeof(mydata));
  }
