/* 
***************************************************************************  
**  Program  : indexHtml, part of I2C_ESP8266_RelaysMux_Test
**  Version  : v1.0
**
**  Copyright (c) 2020 Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.                                                            
***************************************************************************      
*/


static const char indexHTML[] PROGMEM =
  R"(<html charset="UTF-8">
      <style type='text/css'>
        body {background-color: lightgray;}
        #myProgress { width: 100%; background-color: lightgray; }
        #myBar      { width: 1%; height: 5px; background-color: gray;}
        .bottom { position: fixed; font-size: small; bottom:5; } 
        .left-0   {left:  0; padding-left:  10px; }
      </style>
      <body>
      <h1>I2C ESP8266 RelaysMux Test</h1>
      <br/>
        <table id='switches'>
          <thead>
            <tr><th nowrap>Relay Number</th><th nowrap> &nbsp; State &nbsp; </th></tr>
          </thead>
          <tbody>
          </tbody>
        </table>
        <!-- Pin to bottom left corner -->
        <div id="myProgress" class="bottom left-0"><div id="myBar"></div></div>
      </body>
      <script>
        const APIGW='http://'+window.location.host+'/api/';
        'use strict';
        var needBootsTrapMain = true;
        var timeTimer         = 0;
        var walking           = 0;
        var barWidth          = 1;
        window.onload=bootsTrapMain;
        
        function bootsTrapMain() {
          console.log('bootsTrapMain()');
          needBootsTrapMain = false;
            
          clearInterval(timeTimer);  
          refreshRelayState();
          timeTimer = setInterval(refreshRelayState, 2 * 1000); // repeat every 2s
      
        } // bootsTrapMain()
  
        //============================================================================  
        function refreshRelayState()
        {
          fetch(APIGW+"states")
            .then(response => response.json())
            .then(json => {
                //console.log('parsed .., states is ['+ JSON.stringify(json)+']');
                var data = json.states;
                for (var i in data) 
                {
                  var tableRef = document.getElementById('switches').getElementsByTagName('tbody')[0];
                  if( ( document.getElementById('row_'+data[i].relay)) == null )
                  {
                    var newRow  = tableRef.insertRow();
                    newRow.setAttribute('id', 'row_'+data[i].relay);
                    // Insert a cell in the row at index 0
                    var newCell = newRow.insertCell(0);                  // relay
                    var newText = document.createTextNode('');
                    newCell.appendChild(newText);
                    newCell     = newRow.insertCell(1);                  // state
                    newCell.appendChild(newText);
                  }
                  tableCells = document.getElementById('row_'+data[i].relay).cells;
                  tableCells[0].innerHTML = 'Relay('+data[i].relay+')';
                  tableCells[1].setAttribute("id", "cell_"+data[i].relay);
                  if (data[i].state == 1)
                  {
                    if (   tableCells[1].innerHTML != 'ON' 
                        || tableCells[1].style.backgroundColor != 'gray')
                    {
                      tableCells[1].setAttribute("style", "color: white; background: red");
                      tableCells[1].innerHTML = 'ON';
                      tableCells[1].addEventListener("click", clickOff);                  
                    }
                  }
                  else 
                  {
                    if (   tableCells[1].innerHTML != 'Off' 
                        || tableCells[1].style.backgroundColor != 'gray')
                    {
                      tableCells[1].setAttribute("style", "background: lightgreen");
                      tableCells[1].innerHTML = 'Off';
                      tableCells[1].addEventListener("click", clickOn);                  
                    }
                  }
                  tableCells[1].style.textAlign = "center";
                }
                startBar();
                //console.log("-->done..");
            })
            .catch(function(error) {
              var p = document.createElement('p');
              p.appendChild(
                document.createTextNode('Error: ' + error.message)
              );
            }); 
        };  // refreshRelayState()

        //==============================================================
        function sendRelayState(nr, state) 
        {
          const jsonString = {"relay" : nr, "state" : state};
          console.log("sendRelayState: JSON:["+JSON.stringify(jsonString)+"]");
          const other_params = {
              headers : { "content-type" : "application/json; charset=UTF-8"},
              body : JSON.stringify(jsonString),
              method : "POST",
              mode : "cors"
          };

          fetch(APIGW+"state", other_params)
            .then(function(response) {
                  //console.log(response.status );    //=> number 100–599
                  //console.log(response.statusText); //=> String
                  //console.log(response.headers);    //=> Headers
                  //console.log(response.url);        //=> String
                  //console.log(response.text());
                  //return response.text()
            }, function(error) {
              console.log('Error['+error.message+']'); //=> String
            });
      
        } // sendRelayState()
  

        //==============================================================
        function clickOn(e) 
        {
          var relay=parseInt(this.id.substring(5));
          //console.log('clickOn('+this.id+') => ['+relay+']');
          // set background of clicked row
          this.style.backgroundColor = 'gray';
          this.removeEventListener("click", clickOn);
          sendRelayState(relay, 1);
          this.addEventListener("click", clickOff);

        } // clickOn()

        //==============================================================
        function clickOff(e) 
        {
          var relay=parseInt(this.id.substring(5));
          //console.log('clickOff('+this.id+') => ['+relay+']');
          // set background of clicked row
          this.style.backgroundColor = 'gray';
          this.removeEventListener("click", clickOff);
          sendRelayState(relay, 0);
          this.addEventListener("click", clickOn);

        } // clickOff()


        //==============================================================
        function startBar()
        {
          barWidth=100;
          moveBar();
        } // startBar()

        //==============================================================
        function moveBar() 
        {
          if (walking == 0) {
            walking = 1;
            var elem = document.getElementById("myBar");
            barWidth = 1;
            var id = setInterval(frame, 33);
            function frame() {
              if (barWidth >= 100) {
                clearInterval(id);
                walking = 0;
              } else {
                barWidth++;
                elem.style.width = barWidth + "%";
              }
            } // frame()
          } // moveBar()
        } // moveBar()
//===============================================
     </script>
     </html>)";

void sendIndex()
{
  httpServer.send(200, "text/html", indexHTML);

} // sendIndex()



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
