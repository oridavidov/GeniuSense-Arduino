#include <SPI.h>
#include <Ethernet.h>
#include <Time.h>
#include <ArduinoJson.h>
#include <avr/pgmspace.h>

#define DEBUG 
#define VERSION "1.0.1"
#define UNIT_ID "123ABC456"
#define UNIT_TYPE "A"

#define TEMPERATURE_ARRAY_SIZE     4
#define HUMIDITY_ARRAY_SIZE 4
#define LIGHT_ARRAY_SIZE    2
#define PH_ARRAY_SIZE       1
#define OUTLET_ARRAY_SIZE   4

#ifdef DEBUG
 #define DEBUG_PRINT(x)     digitalClockDisplay(); Serial.print (x)
 #define DEBUG_PRINTDEC(x)  digitalClockDisplay(); Serial.print (x, DEC)
 #define DEBUG_PRINTLN(x)   digitalClockDisplay(); Serial.println (x)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTDEC(x)
 #define DEBUG_PRINTLN(x)
#endif 


////////////////////////////////////////////
// A0
// A1
// A2
// A3 - analog mux1 input
// A4 - i2c SDA
// A5 - i2c SCL

// 01 - HW Serial RX
// 02 - HW Serial TX 
// 03 - not in use
// 04 - SD card
// 05 - not in use
// 06 - not in use
// 07 - analog mux1 select-1
// 08 - analog mux1 select-2
// 09 - analog mux1 select-3
// 10 - Ethernet shield
// 11 - Ethernet shield
// 12 - Ethernet shield
// 13 - Ethernet shield

////////////////////////////////////////////
// ********** NETWORK SETTINGS ********** //

byte mac[] = {0x1a, 0x2b, 0xBE, 0xEF, 0xFE, 0xED};
//IPAddress ip(10,0,0,11);
//IPAddress myDns(8, 8, 8, 8);
//IPAddress gateway(10,0,0,138);
//IPAddress subnet(255, 255, 255, 0);

//char server[] = "www.arduino.cc";
//IPAddress server(192, 168, 0, 79);
IPAddress server(10, 0, 0, 3);
const int remoteServerPort = 8080;

// initialize the library instance:
EthernetClient client;

unsigned long lastPostingTime = 0;             // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 60000; // delay between updates, in milliseconds

// ********** DataTick VARIABLES ********** //
double temperature[TEMPERATURE_ARRAY_SIZE] = {25.6, 30, 15.5, 0};
double humidity[HUMIDITY_ARRAY_SIZE] = {60, 77.7, 0, 54.76};
int    light[LIGHT_ARRAY_SIZE] = {20000, 1500};
double ph[PH_ARRAY_SIZE] = {7.8};

char   outlet[OUTLET_ARRAY_SIZE] = {0};


StaticJsonBuffer<JSON_OBJECT_SIZE(5) + JSON_ARRAY_SIZE(8+1)> ruleJsonBuffer;

void setup() {
  // start serial port:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }  
  freeMem("start setup");  
  #ifdef DEBUG
  Serial.println(F("****** GeniuSence Modular Unit Started ******"));
  Serial.print("Software Version: ");
  Serial.println(VERSION);
  Serial.print(F("Posting Interval: "));
  Serial.println(postingInterval);
  #endif
  
  // give the ethernet module time to boot up:
  delay(2000);
  
  // start the Ethernet connection:
  unsigned int connectionInterval = 10;
  while (Ethernet.begin(mac) == 0) {
    Serial.print("Failed to configure Ethernet using DHCP, trying again in ");
    Serial.print(connectionInterval);
    Serial.println(F("seconds"));
    delay(1000 * connectionInterval);
    connectionInterval*=2;
    if (connectionInterval > 640)
      connectionInterval = 640;
    // try to congifure using IP address instead of DHCP:
    //Ethernet.begin(mac, ip, myDns, gateway, subnet);
  }  
        
  // print the Ethernet board/shield's IP address:
  Serial.print(F("My IP address: "));
  Serial.println(Ethernet.localIP());
  
  freeMem("end setup");  
}

void loop() {
  freeMem("start loop");     
  delay(5000);  
  Ethernet.maintain();   

  if (timeStatus() == timeNotSet) {
    //freeMem("before send time request");     
    sendTimeRequest(); 
  }
  //freeMem("before checkResponse");     
  checkForResponse();  

  //collectSensorsData();
  //ProcessRules();

  if (millis() - lastPostingTime > postingInterval) {    
    postData();       
    lastPostingTime = millis(); 
  }    
  
}// end while loop

void checkForResponse() {  
  Serial.flush();
  if (client.available()) {
    char httpCode[16], dataSize[3];
    int i;
    
    // get http code
    for (i=0; i<15; i++) {
      httpCode[i] = client.read();;
    }
    httpCode[15] = '\0';
    
    if (strcmp(httpCode, "HTTP/1.1 200 OK") == 0) {
      Serial.println(F("got 200 ok"));

      // clear http headers
      for (i=0; i<147; i++) {
        client.read();
      }      

      // get the data size
      for (i=0; i<2; i++) {
        dataSize[i] = client.read();
      }
      dataSize[2] = '\0';      
      int len = strtol(dataSize, NULL, 16) + 2;
      Serial.println("data size: " + len);
      char data[len];
      
      // get the data
      for (i=0; i<len; i++) {
        data[i] = client.read();
      }
      data[len] = '\0';
      
      Serial.println(data);
      processData(data);  
      flushInBuffer();            
    }
    else {
      Serial.print(F("got http error code: "));
      Serial.println(httpCode);
      flushInBuffer();
    }
   
    
  if (client.connected()) {
  Serial.println();
  Serial.println(F("disconnecting from the server"));
  client.stop();  
  }    
}
freeMem("end of checkResponse");
} 


void postData() {  
  StaticJsonBuffer<JSON_OBJECT_SIZE(8+1) + JSON_ARRAY_SIZE(11+1)> dataTickJsonBuffer;
  JsonObject& dataTick = dataTickJsonBuffer.createObject();
        
  Serial.println(F("posting dataTick"));

  dataTick["time"]  = now();
  dataTick["unitId"]   = UNIT_ID;
  dataTick["unitType"] = UNIT_TYPE;
  dataTick["msgType"]  = "DATA_TICK";

  // add temperature data
  JsonArray& temperatureArray = dataTick.createNestedArray("temperature");
  for (int i=0; i<TEMPERATURE_ARRAY_SIZE; i++) {
    temperatureArray.add(double_with_n_digits(temperature[i], 2));
  }

  // add humidit data
  JsonArray& humidityArray = dataTick.createNestedArray("humidity");
  for (int i=0; i<HUMIDITY_ARRAY_SIZE; i++) {
    humidityArray.add(double_with_n_digits(humidity[i], 2));
  }

  // add light data
  JsonArray& lightArray = dataTick.createNestedArray("light");
  for (int i=0; i<LIGHT_ARRAY_SIZE; i++) {
    lightArray.add(light[i]);
  }

  // add ph data
  JsonArray& phArray = dataTick.createNestedArray("ph");
  for (int i=0; i<PH_ARRAY_SIZE; i++) {
    phArray.add(double_with_n_digits(ph[i], 2));
  }
   
  Serial.print("json size: ");
  Serial.println(dataTick.measureLength());
  dataTick.printTo(Serial);
 
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();
  
  if (client.connect(server, remoteServerPort)) {
    Serial.println(F("connected to the server"));
    client.println(F("POST /getdata HTTP/1.1"));
    client.println(F("Host: www.GeniuSence.com"));
    client.println(F("Content-Type: application/json"));
    client.println(F("Connection: close"));   
    client.print(F("Content-Length: "));
    client.println(dataTick.measureLength());
    client.println();
    dataTick.printTo(client);        
  }
  delay(2000);
  }


// this method makes a HTTP-GET connection to the server:
void sendTimeRequest() {
  Serial.println(F("sending http-get time request"));
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();

  // if there's a successful connection:
  if (client.connect(server, remoteServerPort)) {
    Serial.println(F("connected..."));
    // send the HTTP PUT request:
    client.println(F("GET /getCurrentTime HTTP/1.1"));
    client.println(F("Content-Type: application/json"));
    client.println(F("User-Agent: arduino-ethernet"));    
    client.println(F("Host: www.gen.com"));
    client.println(F("Connection: close"));
    client.println();
    delay(2000);  
  }
  else {
    // if you couldn't make a connection:
    Serial.println(F("connection failed"));
  }
  freeMem("end of send time request");     
}

//Code to print out the free memory

struct __freelist {
  size_t sz;
  struct __freelist *nx;
};

extern char * const __brkval;
extern struct __freelist *__flp;

uint16_t freeMem(uint16_t *biggest)
{
  char *brkval;
  char *cp;
  unsigned freeSpace;
  struct __freelist *fp1, *fp2;

  brkval = __brkval;
  if (brkval == 0) {
    brkval = __malloc_heap_start;
  }
  cp = __malloc_heap_end;
  if (cp == 0) {
    cp = ((char *)AVR_STACK_POINTER_REG) - __malloc_margin;
  }
  if (cp <= brkval) return 0;

  freeSpace = cp - brkval;

  for (*biggest = 0, fp1 = __flp, fp2 = 0;
     fp1;
     fp2 = fp1, fp1 = fp1->nx) {
      if (fp1->sz > *biggest) *biggest = fp1->sz;
    freeSpace += fp1->sz;
  }

  return freeSpace;
}

uint16_t biggest;

void freeMem(char* message) {
  Serial.print(message);
  Serial.print(":\t");
  Serial.println(freeMem(&biggest));
}

void flushInBuffer() {
      while (client.available()) {
        client.read();
      }
    }

    void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(day());
  Serial.print("/");
  Serial.print(month());
  Serial.print("/");
  Serial.print(year()); 
  Serial.print(" ");
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" "); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

 void processData(char* data) {
        
      JsonObject& response = ruleJsonBuffer.parseObject(data);

      if (!response.success()) {
        Serial.println("parseObject() failed");
      }
      else {
        
        const char* msgtype = response["msgType"];

        //////////  TIME_REQUEST  //////////
        if (strcmp(msgtype, "TIME_REQUEST") == 0) {
           long time = response["time"];
           setTime(time); 
           Serial.print(F("time was update to: "));
           digitalClockDisplay();                     
        }
        
        else {
          Serial.println(F("got unknown msg type"));
        }      
      }  
      }

     

  







