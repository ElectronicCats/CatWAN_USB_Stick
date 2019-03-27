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
#include <TemperatureZero.h>


TemperatureZero TempZero = TemperatureZero();
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

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 10;

// Pin mapping for RFM9X
const lmic_pinmap lmic_pins = {
    .nss = SS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = RFM_RST,
    .dio = {RFM_DIO0, RFM_DIO1, RFM_DIO2},
};

void debug_char(u1_t b) {
  debugSerial.write(b);
}

void debug_hex (u1_t b) {
  debug_char("0123456789ABCDEF"[b >> 4]);
  debug_char("0123456789ABCDEF"[b & 0xF]);
}

void debug_buf (const u1_t* buf, u2_t len) {
  while (len--) {
    debug_hex(*buf++);
    debug_char(' ');
  }
  debug_char('\r');
  debug_char('\n');
}

void onEvent (ev_t ev) {
  switch (ev) {
    case EV_TXCOMPLETE:

      // indicating radio TX complete
      digitalWrite(LED_BUILTIN, LOW);

      SerialUSB.println(F("[LMIC] Radio TX complete (included RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
        SerialUSB.println(F("[LMIC] Received ack"));
      if (LMIC.dataLen) {
        SerialUSB.print(F("[LMIC] Received "));
        SerialUSB.print(LMIC.dataLen);
        SerialUSB.println(F(" bytes of payload"));
        //Serial.write(LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
        //Serial.println(LMIC.frame + LMIC.dataBeg, HEX);
        Serial.write(LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
        Serial.println();
        debug_buf(LMIC.frame + LMIC.dataBeg, LMIC.dataLen);

      }
      break;

    default:
      SerialUSB.println(F("[LMIC] Unknown event"));
      break;
  }
}

void do_send(osjob_t* j, uint8_t *mydata1, uint16_t len) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    SerialUSB.println(F("[LMIC] OP_TXRXPEND, not sending"));
  } else {
    // Prepare upstream data transmission at the next possible time
    LMIC_setTxData2(1, mydata1, len, 0);
  }
}



void setup() {
  pinMode(LED_BUILTIN,OUTPUT);
  // indicating radio TX complete
  digitalWrite(LED_BUILTIN, LOW);
  SerialUSB.begin(115200);
  SerialUSB.println(F("[INFO] LoRa Demo Node 1 Demonstration"));
  
  TempZero.init();
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
  SerialUSB.println(F("[INFO] Collecting info"));

  float temp =   TempZero.readInternalTemperature();
  SerialUSB.print(F("[INFO] Temperature:")); SerialUSB.println(temp);
  int val = round(temp * 10);
  mydata[cnt++] = ch;
  mydata[cnt++] = 0x67;
  mydata[cnt++] = highByte(val);
  mydata[cnt++] = lowByte(val);
  
    SerialUSB.println(F("[LMIC] Start Radio TX"));
    // indicating start radio TX
    for(int i;i<sizeof(mydata);i++){
      SerialUSB.print(mydata[i]);
    }
    SerialUSB.println();
    digitalWrite(LED_BUILTIN, HIGH);
    do_send(&sendjob, mydata, sizeof(mydata));
  }
