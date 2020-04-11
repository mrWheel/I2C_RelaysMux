/* 
***************************************************************************  
**  Program  : restAPI, part of I2C_ESP8266_RelaysMux_Test
**  Version  : v1.0
**
**  Copyright (c) 2020 Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.                                                            
***************************************************************************      
*/


//=======================================================================
void processAPI() 
{
  String words[10];
  char   URI[50];
  
  strncpy( URI, httpServer.uri().c_str(), sizeof(URI) );

  int8_t wc = splitString(URI, '/', words, 10);
  
  if (words[1] != "api")
  {
    sendApiNotFound(URI);
    return;
  } else if ( (httpServer.method() == HTTP_GET) && (words[2] == "states") )
  {
    sendRelayStates();
  }
  else if ( (httpServer.method() == HTTP_POST || httpServer.method() == HTTP_PUT) && (words[2] == "state") )
  {
    setRelayState();
    inactiveTimer = millis();
    loopTestOn    = false;
  }
  else sendApiNotFound(URI);
  
} // processAPI()


//====================================================
void sendRelayStates()
{
  sendStartJsonObj("states");
  for (int i=1; i<= numRelays; i++)
  {
    sendNestedJsonObj(i, relay.digitalRead(i));
  }
  sendEndJsonObj();
  
} // sendRelayStates()


//====================================================
void sendApiNotFound(const char *URI)
{
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  httpServer.send ( 404, "text/plain", "API error\r\n");

} // sendApiNotFound()

static char objSprtr[10] = "";

//=======================================================================
void sendStartJsonObj(const char *objName)
{
  char sBuff[50] = "";
  objSprtr[0]    = '\0';

  snprintf(sBuff, sizeof(sBuff), "{\"%s\":[\r\n", objName);
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  httpServer.send(200, "application/json", sBuff);
  
} // sendStartJsonObj()


//=======================================================================
void sendEndJsonObj()
{
  httpServer.sendContent("\r\n]}\r\n");
  
} // sendEndJsonObj()


//=======================================================================
void sendNestedJsonObj(uint8_t relayNr, uint8_t state)
{
  char jsonBuff[200] = "";
  
  snprintf(jsonBuff, sizeof(jsonBuff), "%s{\"relay\": %d, \"state\": %d }"
                                 , objSprtr, relayNr, state);

  httpServer.sendContent(jsonBuff);
  sprintf(objSprtr, ",\r\n");

} // sendNestedJsonObj(int, int)


//=======================================================================
void setRelayState()
{
  String  wOut[5];
  String  wPair[5];
  String  jsonIn = httpServer.arg(0).c_str();
  uint8_t relayNr, newState;
  
  Serial.printf("setRelayState(%s)\r\n", httpServer.arg(0).c_str());

  jsonIn.replace("{", "");
  jsonIn.replace("}", "");
  jsonIn.replace("\"", "");
  int8_t wp = splitString(jsonIn.c_str(), ',',  wPair, 5) ;
  for (int i=0; i<wp; i++)
  {
    int8_t wc = splitString(wPair[i].c_str(), ':',  wOut, 5) ;
    if (wOut[0].equalsIgnoreCase("relay"))  relayNr   = wOut[1].toInt();
    if (wOut[0].equalsIgnoreCase("state"))  newState  = wOut[1].toInt();
  }
  if (relayNr >= 1 && relayNr <= numRelays)
  {
    if (newState == 0)  relay.digitalWrite((byte)relayNr, LOW);
    else 
    {
      relay.digitalWrite((byte)relayNr, HIGH);
      loopRegister |= (1<< (relayNr-1));
    }
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    httpServer.send(200, "application/json", httpServer.arg(0));
  }
  else
  {
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    httpServer.send (404, "text/plain", "error\r\n");
  }
} // setRelayState()


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
