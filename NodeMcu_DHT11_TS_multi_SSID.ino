
#define USE_SECRET  1

// 1 "TP-Link_BB"    
// 2 "BB_Home2"  
// 3 "P20 Pro"    
// 4 "BB_guest"

//#define USE_TEST_CHANNEL
#define SKIP_TS_COMMUNICATION

#define VERSION               "v2.3"
#define BUILDNUM                  15

#define SERIAL_BOUND_RATE     115200

// DHT-11 module powered by NodeMCU's 5V 
// DHT-11 connected to D5 of NodeMCU
// Pinout: https://circuits4you.com/2017/12/31/nodemcu-pinout/

//#include "DHTesp.h"


#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ThingSpeak.h>

#include "secrets.h"
#include "params.h"
#include <EEPROM.h>


#define ON                         1
#define OFF                        0
 
DHTesp dht;

//==================================
// WIFI
//==================================
const char* ssid =           S_SSID;
const char* password =       S_PASS;



// =================================
// HTTP
// =================================
ESP8266WebServer server(SERVER_PORT);
HTTPClient http;


//==================================
// ThingSpeak
//==================================
WiFiClient client;
unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;


//==================================
// "Data" variables
//==================================
float valC = NAN;
float valF = NAN;
float valH = NAN;
float valT = NAN;

int lastTSUpdateStatus = -1;

long updateCount = 0;
String lastPhaseStatus = "";

//==================================
// Other variables
//==================================
long lastTemp = 0;      //The last measurement
int errorCount = 0;

unsigned long elapsedTime = 0;


#ifndef SKIP_TS_COMMUNICATION
// ==========================================================================================================================
//                                                    ThingSpeak communication
// ==========================================================================================================================

int sendValuesToTSServer(float heatindex, float humidity, float temp1) {

  // ThingSpeak
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  ThingSpeak.setField(1, heatindex);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, temp1);

  // write to the ThingSpeak channel
  lastTSUpdateStatus = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if(lastTSUpdateStatus == 200){
    Serial.println("Channel update successful.");
    ledBlink(2, 200, 100);
    return 1;
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(lastTSUpdateStatus));
    int errorCodeFirstDigit = (abs(lastTSUpdateStatus)/100);
    ledBlink(errorCodeFirstDigit, 50, 50);
    // Increase error count for restarting after defined max error counts..
    errorCount++;
    return 0;
  }
}

int sendValuesToTS(float heatindex, float humidity, float temp1){
  if (client.connect("api.thingspeak.com",80)){   //   "184.106.153.149" or api.thingspeak.com
        
       String postStr = SECRET_WRITE_APIKEY;
       postStr +="&field1=";
       postStr += String(heatindex);
       postStr +="&field2=";
       postStr += String(humidity);
       postStr +="&field3=";
       postStr += String(temp1);
       postStr += "\r\n\r\n";

       client.print("POST /update HTTP/1.1\n");
       client.print("Host: api.thingspeak.com\n");
       client.print("Connection: close\n");
       client.print("X-THINGSPEAKAPIKEY: " + String(SECRET_WRITE_APIKEY) +"\n");
       client.print("Content-Type: application/x-www-form-urlencoded\n");
       client.print("Content-Length: ");
       client.print(postStr.length());
       client.print("\n\n");
       client.print(postStr);
       Serial.println("Values sent to TS server..");

    }
    client.stop();
    delay(100);
    return 1;
}

#endif

// ==========================================================================================================================
//                                                 Hw feedback function
// ==========================================================================================================================

void turnLED(int state){
  if(state > 0){
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  }else{
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

void ledBlink(int blinkCount, int onTime, int offTime){
  for(short int i=0; i<blinkCount; i++){
    turnLED(ON);
    delay(onTime);                      // Wait for a second
    turnLED(OFF);
    if(i < blinkCount-1){
      delay(offTime);
    }
  }
}

// ==========================================================================================================================
//                                                     Other methods
// ==========================================================================================================================

void doRestart(){
  Serial.println("Restart now...");
  ESP.restart();
}


// ==========================================================================================================================
//                                                  Web server functions
// ==========================================================================================================================

void HandleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/html", message);
}

void HandleNotRstEndpoint(){
  doRestart();
}

String generateHtmlHeader(){
  String h = "<!DOCTYPE html>";
  h += "<html lang=\"en\">";
  h += "\n\t<head>";
  h += "\n\t\t<meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  h += "\n\t</head>";
  return h;
}

String generateHtmlBody(){
  String m = "\t<body bgcolor=\"#49B7E8\">";

  m += "\t\t<h4>";
  m += String(SOFTWARE_NAME) + " " + String(VERSION) + " b" + String(BUILDNUM);
  m += "<br>" + String(LOCATION_NAME);
  m += "</h4><br>";

  m += "<center>";

  m += "\t\t<h1>";
  m += "H&otilde;&eacute;rzet:\n\r</h2><h1>";
  m += String(valT);
  m += " &#8451;</h1>\n\r";

  m+= "<h3>(" + String((int)valC) + " &#8451;)</h3>";
  
  m += "<br>";

  m += "<br><h2>";
  m += "P&aacute;ratartalom:\n\r</h2><h1>";
  m += String((int)valH);
  m += " %</h1><br>";

  //m += generateHtmlSaveValuePart();

  m += "</center>";

  m += "\n<br>ErrorCount: " + String(errorCount);
  
  #ifndef SKIP_TS_COMMUNICATION
  m += "\n\n<br>Last update status: " + (lastTSUpdateStatus==200?"OK":String(lastTSUpdateStatus));
  #endif
  
  m += "\n<br>Elasped time: " + String(elapsedTime);
  

  m += "<br><a href=\"http://www.idokep.hu/\" target=\"_blank\" title=\"Idojaras\"><img src=\"//www.idokep.hu/terkep/hu_mini/s_anim.gif\"></a>";
  
  m += "\t</body>\r\n";
  m += "</html>";
  
  return m;
}

void HandleRoot(){
  //Serial.println("HandleRoot called...");
  //String message = "<!DOCTYPE html><html lang=\"en\">";
  //message += "<head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head>";
  String message = generateHtmlHeader();
  message += generateHtmlBody();

  server.send(200, "text/html", message );
}

String getCurrentPhaseState(){
     //Declare an object of class HTTPClient
 
    // old
    //http.begin("192.168.1.170:81/data");  //Specify request destination
    // new
    String statusServerPath = "http://192.168.1.170:81/data";
    http.begin(client, statusServerPath.c_str());  //Specify request destination


    int httpCode = http.GET();                                  //Send the request
    //Serial.print("PhaseStatus - resultCode: " + String(httpCode) + " ps: ");
    
    String payload = "";
    if (httpCode > 0) { //Check the returning code
      payload = http.getString();   //Get the request response payload
      //Serial.print(payload);
      //Serial.println(payload);             //Print the response payload
    }
    //Serial.println("");
 
    http.end();   //Close connection
    return payload;
}

void HandleData(){
  //Serial.println("HandleData called...");
  String message = "" +  String(valC) + " " +  String(valH);
  server.send(200, "text/html", message );
}

void HandleMultipleData(){
  //Serial.println("HandleData called...");
  lastPhaseStatus = getCurrentPhaseState();
  String message = "" +  String(valC) + "|" +  String(valH) + "|" + lastPhaseStatus;
  server.send(200, "text/html", message );
}


void showPureValues(){
  Serial.println("showPureValues() called...");
  String message = "{\n\r";
  
  message += "\t\"IoT device id\": \"" + String(IOT_DEVICE_ID) + "\",\n\r";
  message += "\t\"location\": \"" + String(LOCATION_NAME) + "\",\n\r";
  message += "\t\"elapsedTime\": \"" + String(elapsedTime) + "\",\n\r";
  message += "\t\"values\": {\n\r";
  message += "\t\t\"tempC\": \"" + String(valC) + "\",\n\r";
  message += "\t\t\"tempF\": \"" + String(valF) + "\",\n\r";
  message += "\t\t\"humidity\": \"" + String(valH) + "\",\n\r";
  message += "\t\t\"heatIndex\": \"" + String(valT) + "\"\n\r";
  message += "\t},\n\r";
  message += "\t\"phaseStatus\": \"" + getCurrentPhaseState() + "\"\n\r";
  message += "}\n\r";
  server.send(200, "text/json", message );
}


// ==========================================================================================================================
//                                                        WiFi handling
// ==========================================================================================================================

int connectToWiFi(String ssid, String pw){
  // Setup WIFI
  WiFi.begin(ssid, password);
  Serial.println("Try to connect (SSID: " + String(ssid) + ")");
  
  // Wait for WIFI connection
  int tryCount = 0;
  while ( (WiFi.status() != WL_CONNECTED) && (tryCount < WIFI_TRY_COUNT)) {
    delay(WIFI_WAIT_IN_MS);
    tryCount++;
    Serial.print(".");
  }

  if(WiFi.status() == WL_CONNECTED){
    return 1;
  }else{
    return 0;
  }
}

int initWiFi(){
  
  if(connectToWiFi(ssid, password)){
    Serial.println("");
    Serial.print("Connected. IP address: ");
    Serial.println(WiFi.localIP());
  }else{
    Serial.println("");
    Serial.print("ERROR: Unable to connect to ");
    Serial.println(ssid);
    doRestart();
  }
}


// ==========================================================================================================================
//                                                        Init
// ==========================================================================================================================
 
void setup() {

  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output

  elapsedTime = millis();
  
  EEPROM.write(0, updateCount);

  turnLED(ON);

  errorCount = 0;
  
  Serial.begin(SERIAL_BOUND_RATE);
  Serial.println();
  Serial.println("\n");
  Serial.print(String(TITLE) + " ");
  Serial.println(VERSION);
  
  dht.setup(DHT11_PIN, DHTesp::DHT11); //for DHT11 Connect DHT sensor to GPIO 17
  //dht.setup(DHTpin, DHTesp::DHT22); //for DHT22 Connect DHT sensor to GPIO 17

  initWiFi();


  server.on("/", HandleRoot);
  server.on("/data", HandleData);
  server.on("/data2", HandleMultipleData);
  //server.on ("/save", handleSave);
  server.on("/pure", showPureValues);
  server.on("/rst", HandleNotRstEndpoint);
  server.onNotFound( HandleNotFound );
  server.begin();
  Serial.println("HTTP server started at ip " + WiFi.localIP().toString() + String("@ port: ") + String(SERVER_PORT) );

  turnLED(OFF);

  Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");
  lastTemp = -100000;

  delay(10000);
  
}


// ==========================================================================================================================
//                                                        Loop
// ==========================================================================================================================

void sensorLoop(long now){
  if( (now - lastTemp) > DELAY_BETWEEN_ITERATIONS_IN_MS ){ //Take a measurement at a fixed time (durationTemp = 5000ms, 5s)
    //delay(dht.getMinimumSamplingPeriod());
    //delay(dht.getMinimumSamplingPeriod());
    lastPhaseStatus = getCurrentPhaseState();
    
    lastTemp = now;
    elapsedTime = now;
  
    turnLED(ON);
    float humidity = dht.getHumidity();
    float temperature = dht.getTemperature();
    turnLED(OFF);

    valC = temperature;
    valH = humidity;
    valF = dht.toFahrenheit(temperature);
    valT = dht.computeHeatIndex(temperature, humidity, false);
   
    Serial.print("PhaseStatus: ");
    Serial.print(lastPhaseStatus);
    Serial.print("\t");
    Serial.print(dht.getStatusString());
    Serial.print("\t");
    Serial.print(humidity, 1);
    Serial.print("% \t\t");
    Serial.print(temperature, 1);
    Serial.print(" C\t\t");
    Serial.print(valF, 1);
    Serial.print(" F\t\t");
    Serial.print(valT, 1);
    Serial.print("\t\t");
    Serial.println(dht.computeHeatIndex(dht.toFahrenheit(temperature), humidity, true), 1);

    #ifndef SKIP_TS_COMMUNICATION
    String st = dht.getStatusString();
    if(st.length() < 4){
      /*
      if( sendValuesToTSServer(heatindex, humidity, temperature) ){
        // Reset errorCount because we had a normal value reading and a normal online communication after that..
        errorCount = 0;
      }
      /**/
      
      sendValuesToTSServer(valT, humidity, temperature);
    }else{
      //Show temp and/or humidity reading error..
      //ledBlink(5, 1000, 1000);
      
      // Increase error count for restarting after defined max error counts..
      //errorCount++;
    }
    Serial.println("Error count: " + String(errorCount));
    #endif

    lastTemp = now;
  
    if(errorCount >= ERROR_COUNT_BEFORE_RESTART){
      Serial.println("\nDefined error count (" + String(ERROR_COUNT_BEFORE_RESTART) + ") reched!");
      doRestart();
    }
  }
  
}
 
void loop() {
  long t = millis();
  server.handleClient();
  sensorLoop(t);
}
