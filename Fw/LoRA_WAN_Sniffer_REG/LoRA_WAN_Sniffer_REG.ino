#include <SPI.h>

#define DIO0 RFM_DIO0
#define NSS  SS

void RFM_NSS(bool);
int Message=0;
unsigned int Frame_Counter_Tx = 0x0000;
int serialData;

static const unsigned char PROGMEM S_Table[16][16] = {
  {0x63,0x7C,0x77,0x7B,0xF2,0x6B,0x6F,0xC5,0x30,0x01,0x67,0x2B,0xFE,0xD7,0xAB,0x76},
  {0xCA,0x82,0xC9,0x7D,0xFA,0x59,0x47,0xF0,0xAD,0xD4,0xA2,0xAF,0x9C,0xA4,0x72,0xC0},
  {0xB7,0xFD,0x93,0x26,0x36,0x3F,0xF7,0xCC,0x34,0xA5,0xE5,0xF1,0x71,0xD8,0x31,0x15},
  {0x04,0xC7,0x23,0xC3,0x18,0x96,0x05,0x9A,0x07,0x12,0x80,0xE2,0xEB,0x27,0xB2,0x75},
  {0x09,0x83,0x2C,0x1A,0x1B,0x6E,0x5A,0xA0,0x52,0x3B,0xD6,0xB3,0x29,0xE3,0x2F,0x84},
  {0x53,0xD1,0x00,0xED,0x20,0xFC,0xB1,0x5B,0x6A,0xCB,0xBE,0x39,0x4A,0x4C,0x58,0xCF},
  {0xD0,0xEF,0xAA,0xFB,0x43,0x4D,0x33,0x85,0x45,0xF9,0x02,0x7F,0x50,0x3C,0x9F,0xA8},
  {0x51,0xA3,0x40,0x8F,0x92,0x9D,0x38,0xF5,0xBC,0xB6,0xDA,0x21,0x10,0xFF,0xF3,0xD2},
  {0xCD,0x0C,0x13,0xEC,0x5F,0x97,0x44,0x17,0xC4,0xA7,0x7E,0x3D,0x64,0x5D,0x19,0x73},
  {0x60,0x81,0x4F,0xDC,0x22,0x2A,0x90,0x88,0x46,0xEE,0xB8,0x14,0xDE,0x5E,0x0B,0xDB},
  {0xE0,0x32,0x3A,0x0A,0x49,0x06,0x24,0x5C,0xC2,0xD3,0xAC,0x62,0x91,0x95,0xE4,0x79},
  {0xE7,0xC8,0x37,0x6D,0x8D,0xD5,0x4E,0xA9,0x6C,0x56,0xF4,0xEA,0x65,0x7A,0xAE,0x08},
  {0xBA,0x78,0x25,0x2E,0x1C,0xA6,0xB4,0xC6,0xE8,0xDD,0x74,0x1F,0x4B,0xBD,0x8B,0x8A},
  {0x70,0x3E,0xB5,0x66,0x48,0x03,0xF6,0x0E,0x61,0x35,0x57,0xB9,0x86,0xC1,0x1D,0x9E},
  {0xE1,0xF8,0x98,0x11,0x69,0xD9,0x8E,0x94,0x9B,0x1E,0x87,0xE9,0xCE,0x55,0x28,0xDF},
  {0x8C,0xA1,0x89,0x0D,0xBF,0xE6,0x42,0x68,0x41,0x99,0x2D,0x0F,0xB0,0x54,0xBB,0x16}
};


unsigned char NwkSkey[16] = { 0x41, 0xe2, 0xc1, 0x11, 0xcf, 0x0b, 0xfb, 0x8d, 0x1d, 0x08, 0x98, 0xf0, 0xca, 0xa8, 0x77, 0x62 };
unsigned char AppSkey[16] = { 0x6e, 0x9b, 0x51, 0x67, 0x09, 0x92, 0xc2, 0x40, 0xac, 0x1c, 0xf4, 0xe2, 0x38, 0x9c, 0x8d, 0xf2 };
unsigned char DevAddr[4] = { 0x00, 0x00, 0x00, 0x07 };
unsigned char data[100];


void RFM_Message(){
  Message=1;
  }

void setup() {
  SPI.setDataMode(SPI_MODE0);
  Serial.begin(115200);
  while(!Serial);
  SPI.begin();

  pinMode(NSS, OUTPUT);
  pinMode(DIO0, INPUT);
  digitalWrite(NSS, HIGH);

  Serial.println("Inciando");
  //Initialize RFM module
  
  RFM_Init();
  RFM_Continuous_Receive();
}

void loop() {
  if(Message){
    Serial.println("Rx:");
    unsigned char RFM_Package_Location = 0x00;
    unsigned char RFM_Rx_Length = 0x00;
    if((RFM_Read(0x12) & 0x20) != 0x20){
      Serial.println("CRC Ok");
    }
    else{
      Serial.println("CRC No Ok");
    }
    RFM_Package_Location = RFM_Read(0x10); //Read start position of received package
    RFM_Rx_Length = RFM_Read(0x13);      //Read length of received package
    RFM_Write(0x0D,RFM_Package_Location); //Set SPI pointer to start of package
    for (unsigned char i = 0x00; i < RFM_Rx_Length; i++){
      data[i] = RFM_Read(0x00);
      Serial.print("Dato ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(data[i],HEX);
     }
     Message=0; 
     RFM_Write(0x12, 0xFF); //Clear interrupts
  }
  
  if (Serial.available()) {
   serialData= Serial.read();
   }
   if(serialData=='1'){ 
    uint16_t temp;
    uint8_t Data[2];
    uint8_t Data_Length = 0x02;

    temp = 50;
    Data[0] = (temp >> 8) & 0xff;
    Data[1] = temp & 0xff;

    LORA_Send_Data(Data, Data_Length, Frame_Counter_Tx);
    Frame_Counter_Tx++;
    Serial.println("Dato enviado");
    // reset sleep count
    //sleep_count = 0;
    serialData=0;
   }
   else if(serialData=='2'){
    Serial.print("Status: ");
    Serial.println(RFM_Read(0x18),HEX);
    Serial.print("IRQ: ");
    Serial.println(RFM_Read(0x12),HEX);
    Serial.print("Bytes of payload: ");
    Serial.println(RFM_Read(0x13),HEX);
    Serial.print("Valid headers received: ");
    Serial.print(RFM_Read(0x14),HEX);
    Serial.println(RFM_Read(0x14),HEX);
    Serial.print("Mensaje: ");
    Serial.println(Message);
    Serial.print("OpMode: ");
    Serial.println(RFM_Read(0x01),HEX);
    serialData=0;
    delay(100);
    }
  

}

/*
*****************************************************************************************
* Description: Function used to initialize the RFM module on startup
*****************************************************************************************
*/
void RFM_Init()
{ 
  int ver = RFM_Read(0x42);
  if(ver==18){
  Serial.print("SX1276 or RFM95 detected ");
  }
  else{
    Serial.println("Error detecting RFM");
    Serial.println("Check your connections");
    while(true);
  }
  //Switch RFM to sleep
  RFM_Write(0x01,0x00);

  //Set RFM in LoRa mode
  RFM_Write(0x01,0x80);
  //Set RFM in Standby mode wait on mode ready
  RFM_Write(0x01,0x81);
  
  delay(50);
  
  //Set carrair frequency
  // 868.100 MHz / 61.035 Hz = 14222987 = 0xD9068B
  // 904.400 Mhz / 61.035 Hz = 14866880 = 0xE2D9C0
  // 912.500 Mhz / 61.035 Hz = 14950438 = 0xE42026
  // 902.300 Mhz / 61.035 Hz = 14783321 = 0xE19359
  RFM_Write(0x06,0xE1);
  RFM_Write(0x07,0x93);
  RFM_Write(0x08,0x59);
  
  //PA pin (minimal power)
  RFM_Write(0x09,0xFF);

  //Switch LNA boost on
  //RFM_Write(0x0C,0x23);
  
  RFM_Write(0x1E,0xB4); //SF7 CRC On
  RFM_Write(0x1D,0x72); //125 kHz 4/5 coding rate explicit header mode
  RFM_Write(0x26,0x0C); //Low datarate optimization off AGC auto on 
  //Rx Timeout set to 37 symbols
  RFM_Write(0x1F,0x25);

  //Preamble length set to 8 symbols
  //0x0008 + 4 = 12
  RFM_Write(0x20,0x00);
  RFM_Write(0x21,0x08);

  //Set LoRa sync word
  RFM_Write(0x39,0x34);

  //Set IQ to normal values
  RFM_Write(0x33,0x27);
  RFM_Write(0x3B,0x1D);

  //Set FIFO pointers
  //TX base adress
  RFM_Write(0x0E,0x80);
  //Rx base adress
  RFM_Write(0x0F,0x00);

  //Switch RFM to sleep
  RFM_Write(0x01,0x00);
  Serial.println("Configured");
}



/*
*****************************************************************************************
* Description : Function for change just the bits of operation mode
*
* Arguments   : mode just use the 3 first bits
*               X000 = Sleep mode
*               X001 = Stdby mode
*               X010 = FS mode TX (FSTx)
*               X011 = Transmitter mode (Tx)
*               X100 = FS mode RX (FSRx) 
*               X101 = Receiver continuous mode
*               x110 = Receiver single mode
*               x111 = Channel activity detection
*****************************************************************************************
*/
void RFM_OpMode (int mode) {
    RFM_Write(0x01, 0x80 | mode); //
}


/*
*****************************************************************************************
* Description : Function to switch RFM to continuous receive mode, used for Class C 
*****************************************************************************************
*/
void RFM_Continuous_Receive(void)
{
  attachInterrupt(0, RFM_Message, RISING);
  //Clear all interrupts
  //RFM_Write(0x12, 0xFF);
  //Change DIO 0 back to RxDone
  RFM_Write(0x40,0x00);

  //Invert IQ Back
  RFM_Write(0x33,0x67);
  RFM_Write(0x3B,0x19);
  
  //Change Datarate
  RFM_Write(0x1E,0xA4); //SF10 CRC On
  RFM_Write(0x1D,0x72); //125 kHz 4/5 coding rate explicit header mode
  RFM_Write(0x26,0x04); //Low datarate optimization off AGC auto on

  //Set Hop frequency Period to 0
  RFM_Write(0x24,0x00);
  
  //Set carrair frequency
  // 904.400 Mhz / 61.035 Hz = 14866880 = 0xE2D9C0
  // 912.500 Mhz / 61.035 Hz = 14950438 = 0xE42026
  // 902.300 Mhz / 61.035 Hz = 14783321 = 0xE19359
  // 923.300 Mhz / 61.035 Hz = 15127386 = 0xE6D35A . 
  RFM_Write(0x06,0xE6);
  RFM_Write(0x07,0xD3);
  RFM_Write(0x08,0x5A);

  //Switch to continuous receive
  //Set operation mode to Rx continuous
  RFM_Write(0x01,0x85); 
  delay(50);

}




/*
*****************************************************************************************
* Description : Function contstructs a LoRaWAN package and sends it
*
* Arguments   : *Data pointer to the array of data that will be transmitted
*               Data_Length nuber of bytes to be transmitted
*               Frame_Counter_Up  Frame counter of upstream frames
*****************************************************************************************
*/
void LORA_Send_Data(unsigned char *Data, unsigned char Data_Length, unsigned int Frame_Counter_Tx)
{
  //Define variables
  unsigned char i;

  //Direction of frame is up
  unsigned char Direction = 0x00;

  unsigned char RFM_Data[64];
  unsigned char RFM_Package_Length;

  unsigned char MIC[4];

  //Unconfirmed data up
  unsigned char Mac_Header = 0x40;

  unsigned char Frame_Control = 0x00;
  unsigned char Frame_Port = 0x01;

  //Encrypt the data
  Encrypt_Payload(Data, Data_Length, Frame_Counter_Tx, Direction);


  //Build the Radio Package
  RFM_Data[0] = Mac_Header;

  RFM_Data[1] = DevAddr[3];
  RFM_Data[2] = DevAddr[2];
  RFM_Data[3] = DevAddr[1];
  RFM_Data[4] = DevAddr[0];

  RFM_Data[5] = Frame_Control;

  RFM_Data[6] = (Frame_Counter_Tx & 0x00FF);
  RFM_Data[7] = ((Frame_Counter_Tx >> 8) & 0x00FF);

  RFM_Data[8] = Frame_Port;

  //Set Current package length
  RFM_Package_Length = 9;

  //Load Data
  for(i = 0; i < Data_Length; i++)
  {
    RFM_Data[RFM_Package_Length + i] = Data[i];
  }

  //Add data Lenth to package length
  RFM_Package_Length = RFM_Package_Length + Data_Length;

  //Calculate MIC
  Calculate_MIC(RFM_Data, MIC, RFM_Package_Length, Frame_Counter_Tx, Direction);

  //Load MIC in package
  for(i = 0; i < 4; i++)
  {
    RFM_Data[i + RFM_Package_Length] = MIC[i];
  }

  //Add MIC length to RFM package length
  RFM_Package_Length = RFM_Package_Length + 4;

  //Send Package
  RFM_Send_Package(RFM_Data, RFM_Package_Length);
}


/*
*****************************************************************************************
* Description : Function for sending a package with the RFM
*
* Arguments   : *RFM_Tx_Package Pointer to arry with data to be send
*               Package_Length  Length of the package to send
*****************************************************************************************
*/

void RFM_Send_Package(unsigned char *RFM_Tx_Package, unsigned char Package_Length)
{
  detachInterrupt(0);
  unsigned char i;
  unsigned char RFM_Tx_Location = 0x00;
  
  //Set RFM in Standby mode wait on mode ready
  RFM_Write(0x01,0x81);
  //Clear all interrupts
  RFM_Write(0x12, 0xFF);
  while (digitalRead(DIO5) == LOW)
  {
    Serial.print(".");
  }

  
  delay(10);
  
  //Change DIO mapping to TxDone
  //Switch DIO0 to TxDone
  RFM_Write(0x40,0x40);

  //Set carrier frequency
  // 868.100 MHz / 61.035 Hz = 14222987 = 0xD9068B
  // 904.400 Mhz / 61.035 Hz = 14866880 = 0xE2D9C0
  // 912.500 Mhz / 61.035 Hz = 14950438 = 0xE42026
  // 902.300 Mhz / 61.035 Hz = 14783321 = 0xE19359
  RFM_Write(0x06,0xE4);
  RFM_Write(0x07,0x20);
  RFM_Write(0x08,0x26);

  //SF7 BW 125 kHz
  RFM_Write(0x1E,0x74); //SF7 CRC On
  RFM_Write(0x1D,0x72); //125 kHz 4/5 coding rate explicit header mode
  RFM_Write(0x26,0x04); //Low datarate optimization off AGC auto on

  //Set IQ to normal values
  RFM_Write(0x33,0x27);
  RFM_Write(0x3B,0x1D);

  //Set payload length to the right length
  RFM_Write(0x22,Package_Length);

  //Set SPI pointer to start of Tx part in FiFo
  //RFM_Write(0x0D,RFM_Tx_Location);
  RFM_Write(0x0D,0x80); // hardcoded fifo location according RFM95 specs

  //Write Payload to FiFo
  for (i = 0;i < Package_Length; i++)
  {
    RFM_Write(0x00,*RFM_Tx_Package);
    RFM_Tx_Package++;
  }

  //Switch RFM to Tx
  RFM_Write(0x01,0x83);

  //Wait for TxDone
  while(digitalRead(DIO0) == LOW)
  {
    //Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Dato enviado TxDone: ");
  Serial.println(RFM_Read(0x12),HEX);
  Serial.println();
  //Clear all RegIrqFlags 
  RFM_Write(0x12, 0xFF); 
  //Switch RFM to Rx contuously
  RFM_Continuous_Receive();
  Message=0;
}


/*
*****************************************************************************************
* Description : Funtion that toggle the NSS and configure the SPI
*
* Arguments   : val is the state of the NSS you want to write
*               to start or finish a the communication
*****************************************************************************************
*/
void RFM_NSS(bool val){
  if (!val)
        SPI.beginTransaction(SPISettings(1E6, MSBFIRST, SPI_MODE0));
    else
        SPI.endTransaction();
    digitalWrite(NSS, val);
  
  }


/*
*****************************************************************************************
* Description : Funtion that writes a register from the RFM
*
* Arguments   : RFM_Address Address of register to be written
*         RFM_Data    Data to be written
*****************************************************************************************
*/
void RFM_Write(unsigned char RFM_Address, unsigned char RFM_Data)
{
  //Set NSS pin Low to start communication
  //digitalWrite(NSS,LOW);
  RFM_NSS(0);

  //Send Addres with MSB 1 to make it a writ command
  SPI.transfer(RFM_Address | 0x80);
  //Send Data
  SPI.transfer(RFM_Data);

  //Set NSS pin High to end communication
  RFM_NSS(1);
  //digitalWrite(NSS,HIGH);
}

/*
*****************************************************************************************
* Description : Funtion that reads a register from the RFM
*
* Arguments   : RFM_Address Address of register to be read
*****************************************************************************************
*/
int RFM_Read(int addr) {
    RFM_NSS(0);
    SPI.transfer(addr & 0x7F);
    int val = SPI.transfer(0x00);
    RFM_NSS(1);
    return val;
}




/*
*****************************************************************************************
* Description : Function to know the status of the LoRA Modem on the Rx time 
*               It may return 5 bits:  
*                                    4 - Modem clear 
*                                    3 - Header info calid
*                                    2 - Rx on-going
*                                    1 - Signal synchronized
*                                    0 - Signal detected
*****************************************************************************************
*/
int RFM_Status(void){
  return RFM_Read(0x18)&0x1F;
  }




/******************************************Starts AES shit**********************************************/



void Encrypt_Payload(unsigned char *Data, unsigned char Data_Length, unsigned int Frame_Counter, unsigned char Direction)
{
  unsigned char i = 0x00;
  unsigned char j;
  unsigned char Number_of_Blocks = 0x00;
  unsigned char Incomplete_Block_Size = 0x00;

  unsigned char Block_A[16];

  //Calculate number of blocks
  Number_of_Blocks = Data_Length / 16;
  Incomplete_Block_Size = Data_Length % 16;
  if(Incomplete_Block_Size != 0)
  {
    Number_of_Blocks++;
  }

  for(i = 1; i <= Number_of_Blocks; i++)
  {
    Block_A[0] = 0x01;
    Block_A[1] = 0x00;
    Block_A[2] = 0x00;
    Block_A[3] = 0x00;
    Block_A[4] = 0x00;

    Block_A[5] = Direction;

    Block_A[6] = DevAddr[3];
    Block_A[7] = DevAddr[2];
    Block_A[8] = DevAddr[1];
    Block_A[9] = DevAddr[0];

    Block_A[10] = (Frame_Counter & 0x00FF);
    Block_A[11] = ((Frame_Counter >> 8) & 0x00FF);

    Block_A[12] = 0x00; //Frame counter upper Bytes
    Block_A[13] = 0x00;

    Block_A[14] = 0x00;

    Block_A[15] = i;

    //Calculate S
    AES_Encrypt(Block_A,AppSkey); //original


    //Check for last block
    if(i != Number_of_Blocks)
    {
      for(j = 0; j < 16; j++)
      {
        *Data = *Data ^ Block_A[j];
        Data++;
      }
    }
    else
    {
      if(Incomplete_Block_Size == 0)
      {
        Incomplete_Block_Size = 16;
      }
      for(j = 0; j < Incomplete_Block_Size; j++)
      {
        *Data = *Data ^ Block_A[j];
        Data++;
      }
    }
  }
}

void Calculate_MIC(unsigned char *Data, unsigned char *Final_MIC, unsigned char Data_Length, unsigned int Frame_Counter, unsigned char Direction)
{
  unsigned char i;
  unsigned char Block_B[16];

  unsigned char Key_K1[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  unsigned char Key_K2[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  //unsigned char Data_Copy[16];

  unsigned char Old_Data[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  unsigned char New_Data[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };


  unsigned char Number_of_Blocks = 0x00;
  unsigned char Incomplete_Block_Size = 0x00;
  unsigned char Block_Counter = 0x01;

  //Create Block_B
  Block_B[0] = 0x49;
  Block_B[1] = 0x00;
  Block_B[2] = 0x00;
  Block_B[3] = 0x00;
  Block_B[4] = 0x00;

  Block_B[5] = Direction;

  Block_B[6] = DevAddr[3];
  Block_B[7] = DevAddr[2];
  Block_B[8] = DevAddr[1];
  Block_B[9] = DevAddr[0];

  Block_B[10] = (Frame_Counter & 0x00FF);
  Block_B[11] = ((Frame_Counter >> 8) & 0x00FF);

  Block_B[12] = 0x00; //Frame counter upper bytes
  Block_B[13] = 0x00;

  Block_B[14] = 0x00;
  Block_B[15] = Data_Length;

  //Calculate number of Blocks and blocksize of last block
  Number_of_Blocks = Data_Length / 16;
  Incomplete_Block_Size = Data_Length % 16;

  if(Incomplete_Block_Size != 0)
  {
    Number_of_Blocks++;
  }

  Generate_Keys(Key_K1, Key_K2);

  //Preform Calculation on Block B0

  //Preform AES encryption
  AES_Encrypt(Block_B,NwkSkey);

  //Copy Block_B to Old_Data
  for(i = 0; i < 16; i++)
  {
    Old_Data[i] = Block_B[i];
  }

  //Preform full calculating until n-1 messsage blocks
  while(Block_Counter < Number_of_Blocks)
  {
    //Copy data into array
    for(i = 0; i < 16; i++)
    {
      New_Data[i] = *Data;
      Data++;
    }

    //Preform XOR with old data
    XOR(New_Data,Old_Data);

    //Preform AES encryption
    AES_Encrypt(New_Data,NwkSkey);

    //Copy New_Data to Old_Data
    for(i = 0; i < 16; i++)
    {
      Old_Data[i] = New_Data[i];
    }

    //Raise Block counter
    Block_Counter++;
  }

  //Perform calculation on last block
  //Check if Datalength is a multiple of 16
  if(Incomplete_Block_Size == 0)
  {
    //Copy last data into array
    for(i = 0; i < 16; i++)
    {
      New_Data[i] = *Data;
      Data++;
    }

    //Preform XOR with Key 1
    XOR(New_Data,Key_K1);

    //Preform XOR with old data
    XOR(New_Data,Old_Data);

    //Preform last AES routine
    // read NwkSkey from PROGMEM
    AES_Encrypt(New_Data,NwkSkey);
  }
  else
  {
    //Copy the remaining data and fill the rest
    for(i =  0; i < 16; i++)
    {
      if(i < Incomplete_Block_Size)
      {
        New_Data[i] = *Data;
        Data++;
      }
      if(i == Incomplete_Block_Size)
      {
        New_Data[i] = 0x80;
      }
      if(i > Incomplete_Block_Size)
      {
        New_Data[i] = 0x00;
      }
    }

    //Preform XOR with Key 2
    XOR(New_Data,Key_K2);

    //Preform XOR with Old data
    XOR(New_Data,Old_Data);

    //Preform last AES routine
    AES_Encrypt(New_Data,NwkSkey);
  }

  Final_MIC[0] = New_Data[0];
  Final_MIC[1] = New_Data[1];
  Final_MIC[2] = New_Data[2];
  Final_MIC[3] = New_Data[3];
}

void Generate_Keys(unsigned char *K1, unsigned char *K2)
{
  unsigned char i;
  unsigned char MSB_Key;

  //Encrypt the zeros in K1 with the NwkSkey
  AES_Encrypt(K1,NwkSkey);

  //Create K1
  //Check if MSB is 1
  if((K1[0] & 0x80) == 0x80)
  {
    MSB_Key = 1;
  }
  else
  {
    MSB_Key = 0;
  }

  //Shift K1 one bit left
  Shift_Left(K1);

  //if MSB was 1
  if(MSB_Key == 1)
  {
    K1[15] = K1[15] ^ 0x87;
  }

  //Copy K1 to K2
  for( i = 0; i < 16; i++)
  {
    K2[i] = K1[i];
  }

  //Check if MSB is 1
  if((K2[0] & 0x80) == 0x80)
  {
    MSB_Key = 1;
  }
  else
  {
    MSB_Key = 0;
  }

  //Shift K2 one bit left
  Shift_Left(K2);

  //Check if MSB was 1
  if(MSB_Key == 1)
  {
    K2[15] = K2[15] ^ 0x87;
  }
}

void Shift_Left(unsigned char *Data)
{
  unsigned char i;
  unsigned char Overflow = 0;
  //unsigned char High_Byte, Low_Byte;

  for(i = 0; i < 16; i++)
  {
    //Check for overflow on next byte except for the last byte
    if(i < 15)
    {
      //Check if upper bit is one
      if((Data[i+1] & 0x80) == 0x80)
      {
        Overflow = 1;
      }
      else
      {
        Overflow = 0;
      }
    }
    else
    {
      Overflow = 0;
    }

    //Shift one left
    Data[i] = (Data[i] << 1) + Overflow;
  }
}

void XOR(unsigned char *New_Data,unsigned char *Old_Data)
{
  unsigned char i;

  for(i = 0; i < 16; i++)
  {
    New_Data[i] = New_Data[i] ^ Old_Data[i];
  }
}

/*
*****************************************************************************************
* Title         : AES_Encrypt
* Description  :
*****************************************************************************************
*/
void AES_Encrypt(unsigned char *Data, unsigned char *Key)
{
  unsigned char Row, Column, Round = 0;
  unsigned char Round_Key[16];
    unsigned char State[4][4];

  //  Copy input to State arry
  for( Column = 0; Column < 4; Column++ )
  {
    for( Row = 0; Row < 4; Row++ )
    {
      State[Row][Column] = Data[Row + (Column << 2)];
    }
  }

  //  Copy key to round key
  memcpy( &Round_Key[0], &Key[0], 16 );

  //  Add round key
  AES_Add_Round_Key( Round_Key, State );

  //  Preform 9 full rounds with mixed collums
  for( Round = 1 ; Round < 10 ; Round++ )
  {
    //  Perform Byte substitution with S table
    for( Column = 0 ; Column < 4 ; Column++ )
    {
      for( Row = 0 ; Row < 4 ; Row++ )
      {
        State[Row][Column] = AES_Sub_Byte( State[Row][Column] );
      }
    }

    //  Perform Row Shift
    AES_Shift_Rows(State);

    //  Mix Collums
    AES_Mix_Collums(State);

    //  Calculate new round key
    AES_Calculate_Round_Key(Round, Round_Key);

        //  Add the round key to the Round_key
    AES_Add_Round_Key(Round_Key, State);
  }

  //  Perform Byte substitution with S table whitout mix collums
  for( Column = 0 ; Column < 4 ; Column++ )
  {
    for( Row = 0; Row < 4; Row++ )
    {
      State[Row][Column] = AES_Sub_Byte(State[Row][Column]);
    }
  }

  //  Shift rows
  AES_Shift_Rows(State);

  //  Calculate new round key
  AES_Calculate_Round_Key( Round, Round_Key );

    //  Add round key
  AES_Add_Round_Key( Round_Key, State );

  //  Copy the State into the data array
  for( Column = 0; Column < 4; Column++ )
  {
    for( Row = 0; Row < 4; Row++ )
    {
      Data[Row + (Column << 2)] = State[Row][Column];
    }
  }
} // AES_Encrypt


/*
*****************************************************************************************
* Title         : AES_Add_Round_Key
* Description :
*****************************************************************************************
*/
void AES_Add_Round_Key(unsigned char *Round_Key, unsigned char (*State)[4])
{
  unsigned char Row, Collum;

  for(Collum = 0; Collum < 4; Collum++)
  {
    for(Row = 0; Row < 4; Row++)
    {
      State[Row][Collum] ^= Round_Key[Row + (Collum << 2)];
    }
  }
} // AES_Add_Round_Key


/*
*****************************************************************************************
* Title         : AES_Sub_Byte
* Description :
*****************************************************************************************
*/
unsigned char AES_Sub_Byte(unsigned char Byte){
//  unsigned char S_Row,S_Collum;
//  unsigned char S_Byte;
//
//  S_Row    = ((Byte >> 4) & 0x0F);
//  S_Collum = ((Byte >> 0) & 0x0F);
//  S_Byte   = S_Table [S_Row][S_Collum];

  //return S_Table [ ((Byte >> 4) & 0x0F) ] [ ((Byte >> 0) & 0x0F) ]; // original
  return pgm_read_byte(&(S_Table [((Byte >> 4) & 0x0F)] [((Byte >> 0) & 0x0F)]));
} //    AES_Sub_Byte


/*
*****************************************************************************************
* Title         : AES_Shift_Rows
* Description :
*****************************************************************************************
*/
void AES_Shift_Rows(unsigned char (*State)[4])
{
  unsigned char Buffer;

  //Store firt byte in buffer
  Buffer      = State[1][0];
  //Shift all bytes
  State[1][0] = State[1][1];
  State[1][1] = State[1][2];
  State[1][2] = State[1][3];
  State[1][3] = Buffer;

  Buffer      = State[2][0];
  State[2][0] = State[2][2];
  State[2][2] = Buffer;
  Buffer      = State[2][1];
  State[2][1] = State[2][3];
  State[2][3] = Buffer;

  Buffer      = State[3][3];
  State[3][3] = State[3][2];
  State[3][2] = State[3][1];
  State[3][1] = State[3][0];
  State[3][0] = Buffer;
}   //  AES_Shift_Rows


/*
*****************************************************************************************
* Title         : AES_Mix_Collums
* Description :
*****************************************************************************************
*/
void AES_Mix_Collums(unsigned char (*State)[4])
{
  unsigned char Row,Collum;
  unsigned char a[4], b[4];


  for(Collum = 0; Collum < 4; Collum++)
  {
    for(Row = 0; Row < 4; Row++)
    {
      a[Row] =  State[Row][Collum];
      b[Row] = (State[Row][Collum] << 1);

      if((State[Row][Collum] & 0x80) == 0x80)
      {
        b[Row] ^= 0x1B;
      }
    }

    State[0][Collum] = b[0] ^ a[1] ^ b[1] ^ a[2] ^ a[3];
    State[1][Collum] = a[0] ^ b[1] ^ a[2] ^ b[2] ^ a[3];
    State[2][Collum] = a[0] ^ a[1] ^ b[2] ^ a[3] ^ b[3];
    State[3][Collum] = a[0] ^ b[0] ^ a[1] ^ a[2] ^ b[3];
  }
}   //  AES_Mix_Collums



/*
*****************************************************************************************
* Title         : AES_Calculate_Round_Key
* Description :
*****************************************************************************************
*/
void AES_Calculate_Round_Key(unsigned char Round, unsigned char *Round_Key)
{
  unsigned char i, j, b, Rcon;
  unsigned char Temp[4];


    //Calculate Rcon
  Rcon = 0x01;
  while(Round != 1)
  {
    b = Rcon & 0x80;
    Rcon = Rcon << 1;

    if(b == 0x80)
    {
      Rcon ^= 0x1b;
    }
    Round--;
  }

  //  Calculate first Temp
  //  Copy laste byte from previous key and subsitute the byte, but shift the array contents around by 1.
    Temp[0] = AES_Sub_Byte( Round_Key[12 + 1] );
    Temp[1] = AES_Sub_Byte( Round_Key[12 + 2] );
    Temp[2] = AES_Sub_Byte( Round_Key[12 + 3] );
    Temp[3] = AES_Sub_Byte( Round_Key[12 + 0] );

  //  XOR with Rcon
  Temp[0] ^= Rcon;

  //  Calculate new key
  for(i = 0; i < 4; i++)
  {
    for(j = 0; j < 4; j++)
    {
      Round_Key[j + (i << 2)]  ^= Temp[j];
      Temp[j]                   = Round_Key[j + (i << 2)];
    }
  }
}   //  AES_Calculate_Round_Key
