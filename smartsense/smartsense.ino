#include <SPI.h>
#include <Ethernet.h>
#include <aJSON.h>
#include <avr/pgmspace.h>

#define DEBUG 
#define VERSION 1.0

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
IPAddress ip(10,0,0,11);
IPAddress myDns(8, 8, 8, 8);
IPAddress gateway(10,0,0,138);
IPAddress subnet(255, 255, 255, 0);

//char server[] = "www.arduino.cc";
//IPAddress server(192, 168, 0, 79);
IPAddress server(10, 0, 0, 3);
int remoteServerPort = 8080;

// initialize the library instance:
EthernetClient client;

unsigned long lastConnectionTime = 0;             // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 60000; // delay between updates, in milliseconds

// ********** SYSTEM VARIABLES ********** //
aJsonObject* root;
char unit_id[] = "123ABC456";
char unit_type[] = "A";
double temp1 = 33.3;
double temp2 = 50;
double humidity1 = 66.6;
double humidity2 = 60;
String incomingData;


void setup() {
  // start serial port:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }    
  
  #ifdef DEBUG
  Serial.println("****** GeniuSence Modular Unit Started ******");
  Serial.print("Software Version: ");
  Serial.println(VERSION);
  Serial.print("Posting Interval: ");
  Serial.println(postingInterval);
  #endif
  
  // give the ethernet module time to boot up:
  delay(1000);
  
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP, trying with static ip");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns, gateway, subnet);
  }  
      
  // print the Ethernet board/shield's IP address:
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
  freeMem("end setup");  
}

void loop() {
  delay(5000);
  
  Ethernet.maintain();   
  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  while (client.available()) {    
    char c = client.read();
    incomingData += c;

    if(incomingData.endsWith("Connection: close")) {
      incomingData="";
    }    
  }

  Serial.println(incomingData);
  Serial.flush();
  
  
  freeMem("after print incoming data"); 

  // if ten seconds have passed since your last connection,
  // then connect again and send data:
  if (millis() - lastConnectionTime > postingInterval) {
    postRequest();     
    lastConnectionTime = millis(); 
  }  
  
  freeMem("end of loop");
}

void postRequest() {
  Serial.println("posting....");
  buildJSON();    
  char* string = aJson.print(root);
  
  if (string != NULL) {
    Serial.print("string: ");
    Serial.println(string);
  }
  
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();
  
  if (client.connect(server, remoteServerPort)) {
    Serial.println("connected to the server");
    client.println("POST /getdata HTTP/1.1");
    client.println("Host: www.GeniuSence.com");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(strlen(string));
    client.println();
    client.print(string);
    client.println();
  }
  delay(2000);

  /* 
  if (client.connected()) {
  Serial.println();
  Serial.println("disconnecting from the server");
  client.stop();  
  }  
  */
  aJson.deleteItem(root);
    free(string);
  
}

// this method makes a HTTP-GET connection to the server:
void httpRequest() {
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();

  // if there's a successful connection:
  if (client.connect(server, remoteServerPort)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    client.println("GET /test HTTP/1.1");
    client.println("Host: Localhost");
    client.println("User-Agent: arduino-ethernet");
    client.println("Connection: close");
    client.println();

    // note the time that the connection was made:
    lastConnectionTime = millis();
  }
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }
  
}




void buildJSON() {  
  root = aJson.createObject();
  if (root != NULL) {
    //printProgStr( OBJECT_CREATE_STRING);
    Serial.println("create object root");
  } 
  else {
    //printProgStr( OBJECT_CREATION_FAILED_STRING);
    Serial.println("object root failed to create");
    return;
  }
  
  aJson.addStringToObject(root,"unit_id", unit_id);
  aJson.addStringToObject(root,"unit_type", unit_type);
  //aJson.addFalseToObject (fmt,"interlace");
  aJson.addNumberToObject(root, "temperature1", temp1);
  aJson.addNumberToObject(root, "temperature2", temp2);
  aJson.addNumberToObject(root, "humidity1", humidity1);
  aJson.addNumberToObject(root, "humidity2", humidity2);    
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


