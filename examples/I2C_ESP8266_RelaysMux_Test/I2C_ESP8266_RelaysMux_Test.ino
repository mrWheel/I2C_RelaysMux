/*
***************************************************************************
**
**  Program     : I2C_ESP8266_RelaysMux_Test
*/
#define _FW_VERSION  "v1.0 (15-04-2020)"
/*
**  Description : Test I2C Relay Multiplexer
**
**  Copyright (c) 2020 Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.
***************************************************************************
*/

/*
**  Connect I2C_RelaysMux to the Arduino UNI 
**  
**                                +---------------------
**   -------------------\         |
**     8/16 relays  GND o---------o GND
**     board        SCL o---------o GPIO-05    ESP8266
**     with         SDA o---------o GPIO-04
**     I2C_Mux      Vin o---------o 3v3
**   -------------------/         |
**                                +---------------------
**
**  You can enter commands, terminated by ";" and [Enter] in the 
**  Serial Monitor and watch the relays react.
**  For an 8 relays board you should enter "board=8" one time
**  before doing anything else. This setting is saved in EEPROM and
**  you never have to enter this again.
**  Same goes for the I2C address of the I2C_RelaysMux. You can  
**  change this setting with the I2CME.setI2Caddress(newAddress)
**   method.
**  The newAddress will also be saved in EEPROM and used the next
**  time you (re)boot the I2C_Multiplexer.
**  
**  You can connect to the ESP8266 by telnet (PuTTY) on port 23 or
**  by entering the IP address in your browser.
*/


#define I2C_MUX_ADDRESS      0x48    // the 7-bit address 
//#define _SDA                  4
//#define _SCL                  5

#define LOOP_INTERVAL        1000
#define INACTIVE_TIME      300000

#include <I2C_RelaysMux.h>

#include <TelnetStream.h>       // https://github.com/jandrassy/TelnetStream/commit/1294a9ee5cc9b1f7e51005091e351d60c8cddecf

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>   // Version 1.0.0 - part of ESP8266 Core https://github.com/esp8266/Arduino
#include <ESP8266mDNS.h>        // part of ESP8266 Core https://github.com/esp8266/Arduino
#include <WiFiUdp.h>            // part of ESP8266 Core https://github.com/esp8266/Arduino

#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK  "your-password"
#endif

const char* ssid     = STASSID;
const char* password = STAPSK;

unsigned long startTime = millis();

I2CMUX            relay; //Create instance of the I2CMUX object
ESP8266WebServer  httpServer (80);

byte          actI2Caddress = I2C_MUX_ADDRESS;
byte          whoAmI;
byte          majorRelease, minorRelease;
bool          I2C_MuxConnected;
int           numRelays; 
uint32_t      loopTimer, inactiveTimer;
bool          loopTestOn = false;
uint16_t      loopRegister = 0;
String        command;
byte          commandBy = 1;  // 1= Serial, 2=TelnetStream



//===========================================================================================
void startTelnet() 
{
  TelnetStream.begin();
  Serial.println(F("\nTelnet server started .."));
  TelnetStream.flush();

} // startTelnet()


//=======================================================================
void startMDNS(const char *Hostname) 
{
  Serial.printf("[1] mDNS setup as [%s.local]\r\n", Hostname);
  if (MDNS.begin(Hostname))               // Start the mDNS responder for Hostname.local
  {
    Serial.printf("[2] mDNS responder started as [%s.local]\r\n", Hostname);
  } 
  else 
  {
    Serial.println(F("[3] Error setting up MDNS responder!\r\n"));
  }
  MDNS.addService("http", "tcp", 80);
  
} // startMDNS()


//===========================================================================================
byte findSlaveAddress(Stream *sOut, byte startAddress)
{
  byte  error;
  bool  slaveFound = false;

  if (startAddress == 0xFF) startAddress = 1;
  sOut->print("Scan I2C addresses ...");
  for (byte address = startAddress; address < 127; address++) {
    //sOut->print("test address [0x"); sOut->print(address, HEX); sOut->println("]");
    //sOut->flush();
    sOut->print(".");
    
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error) {
      //sOut->print(F("-> Error["));
      //sOut->print(error);
      //sOut->println(F("]"));
    } else {
      slaveFound = true;
      sOut->print(F("\nFound one!\n at @[0x"));
      sOut->print(address , HEX);
      sOut->print(F("]"));
      return address;
    }
    yield();
    delay(25);
  }
  return 0xFF;

} // findSlaveAddress()


//===========================================================================================
void setLoopRegister()
{
  loopRegister = 0;  
  for (int p=1; p<=numRelays; p++) {
    if (relay.digitalRead(p) == 1)
    {
      loopRegister |= (1<< (p-1));
    }
  }
  
} // setLoopRegister()


//===========================================================================================
void displayPinState(Stream *sOut)
{
  sOut->print("  Pin: ");
  for (int p=numRelays; p>=1; p--) {
    sOut->print(p % 10);
  }
  sOut->println();

  sOut->print("State: ");
  for (int p=numRelays; p>=1; p--) {
    int pState = relay.digitalRead(p);
    if (pState == HIGH) sOut->print("H");
    else                sOut->print("L");
  }
  sOut->println();
  
} //  displayPinState()


//===========================================================================================
uint16_t rightRotate(uint16_t n, uint16_t d, byte s) 
{ 
   return (n >> d)|(n << (s - d)); 
   
} // rightRotate()


//===========================================================================================
void loopRelays()
{
    inactiveTimer = millis();

//  whoAmI       = relay.getWhoAmI();
//  if (whoAmI != I2C_MUX_ADDRESS && whoAmI != 0x24) {
//    Serial.println("No connection with Multiplexer .. abort!");
//    TelnetStream.println("No connection with Multiplexer .. abort!");
//    loopTestOn = false;
//    return;
//  }
    numRelays    = relay.getNumRelays();
    if (loopRegister == 0) loopRegister = 7;
    loopRegister = rightRotate(loopRegister, 1, numRelays);

    for (int i=0; i<numRelays; i++)
    {
      if (loopRegister & (1<<i)) relay.digitalWrite((i+1), HIGH);
      else                       relay.digitalWrite((i+1), LOW);
    }
    for (int i=(numRelays+1); i<=16; i++)
    {
      loopRegister &= ~(1<< (i-1));
      relay.digitalWrite((i+1), LOW);
    }
    relay.showRegister(sizeof(loopRegister), &loopRegister, &Serial);
    relay.showRegister(sizeof(loopRegister), &loopRegister, &TelnetStream);

} // loopRelays()


//===========================================================================================
bool Mux_Status(Stream *sOut)
{
  whoAmI       = relay.getWhoAmI();
  sOut->print("whoAmI[0x"); sOut->print(whoAmI, HEX); sOut->println("]");
  if (whoAmI != I2C_MUX_ADDRESS && whoAmI != 0x24) {
    sOut->print("whoAmI returned [0x"); sOut->print(whoAmI, HEX); sOut->println("]");
    return false;
  }
  displayPinState(sOut);
  //Serial.println("getMajorRelease() ..");
  majorRelease = relay.getMajorRelease();
  //Serial.println("getMinorRelease() ..");
  minorRelease = relay.getMinorRelease();
    
  sOut->print("\nSlave say's he's [0x");sOut->print(whoAmI, HEX); 
  sOut->print("] Release[");            sOut->print(majorRelease);
  sOut->print(".");                     sOut->print(minorRelease);
  sOut->println("]");
  numRelays = relay.getNumRelays();
  sOut->print("Board has [");  sOut->print(numRelays);
  sOut->println("] relays\r\n"); 

} // Mux_Status()


//===========================================================================================
bool ScanI2Cbus(Stream *sOut, byte startAddress)
{
    actI2Caddress = findSlaveAddress(sOut, startAddress);
    sOut->println();
    if (actI2Caddress != 0xFF) {
      sOut->print(F("\nConnecting to  I2C-relay .."));
      sOut->print(F(". connecting with slave @[0x"));
      sOut->print(actI2Caddress, HEX);
      sOut->println(F("]"));
      sOut->flush();
      if (relay.begin(Wire, actI2Caddress)) {
        majorRelease = relay.getMajorRelease();
        minorRelease = relay.getMinorRelease();
        sOut->print(F(". connected with slave @[0x"));
        sOut->print(actI2Caddress, HEX);
        sOut->print(F("] Release[v"));
        sOut->print(majorRelease);
        sOut->print(F("."));
        sOut->print(minorRelease);
        sOut->println(F("]"));
        sOut->flush();
        actI2Caddress = relay.getWhoAmI();
        I2C_MuxConnected = true;
        return true;
        
      } else {
        sOut->println(F(".. Error connecting to I2C slave .."));
        sOut->flush();
        I2C_MuxConnected = false;
        delay(5000);
      }
    }

    return false;
  
} // ScanI2Cbus()


//===========================================================================================
void help(Stream *sOut)
{
  sOut->println();
  sOut->println(F("  Commands are:"));
  sOut->println(F("    n=1;         -> sets relay n to 'closed'"));
  sOut->println(F("    n=0;         -> sets relay n to 'open'"));
  sOut->println(F("    all=1;       -> sets all relay's to 'closed'"));
  sOut->println(F("    all=0;       -> sets all relay's to 'open'"));
  sOut->println(F("    address48;   -> sets I2C address to 0x48"));
  sOut->println(F("    address24;   -> sets I2C address to 0x24"));
  sOut->println(F("    board8;      -> set's board to 8 relay's"));
  sOut->println(F("    board16;     -> set's board to 16 relay's"));
  sOut->println(F("    status;      -> I2C mux status"));
  sOut->println(F("    pinstate;    -> List's state of all relay's"));
  sOut->println(F("    looptest;    -> Chasing Relays test"));
  sOut->println(F("    muxtest;     -> On board test"));
  sOut->println(F("    whoami;      -> shows I2C address Slave MUX"));
  sOut->println(F("    writeconfig; -> write config to eeprom"));
  sOut->println(F("    reboot;      -> reboot I2C Mux"));
  sOut->println(F("  * reScan;      -> re-scan I2C devices"));
  sOut->println(F("    help;        -> shows all commands"));

} // help()


//===========================================================================================
void setAllToZero()
{
  relay.setNumRelays(16);
  for (int i=1; i<=16; i++) relay.digitalWrite(i, LOW); 
  relay.setNumRelays(numRelays);
  
} // setAllToZero()

//===========================================================================================
void executeCommand(String command)
{
  inactiveTimer = millis();
  loopTestOn    = false;

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
  if (command == "all=0")       setAllToZero();
  if (command == "address48")   {actI2Caddress = 0x48; relay.setI2Caddress(actI2Caddress); }
  if (command == "address24")   {actI2Caddress = 0x24; relay.setI2Caddress(actI2Caddress); }
  if (command == "board8")      {numRelays = 8;  relay.setNumRelays(numRelays); command = "all=0"; }
  if (command == "board16")     {numRelays = 16; relay.setNumRelays(numRelays); command = "all=0"; }
  if (command == "status")      { if (commandBy == 1) Mux_Status(&Serial);
                                  else          Mux_Status(&TelnetStream);
                                }
  if (command == "pinstate")    { if (commandBy == 1) displayPinState(&Serial);
                                  else          displayPinState(&TelnetStream);
                                }
  if (command == "looptest")    loopTestOn = true;
  if (command == "muxtest")     relay.writeCommand(1<<CMD_TESTRELAYS);
  if (command == "whoami")      { if (commandBy == 1) {
                                    Serial.print(">>> I am 0x");
                                    Serial.println(relay.getWhoAmI(), HEX);
                                  }
                                  else {
                                    TelnetStream.print(">>> I am 0x");
                                    TelnetStream.println(relay.getWhoAmI(), HEX);
                                  }
                                }
  if (command == "readconfig")  relay.writeCommand(1<<CMD_READCONF);
  if (command == "writeconfig") relay.writeCommand(1<<CMD_WRITECONF);
  if (command == "reboot")      relay.writeCommand(1<<CMD_REBOOT);
  if (command == "help")        { if (commandBy == 1) help(&Serial); else help(&TelnetStream); }
  if (command == "rescan")      { if (commandBy == 1) ScanI2Cbus(&Serial, 1);
                                  else          ScanI2Cbus(&TelnetStream, 1);
                                }
} // executeCommand()


//===========================================================================================
void readSerial()
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
  
  if (command.length() < 1) { help(&Serial); return; }

  Serial.print("command["); Serial.print(command); Serial.println("]");

  commandBy = 1;
  executeCommand(command);
  
} // readSerial()


//===========================================================================================
void readTelnet()
{
  String command = "";
  if (!TelnetStream.available()) {
    return;
  }
  TelnetStream.setTimeout(500);  // ten seconds
  command = TelnetStream.readStringUntil(';');
  command.toLowerCase();
  command.trim();
  for (int i = 0; i < command.length(); i++)
  {
    if (command[i] < ' ' || command[i] > '~') command[i] = 0;
  }
  
  if (command.length() < 1) { help(&TelnetStream); return; }

  TelnetStream.print("command["); TelnetStream.print(command); TelnetStream.println("]");

  commandBy = 2;
  executeCommand(command);
  
} // readTelnet()


//===========================================================================================
void setup()
{
  Serial.begin(115200);
  Serial.println("\r\nAnd than it begins ...\r\n");

  startTelnet();

/* 
**  Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
**  would try to act as both a client and an access-point and could cause
**  network-issues with your other WiFi-devices on your WiFi-network. 
*/
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("WiFi connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  Serial.println(F("Please connect Telnet Client, exit with ^] and 'quit'"));
  
  startMDNS("testMux");
  
  Serial.print("Free Heap[B]: ");
  Serial.println(ESP.getFreeHeap());

  Serial.println(F("\r\nStart I2C-Relay-Multiplexer Test ....\r\n"));
  //Wire.end(); // in case of a reScan..
  Serial.print(F("Setup Wire .."));
  //Wire.begin(_SDA, _SCL); // join i2c bus (address optional for master)
  Wire.begin();
  Wire.setClock(100000L); // <-- don't make this 400000. It won't work
  //Wire.setClock(200000L); // <-- don't make this 400000. It won't work
  Serial.println(F(".. done\r\n"));
  Serial.flush();

  I2C_MuxConnected = false;
  while (!ScanI2Cbus(&Serial, 1))
  {
    Serial.println(F("\r\n.. no I2C slave found..Start rescan ..\r\n"));
    Serial.flush();
    I2C_MuxConnected = false;
    delay(5000);
  } // while not connected

  loopTimer     = millis();
  loopTestOn    = false;
  inactiveTimer = millis();

  if (I2C_MuxConnected) Mux_Status(&Serial);

  help(&Serial);
  help(&TelnetStream);
  
  httpServer.on("/",            HTTP_GET, sendIndex);
  httpServer.on("/index",       HTTP_GET, sendIndex);
  httpServer.on("/index.html",  HTTP_GET, sendIndex);
  httpServer.on("/api",         HTTP_GET, processAPI);
  httpServer.onNotFound([]() 
  {
    //Serial.printf("in 'onNotFound()'!! [%s] => \r\n", String(httpServer.uri()).c_str());
    if (httpServer.uri().indexOf("/api/") == 0) 
    {
      processAPI();
    }
    else
    {
      httpServer.send(404, "text/plain", "Error\r\n");
    }
  });

  httpServer.begin();
  Serial.print(F("\r\nHTTP server gestart. Go to \"http://"));
  Serial.print(WiFi.localIP());
  Serial.println(F("/\"\r\n"));

  Serial.println(F("setup() done .. \r\n"));

} // setup()


//===========================================================================================
void loop()
{
  httpServer.handleClient();
  MDNS.update();

  if (loopTestOn)
  {
    inactiveTimer = millis();

    if ((millis() - loopTimer) > LOOP_INTERVAL) 
    {
      loopTimer = millis();
      loopRelays();      
    }
  }
  else
  {
    setLoopRegister();
  }

  readSerial();
  readTelnet();

  if ((millis() - inactiveTimer) > INACTIVE_TIME) 
  {
    loopTestOn = true;
  }
   
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
****************************************************************************
*/
