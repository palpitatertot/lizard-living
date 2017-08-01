// Requires NeoPixel library
// Requires ESP9266 Board

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define STRIP_BUTTON D2
#define STRIP_PIN D6
#define STRIP_LENGTH 41

uint8_t gDay = 255, bDay = 255, rDay = 255, 
        gNight = 2, bNight = 30, rNight = 15;

// Set targets to night in case of power failure, dim is better than too bright
uint8_t gTarget = gNight, bTarget = bNight, rTarget = rNight,
        gLevel = 0, bLevel = 0,  rLevel = 0;

bool isLEDButtonPressed = false;

// REPLACE WITH YOUR INFORMATION
const char* ssid = "";
const char* password = "";

WiFiServer server(80);
WiFiUDP udp;
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; 
byte packetBuffer[NTP_PACKET_SIZE]; 

const unsigned int localPortNTP = 2390;      // local port to listen for UDP packets

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRIP_LENGTH, STRIP_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  pinMode (STRIP_BUTTON, INPUT);
  strip.begin();
  strip.show(); // Initialize all pixels to 'low'

  Serial.begin(115200);
  delay(10);
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
  server.begin();
  Serial.println("Server started");
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  Serial.println("Starting UDP");
  udp.begin(localPortNTP);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());

  // TODO maybe bake this into Daylight Savings and have it all done in the function?
  int hour = getHour();
  hour -= 6;
  hour %= 24;
  Serial.print("The hour is ");
  Serial.println(hour);
  
  if(hour > 5 && hour < 21){
    cycleDayNight();
  }
  
}

void loop() {
  
  if(gLevel != gTarget || bLevel != bTarget || rLevel != rTarget){
    changeLEDs();
    strip.show();
    delay(50);
  }

  int val = digitalRead(STRIP_BUTTON);
  if (val == LOW && !isLEDButtonPressed){
    isLEDButtonPressed=true;
    cycleDayNight();
    Serial.println("Button Pressed");    
  } 
  else if (isLEDButtonPressed && val == HIGH){
    Serial.println("Button Released");  
    isLEDButtonPressed = false;
  }
    
  WiFiClient client = server.available();
  if (client) {
    Serial.println("new client");
    while (!client.available()) {
      delay(1);
    }
 
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();
  
    if (request.indexOf("/CYCLE_LEDS") != -1)  {
      cycleDayNight();
    }
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("");
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<br><br>");
    client.println("<a href=\"/CYCLE_LEDS\"\"><button>Cycle LEDs</button></a>");
    client.println("<br><br>");
    client.println("</html>");
    delay(1);
    Serial.println("Client disonnected");
    Serial.println("");
  }
}

void cycleDayNight(){
  Serial.println("Cycling Day and Night");
  if(gTarget == gDay){
        gTarget = gNight;
        bTarget = bNight;
        rTarget = rNight;
      }
      else {
        gTarget = gDay;
        bTarget = bDay;
        rTarget = rDay;
      }
}

//TODO ugly
void changeLEDs(){
  if(gLevel != gTarget){
      if(gLevel < gTarget){
        ++gLevel;
     }
     else {
       --gLevel;
     }
    }
    if(bLevel != bTarget){
      if(bLevel < bTarget){
        ++bLevel;
     }
     else {
       --bLevel;
     }
    }
    if(rLevel != rTarget){
      if(rLevel < rTarget){
        ++rLevel;
     }
     else {
       --rLevel;
     }
    }
    for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, rLevel, gLevel, bLevel);
    }
}

int getHour(){
  int hour;
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP);
  delay(1000);

  while(true){
    int cb = udp.parsePacket();
    if (!cb) {
      Serial.println("no packet yet");
    }
    else {
      Serial.print("packet received, length=");
      Serial.println(cb);
      udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  
      //the timestamp starts at byte 40 of the received packet and is four bytes
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      Serial.print("Unix time = ");
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      unsigned long epoch = secsSince1900 - seventyYears;
      Serial.println(epoch);
 
      Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
      hour = (epoch  % 86400L) / 3600;
      Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
      Serial.print(':');
      if ( ((epoch % 3600) / 60) < 10 ) {
        Serial.print('0');
      }
      Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
      Serial.print(':');
      if ( (epoch % 60) < 10 ) {
        Serial.print('0');
      }
      Serial.println(epoch % 60); // print the second
    }
    return hour;
  }
}

unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  
  udp.beginPacket(address, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
