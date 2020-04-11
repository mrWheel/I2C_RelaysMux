/*
***************************************************************************  
**
**  File    : I2C_RelaysMux.h
**  Version : v1.0
**
**  Copyright (c) 2020 Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.                                                            
***************************************************************************      
*/


#ifndef _I2C_RELAYSMUX_H
#define _I2C_RELAYSMUX_H

#include "Arduino.h"
#include "Wire.h"

#define I2C_SLAVE_ADDRESS 0x48

// Commando's
enum  {  CMD_PINMODE, CMD_DIGITALWRITE, CMD_DIGITALREAD
       , CMD_TESTRELAYS, CMD_SPARE5
       , CMD_READCONF, CMD_WRITECONF, CMD_REBOOT 
      };

// Map to the various registers on the I2C Multiplexer
enum encoderRegisters {
  I2CMUX_STATUS          = 0x00,
  I2CMUX_MAJORRELEASE    = 0x01,
  I2CMUX_MINORRELEASE    = 0x02,
  I2CMUX_LASTGPIOSTATE   = 0x03,
  I2CMUX_WHOAMI          = 0x04,
  I2CMUX_NUMBEROFRELAYS  = 0x05,

  //----
  I2CMUX_COMMAND         = 0xF0   // -> this is NOT a "real" register!!
};

#define _WRITEDELAY 20
#define _READDELAY  20

class I2CMUX
{

public:
  I2CMUX();

  bool    begin(TwoWire &wireBus = Wire, uint8_t deviceAddress = I2C_SLAVE_ADDRESS);
  bool    isConnected();
  byte    getMajorRelease();
  byte    getMinorRelease();
  byte    getWhoAmI();
  byte    getNumRelays();
  byte    getStatus();
  bool    writeCommand(byte);
  bool    pinMode(byte, byte); 
  bool    digitalRead(byte); 
  bool    digitalWrite(byte, byte); 
  bool    setI2Caddress(uint8_t newAddress);  // set a new I2C address for this Slave (1 .. 127)        
  bool    setNumRelays(uint8_t numRelays);    // set the number of relays on the board (8 or 16)        
  void    showRegister(size_t const size, void const * const ptr, Stream *outp);
  
private:
  TwoWire           *_I2Cbus;
  uint8_t           _I2Caddress;
  uint8_t           _I2CnumRelays;
  volatile uint8_t  _status;
  uint32_t          _statusTimer;

  uint8_t   readReg1Byte(uint8_t reg);
  int16_t   readReg2Byte(uint8_t reg);
  int32_t   readReg4Byte(uint8_t reg);

  bool      writeReg1Byte(uint8_t reg, uint8_t val);
  bool      writeReg2Byte(uint8_t reg, int16_t val);
  bool      writeReg3Byte(uint8_t reg, int32_t val);
  bool      writeReg4Byte(uint8_t reg, int32_t val);
  bool      writeCommand2Bytes(byte CMD, byte GPIO_PIN);
  bool      writeCommand3Bytes(byte CMD, byte GPIO_PIN, byte HIGH_LOW);

};

#endif

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
