/*
***************************************************************************  
**
**  File    : I2C_RelaysMux.cpp
**  Version : v0.6.3
**
**  Copyright (c) 2020 Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.                                                            
***************************************************************************      
*/

#include "/Users/WillemA/Documents/ArduinoProjects/I2C_Relay_Mux/I2C_MuxLib/src/I2C_RelaysMux.h"

// Constructor
I2CMUX::I2CMUX() { }

// Initializes the I2C_Multiplexer
// Returns false if I2C_Multiplexer is not detected
//-------------------------------------------------------------------------------------
bool I2CMUX::begin(TwoWire &wireBus, uint8_t deviceAddress)
{
  _I2Cbus = &wireBus;
  _I2Cbus->begin(); 
  _I2Cbus->setClock(100000L); // <-- don't be smart! This is the max you can get

  _I2Caddress = deviceAddress;

  if (isConnected() == false)
    return (false); // Check for I2C_Relay_Multiplexer presence
  
  return (true); // Everything is OK!

} // begin()

//-------------------------------------------------------------------------------------
bool I2CMUX::isConnected()
{
  _I2Cbus->beginTransmission((uint8_t)_I2Caddress);
  if (_I2Cbus->endTransmission() != 0)
    return (false); // I2C Slave did not ACK
  return (true);
} // isConnected()

//-------------------------------------------------------------------------------------
byte I2CMUX::getMajorRelease()
{
  while ((millis() - _statusTimer) < _READDELAY) {
    delay(1);
  }
  _statusTimer = millis();
  return (readReg1Byte(I2CMUX_MAJORRELEASE));
}
//-------------------------------------------------------------------------------------
byte I2CMUX::getMinorRelease()
{
  while ((millis() - _statusTimer) < _READDELAY) {
    delay(1);
  }
  _statusTimer = millis();
  return (readReg1Byte(I2CMUX_MINORRELEASE));
}

//-------------------------------------------------------------------------------------
byte I2CMUX::getWhoAmI()
{
  while ((millis() - _statusTimer) < _READDELAY) {
    delay(1);
  }
  _statusTimer = millis();
  return (readReg1Byte(I2CMUX_WHOAMI));
}

//-------------------------------------------------------------------------------------
byte I2CMUX::getNumRelays()
{
  while ((millis() - _statusTimer) < _READDELAY) {
    delay(1);
  }
  _statusTimer = millis();
  return (readReg1Byte(I2CMUX_NUMBEROFRELAYS));
}

//-------------------------------------------------------------------------------------
byte I2CMUX::getStatus()
{
  while ((millis() - _statusTimer) < _READDELAY) {
    delay(1);
  }
  _statusTimer = millis();
  uint8_t tmpStatus = (byte)readReg1Byte(I2CMUX_STATUS);
  _status |= (byte)tmpStatus;
  return (tmpStatus);
}

//-------------------------------------------------------------------------------------
bool I2CMUX::writeCommand(byte command)
{
  Serial.print("Command ["); Serial.print(command); Serial.println("]");
  return (writeReg1Byte(I2CMUX_COMMAND, command));
}

//-------------------------------------------------------------------------------------
bool I2CMUX::pinMode(byte GPIO_PIN, byte PINMODE)
{
  return(writeCommand3Bytes(_BV(CMD_PINMODE), GPIO_PIN, PINMODE));
}

//-------------------------------------------------------------------------------------
bool I2CMUX::digitalRead(byte GPIO_PIN)
{
  if (writeCommand2Bytes(_BV(CMD_DIGITALREAD), GPIO_PIN)) {
    delay(2);
    return (readReg1Byte(I2CMUX_LASTGPIOSTATE));
  }
}

//-------------------------------------------------------------------------------------
bool I2CMUX::digitalWrite(byte GPIO_PIN, byte HIGH_LOW)
{
  return(writeCommand3Bytes(_BV(CMD_DIGITALWRITE), GPIO_PIN, HIGH_LOW));
}

// Change the I2C address of this I2C Slave address to newAddress
//-------------------------------------------------------------------------------------
bool I2CMUX::setI2Caddress(uint8_t newAddress)
{
  Serial.print("setI2Caddress(): register[0x"); 
  Serial.print(I2CMUX_WHOAMI, HEX); 
  Serial.println("]");
  if (writeReg1Byte(I2CMUX_WHOAMI, newAddress)) {
    // Once the address is changed, we need to change it in the library
    _I2Caddress = newAddress;
    writeCommand(1<<CMD_WRITECONF);
    return true;
  }
  return false;

} // newAddress()

// Set number of relays on board (8 or 16)
//-------------------------------------------------------------------------------------
bool I2CMUX::setNumRelays(uint8_t numRelays)
{
  if (numRelays == 8 || numRelays == 16) 
  {
    Serial.print("setNumRelays(): register[0x"); 
    Serial.print(I2CMUX_NUMBEROFRELAYS, HEX); 
    Serial.println("]");
    _I2CnumRelays = numRelays;
    if (writeReg1Byte(I2CMUX_NUMBEROFRELAYS, numRelays)) {
      writeCommand(1<<CMD_WRITECONF);
      return true;
    }
  }
  return false;

} // setNumRelays()


//-------------------------------------------------------------------------------------
//-------------------------- READ FROM REGISTERS --------------------------------------
//-------------------------------------------------------------------------------------

// Reads a uint8_t from a register @addr
//-------------------------------------------------------------------------------------
uint8_t I2CMUX::readReg1Byte(uint8_t addr)
{  
  while ((millis() - _statusTimer) < _READDELAY) {
    delay(1);
  }
  _statusTimer = millis();

  _I2Cbus->beginTransmission((uint8_t)_I2Caddress);
  _I2Cbus->write(addr);
  if (_I2Cbus->endTransmission() != 0) {
    return (0); // Slave did not ack
  }

  _I2Cbus->requestFrom((uint8_t)_I2Caddress, (uint8_t) 1);
  delay(5);
  if (_I2Cbus->available()) {
    return (_I2Cbus->read());
  }

  return (0); // Slave did not respond
}

// Reads an int16_t from a register @addr
//-------------------------------------------------------------------------------------
int16_t I2CMUX::readReg2Byte(uint8_t addr)
{
  while ((millis() - _statusTimer) < _READDELAY) {
    delay(1);
  }
  _statusTimer = millis();

  _I2Cbus->beginTransmission((uint8_t)_I2Caddress);
  _I2Cbus->write(addr);
  if (_I2Cbus->endTransmission() != 0) {
    return (0); // Slave did not ack
  }

  _I2Cbus->requestFrom((uint8_t)_I2Caddress, (uint8_t) 2);
  delay(5);
  if (_I2Cbus->available()) {
    uint8_t LSB = _I2Cbus->read();
    uint8_t MSB = _I2Cbus->read();
    return ((int16_t)MSB << 8 | LSB);
  }

  return (0); // Slave did not respond
}

// Reads an int32_t from a register @addr
//-------------------------------------------------------------------------------------
int32_t I2CMUX::readReg4Byte(uint8_t addr)
{
  while ((millis() - _statusTimer) < _READDELAY) {
    delay(1);
  }
  _statusTimer = millis();

  _I2Cbus->beginTransmission((uint8_t)_I2Caddress);
  _I2Cbus->write(addr);
  if (_I2Cbus->endTransmission() != 0) {
    return (0); // Slave did not ack
  }

  _I2Cbus->requestFrom((uint8_t)_I2Caddress, (uint8_t) 4);
  delay(5);
  if (_I2Cbus->available()) {
    uint8_t LSB   = _I2Cbus->read();
    uint8_t mLSB  = _I2Cbus->read();
    uint8_t mMSB  = _I2Cbus->read();
    uint8_t MSB   = _I2Cbus->read();
    uint32_t comb = MSB << 24 | mMSB << 16 | mLSB << 8 | LSB;
    return ((int32_t)MSB << 24 | mMSB << 16 | mLSB << 8 | LSB);
  }

  return (0); // Slave did not respond
}

//-------------------------------------------------------------------------------------
//-------------------------- WRITE TO REGISTERS ---------------------------------------
//-------------------------------------------------------------------------------------

// Write a 1 byte value to a register
//-------------------------------------------------------------------------------------
bool I2CMUX::writeReg1Byte(uint8_t addr, uint8_t val)
{
  while ((millis() - _statusTimer) < _WRITEDELAY) {
    delay(1);
  }
  _statusTimer = millis();

  Serial.print("writeReg1Byte(");
  Serial.print(addr);
  Serial.print(", ");
  Serial.print(val);
  Serial.println(")");

  _I2Cbus->beginTransmission((uint8_t)_I2Caddress);
  _I2Cbus->write(addr);
  _I2Cbus->write(val);
  if (_I2Cbus->endTransmission() != 0) {
    return (false); // Slave did not ack
  }

  return (true);
}

// Write a 2 byte value to a register
//-------------------------------------------------------------------------------------
bool I2CMUX::writeReg2Byte(uint8_t addr, int16_t val)
{
  while ((millis() - _statusTimer) < _WRITEDELAY) {
    delay(1);
  }
  _statusTimer = millis();

  _I2Cbus->beginTransmission((uint8_t)_I2Caddress);
  _I2Cbus->write(addr);
  _I2Cbus->write(val & 0xFF); // LSB
  _I2Cbus->write(val >> 8);   // MSB
  if (_I2Cbus->endTransmission() != 0) {
    return (false); // Slave did not ack
  }

  return (true);
}


// Write a 3 byte value to a register
//-------------------------------------------------------------------------------------
bool I2CMUX::writeReg3Byte(uint8_t addr, int32_t val)
{
  while ((millis() - _statusTimer) < _WRITEDELAY) {
    delay(1);
  }
  _statusTimer = millis();

  _I2Cbus->beginTransmission((uint8_t)_I2Caddress);
  _I2Cbus->write(addr);
  _I2Cbus->write(val &0xFF);     // LSB
  _I2Cbus->write(val >> 8);       // mLSB
  _I2Cbus->write(val >> 16);      // mMSB
  //_I2Cbus->write(val >> 24);    // MSB
  if (_I2Cbus->endTransmission() != 0) {
    return (false); // Slave did not ack
  }

  return (true);
}

// Write a 4 byte value to a register
//-------------------------------------------------------------------------------------
bool I2CMUX::writeReg4Byte(uint8_t addr, int32_t val)
{
  while ((millis() - _statusTimer) < _WRITEDELAY) {
    delay(1);
  }
  _statusTimer = millis();

  _I2Cbus->beginTransmission((uint8_t)_I2Caddress);
  _I2Cbus->write(addr);
  _I2Cbus->write(val & 0xFF); // LSB
  _I2Cbus->write(val >> 8);   // mLSB
  _I2Cbus->write(val >> 16);  // mMSB
  _I2Cbus->write(val >> 24);  // MSB
  if (_I2Cbus->endTransmission() != 0) {
    return (false); // Slave did not ack
  }

  return (true);
}

// Write a 2 byte's command I2C_Mux Slave
//-------------------------------------------------------------------------------------
bool I2CMUX::writeCommand2Bytes(byte CMD, byte GPIO_PIN)
{
  while ((millis() - _statusTimer) < _WRITEDELAY) {
    delay(5);
  }
  _statusTimer = millis();

  //Serial.printf("\nwriteCommand2Bytes: CMD[%d], GPIO[%d] \n", CMD, GPIO_PIN);
  _I2Cbus->beginTransmission((uint8_t)_I2Caddress);
  _I2Cbus->write(I2CMUX_COMMAND);
  // val is [-------- cccccccc pppppppp]
  _I2Cbus->write(CMD &0xFF);          // Command
  _I2Cbus->write(GPIO_PIN &0xFF);     // GPIO_PIN
  if (_I2Cbus->endTransmission() != 0) {
    return (false); // Slave did not ack
  }
  return (true);
}

// Write a 3 byte's command I2C_Mux Slave
//-------------------------------------------------------------------------------------
bool I2CMUX::writeCommand3Bytes(byte CMD, byte GPIO_PIN, byte HIGH_LOW)
{
  while ((millis() - _statusTimer) < _WRITEDELAY) {
    delay(5);
  }
  _statusTimer = millis();

  //Serial.printf("\nwriteCommand3Bytes: CMD[%d], GPIO[%d], HL[%d] \n", CMD, GPIO_PIN, HIGH_LOW);
  _I2Cbus->beginTransmission((uint8_t)_I2Caddress);
  _I2Cbus->write(I2CMUX_COMMAND);
  // val is [-------- cccccccc pppppppp vvvvvvvv]
  _I2Cbus->write(CMD &0xFF);          // Command
  _I2Cbus->write(GPIO_PIN &0xFF);     // GPIO_PIN
  _I2Cbus->write(HIGH_LOW &0xFF);     // HIGH_LOW
  if (_I2Cbus->endTransmission() != 0) {
    return (false); // Slave did not ack
  }

  return (true);
}

//-------------------------------------------------------------------------------------
//-------------------------- HELPERS --------------------------------------------------
//-------------------------------------------------------------------------------------

//===========================================================================================
//assumes little endian
void I2CMUX::showRegister(size_t const size, void const * const ptr, Stream *outp)
{
  unsigned char *b = (unsigned char*) ptr;
  unsigned char byte;
  int i, j;
  outp.print("[");
  for (i=size-1; i>=0; i--) {
    if (i < (size-1)) outp.print(" ");
    for (j=7; j>=0; j--) {
      byte = (b[i] >> j) & 1;
      outp.print(byte);
    }
  }
  outp.println("]");
  
} // showRegister()


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
