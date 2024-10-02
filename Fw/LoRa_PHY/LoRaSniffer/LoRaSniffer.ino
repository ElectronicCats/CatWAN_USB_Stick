#define SERIALCOMMAND_HARDWAREONLY

#include <SerialCommand.h>  // Library for processing serial commands
#include <SPI.h>  // Library for SPI communication
#include <LoRa.h>  // Library for LoRa communication
#include <Crypto.h>  // Library for cryptographic functions
#include <AES.h>  // Library for AES encryption

// Initialize AES128 object for encryption
AES128 aes;

// 128-bit encryption key for AES
byte aes_key[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
// Initialization vector for AES (not used in this example)
byte aes_iv[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Variables for sensor simulation
bool sensorActive = false;
unsigned long lastSendTime = 0;
unsigned long sendInterval = 5000;  // Interval for sending sensor data (5 seconds)

// SerialCommand object for handling serial commands
SerialCommand SCmd;

// Variables for LoRa radio configuration
float fwVersion = 0.2;  // Firmware version
float frequency = 915;  // Default frequency in MHz
int spreadFactor = 7;   // Default spreading factor
int bwReference = 7;    // Default bandwidth reference (125 kHz)
bool rx_status = false;  // Status of the LoRa receiver

void setup() {
  // Setup the built-in LED for output
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);  // Set LED to off by default

  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial);  // Wait until the serial port is ready

  // Welcome message and information about the commands
  Serial.println("Welcome to the LoRa Sniffer CLI " + String(fwVersion, 1) + "v\n");
  Serial.println("With this sketch you can scan the LoRa spectrum");
  Serial.println("Changing the Frequency, Spreading Factor, BandWidth or the IQ signals of the radio.");
  Serial.println("Type help to get the available commands.");
  Serial.println("Electronic Cats ® 2020");

  // Set AES encryption key
  aes.setKey(aes_key, sizeof(aes_key));

  // Setup command handlers for different serial commands
  SCmd.addCommand("help", help);
  SCmd.addCommand("set_rx", set_rx);
  SCmd.addCommand("set_tx", set_tx1);
  SCmd.addCommand("set_tx_ascii", set_tx2);
  SCmd.addCommand("set_freq", set_freq);
  SCmd.addCommand("set_sf", set_sf);
  SCmd.addCommand("set_bw", set_bw);
  SCmd.addCommand("get_config", get_config);
  SCmd.addCommand("get_freq", get_freq);
  SCmd.addCommand("get_sf", get_sf);
  SCmd.addCommand("get_bw", get_bw);
  SCmd.addCommand("start_sensor", start_sensor);  // Command to start the sensor simulation

  SCmd.setDefaultHandler(unrecognized);  // Handler for unrecognized commands

  // Initialize LoRa module with specific pins for SS, RST, and DIO0
  LoRa.setPins(SS, RFM_RST, RFM_DIO0);

  // Start LoRa communication on 915 MHz frequency
  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);  // Stop execution if LoRa initialization fails
  }
  
  // Set the callback function to handle incoming packets
  LoRa.onReceive(onReceive);
}

void loop() {
  // Read and process serial commands
  SCmd.readSerial();

  // Sensor simulation: send encrypted data at regular intervals
  if (sensorActive && (millis() - lastSendTime >= sendInterval)) {
    lastSendTime = millis();  // Update the last send time

    // Generate random sensor data
    int randomValue = random(0, 255);
    Serial.print("Data before encryption (decimal): ");
    Serial.println(randomValue);

    // Convert the random value to hexadecimal format for encryption
    byte plainText[16] = { 0 };
    sprintf((char *)plainText, "%02X", randomValue);  // Convert to hexadecimal

    // Encrypt the plain text using AES
    byte encryptedText[16] = { 0 };
    aes.encryptBlock(encryptedText, plainText);

    // Send the encrypted data over LoRa
    LoRa.beginPacket();
    LoRa.write(encryptedText, sizeof(encryptedText));
    LoRa.endPacket();

    Serial.println("Random encrypted data sent successfully!");
  }
}

void help() {
  // Display help information about available commands
  Serial.println("Fw version: " + String(fwVersion, 1) + "v");
  Serial.println("\tConfiguration commands:");
  Serial.println("\tset_rx");
  Serial.println("\tset_tx");
  Serial.println("\tset_tx_ascii");
  Serial.println("\tset_freq");
  Serial.println("\tset_sf");
  Serial.println("\tset_bw");
  Serial.println("\tget_config");
  Serial.println("Type 'start_sensor' to simulate sensor data transmission.");
}

/********** Set configuration commands **************/

void set_rx() {
  // Command to set the radio to receive mode
  char *arg;
  arg = SCmd.next();  // Get the next argument from the command
  if (arg != NULL) {
    // Convert the argument to frequency and set LoRa frequency
    frequency = atof(arg);
    if (frequency > 902 && frequency < 923) {
      long freq = frequency * 1000000;
      LoRa.setFrequency(freq);
      Serial.println("LoRa radio receiving at " + String(frequency) + " Mhz");
      LoRa.receive();  // Start receiving LoRa packets
      rx_status = true;
    } else {
      Serial.println("Error setting the frequency");
      Serial.println("Value must be between 902 Mhz and 923");
    }
  } else {
    Serial.println("LoRa radio receiving at " + String(frequency) + " Mhz");
    LoRa.receive();
    rx_status = true;
  }
}

void set_tx1() {
  // Command to set LoRa radio to transmit mode
  char *arg = SCmd.next();
  if (arg != NULL) {
    rx_status = false;
  } else {
    Serial.println("No argument");
  }
}

void set_tx2() {
  // Command to transmit ASCII data over LoRa
  char *arg = SCmd.next();
  if (arg != NULL) {
    rx_status = false;
  } else {
    Serial.println("No argument");
  }
}

void set_freq() {
  // Command to set the frequency of the LoRa module
  char *arg = SCmd.next();
  frequency = atof(arg);
  if (arg != NULL) {
    if (frequency > 902 && frequency < 923) {
      long freq = frequency * 1000000;
      LoRa.setFrequency(freq);
      Serial.println("Frequency set to " + String(frequency) + " Mhz");
      rx_status = false;
    } else {
      Serial.println("Error setting the frequency");
      Serial.println("Value must be between 902 Mhz and 923");
    }
  } else {
    Serial.println("No argument");
  }
}

void set_sf() {
  // Command to set the spreading factor of LoRa communication
  char *arg = SCmd.next();
  if (arg != NULL) {
    spreadFactor = atoi(arg);
    if (spreadFactor < 6 || spreadFactor > 12) {
      Serial.println("Error setting the Spreading factor");
      Serial.println("Value must be between 6 and 12");
      return;
    } else {
      LoRa.setSpreadingFactor(spreadFactor);
      Serial.println("Spreading factor set to " + String(spreadFactor));
      rx_status = false;
    }
  } else {
    Serial.println("No argument");
  }
}

void set_bw() {
  // Command to set the bandwidth of LoRa communication
  char *arg = SCmd.next();
  int bwRefResp = bwReference;
  bwReference = atoi(arg);
  if (arg != NULL) {
    switch (bwReference) {
      case 0:
        LoRa.setSignalBandwidth(7.8E3);
        Serial.println("Bandwidth set to 7.8 Khz");
        break;
      case 1:
        LoRa.setSignalBandwidth(10.4E3);
        Serial.println("Bandwidth set to 10.4 Khz");
        break;
      case 2:
        LoRa.setSignalBandwidth(15.6E3);
        Serial.println("Bandwidth set to 15.6 Khz");
        break;
      case 3:
        LoRa.setSignalBandwidth(20.8E3);
        Serial.println("Bandwidth set to 20.8 Khz");
        break;
      case 4:
        LoRa.setSignalBandwidth(31.25E3);
        Serial.println("Bandwidth set to 31.25 Khz");
        break;
      case 5:
        LoRa.setSignalBandwidth(41.7E3);
        Serial.println("Bandwidth set to 41.7 Khz");
        break;
      case 6:
        LoRa.setSignalBandwidth(62.5E3);
        Serial.println("Bandwidth set to 62.5 Khz");
        break;
      case 7:
        LoRa.setSignalBandwidth(125E3);
        Serial.println("Bandwidth set to 125 Khz");
        break;
      case 8:
        LoRa.setSignalBandwidth(250E3);
        Serial.println("Bandwidth set to 250 Khz");
        break;
      default:
        Serial.println("Error setting the bandwidth value must be between 0-8");
        bwReference = bwRefResp;
        break;
    }
    rx_status = false;
  } else {
    Serial.println("No argument");
  }
}

// Command to start sensor simulation
void start_sensor() {
  sensorActive = true;
  Serial.println("Sensor simulation started. Sending random data...");
}

/********** Get configuration information **************/

void get_freq() {
  // Display the current frequency
  Serial.print("Frequency = ");
  Serial.println(frequency);
}

void get_sf() {
  // Display the current spreading factor
  Serial.print("Spreading factor = ");
  Serial.println(spreadFactor);
}

void get_bw() {
  // Display the current bandwidth
  Serial.println("Bandwidth = ");
  switch (bwReference) {
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

void get_config() {
  // Display all current radio configuration settings
  Serial.println("Radio configurations: ");
  Serial.println("Frequency = " + String(frequency) + " Mhz");
  Serial.println("Spreading Factor = " + String(spreadFactor));
  Serial.print("Bandwidth = ");
  switch (bwReference) {
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
  Serial.println("Rx active = " + String(rx_status));
}

void unrecognized(const char *command) {
  // Handler for unrecognized commands
  Serial.println("Command not found, type help to get the valid commands");
}

void onReceive(int packetSize) {
  // Function to handle received packets
  Serial.print("Received encrypted packet: ");

  // Buffer to hold the received encrypted packet
  byte encryptedText[16];
  for (int i = 0; i < packetSize && i < 16; i++) {
    encryptedText[i] = LoRa.read();
    Serial.print(encryptedText[i], HEX);  // Print each byte in hex
    Serial.print(" ");
  }
  Serial.println();

  // Decrypt the received packet
  byte decryptedText[16] = { 0 };
  aes.decryptBlock(decryptedText, encryptedText);

  // Convert decrypted text to a number (decimal)
  int receivedValue = strtol((char *)decryptedText, NULL, 16);

  // Print the decrypted value in decimal format
  Serial.print("Decrypted value (decimal): ");
  Serial.println(receivedValue);

  // Print the RSSI (Received Signal Strength Indicator)
  Serial.print("RSSI: ");
  Serial.println(LoRa.packetRssi());
}
