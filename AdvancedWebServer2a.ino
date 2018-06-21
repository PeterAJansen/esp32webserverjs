/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Wifi
#include <WiFi.h>
#include <WiFiClient.h>
#include "WebServer.h"
#include <ESPmDNS.h>

// Sensors
#include <SparkFunBME280.h>
#include <SparkFunCCS811.h>

#define CCS811_ADDR 0x5B //Default I2C Address
//#define CCS811_ADDR 0x5A //Alternate I2C Address
#define PIN_NOT_WAKE 5


// Web server
WebServer server ( 80 );
const char* ssid     = "Diao'net_EXT";
const char* password = "Whao82!!";
//const char *ssid = "YourSSIDHere";
//const char *password = "YourPSKHere";


// Sensors
CCS811 myCCS811(CCS811_ADDR);
BME280 myBME280;

#define MAX_DATA_POINTS 40
int data[MAX_DATA_POINTS]; 
int curDataIdx = 0;


// LED
#ifdef LED_BUILTIN
const int led = LED_BUILTIN;
#else
const int led = 13;
#endif

// Threading
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif



/*
 * Sensors
 */
void initBME280CS811() {

  //This begins the CCS811 sensor and prints error status of .begin()
  CCS811Core::status returnCode = myCCS811.begin();
  Serial.print("CCS811 begin exited with: ");
  //Pass the error code to a function to print the results
  //printDriverError( returnCode );
  Serial.println();

  //For I2C, enable the following and disable the SPI section
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = 0x77;

  //Initialize BME280
  //For I2C, enable the following and disable the SPI section
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = 0x77;
  myBME280.settings.runMode = 3; //Normal mode
  myBME280.settings.tStandby = 0;
  myBME280.settings.filter = 4;
  myBME280.settings.tempOverSample = 5;
  myBME280.settings.pressOverSample = 5;
  myBME280.settings.humidOverSample = 5;

  //Calling .begin() causes the settings to be loaded
  delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
  myBME280.begin();
  
}

void readBME280CCS811() {

  //Check to see if data is available
  if (myCCS811.dataAvailable())
  {
    //Calling this function updates the global tVOC and eCO2 variables
    myCCS811.readAlgorithmResults();
    //printInfoSerial fetches the values of tVOC and eCO2
    //printInfoSerial();

    float BMEtempC = myBME280.readTempC();
    float BMEhumid = myBME280.readFloatHumidity();

    Serial.print("Applying new values (deg C, %): ");
    Serial.print(BMEtempC);
    Serial.print(",");
    Serial.println(BMEhumid);
    Serial.println();

    //This sends the temperature data to the CCS811
    myCCS811.setEnvironmentalData(BMEhumid, BMEtempC);

    data[curDataIdx] = BMEhumid;
    curDataIdx = (curDataIdx + 1) % MAX_DATA_POINTS;
    
  }
  else if (myCCS811.checkForStatusError())
  {
    //If the CCS811 found an internal error, print it.
    //printSensorError();
  }

  //delay(2000); //Wait for next reading

  
}


/*
 * Web server
 */
 
void handleRoot() {
  digitalWrite ( led, 1 );
  char temp[1000];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

/*
  snprintf ( temp, 400,

"<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266/ESP32 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266/ESP32!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <img src=\"/test.svg\" />\
  </body>\
</html>",

    hr, min % 60, sec % 60
  );
*/

/*
snprintf ( temp, 1000,

"<html>\
  <head>\    
    <title>ESP8266/ESP32 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266/ESP32!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <svg id=\"svg\" width=\"100\" height=\"100\"></svg>\
    <script language=\"Javascript\" type=\"text/javascript\">\    
    function updateImage() {\
      var svgns = \"http://www.w3.org/2000/svg\";\
      var svg = document.getElementById('svg');\
      var shape = document.createElementNS(svgns, \"circle\");\
      shape.setAttributeNS(null, \"cx\", 25);\
      shape.setAttributeNS(null, \"cy\", 25);\
      shape.setAttributeNS(null, \"r\",  20);\
      shape.setAttributeNS(null, \"fill\", \"green\");\
      svg.appendChild(shape);\
      setTimeout(\"updateImage()\", 250);\
    }\
    updateImage();\
    </script>\    
  </body>\
</html>",

    hr, min % 60, sec % 60
  );
  */
const char HTML[] PROGMEM = "// Conversion tool: http://tomeko.net/online_tools/cpp_text_escape.php?lang=en\n<html>\n  <head>\n    <title>ESP32 Demo</title>\n    <style>\n      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\n    </style>\n  </head>\n  <body>\n    <h1>Hello from ESP8266/ESP32!</h1>\n    <p>Uptime: %02d:%02d:%02d</p>\n    <svg id=\"svg\" width=\"100\" height=\"100\"></svg>\n    <script language=\"Javascript\" type=\"text/javascript\">\n\t/*\n\tfunction get_json(url, fn) {\n\t\thttp.get(url, function(res) {\n\t\t\tvar body = '';\n\t\t\tres.on('data', function(chunk) {\n\t\t\t\tbody += chunk;\n\t\t\t});\n\n\t\t\tres.on('end', function() {\n\t\t\t\tvar response = JSON.parse(body);\n\t\t\t\tfn(response);\n\t\t\t});\n\t\t});\n\t};\n\t*/\n\n\tvar getJSON = function(url, callback) {\n\t\tvar xhr = new XMLHttpRequest();\n\t\txhr.open('GET', url, true);\n\t\txhr.responseType = 'json';\n\t\txhr.onload = function() {\n\t\t\tvar status = xhr.status;\n\t\t\tif (status === 200) {\n\t\t\t\tcallback(null, xhr.response);\n\t\t\t} else {\n\t\t\t\tcallback(status, xhr.response);\n\t\t\t}\n\t\t};\n\t\txhr.send();\n\t};\t\n\n\tfunction drawImage(data) {\n\t\tvar pixelSize = 30;\n\t\tvar sizeX = data.sizex;\n\t\tvar sizeY = data.sizey;\t  \n\t\n\t\t//var data = JSON.parse( getJSON('/data.json') );\n\t\tvar svgns = \"http://www.w3.org/2000/svg\";\n\t\t\t\t\n\t\n\t\t/*\n\t\tvar svg1 = document.getElementById('svg');\t\t\n\t\tsvg1.parentNode.removeChild(svg1);\t\t\n\t\t*/\n\t\tvar svg = document.getElementById('svg');\n\t\twhile (svg.firstChild) {\n\t\t\tsvg.removeChild(svg.firstChild);\n\t\t}\n\t  \n\t\t//var svg = document.createElementNS(\"http://www.w3.org/2000/svg\", \"svg\");'\n\t\t/*\n\t\tvar svg = document.createElementNS(svgns, \"svg\");\n\t\t*/\n\t\tsvg.setAttribute('style', 'border: 1px solid black');\n\t\tsvg.setAttribute('width', (sizeX * pixelSize).toString());\n\t\tsvg.setAttribute('height', (sizeY * pixelSize).toString());\n\t\tsvg.setAttributeNS(\"http://www.w3.org/2000/xmlns/\", \"xmlns:xlink\", \"http://www.w3.org/1999/xlink\");\n\t\t//document.body.appendChild(svg);\t  \n\t\t//*/\n\t  \n\t\tvar pixelIdx = 0;\n\t\tfor (y=0; y<sizeY; y++) {\n\t\t\tfor (x=0; x<sizeX; x++) {\t\t\t \n\t\t\t\tvar value = data.data[pixelIdx];\n\t\t\t\tvar color = \"RGB(\" + value * 5 + \", 0, 0)\";\t\t\n\t\t\n\t\t\t\tvar shape = document.createElementNS(svgns, \"rect\");\n\t\t\t\tshape.setAttributeNS(null, \"x\", x * pixelSize);\n\t\t\t\tshape.setAttributeNS(null, \"y\", y * pixelSize);\n\t\t\t\tshape.setAttributeNS(null, \"width\",  pixelSize);\n\t\t\t\tshape.setAttributeNS(null, \"height\",  pixelSize);\n\t\t\t\tshape.setAttributeNS(null, \"fill\", color);\n\t\t\t\tsvg.appendChild(shape);\n\t\t\t\n\t\t\t\tpixelIdx += 1;\n\t\t\t}\n\t\t}\n\t}\n\t\n\t\n    function updateImage() {\n\t\tsetTimeout(\"updateImage()\", 250);\n\t\n\t\tgetJSON('/data.json', function(status, json) {\t\t\n\t\t\tif (status == null) {\n\t\t\t\tconsole.log(json);\n\t\t\t\t//var jsonParsed = JSON.parse(json);\t\t\t\t\n\t\t\t\t//console.log(jsonParsed);\n\n\t\t\t\tdrawImage(json);\n\t\t\t} else {\n\t\t\t\tconsole.log(status);\n\t\t\t}\n\t\t\t// the JSON data is here\n\t\t\t//drawImage(json);\n\t\t});\n\t\n\t}\n\t\n    updateImage();\n    </script>\n  </body>\n</html>";
/*
snprintf ( temp, 1000,

"<html>\
  <head>\    
    <title>ESP8266/ESP32 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266/ESP32!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <svg id=\"svg\" width=\"100\" height=\"100\"></svg>\
    <script language=\"Javascript\" type=\"text/javascript\">\    
    function updateImage() {\
      var data = JSON.parse( );\
      var svgns = \"http://www.w3.org/2000/svg\";\
      var svg = document.getElementById('svg');\
      var shape = document.createElementNS(svgns, \"circle\");\
      shape.setAttributeNS(null, \"cx\", 25);\
      shape.setAttributeNS(null, \"cy\", 25);\
      shape.setAttributeNS(null, \"r\",  20);\
      shape.setAttributeNS(null, \"fill\", \"green\");\
      svg.appendChild(shape);\
      setTimeout(\"updateImage()\", 250);\
    }\
    updateImage();\
    </script>\    
  </body>\
</html>",

    hr, min % 60, sec % 60
  );
*/
  
  //server.send ( 200, "text/html", temp );
  server.send ( 200, "text/html", HTML );
  digitalWrite ( led, 0 );
}

void handleNotFound() {
  digitalWrite ( led, 1 );
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
  digitalWrite ( led, 0 );
}




void setup() {
    xTaskCreatePinnedToCore(
                    run1,     /* Task function. */
                    "run1",       /* String with name of task. */
                    10000,            /* Stack size in words. */
                    NULL,             /* Parameter passed as input of the task */
                    10,                /* Priority of the task. */
                    NULL, 1);            /* Task handle. */

  xTaskCreatePinnedToCore(
                    run2,     /* Task function. */
                    "run2",       /* String with name of task. */
                    10000,            /* Stack size in words. */
                    NULL,             /* Parameter passed as input of the task */
                    1,                /* Priority of the task. */
                    NULL, 1);            /* Task handle. */

}

void loop() {
  // do nothing
}

void run1(void * parameters) {
  setup1();
  while (1) {
    loop1();
  }
}

void run2(void * parameters) {
  setup2();
  while (1) {
    loopSensors();
    vTaskDelay(1);
  }
}


void setup1 ( void ) {
  pinMode ( led, OUTPUT );
  digitalWrite ( led, 0 );
  Serial.begin ( 115200 );
  WiFi.begin ( ssid, password );
  Serial.println ( "" );

  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  Serial.println ( "" );
  Serial.print ( "Connected to " );
  Serial.println ( ssid );
  Serial.print ( "IP address: " );
  Serial.println ( WiFi.localIP() );

#ifdef ESP8266
  if ( MDNS.begin ( "esp8266" ) ) {
#else
  if ( MDNS.begin ( "esp32" ) ) {
#endif
    Serial.println ( "MDNS responder started" );
  }

  server.on ( "/", handleRoot );
  server.on ( "/test.svg", drawGraph );
  server.on ( "/inline", []() {
    server.send ( 200, "text/plain", "this works as well" );
  } );

  server.on ( "/data.json", sendJSON );

  
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "HTTP server started" );


 
}

void setup2() {
  // Sensors
  Serial.println ("Initializing sensors...");
  initBME280CS811();  
  
}


void loop1 ( void ) {
  server.handleClient();
  //Serial.println("tick");

  loopSensors();
}

long lastSensorUpdate = 0;
void loopSensors() {
  long delta = millis() - lastSensorUpdate;
  if (delta > 1000) {    
    Serial.println("Update...");
    
    lastSensorUpdate = millis();  
    readBME280CCS811();
  } 

  delay(100); //## Test of blocking

}

void drawGraph() {
  String out = "";
  char temp[100];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\">\n";
  //int y = rand() % 130;
  int y = 0;
  //for (int x = 10; x < 390; x+= 10) {  
  int xScale = 10;
  for (int x = 0; x < MAX_DATA_POINTS; x+= 1) {
    //int y2 = rand() % 130;     
    int y2 = data[x];
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", (x*xScale), 140 - y, (x+1)*xScale, 140 - y2);
    out += temp;
    y = y2;
  }
  out += "</g>\n</svg>\n";

  server.send ( 200, "image/svg+xml", out);
}

// test
void sendJSON() {
  String out = "";
  char temp[1000];
  //out += "{\"name\":\"John\",\"today\":\"2018-06-18T07:15:45.059Z\",\"city\":\"New York\"}";
  //out += "{\"sizex\": 5, \"sizey\": 4, \"data\": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20]}";
  int sizex = random(5, 15);
  int sizey = random(5, 15);
  out += "{\"sizex\": " + String(sizex);
  out += ", \"sizey\": " + String(sizey);
  out += ", \"data\": [";
  
  for (int y=0; y<sizey; y++) {
    for (int x=0; x<sizex; x++) {
      out += String( random(1, 30) );
      if ((y == sizey-1) && (x == sizex-1)) {
        // do nothing
      } else {
        out += ", ";
      }
    }
  }

  out += "]}";

  server.send ( 200, "application/json", out);
}
