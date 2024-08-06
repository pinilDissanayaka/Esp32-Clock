#include "TM1637Display.h"
#include "Adafruit_NeoPixel.h"
#include "NTPClient.h"
#include "WiFiManager.h"
#include <RTClib.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        17
#define display_CLK 18
#define display_DIO 19

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 35


//========================USEFUL VARIABLES=============================
int Display_backlight = 2; // Adjust it 0 to 7
int led_ring_brightness = 10; // Adjust it 0 to 255
int led_ring_brightness_flash = 250; // Adjust it 0 to 255

// ========================================================================
// ========================================================================


// Adjust the color of the led ring
#define red 00
#define green 20
#define blue 255

// When setting up the NeoPixel library, we tell it how many pixels,
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
TM1637Display display(display_CLK, display_DIO);
int flag = 0;
int alarmFlag=0;

// Initialize RTC
RTC_DS3231 rtc;

WebServer server(80);

int alarmHour;
int alarmMinute;

void setup() {
  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);

  display.setBrightness(Display_backlight);
  pixels.begin(); // INITIALIZE NeoPixel pixels object
  pixels.setBrightness(led_ring_brightness);

  Serial.begin(115200);
  Serial.println("\n Starting");

  setupWiFiManager();
  //setSoftAP(); // setup access point for esp32


  setRTCTime();

  for(int i=0; i<NUMPIXELS;i++){
    pixels.setPixelColor(i, pixels.Color(red, green, blue));
    pixels.show();
    delay(50);
  }

  flash_cuckoo();// white flash

  rtc.begin();

}

void loop() {
  server.handleClient();
  // switch on the ring in blue
  pixels.clear(); // Set all pixel colors to 'off'
  blue_light();

  DateTime now=getRTCTime();

  writeTime(now.hour(), now.minute()); //write time on the display

  // switch on the blue leds
  digitalWrite(25,HIGH);
  digitalWrite(26,HIGH);

  // Animation every hour
  if(now.minute()== 00 && flag==0)
  {
    display_cuckoo();
    flag=1;
  }
  if(now.minute()>=02)
  {
    flag=0;
  }

  // Animation for alarm
  if(now.hour()== alarmHour && now.hour()==alarmMinute && alarmFlag==0))
  {
    ringAlarm();
    alarmFlag=1;
  }
  if(now.minute()>=02)
  {
    alarmFlag=0;
  }

}

void blue_light(){

  pixels.setBrightness(led_ring_brightness);

  for(int i=0; i<=NUMPIXELS; i++){
    pixels.setPixelColor(i, pixels.Color(red, green, blue));
  }
  pixels.show(); 
}


void red_light(){
  pixels.setBrightness(led_ring_brightness);

  for(int i=0; i<=NUMPIXELS; i++){
    pixels.setPixelColor(i, pixels.Color(255, 0, 0));
  }
  pixels.show(); 

}

void flash_cuckoo(){
	pixels.setBrightness(led_ring_brightness_flash);

  for(int i=0; i<=NUMPIXELS; i++){
    pixels.setPixelColor(i, pixels.Color(250,250,250));
  }
    
  pixels.show();

  for (int i=led_ring_brightness_flash; i>10 ; i--){
    pixels.setBrightness(i);
    pixels.show();
    delay(7);
  }

  blue_light();
  
}

void display_cuckoo(){

  for (int i =0 ; i<88 ; i++)
  {
    display.showNumberDecEx(i,0b01000000,true,2,0);
    display.showNumberDecEx(i,0b01000000,true,2,2);

  }

  display.showNumberDecEx(88,0b01000000,true,2,0);
  display.showNumberDecEx(88,0b01000000,true,2,2);
  flash_cuckoo();
  delay(2000);

}

void writeTime(int hour, int minute){

  if (hour == 0)
  { 
    hour = 12;
  }
  else if (hour == 12)
  { 
    hour = hour;
  }
  else if (hour >= 13) {
    hour = hour - 12;
  }
  else {
    hour = hour;
  }

  // write the time on the display
  display.showNumberDecEx(hour,0b01000000,true,2,0);
  display.showNumberDecEx(minute,0b01000000,true,2,2);
}

void setupWiFiManager(){
  WiFiManager manager;   

  //manager.resetSettings();

  manager.setTimeout(180);

  //fetches ssid and password and tries to connect, if connections succeeds it starts an access point with the name called "IRON_MAN_ARC" and waits in a blocking loop for configuration
  bool res = manager.autoConnect("IRON_MAN_ARC","password");
  
  if(!res) {

    Serial.println("failed to connect and timeout occurred");

    for(int i=0; i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(red, green, blue));
      pixels.show();
      delay(50);
    }
    ESP.restart(); //reset and try again
  }
  pixels.clear(); // Set all pixel colors to 'off'
}

void setRTCTime(){
  int UTC = 2; // UTC = value in hour (SUMMER TIME) [For example: Paris UTC+2 => UTC=2]

  const long utcOffsetInSeconds = 3600; // UTC + 1H / Offset in second

  // Define NTP Client to get time
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds*UTC);
  
  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    red_light();
    while (1);
  }

  // Initialize NTP Client
  timeClient.begin();

  timeClient.update();

  // Get NTP time
  unsigned long epochTime = timeClient.getEpochTime();
  DateTime now = DateTime(epochTime);

  // Set RTC time
  rtc.adjust(now);
  
  Serial.println("Time set successfully");

  // Print current time to serial monitor
  printDateTime(now);
  pixels.clear(); // Set all pixel colors to 'off'
}

// Function to print date and time
void printDateTime(DateTime dt) {
  char buffer[20];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
  Serial.println(buffer);
}

DateTime getRTCTime(){
  DateTime now = rtc.now();

  return now;
}

void setSoftAP(){
  // Set ESP32 as an access point with a fixed IP address
  WiFi.softAP("IRON_MAN_ARC", "password", 7); // SSID: ESP32_AP, Password: password, Channel: 7
  IPAddress ip(192, 168, 1, 1); // Set the IP address of the access point
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(ip, gateway, subnet);

  // Print the IP address of the access point
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());


  server.on("/", handleRoot);
  server.on("/set", handleSet);

  server.begin();
  Serial.println("HTTP server started");
}

void handleRoot() {
  char index_html[] PROGMEM = R"rawliteral(
                                          <!DOCTYPE html>
                                          <html>
                                          <head>
                                            <title>Set Alarms</title>
                                          </head>
                                          <body>
                                            <h2>Set Relay Times</h2>
                                            <form action="/set" method="POST">
                                              ON Time (HH:MM):<br>
                                              <input type="text" name="alarmHour"><br>
                                              OFF Time (HH:MM):<br>
                                              <input type="text" name="alarmMinute"><br>
                                              <input type="submit" value="Set Times">
                                            </form>
                                          </body>
                                          </html>
                                          )rawliteral";

  server.send(200, "text/html", index_html);
}

void handleSet() {
  alarmHour = server.arg("alarmHour").toInt();
  alarmMinute = server.arg("alarmMinute").toInt();

  char set_html[] PROGMEM = R"rawliteral(
                                          <!DOCTYPE html>
                                          <html>
                                          <head>
                                            <title>Set Alarms</title>
                                          </head>
                                          <body>
                                            <h2>Times set successfully!</h2>
                                          </body>
                                          </html>
                                          )rawliteral";

  server.send(200, "text/html", set_html);
}

void ringAlarm(){
  pixels.setBrightness(led_ring_brightness_flash);

  for(int i=0; i<=NUMPIXELS; i++){
    pixels.setPixelColor(i, pixels.Color(250,250,250));
  }
  pixels.show();

  for (int i=led_ring_brightness_flash; i>10 ; i--){
    pixels.setBrightness(i);
    pixels.show();
    delay(7);
  }
  blue_light();

}








