/*
***************************************************************************
**
**    Program : I2Cstuff (part of I2C_ATmega_RelaysMux)
**
**    Copyright (C) 2020 Willem Aandewiel
**
**    TERMS OF USE: MIT License. See bottom of file.
***************************************************************************
*/

//------------------------------------------------------------------
void startI2C()
{
  Wire.end();

  Wire.begin(registerStack.whoAmI);
  //Wire.begin(_I2C_DEFAULT_ADDRESS);
  Wire.setClock(100000L);

  // (Re)Declare the Events.
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

} // startI2C()


//------------------------------------------------------------------
boolean isConnected()
{
  Wire.beginTransmission((uint8_t)0);
  if (Wire.endTransmission() != 0) {
    return (false); // Master did not ACK
  }
  return (true);

} // isConnected()


//------------------------------------------------------------------
void processCommand(byte command)
{
  byte GPIO_PIN, PINMODE, HIGH_LOW, GPIOSTATE;
  if ((command & (1<<CMD_PINMODE))) {
    GPIO_PIN = Wire.read();
    PINMODE = Wire.read();
    if (registerStack.numberOfRelays == 8)
    {
      if (GPIO_PIN >= 1 && GPIO_PIN <= 8) 
      {
        pinMode(p2r8[GPIO_PIN], PINMODE);
      }
    }
    else
    {
      if (GPIO_PIN >= 1 && GPIO_PIN <= 16) 
      {
        pinMode(p2r16[GPIO_PIN], PINMODE);
      }
    }
  }
  else if ((command & (1<<CMD_DIGITALWRITE))) {
    GPIO_PIN = Wire.read();
    HIGH_LOW = Wire.read();
    if (registerStack.numberOfRelays == 8)
    {
      if (GPIO_PIN >= 1 && GPIO_PIN <= 8) 
      {
        digitalWrite(p2r8[GPIO_PIN], !HIGH_LOW);
      }
    }
    else
    {
      if (GPIO_PIN >= 1 && GPIO_PIN <= 16) 
      {
        digitalWrite(p2r16[GPIO_PIN], !HIGH_LOW);
      }
    }
  }
  else if ((command & (1<<CMD_DIGITALREAD))) {
    GPIO_PIN = Wire.read();
    if (registerStack.numberOfRelays == 8)
    {
      if (GPIO_PIN >= 1 && GPIO_PIN <= 8) 
      {
        registerStack.lastGpioState = !digitalRead(p2r8[GPIO_PIN]);
      }
    }
    else
    {
      if (GPIO_PIN >= 1 && GPIO_PIN <= 16) 
      {
        registerStack.lastGpioState = !digitalRead(p2r16[GPIO_PIN]);
      }
    }
  }
  if ((command & (1<<CMD_TESTRELAYS))) 
  {
    testRelays();
  }
  if ((command & (1<<CMD_WRITECONF))) 
  {
    writeConfig();
  }
  if ((command & (1<<CMD_READCONF))) 
  {
    readConfig();
  }
  //-----> execute reBoot always last!! <-----
  if ((command & (1<<CMD_REBOOT))) 
  {
    reBoot();
  }

} // processCommand()

//------------------------------------------------------------------
//-- The master sends updated info that will be stored in the ------
//-- register(s)
//-- All Setters end up here ---------------------------------------
void receiveEvent(int numberOfBytesReceived) 
{
  //(void)numberOfBytesReceived;  // cast unused parameter to void to avoid compiler warning
  
  registerNumber = Wire.read(); // Get the memory map offset from the user
  
  if (registerNumber == _CMD_REGISTER) {   // command
    byte command = Wire.read(); // read the command
    processCommand(command);
    return;
  }

  //Begin recording the following incoming bytes to the temp memory map
  //starting at the registerNumber (the first byte received)
  for (byte x = 0 ; x < numberOfBytesReceived - 1 ; x++) {
    byte temp = Wire.read();
    if ( (x + registerNumber) < sizeof(registerLayout)) {
      //Store the result into the register map
      if ((registerNumber + x) < 4 && (registerNumber + x) >= 6) {
        return;
      }
      registerPointer[registerNumber + x] = temp;
      //--- address change is a special case: writeConfig
      if ((registerNumber + x) == _I2CMUX_WHOAMI) 
      {
        writeConfig();
        reBoot();
      }
    }
  }

} //  receiveEvent()

//------------------------------------------------------------------
//-- The master aks's for the data from registerNumber onwards -----
//-- in the register(s) --------------------------------------------
//-- All getters get there data from here --------------------------
void requestEvent()
{
  //----- return max. 4 bytes to master, starting at registerNumber -------
  for (uint8_t x = 0; ( (x < 4) && (x + registerNumber) < (sizeof(registerLayout) - 1) ); x++) {
    Wire.write(registerPointer[(x + registerNumber)]);
  }

} // requestEvent()


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
