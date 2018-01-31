/*
 *  This sketch demonstrates how to set up a simple HTTP-like server.
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/gpio/0 will set the GPIO2 low,
 *    http://server_ip/gpio/1 will set the GPIO2 high
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "DaSE-314-Printer";
const char* password = "dase314314";

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);


unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;



int cooling_pin = D8;
int cooling_status = 0;

int last_udp, now;
bool waiting_udp_response;

bool auto_off_flag;
bool auto_on_23_flag;

void setup() {
  Serial.begin(115200);
  delay(10);

  // prepare GPIO
  pinMode(cooling_pin, OUTPUT);
  digitalWrite(cooling_pin, 1);
  //last_write_pin = millis();

  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
  

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  waiting_udp_response = false;
  last_udp = millis();
  auto_off_flag=true;
  auto_on_23_flag=true;
  
}

void loop() {
  // Check if a client has connected
  
  WiFiClient client = server.available();
  if (!client) {
    now = millis();
    if (waiting_udp_response && now-last_udp > 1000) {
      waiting_udp_response = false;
      int cb = udp.parsePacket();
      if (!cb) {
        Serial.println("no packet yet");
      }
      else {
        //Serial.print("packet received, length=");
        //Serial.println(cb);
        // We've received a packet, read the data from it
        udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    
        //the timestamp starts at byte 40 of the received packet and is four bytes,
        // or two words, long. First, esxtract the two words:
    
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        // combine the four bytes (two words) into a long integer
        // this is NTP time (seconds since Jan 1 1900):
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        //Serial.print("Seconds since Jan 1 1900 = " );
        //Serial.println(secsSince1900);
    
        // now convert NTP time into everyday time:
        //Serial.print("Unix time = ");
        // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
        const unsigned long seventyYears = 2208988800UL;
        // subtract seventy years:
        unsigned long epoch = secsSince1900 - seventyYears + 28800; //UTC+8
        // print Unix time:
        //Serial.println(epoch);
    
    
        // print the hour, minute and second:
        //Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
        //Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
        //Serial.print(':');
        //if ( ((epoch % 3600) / 60) < 10 ) {
          // In the first 10 minutes of each hour, we'll want a leading '0'
          //Serial.print('0');
        //}
        //Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
        //Serial.print(':');
        //if ( (epoch % 60) < 10 ) {
          // In the first 10 seconds of each minute, we'll want a leading '0'
          //Serial.print('0');
        //}
        //Serial.println(epoch % 60); // print the second
        int hour = (epoch % 86400L) / 3600;
        int minute = (epoch  % 3600) / 60;
        int second = (epoch % 60);

        if (0 == hour && 0 == minute) {
          if (auto_off_flag) {
            auto_off_flag = false;
            auto_on_23_flag = true;
            cooling_status = 0;
            digitalWrite(cooling_pin,cooling_status);
          }
        }

        if (7 == hour && 0 == minute) {
          if (auto_on_23_flag) {
            auto_on_23_flag = false;
            auto_off_flag = true;
            cooling_status = 1;
            digitalWrite(cooling_pin,cooling_status);
          }
        }

        if (8 == hour && 50 == minute) {
          if (auto_off_flag) {
            auto_off_flag = false;
            auto_on_23_flag = true;
            cooling_status = 0;
            digitalWrite(cooling_pin,cooling_status);
          }
        }
        

        if (22 == hour && 0 == minute) {
          if (auto_on_23_flag) {
            auto_on_23_flag = false;
            auto_off_flag = true;
            cooling_status = 1;
            digitalWrite(cooling_pin,cooling_status);
          }
        }
 
      }
    }
    else if (now - last_udp > 10*1000) {
      WiFi.hostByName(ntpServerName, timeServerIP); 
      sendNTPpacket(timeServerIP);
      last_udp = millis();
      waiting_udp_response=true;
    }
    return;
  }
  
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  
  // Match the request
  if (req.indexOf("/gpio/1") != -1) {
    cooling_status = 1;
    digitalWrite(cooling_pin,cooling_status);
  }
  else if (req.indexOf("/gpio/0") != -1) {
    cooling_status = 0;
    digitalWrite(cooling_pin,cooling_status);
  }
  else {
    Serial.println("invalid request");
    client.stop();
    return;
  }

  // Set GPIO2 according to the request
  //digitalWrite(2, val);
  
  client.flush();

  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nLight is now ";
  s += (cooling_status)?"On":"Off";
  s += "\r\n</html>\r\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}


// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  //Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

