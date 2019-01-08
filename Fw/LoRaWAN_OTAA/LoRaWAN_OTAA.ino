/*******************************************************************************
  Created by Eduardo Contreras @ Electronic Cats 2018
  Based on Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
  PLEASE REFER TO THIS LMIC LIBRARY https://github.com/things-nyc/arduino-lmic
  
  LoRaShield
  In this example you can send GPS information (Compatible with the L80)
  or a temperature read by the ADC  
  Example
 *******************************************************************************/

#include <lmic.h> 
#include <hal/hal.h> 
#include <SPI.h> 
 
unsigned long previousMillis = 0; 

// This EUI must be in little-endian format, so least-significant-byte first. 
static const u1_t PROGMEM DEVEUI[8]= {0x83,0x34,0x27,0x82,0x91,0x12,0x29,0x91};
// This should also be in little endian format, see above.  
static const u1_t PROGMEM APPEUI[8]= {0x76,0x28,0x81,0x73,0x61,0x35,0xFF,0xFF}; 
// This key should be in big endian format so normal key should be place
static const u1_t PROGMEM APPKEY[16] = {0xFF,0x21,0x35,0x16,0x23,0x21,0x3F,0x23,0xF2,0x3F,0x23,0xF2,0x3F,0x23,0xF2,0x31}; 
 
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);} 
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);} 
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);} 
 
 
static osjob_t sendjob; 
static osjob_t blinkjob; 
static osjob_t readJob; 
 
const unsigned TX_INTERVAL = 10000;  //In millisecons
 
// Pin mapping 
const lmic_pinmap lmic_pins = { 
    .nss = SS, 
    .rxtx = LMIC_UNUSED_PIN, 
    .rst = RFM_RST, 
    .dio = {RFM_DIO0, RFM_DIO1, RFM_DIO2}, 
}; 
 
void do_send(osjob_t* j, uint8_t *mydata, uint16_t len) { 
  // Check if there is not a current TX/RX job running 
  if (LMIC.opmode & OP_TXRXPEND) { 
    Serial.println(F("[LMIC] OP_TXRXPEND, not sending")); 
  } else { 
    // Prepare upstream data transmission at the next possible time. 
    LMIC_setTxData2(1, mydata, len, 0); 
  } 
} 
 
static void blinkfunc (osjob_t* j) { 
  digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN)); 
  os_setTimedCallback(j, os_getTime()+ms2osticks(100), blinkfunc);  // reschedule blink job 
} 
 
static void readfunc(osjob_t* j) { 
 
  uint8_t len = 0; 
  uint8_t mydata[4]; 
  uint8_t cnt = 0; 
  uint8_t ch = 0; 

  len += 4; 
  Serial.println(F("[INFO] Collecting temperature info")); 
 
  float temp = 26.4; 
  Serial.print(F("[INFO] Temperature:")); Serial.println(temp); 
  int val = round(temp * 10);
  mydata[cnt++] = ch; 
  mydata[cnt++] = 0x67; 
  mydata[cnt++] = highByte(val); 
  mydata[cnt++] = lowByte(val); 
 
  if (cnt == len) { 
    Serial.println(F("[LMIC] Start Radio TX")); 
    // indicating start radio TX 
    digitalWrite(LED_BUILTIN, HIGH); 
 
    do_send(&sendjob, mydata, sizeof(mydata)); 
  } 
  else{ 
    Serial.println(F("[ERROR] Data stack incorrect")); 
  } 
} 

 
void onEvent (ev_t ev) { 
    Serial.print("[LMIC] "); 
    switch(ev) { 
        case EV_JOINING: 
            Serial.println(F("EV_JOINING")); 
             blinkfunc(&blinkjob); 
            break; 
        case EV_JOINED: 
            Serial.println(F("EV_JOINED")); 
            LMIC_setDrTxpow(DR_SF9,14); 
            // Disable link check validation (automatically enabled 
            // during join, but not supported by TTN at this time). 
            os_clearCallback(&blinkjob); 
            os_clearCallback(&sendjob); 
            digitalWrite(LED_BUILTIN,LOW); 
            readfunc(&readJob); 
            LMIC_setLinkCheckMode(0); 
            break; 
        case EV_JOIN_FAILED: 
            Serial.println(F("EV_JOIN_FAILED")); 
            break; 
        case EV_REJOIN_FAILED: 
            Serial.println(F("EV_REJOIN_FAILED")); 
            break; 
        case EV_TXCOMPLETE: 
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)")); 
            if (LMIC.txrxFlags & TXRX_ACK) 
              Serial.println(F("Received ack")); 
            if (LMIC.dataLen) { 
              Serial.println(F("Received ")); 
              Serial.println(LMIC.dataLen); 
              Serial.println(F(" bytes of payload")); 
            }
            digitalWrite(LED_BUILTIN, LOW); 
            os_setTimedCallback(&readJob, os_getTime()+ms2osticks(TX_INTERVAL), readfunc);  // reschedule Tx time 
            break; 
        case EV_RXCOMPLETE: 
            // data received in ping slot 
            Serial.println(F("EV_RXCOMPLETE")); 
            break; 
         default: 
            Serial.println(F("Unknown event"));  
            //Serial.println(ev); 
            break; 
    } 
} 
 
void setup() { 
  Serial.begin(115200); 
  Serial.println(F("[INFO] LoRa Demo Node 1 Demonstration")); 
 
  //initialize LED as output and at low state 
  pinMode(LED_BUILTIN, OUTPUT); 
  digitalWrite(LED_BUILTIN, LOW); 
 
  os_init(); 
  LMIC_reset(); 
 
  for (int channel=0; channel<72; ++channel) { 
      LMIC_disableChannel(channel); 
    } 
 /*
      //Subband 1 
      LMIC_enableChannel(8);
      LMIC_enableChannel(9);
      LMIC_enableChannel(10);  //904.3Mhz
      LMIC_enableChannel(11);
      LMIC_enableChannel(12);
      LMIC_enableChannel(13);
      LMIC_enableChannel(14);
      LMIC_enableChannel(15);
      LMIC_enableChannel(65); 
   */
  //Beelan
    
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
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100); 
  previousMillis = millis(); 
  uint8_t first=0xFF; 
  do_send(&sendjob, &first, sizeof(first)); 
    } 
   
void loop() { 
    os_runloop_once(); 
} 
