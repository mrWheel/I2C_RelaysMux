/*
***************************************************************************  
**
**    Program : I2C_Multiplexer.ino
**    Date    : 08-04-2020
*/
#define _MAJOR_VERSION  1
#define _MINOR_VERSION  7
/*
**
**  Copyright (c) 2020 Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.                                                            
***************************************************************************      
**
*     See: http://www.engbedded.com/fusecalc
*     
*     Set fuses:
*     
*     -U lfuse:w:0xff:m -U hfuse:w:0xd6:m -U efuse:w:0xff:m
*     
*     //Use the standard Arduino UNO bootloader
*     
* Settings:
*     Board:  Arduino/Genuino UNO
*     Chip:   ATmega328P
*     Clock: source: Ext. Crystal Osc. 16 MHz (Ceramic Resonator)
*     B.O.D. Level: B.O.D. Enabled (1.8v)    [if possible]
*     B.O.D. Mode (active): B.O.D. Disabled  [if possible]
*     B.O.D. Mode (sleep): B.O.D. Disabled   [if possible]
*     Save EEPROM: EEPROM retained           [if possible]
*     
*     Fuses OK (E:FF, H:D7, L:F7) << seems ok
*     
*         ..Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17\
*         /bin/avrdude -P usb -c usbtiny -b 9600 -B 100 -p ATmega328P \
*         -U lfuse:w:0xf7:m -U hfuse:w:0xd7:m -U efuse:w:0xff:m <<
*         -U lfuse:w:0xf7:m -U hfuse:w:0xd4:m -U efuse:w:0xff:m
*         -U lfuse:w:0xff:m -U hfuse:w:0xd6:m -U efuse:w:0xff:m
*         -U lfuse:w:0x7F:m -U hfuse:w:0xD6:m -U efuse:w:0xff:m -- NOT
*
*/

//#define DEBUG_ON

// #include <avr/wdt.h>
// #include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>

#define _I2C_DEFAULT_ADDRESS  0x48  // 72 dec

#define _RELAY_ON             0
#define _RELAY_OFF            255

#ifdef DEBUG_ON
  #define Dbegin(...)     ({ Serial.begin(__VA_ARGS__); })
  #define Dprint(...)     ({ Serial.print(__VA_ARGS__); })
  #define Dprintln(...)   ({ Serial.println(__VA_ARGS__); })
  #define Dflush()        ({ Serial.flush(); })
#else
  #define Dbegin(...) 
  #define Dprint(...) 
  #define Dprintln(...) 
  #define Dflush() 
#endif

struct registerLayout {
  byte      status;         // 0x00
  byte      majorRelease;   // 0x01
  byte      minorRelease;   // 0x02
  byte      lastGpioState;  // 0x03
  byte      whoAmI;         // 0x04
  byte      numberOfRelays; // 0x05
  byte      filler[3];      // 0x06 .. 0x08
};


#define _CMD_REGISTER           0xF0
#define _I2CMUX_WHOAMI          0x04
#define _I2CMUX_NUMBEROFRELAYS  0x05

//These are the defaults for all settings
registerLayout registerStack = {
  .status         = 0,                    // 0x00 - RO
  .majorRelease   = _MAJOR_VERSION,       // 0x01 - RO
  .minorRelease   = _MINOR_VERSION,       // 0x02 - RO
  .lastGpioState  = 0 ,                   // 0x03 - RO
  .whoAmI         = _I2C_DEFAULT_ADDRESS, // 0x04 - RW
  .numberOfRelays = 16,                   // 0x05 - RW
  .filler =  {0xFF, 0xFF, 0xFF}           // 0x06 .. 0x08
};
  //----
byte  I2CMUX_COMMAND         = 0xF0 ; // -> this is NOT a "real" register!!


//Cast 32bit address of the object registerStack with uint8_t so we can increment the pointer
uint8_t           *registerPointer = (uint8_t *)&registerStack;
char d1[10];

volatile byte     registerNumber; 
char d2[10];

//                    x  <-------PD--------->  <---PB--->  <-----PC----->
//         relay      0, 1, 2, 3, 4, 5, 6, 7,  8, 9,10,11, 12,13,14,15,16
int8_t p2r16[17] = { -1, 1, 0, 3, 2, 5, 4, 7,  6, 9, 8,13, 10,15,14,17,16} ;  // 16 relay board
int8_t  p2r8[17] = { -1, 1, 3, 5, 7, 9,13,15, 17, 0, 0, 0,  0, 0, 0, 0, 0} ;  //  8 relay board


//------ commands ----------------------------------------------------------
enum  {  CMD_PINMODE, CMD_DIGITALWRITE, CMD_DIGITALREAD
       , CMD_TESTRELAYS, CMD_SPARE5
       , CMD_READCONF, CMD_WRITECONF, CMD_REBOOT 
      };


//==========================================================================
void DprintRegisters(const char *fromFunc)
{
  Dprint("\r\n"); Dprintln(fromFunc);
  Dprint(">> status       ["); Dprint(registerPointer[0]);        Dprintln("]");
  Dprint(">> majorRelease ["); Dprint(registerPointer[1]);        Dprintln("]");
  Dprint(">> minorRelease ["); Dprint(registerPointer[2]);        Dprintln("]");
  Dprint(">> whoAmI       ["); Dprint(registerPointer[4],HEX);    Dprintln("]");
  Dprint(">> numRelays    ["); Dprint(registerPointer[5]);        Dprintln("]\r\n");

} // DprintRegisters()

//==========================================================================
void wait(uint16_t msecs)
{
  sei();
  delay(msecs);
  cli();
  
} // wait()


//==========================================================================
void oneLoop()
{
    for (int i=1; i<=registerStack.numberOfRelays; i++) 
    {
      Dprint("L");
      if (registerStack.numberOfRelays == 8)
            digitalWrite(p2r8[i],  LOW);
      else  digitalWrite(p2r16[i], LOW);
      //wdt_reset();
      wait(750);
    }
    wait(1000);
    Dprintln();
    //wdt_reset();
    for (int i=1; i<=registerStack.numberOfRelays; i++) 
    {
      Dprint("H");
      if (registerStack.numberOfRelays == 8)
            digitalWrite(p2r8[i],  HIGH);
      else  digitalWrite(p2r16[i], HIGH);
      wait(250);
      //wdt_reset();
    }
    Dprintln();

} // oneLoop()


//==========================================================================
void testRelays()
{
  digitalWrite(0, LOW);
  Dprintln("\r\ntestRelays()");
  for (int x=0; x<10;x++) {
    oneLoop();
  }

} //  testRelays()


//==========================================================================
void reBoot()
{
  Dprintln("reboot");
  //wdt_reset();
  //for (int i=0; i<=10; i++) {
  while(true) 
  {
    Dprint(".");
    digitalWrite(p2r16[11], LOW);
    wait(500);
    digitalWrite(p2r16[11], HIGH);
    wait(500);
  }
  //while (true) {} // let WDT crash
  setup();

} //  reBoot()


//==========================================================================
void setup()
{
  //MCUSR=0x00; //<<<-- keep this in!
  //wdt_disable();

  Dbegin(115200);     // this is PD0 (RX) and PD1 (TX)
  Dprint("\n\n(re)starting I2C Mux Slave ..");
  Dprint(_MAJOR_VERSION); Dprint("."); Dprintln(_MINOR_VERSION);
  
  PORTB = B00100111;  // PB0=D08,PB1=D09,PB2=D10,PB5=D13
  DDRB  = B00100111;  // set GPIO pins on PORTB to OUTPUT
  PORTC = B00001111;  // PC0=D12,PC1=D13,PC2=D14,PC3=D15
  DDRC  = B00001111;  // set GPIO pins on PORTC to OUTPUT  
  PORTD = B11111111;  // PD0=D00,PD1=D01,PC2=D02,PC3=D03,PC4=D04,PD5=D05,PD6=D06,PD7=D07
  DDRD  = B11111111;  // set all GPIO pins on PORTD to OUTPUT 

  registerStack.lastGpioState = LOW;    

  readConfig();
  
  startI2C();

  Dprintln("\r\nDone..!\r\n");
  
  //wdt_enable(WDTO_4S);

} // setup()


//==========================================================================
void loop()
{
   //wdt_reset();
   
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
