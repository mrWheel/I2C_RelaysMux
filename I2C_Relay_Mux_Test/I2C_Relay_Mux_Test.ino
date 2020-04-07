/*
***************************************************************************
**
**  Program     : I2C_Mux_Test
*/
#define _FW_VERSION  "v0.4 (05-04-2020)"
/*
**  Description : Test FTM I2C Relay Multiplexer
**
**  Copyright (c) 2020 Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.
***************************************************************************
*/


#define I2C_MUX_ADDRESS      0x48    // the 7-bit address 
//#define _SDA                  4
//#define _SCL                  5
#define LED_ON                  0
#define LED_OFF               255
#define TESTPIN                13
#define LED_RED                12
#define LED_GREEN              13
#define LED_WHITE              14

#define LOOP_INTERVAL        1000
#define PROFILING

//#define Debugf      Serial.printf

#include "I2C_MuxLib.h"


I2CMUX  relay; //Create instance of the I2CMUX object

byte          actI2Caddress = I2C_MUX_ADDRESS;
byte          whoAmI;
byte          majorRelease, minorRelease;
bool          I2C_MuxConnected;
int           count4Wait = 0;
int           numRelays; 
uint32_t      loopTimer, switchBoardTimer, builtinLedTimer, ledRedTimer, ledGreenTimer, ledWhiteTimer;
uint32_t      offSet;
bool          loopTestOn = false;
byte          loopState = 0;
String        command;

//===========================================================================================
void requestEvent()
{
  // nothing .. yet
}

//===========================================================================================
//assumes little endian
void printRegister(size_t const size, void const * const ptr)
{
  unsigned char *b = (unsigned char*) ptr;
  unsigned char byte;
  int i, j;
  Serial.print(F("["));
  for (i=size-1; i>=0; i--) {
    for (j=7; j>=0; j--) {
      byte = (b[i] >> j) & 1;
      Serial.print(byte);
    }
  }
  Serial.print(F("] "));
} // printRegister()


//===========================================================================================
byte findSlaveAddress(byte startAddress)
{
  byte  error;
  bool  slaveFound = false;

  if (startAddress == 0xFF) startAddress = 1;
  Serial.print("Scan I2C addresses ...");
  for (byte address = startAddress; address < 127; address++) {
    //Serial.print("test address [0x"); Serial.print(address, HEX); Serial.println("]");
    //Serial.flush();
    Serial.print(".");
    
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error) {
      //Serial.print(F("-> Error["));
      //Serial.print(error);
      //Serial.println(F("]"));
    } else {
      slaveFound = true;
      Serial.print(F("\nFound one!\n at @[0x"));
      Serial.print(address , HEX);
      Serial.print(F("]"));
      return address;
    }
  }
  return 0xFF;

} // findSlaveAddress()


//===========================================================================================
void displayPinState()
{
  Serial.print("  Pin: ");
  for (int p=1; p<=numRelays; p++) {
    Serial.print(p % 10);
  }
  Serial.println();

  Serial.print("State: ");
  for (int p=1; p<=numRelays; p++) {
    int pState = relay.digitalRead(p);
    if (pState == HIGH)
          Serial.print("H");
    else  Serial.print("L");
  }
  Serial.println();
  
} //  displayPinState()


//===========================================================================================
void loopRelays()
{
    whoAmI       = relay.getWhoAmI();
    if (whoAmI != I2C_MUX_ADDRESS && whoAmI != 0x24) {
      Serial.println("No connection with Multiplexer .. abort!");
      loopTestOn = false;
      return;
    }
    loopState++;
    for (int i=0; i<8; i++)
    {
      if (loopState & (1<<i)) relay.digitalWrite((i+1), HIGH);
      else                    relay.digitalWrite((i+1), LOW);
    }

} // loopRelays()


//===========================================================================================
bool Mux_Status()
{
  whoAmI       = relay.getWhoAmI();
  Serial.print("whoAmI[0x"); Serial.print(whoAmI, HEX); Serial.println("]");
  if (whoAmI != I2C_MUX_ADDRESS && whoAmI != 0x24) {
    Serial.print("whoAmI returned [0x"); Serial.print(whoAmI, HEX); Serial.println("]");
    return false;
  }
  displayPinState();
  //Serial.println("getMajorRelease() ..");
  majorRelease = relay.getMajorRelease();
  //Serial.println("getMinorRelease() ..");
  minorRelease = relay.getMinorRelease();
    
  Serial.print("\nSlave say's he's [0x");Serial.print(whoAmI, HEX); 
  Serial.print("] Release[");            Serial.print(majorRelease);
  Serial.print(".");                     Serial.print(minorRelease);
  Serial.println("]");
  numRelays = relay.getNumRelays();
  Serial.print("Board has [");  Serial.print(numRelays);
  Serial.println("] relays\r\n"); 

} // Mux_Status()



//===========================================================================================
void help()
{
  Serial.println();
  Serial.println(F("  Commands are:"));
  Serial.println(F("    n=1;         -> sets relay n to 'closed'"));
  Serial.println(F("    n=0;         -> sets relay n to 'open'"));
  Serial.println(F("    all=1;       -> sets all relay's to 'closed'"));
  Serial.println(F("    all=0;       -> sets all relay's to 'open'"));
  Serial.println(F("    adres=48;    -> sets I2C address to 0x48"));
  Serial.println(F("    adres=24;    -> sets I2C address to 0x24"));
  Serial.println(F("    board=8;     -> set's board to 8 relay's"));
  Serial.println(F("    board=16;    -> set's board to 16 relay's"));
  Serial.println(F("    status;      -> I2C mux status"));
  Serial.println(F("    pinstate;    -> List's state of all relay's"));
  Serial.println(F("    looptest;    -> looping"));
  Serial.println(F("    testrelays;  -> longer test"));
  Serial.println(F("    whoami;      -> shows I2C address Slave MUX"));
  Serial.println(F("    writeconfig; -> write config to eeprom"));
  Serial.println(F("    reboot;      -> reboot I2C Mux"));
  Serial.println(F("  * reScan;      -> re-scan I2C devices"));

} // help()


//===========================================================================================
void readCommand()
{
  String command = "";
  if (!Serial.available()) {
    return;
  }
  Serial.setTimeout(500);  // ten seconds
  command = Serial.readStringUntil(';');
  command.toLowerCase();
  command.trim();
  for (int i = 0; i < command.length(); i++)
  {
    if (command[i] < ' ' || command[i] > '~') command[i] = 0;
  }

  loopTestOn = false;
  
  if (command.length() < 1) { help(); return; }
  Serial.print("command["); Serial.print(command); Serial.println("]");
  
  if (command == "adres=48") {actI2Caddress = 0x48; relay.setI2Caddress(actI2Caddress); }
  if (command == "adres=24") {actI2Caddress = 0x24; relay.setI2Caddress(actI2Caddress); }
  if (command == "board=8")  {numRelays = 8;  relay.setNumRelays(numRelays); }
  if (command == "board=16") {numRelays = 16; relay.setNumRelays(numRelays); }
  if (command == "0=1")       relay.digitalWrite(0,  HIGH); 
  if (command == "0=0")       relay.digitalWrite(0,  LOW); 
  if (command == "1=1")       relay.digitalWrite(1,  HIGH); 
  if (command == "1=0")       relay.digitalWrite(1,  LOW); 
  if (command == "2=1")       relay.digitalWrite(2,  HIGH); 
  if (command == "2=0")       relay.digitalWrite(2,  LOW); 
  if (command == "3=1")       relay.digitalWrite(3,  HIGH); 
  if (command == "3=0")       relay.digitalWrite(3,  LOW); 
  if (command == "4=1")       relay.digitalWrite(4,  HIGH); 
  if (command == "4=0")       relay.digitalWrite(4,  LOW); 
  if (command == "5=1")       relay.digitalWrite(5,  HIGH); 
  if (command == "5=0")       relay.digitalWrite(5,  LOW); 
  if (command == "6=1")       relay.digitalWrite(6,  HIGH); 
  if (command == "6=0")       relay.digitalWrite(6,  LOW); 
  if (command == "7=1")       relay.digitalWrite(7,  HIGH); 
  if (command == "7=0")       relay.digitalWrite(7,  LOW); 
  if (command == "8=1")       relay.digitalWrite(8,  HIGH); 
  if (command == "8=0")       relay.digitalWrite(8,  LOW); 
  if (command == "9=1")       relay.digitalWrite(9,  HIGH); 
  if (command == "9=0")       relay.digitalWrite(9,  LOW); 
  if (command == "10=1")      relay.digitalWrite(10, HIGH); 
  if (command == "10=0")      relay.digitalWrite(10, LOW); 
  if (command == "11=1")      relay.digitalWrite(11, HIGH); 
  if (command == "11=0")      relay.digitalWrite(11, LOW); 
  if (command == "12=1")      relay.digitalWrite(12, HIGH); 
  if (command == "12=0")      relay.digitalWrite(12, LOW); 
  if (command == "13=1")      relay.digitalWrite(13, HIGH); 
  if (command == "13=0")      relay.digitalWrite(13, LOW); 
  if (command == "14=1")      relay.digitalWrite(14, HIGH); 
  if (command == "14=0")      relay.digitalWrite(14, LOW); 
  if (command == "15=1")      relay.digitalWrite(15, HIGH); 
  if (command == "15=0")      relay.digitalWrite(15, LOW); 
  if (command == "16=1")      relay.digitalWrite(16, HIGH); 
  if (command == "16=0")      relay.digitalWrite(16, LOW); 
  if (command == "all=1")
  {
    for (int i=1; i<=numRelays; i++) relay.digitalWrite(i, HIGH); 
  }
  if (command == "all=0")
  {
    for (int i=1; i<=numRelays; i++) relay.digitalWrite(i, LOW); 
  }
  if (command == "status")      Mux_Status();
  if (command == "pinstate")    displayPinState();
  if (command == "looptest")    loopTestOn = true;
  if (command == "testrelays")  relay.writeCommand(1<<CMD_TESTRELAYS);
  if (command == "whoami")      Serial.println(relay.getWhoAmI(), HEX);
  if (command == "readconfig")  relay.writeCommand(1<<CMD_READCONF);
  if (command == "writeconfig") relay.writeCommand(1<<CMD_WRITECONF);
  if (command == "reboot")      relay.writeCommand(1<<CMD_REBOOT);
  if (command == "help")        help();
  if (command == "rescan")      setup();
  
} // readCommand()


//===========================================================================================
void setup()
{
  Serial.begin(115200);
  Serial.println(F("\r\nStart I2C-Relay-Multiplexer Test ....\r\n"));
  Wire.end(); // in case of a reScan..
  Serial.print(F("Setup Wire .."));
//Wire.begin(_SDA, _SCL); // join i2c bus (address optional for master)
  Wire.begin();
//Wire.setClock(100000L); // <-- don't make this 400000. It won't work
  Serial.println(F(".. done"));
  Serial.flush();

  actI2Caddress = findSlaveAddress(1);
  if (actI2Caddress != 0xFF) {
    Serial.print(F("\nConnecting to  I2C-relay .."));
    if (relay.begin(Wire, actI2Caddress)) {
      majorRelease = relay.getMajorRelease();
      minorRelease = relay.getMinorRelease();
      Serial.print(F(". connected with slave @[0x"));
      Serial.print(actI2Caddress, HEX);
      Serial.print(F("] Release[v"));
      Serial.print(majorRelease);
      Serial.print(F("."));
      Serial.print(minorRelease);
      Serial.println(F("]"));
      actI2Caddress = relay.getWhoAmI();
      I2C_MuxConnected = true;
    } else {
      Serial.println(F(".. Error connecting to I2C slave .."));
      I2C_MuxConnected = false;
    }
  }

  loopTimer = millis() + LOOP_INTERVAL;
  Mux_Status();
  Serial.println(F("setup() done .. \n"));

  help();

} // setup()


//===========================================================================================
void loop()
{
  if (loopTestOn)
  {
    if ((millis() - loopTimer) > LOOP_INTERVAL) 
    {
      loopTimer = millis();
      loopRelays();      
    }
  }

  readCommand();
   
} // loop()

/***************************************************************************
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the
* following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
* OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
* THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
***************************************************************************/
