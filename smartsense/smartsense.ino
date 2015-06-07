#include <SPI.h>
#include <Ethernet.h>

#define DEBUG 
#define VERSION 1.0

// ********** NETWORK SETTINGS ********** //

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(10,0,0,11);
IPAddress myDns(8, 8, 8, 8);
IPAddress gateway(10, 0, 0, 138);
IPAddress subnet(255, 255, 255, 0);

//char server[] = "www.arduino.cc";
IPAddress server(10,0,0,3);
int remoteServerPort = 8080;

// initialize the library instance:
EthernetClient client;

unsigned long lastConnectionTime = 0;             // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 10000; // delay between updates, in milliseconds


// ********** SYSTEM VARIABLES ********** //

double temp1;
double temp2;
double humidity1;
double humidity2;

String postdata;

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
  #endif

  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP, trying with static ip");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns, gateway, subnet);
  }
  // give the ethernet module time to boot up:
  delay(1000);
    
  // print the Ethernet board/shield's IP address:
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  Ethernet.maintain(); 
  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  if (client.available()) {
    char c = client.read();
    Serial.write(c);
  }

  // if ten seconds have passed since your last connection,
  // then connect again and send data:
  if (millis() - lastConnectionTime > postingInterval) {
    Serial.println("time to post");
    postRequest();
  }

}

void postRequest() {
  
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();
  
  if (client.connect(server, remoteServerPort)) {
    Serial.println("connected to the server");
    client.println("POST /test HTTP/1.1");
    client.println("Host: www.GeniuSence.com");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(postdata.length());
    client.println();
    client.print(postdata);
    client.println();
  }
  delay(2000);
   
  if (client.connected()) {
  Serial.println();
  Serial.println("disconnecting from the server");
  client.stop();  
  }
}

// this method makes a HTTP connection to the server:
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


