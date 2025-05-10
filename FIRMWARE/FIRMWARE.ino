


//Global headers
#include <Arduino.h>
#include <U8g2lib.h>
#include "Freenove_WS2812_Lib_for_ESP32.h"
#include <SPI.h>
#include <WiFi.h>
#include "time.h"
#include <Preferences.h>

#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>

//Local headers
#include "BSK_2025.h"
#include "BSK_Logo.h"
#include "FP_Logo.h"
#include "Battery_Image.h"
#include "Oompa.h"
#include "Small_Battery.h"
#include "htp_smiley.h"

#define CHARGING_TIMEOUT 3000 //screen time out 30s
#define BATTERY_READ 2000 //Battery read interval 10s
#define FRAME_TIME 400 //Animation frames speed
#define TIME_UPDATE 30000 //Time update interval 30s
#define SCROLL_RATE 30 //Text scroll speed 10ms
#define BLE_SCAN_INTERVAL 2000
#define U8G2_16BIT //Allows longer text width

Preferences preferences;

BLEScan *pBLEScan;

U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ CS, /* dc=*/ DC, /* reset=*/ RES); //OLED initializer

Freenove_ESP32_WS2812 statusLED = Freenove_ESP32_WS2812(1, STAT_LED, 0, TYPE_GRB); //Stat LED initializer


//EEPROM Vars
String ssid;
String password;
String name;
String BT_Dev = "";
int screenTimeout;
long  gmtOffset_sec; //-5Hr
bool debug;
int scrollSpeed;
int cutOff;

//Constants
const char* ntpServer = "Time.nist.gov"; //NTP time server
const int   daylightOffset_sec = 3600; //1Hr



int oompa = 1; //Number of Oompas
int old_oompa = 0; //Current Oompa number
int logo = 0; //Logo to dispay
uint8_t old_buttons = 0x00; //Current press var
int old_logo = 0; //Current Logo
int old_frame = 0; //Current animation frame
bool screenOn = true; //Screen On var 
uint8_t state, oldState = 0; //NP LED status vr
int new_time, old_screenTime = 0; //time keepers
uint8_t batLevel, old_batLevel = 0; //Battery level change dectector
uint8_t onState, old_onState = 0; //On Switch Change
float batVoltage = 0; //battery Voltage
int raw_batVoltage = 0; //ADC reading
uint8_t batteryFrame = 0; //Animation frame tracker
float voltageOffset = 0.001711011052; //Voltage divider voltage offset
double batPercent = 0; //Battery percentage
int batTime = 0;
uint8_t logoCount = 3; //Logo index, set this equal to the number of logos
uint8_t stateCount = 8; //State index
uint8_t buttons = 0x00; //Holds button presses

uint8_t screenCount = 4; //Screen index chnage to 4 to show time screen
int screen = 0; //screen var
int BT_Count = 0;

u8g2_uint_t scrollIndex = 0;
int scrollTime = 0;

/* WIFI Time Vars*/
char chBuffer[8];
struct tm timeinfo;
bool timeReceived = false;

int BLEScan_time = 0;

void setup() {
  Serial.begin(115200);

  //Read onbard EEPROM vars
  preferences.begin("credentials", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  name = preferences.getString("name", "Your Name Here       ");
  screenTimeout = preferences.getInt("screenTimeout",0);
  gmtOffset_sec = preferences.getLong("gmtOffset_sec",EST);
  debug =  preferences.getBool("debug",false);
  scrollSpeed = preferences.getInt("scrollSpeed", 30);
  cutOff = preferences.getInt("cutOff", -60);
  preferences.end();


  Serial.println("");
  Serial.println("");

  //Button pin mode assignments
  pinMode(L_UP, INPUT);
  pinMode(L_RIGHT, INPUT);
  pinMode(L_DOWN, INPUT);
  pinMode(L_LEFT, INPUT);
  pinMode(R_UP, INPUT);
  pinMode(R_RIGHT, INPUT);
  pinMode(R_DOWN, INPUT);
  pinMode(R_LEFT, INPUT);

  //Board input assignments
  pinMode(BAT_SENSE, INPUT);
  pinMode(CHARGING, INPUT);
  pinMode(ON_SENSE, INPUT);

  //Board output assignments
  pinMode(BL_EN, OUTPUT);
  digitalWrite(BL_EN, HIGH); //Turn screen on

  u8g2.begin(); //Display Initializer
  statusLED.begin(); // INITIALIZE Status LED (REQUIRED)
  

  raw_batVoltage = analogRead(BAT_SENSE); //
  batVoltage = raw_batVoltage*voltageOffset;
  if(strlen(ssid.c_str()) > 1){
    Serial.print("WiFi SSID: ");
    Serial.println(ssid);
    delay(100);
    timeReceived = updateTime(ssid.c_str(), password.c_str(), ntpServer, gmtOffset_sec, daylightOffset_sec); // Wifi Time
  }else{
    Serial.println("-- No WiFi Info saved --");
  }

  BLEDevice::init("");
  
  
 

  new_time = millis(); //Initialize loop time
  screenOn = true;
  Serial.println("");
  Serial.println("***************Setup Complete*****************");
  Serial.println("");
  printMenu();
}

void loop() {
  bool onState = !digitalRead(ON_SENSE);
  bool charging = !digitalRead(CHARGING);

  buttons = getButtons();
  if(debug && buttons != old_buttons){
    Serial.print("Screen: ");
    Serial.println(screen);
    Serial.print("Logo: ");
    Serial.println(logo);
    Serial.print("Oompa: ");
    Serial.println(oompa);
    Serial.print("Status: ");
    Serial.println(state);
    Serial.print("Buttons: ");
    Serial.println(buttons);
    Serial.println("");
  }

  

  if(((new_time - old_screenTime) >= screenTimeout && screenTimeout != 0) || (((new_time - old_screenTime) >= CHARGING_TIMEOUT) && !onState)){
    u8g2.firstPage();
    if(screenOn){
      do{
        u8g2.clearDisplay();
      }while(u8g2.nextPage());
    }
    digitalWrite(BL_EN, LOW);
    screenOn = false;
    if(buttons != 0x00){ 
      old_screenTime = new_time;
    } 
  }else{
    if(!screenOn){ 
      digitalWrite(BL_EN, HIGH);
      screenOn = true;
    }
  }
    
  if(!onState && screenOn){
    if((new_time - old_frame) >= FRAME_TIME){
      old_frame = new_time;
      raw_batVoltage = analogRead(BAT_SENSE);
      batVoltage = raw_batVoltage*voltageOffset;
      double pre = batVoltage/3.7;
      double pre1 = 1 + pow(pre,80);
      double pre2 = pow(pre1,0.165);
      batPercent = 123-(123/pre2);
      batLevel = getLevel(batPercent);

      u8g2.firstPage();
      do{
        showBattery(batteryFrame,1);
      }while(u8g2.nextPage());
      batteryFrame++; //Index animation frame

      if(batteryFrame > 4 && charging){
        batteryFrame = batLevel-1;
      }else{
        batteryFrame = 4;
      }
    }
  }else if(screenOn){
    switch(screen){
      case 0: drawPageOne();break;
      case 1: drawPageTwo();break;
      case 2: drawPageThree(BATTERY_READ);break;
      case 3: drawPageFour(scrollSpeed);break;
    }
    if(state != oldState){
      setStatus(state, 10);
      oldState = state;
    }
  }
  serialHandler();
  new_time = millis();
}



void showBattery(uint8_t lvl, uint8_t color) {
  u8g2.setDrawColor(color);
  switch(lvl){
    case 0 : u8g2.drawXBM(9,2, bat_width,bat_height,bat_0_bits); break;
    case 1 : u8g2.drawXBM(9,2, bat_width,bat_height,bat_25_bits); break;
    case 2 : u8g2.drawXBM(9,2, bat_width,bat_height,bat_50_bits); break;
    case 3 : u8g2.drawXBM(9,2, bat_width,bat_height,bat_75_bits); break;
    case 4 : u8g2.drawXBM(9,2, bat_width,bat_height,bat_100_bits); break;
  }
}

void showLogo(uint8_t img, uint8_t color) {
  //Serial.println("Show Logo");
  u8g2.setDrawColor(color);
  switch(img){
    case 0 : u8g2.drawXBM(0,1, bsk_logo_width,bsk_logo_height,bsk_logo_bits); break;
    case 1 : u8g2.drawXBM(0,1, htp_smiley_width,htp_smiley_height,htp_smiley_bits); break;
    default : u8g2.drawXBM(0,1, fp_logo_width,fp_logo_height,fp_logo_bits);
  }

}

float getBattery(void) {
  //Serial.println("Get Battery");
  float level = analogRead(BAT_SENSE)*0.0018;
  return level;
}

void setStatus(uint8_t stat, uint8_t lvl) {
  switch(stat){
    case 0 :statusLED.setLedColorData(0, lvl, 0, 0); statusLED.show(); break;
    case 1 :statusLED.setLedColorData(0, lvl, lvl, 0); statusLED.show(); break;
    case 2 :statusLED.setLedColorData(0, lvl, lvl, lvl); statusLED.show(); break;
    case 3 :statusLED.setLedColorData(0, 0, lvl, 0); statusLED.show(); break;
    case 4 :statusLED.setLedColorData(0, 0, lvl, lvl); statusLED.show(); break;
    case 5 :statusLED.setLedColorData(0, 0, 0, lvl); statusLED.show(); break;
    case 6 :statusLED.setLedColorData(0, 0, lvl, 0); statusLED.show(); break;
    default :statusLED.setLedColorData(0, 0, 0, 0); statusLED.show();
  }
}
uint8_t getButtons(){
  //Serial.println("Get Buttons");
  uint8_t button_reg = 0x00;
  if(touchRead(L_UP) > TOUCHED){
    button_reg = button_reg | 0x01;
  }else{
     button_reg = button_reg & (0xFF ^ 0x01);
  }
  if(touchRead(L_RIGHT) > TOUCHED){
    button_reg = button_reg | 0x02;
  }else{
     button_reg = button_reg & (0xFF ^ 0x02);
  }
  if(touchRead(L_DOWN) > TOUCHED){
    button_reg = button_reg | 0x04;
  }else{
     button_reg = button_reg & (0xFF ^ 0x04);
  }
  if(touchRead(L_LEFT) > TOUCHED){
    button_reg = button_reg | 0x08;
  }else{
     button_reg = button_reg & (0xFF ^ 0x08);
  }
  if(touchRead(R_UP) > TOUCHED){
    button_reg = button_reg | 0x10;
  }else{
     button_reg = button_reg & (0xFF ^ 0x10);
  }
  if(touchRead(R_RIGHT) > TOUCHED){
    button_reg = button_reg | 0x20;
  }else{
     button_reg = button_reg & (0xFF ^ 0x20);
  }
  if(touchRead(R_DOWN) > TOUCHED){
    button_reg = button_reg | 0x40;
  }else{
     button_reg = button_reg & (0xFF ^ 0x40);
  }
  if(touchRead(R_LEFT) > TOUCHED){
    button_reg = button_reg | 0x80;
  }else{
     button_reg = button_reg & (0xFF ^ 0x80);
  }
  return button_reg;

}
void drawOompa(uint8_t num, uint8_t color){
  //Serial.println("Draw Oompa");
  uint8_t x = 0;
  uint8_t y = 12;
  u8g2.setDrawColor(color);
  for(int i=0; i < num; i++){
    u8g2.drawXBM(x,y, oompa_width,oompa_height,oopma_bits);
    x += 18;
    if(x > (128-18)){
      y += 26;
      x = 0;
    }
  }
}
void showLevel(int x, int y, uint8_t lvl){
  //Serial.println("Show Level");
  switch(lvl){
    case 0: showBatterySmall(x,y,0,1);break;
    case 1: showBatterySmall(x,y,1,1);break;
    case 2: showBatterySmall(x,y,2,1);break;
    case 3: showBatterySmall(x,y,3,1);break;
    case 4: showBatterySmall(x,y,4,1);break;
  }
}
void showBatterySmall(int x, int y, uint8_t lvl, uint8_t color) {
  
  u8g2.setDrawColor(color);
  switch(lvl){
    case 0 : u8g2.drawXBM(x,y, bat_42x24_width,bat_42x24_height,bat_00_42x24_bits); break;
    case 1 : u8g2.drawXBM(x,y, bat_42x24_width,bat_42x24_height,bat_25_42x24_bits); break;
    case 2 : u8g2.drawXBM(x,y, bat_42x24_width,bat_42x24_height,bat_50_42x24_bits); break;
    case 3 : u8g2.drawXBM(x,y, bat_42x24_width,bat_42x24_height,bat_75_42x24_bits); break;
    case 4 : u8g2.drawXBM(x,y, bat_42x24_width,bat_42x24_height,bat_100_42x24_bits); break;
  }

}
uint8_t getLevel(float bat){
  uint8_t lvl = 0;
  if(bat > 75){
    lvl=4;
  }else if(bat > 50){
    lvl=3;
  }else if(bat > 25){
    lvl=2;
  }else if(bat > 0){
    lvl=1;
  }else{
    lvl=0;
  }
  return lvl;
}
void drawInfoBar(int y, uint8_t radio, uint8_t bat, char * time){

  u8g2.setFont(u8g2_font_battery24_tr);
  u8g2.setFontDirection(1);
  switch(bat){
    case 4: u8g2.drawStr(100,1+y, "\u0034");break;
    case 3: u8g2.drawStr(100,1+y, "\u0033");break;
    case 2: u8g2.drawStr(100,1+y, "\u0032");break;
    case 1: u8g2.drawStr(100,1+y, "\u0031");break;
    case 0: u8g2.drawStr(100,1+y, "\u0030");break;
  }
  if(radio){
    u8g2.setFont(u8g2_font_streamline_interface_essential_wifi_t);
    u8g2.setFontDirection(3);
    u8g2.drawStr(102,21, "\u0030");
  }
  if(timeReceived){
    u8g2.setFont(u8g2_font_9x18B_mr);
    u8g2.setFontDirection(0);
    u8g2.drawStr(0,15+y,time);
  }else{
    u8g2.setFont(u8g2_font_amstrad_cpc_extended_8f);
    u8g2.setFontDirection(0);
    u8g2.drawStr(0,15+y,"No Time Info");
  }

}
bool updateTime(const char* ssid, const char* password, const char* ntpServer, const long gmtOffset_sec, const int daylightOffset_sec ){
  bool timeRecieved = true;
  
   // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  u8g2.firstPage();
      do{
        u8g2.setFont(u8g2_font_9x18B_mr);
        u8g2.setFontDirection(0);
        u8g2.drawStr(0,20,"Connecting to:");
        u8g2.drawStr(0,40,ssid);
      }while(u8g2.nextPage());

  int i = 0;
  int j = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
    do{
       u8g2.drawStr(i*10,55,"->");
       i++;
       if(i>12){
        i=0;
         u8g2.drawStr(i*10,55,"               ");
         j++;
       }
    }while(u8g2.nextPage());
    if(j>70){
      break;
    }
  }
  if(j<100){
  Serial.println("");
  Serial.println("WiFi connected.");
  u8g2.firstPage();
  do{
    u8g2.setFont(u8g2_font_9x18B_mr);
    u8g2.setFontDirection(0);
    u8g2.drawStr(20,40,"Connected!");
  }while(u8g2.nextPage());
  delay(100);
  }else{
    Serial.println("");
    Serial.println("WiFi not connected.");
    u8g2.firstPage();
    do{
      u8g2.setFont(u8g2_font_9x18B_mr);
      u8g2.setFontDirection(0);
      u8g2.drawStr(20,35,"Connection");
      u8g2.drawStr(20,55,"Failed");
    }while(u8g2.nextPage());
    delay(500);
  }
  
  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
      timeRecieved = false;
      return timeReceived;
    }else{
      timeReceived = true;
    }
  strftime(chBuffer, sizeof(chBuffer), "%I:%M%p", &timeinfo);


  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  return timeRecieved;
}
void serialHandler(void){
  String input = "";
  while(Serial.available() > 0){
      input = Serial.readString(); // Recieve String
      input.trim(); //Trim white space

    if(input != "\n"){
      int index = input.indexOf(":");
      if(input.substring(0,index) == "ssid"){ // Handles [ssid] command
        preferences.begin("credentials", false);
        preferences.putString("ssid", input.substring(index+1));
        ssid = preferences.getString("ssid","");
        preferences.end();
        Serial.print("SSID set to: ");
        Serial.println(ssid);
      }else if(input.substring(0,index) == "pw"){ // Handles [password] command
        preferences.begin("credentials", false);
        preferences.putString("password", input.substring(index+1));
        password = preferences.getString("password","");
        preferences.end();
        Serial.print("Password set to: ");
        Serial.println(password);
      }else if(input.substring(0,index) == "reboot"){ // Handles [reboot] command
        ESP.restart();
      }else if(input.substring(0,index) == "name"){ // Handles [name] command
        String nameMod = input.substring(index+1);
        nameMod = nameMod + "       ";
        preferences.begin("credentials", false);
        preferences.putString("name", nameMod.c_str());
        name = preferences.getString("name","");
        preferences.end();
        Serial.print("Name set to: ");
        Serial.println(name);
      }else if(input.substring(0,index) == "timeout"){ // Handles [timeout] command
        preferences.begin("credentials", false);
        preferences.putInt("screenTimeout", input.substring(index+1).toInt());
        screenTimeout = preferences.getInt("screenTimeout",30000);
        preferences.end();
        Serial.print("Screen timeout set to: ");
        Serial.println(screenTimeout);
        digitalWrite(BL_EN, HIGH);
        screenOn = true;
      }else if(input.substring(0,index) == "reset"){ // Handles [reset] command
        preferences.begin("credentials", false);
        preferences.putInt("screenTimeout",30000);
        screenTimeout = preferences.getInt("screenTimeout",30000);
        preferences.putString("name", "");
        name = preferences.getString("name","");
        preferences.putString("password", "");
        password = preferences.getString("password","");
        preferences.putString("ssid", "");
        ssid = preferences.getString("ssid","");
        preferences.end();
        Serial.print("Screen timeout set to: ");
        Serial.println(screenTimeout);
        digitalWrite(BL_EN, HIGH);
        Serial.print("Name set to: ");
        Serial.println(name);
        Serial.print("Password set to: ");
        Serial.println(password);
        Serial.print("SSID set to: ");
        Serial.println(ssid);
        delay(500);
        ESP.restart();
      }else if(input.substring(0,index) == "timezone"){ // Handles [timezone] command
        preferences.begin("credentials", false);
        if(input.substring(index+1) == "EST"){
          preferences.putLong("gmtOffset_sec", EST);
        }else if(input.substring(index+1) == "CST"){
          preferences.putLong("gmtOffset_sec", CST);
        }else if(input.substring(index+1) == "MNT"){
          preferences.putLong("gmtOffset_sec", MNT);
        }else if(input.substring(index+1) == "PST"){
          preferences.putLong("gmtOffset_sec", PST);
        }
        gmtOffset_sec = preferences.getLong("gmtOffset_sec",30000);
        preferences.end();
        Serial.print("Timezone set to: ");
        Serial.println(gmtOffset_sec);
        delay(500);
        ESP.restart();
      }else if(input.substring(0,index) == "debug"){ // Handles [timezone] command
        preferences.begin("credentials", false);
        preferences.putBool("debug", !debug);
        debug = preferences.getBool("debug", false);
        preferences.end();
        Serial.print("debug set to: ");
        Serial.println(debug);
      }else if(input.substring(0,index) == "scroll"){ // Handles [timezone] command
        int speed = input.substring(index+1).toInt();
        preferences.begin("credentials", false);
        preferences.putInt("scrollSpeed", speed);
        scrollSpeed = preferences.getInt("scrollSpeed", 30);
        preferences.end();
        Serial.print("Scroll Speed set to: ");
        Serial.println(scrollSpeed);
      }else if(input.substring(0,index) == "cutoff"){ // Handles [timezone] command
        int dB = input.substring(index+1).toInt();
        preferences.begin("credentials", false);
        preferences.putInt("cutOff", dB);
        cutOff = preferences.getInt("cutOff", -60);
        preferences.end();
        Serial.print("dB cutoff set to: ");
        Serial.println(cutOff);
      }else{
        Serial.println("******Bad Command******");
        printMenu();
      }
    }
  }
}
void printMenu(void){
  Serial.println("**********************************************");
  Serial.println("***************Menu Commands******************");
  Serial.println("**********************************************");
  Serial.println("[name:<type your name here>]");
  Serial.println("[ssid:<WiFi Name>]");
  Serial.println("[pw:<WiFi Password>]");
  Serial.println("[timeout:<screen time out in seconds>] 0 = OFF");
  Serial.println("[reboot] Reboots Badge");
  Serial.println("[reset] Clears saved data");
  Serial.println("[scroll] changes the scrolling speed (lower number is faster)");
  Serial.println("[debug] turns on debug");
  Serial.println("[timezone] Chnages time zone EST, CST, MNT, PST");
  Serial.println("[cutoff] changes BT dB sensitivity. (0 to -99)");
  Serial.println("**********************************************");
  Serial.println("**********************************************");
  Serial.println("**********************************************");
}
void textScroll(int y, const char * text, u8g2_uint_t offset, u8g2_uint_t width){
  u8g2_uint_t x = offset;
  do {								// repeated drawing of the scrolling text...
      u8g2.drawUTF8(x, y, text);			// draw the scolling text
      x += width;						// add the pixel width of the scrolling text
    } while( x < u8g2.getDisplayWidth() );		// draw again until the complete display is filled
}
void showVoltage(int x, int y, float batVolts){
  char result[8]; // Buffer big enough for 7-character float
  dtostrf(batVolts, 4, 2, result); // Leave room for too large numbers!
  String batVoltsStr;
  batVoltsStr += result;
  batVoltsStr += " Volts";
   u8g2.setFont(u8g2_font_8x13B_mf);
   u8g2.setColorIndex(1);
  if(digitalRead(CHARGING)){
    u8g2.drawStr(x,y,batVoltsStr.c_str());
  }else{
    u8g2.drawStr(x,y,"Charging");
  }
}

/* Functions to hold different pages */
void drawPageOne(void){
  if(buttons != old_buttons){
    switch(buttons){
      case 0x01: screen = (screen+1)%screenCount; break;
      case 0x02: logo = (logo+1)%logoCount; break;
      case 0x04: if(screen > 0)screen--; else screen = screenCount-1; break;
      case 0x08: if(logo > 0) logo--; else logo = logoCount-1; break;
      case 0x10: break; //Don't care presses
      case 0x20: state = (state+1)%stateCount; break;
      case 0x40: break; //Don't care presses
      case 0x80: if(state > 0) state--;else state = stateCount-1; break;
    }
    old_buttons = buttons;
  } 
  // Check for screen change
    u8g2.firstPage();
    do{
      //print items to the screen
      showLogo(logo,1);    
    }while(u8g2.nextPage());
}
void drawPageTwo(void){
  if(buttons != old_buttons){
    switch(buttons){
      case 0x01: screen = (screen+1)%screenCount; break;
      case 0x02: break; //Don't care presses
      case 0x04: if(screen > 0){ screen--; }else{ screen = screenCount-1;} break;
      case 0x08: break; //Don't care presses
      case 0x10: if(oompa < 14){oompa++;}else{oompa = 14;} break;
      case 0x20: state = (state+1)%stateCount; break;
      case 0x40: if(oompa > 1){oompa--;}else{oompa = 1;} break;
      case 0x80: if(state > 0) state--;else state = stateCount-1; break;
    }
    old_buttons = buttons;
  }
  u8g2.firstPage();
  
  do{
    //print items to screen
      if(strlen(BT_Dev.c_str()) > 1){
      u8g2.setFont(u8g2_font_6x12_mr);
      int width = u8g2.getUTF8Width(BT_Dev.c_str());
      int x_pos = (128 - width)/2;
      u8g2.setDrawColor(1);
      u8g2.drawStr(x_pos, 10, BT_Dev.c_str());
      drawOompa(BT_Count, 0);
    }else{
      u8g2.setFont(u8g2_font_6x12_mr);
      u8g2.setDrawColor(1);
      u8g2.drawStr((128 - u8g2.getUTF8Width("Press"))/2, 10, "Press");
      u8g2.drawStr((128 - u8g2.getUTF8Width("[Right Down]"))/2, 22, "[Right Down]");
      u8g2.drawStr((128 - u8g2.getUTF8Width("To Scan"))/2, 34, "To Scan");
    }
  }while(u8g2.nextPage());

  if(buttons == 0x40){
    BT_Count = checkBLE();
    if(BT_Count > 14 ){
      BT_Count = 14;
    }
  }

}
void drawPageThree(int rate){
  if(buttons != old_buttons){
    switch(buttons){
      case 0x01: screen = (screen+1)%screenCount; break;
      case 0x02: break; //Don't care presses
      case 0x04: if(screen > 0){ screen--; }else{ screen = screenCount-1;} break;
      case 0x08: break; //Don't care presses
      case 0x10: break; //Don't care presses
      case 0x20: state = (state+1)%stateCount; break;
      case 0x40: break; //Don't care presses
      case 0x80: if(state > 0) state--;else state = stateCount-1; break;
    }
    old_buttons = buttons;
  } 
  if ((new_time - batTime) >= rate){ // Update on cycle or screen change
    // Update Data
    raw_batVoltage = analogRead(BAT_SENSE);
    batVoltage = raw_batVoltage*voltageOffset;
    double pre = batVoltage/3.7;
    double pre1 = 1 + pow(pre,80);
    double pre2 = pow(pre1,0.165);
    batPercent = 123-(123/pre2);
    batLevel = getLevel(batPercent);    

    //Update Screen
    u8g2.firstPage();
    do{
      //print items to screen
      showVoltage(5,10,batVoltage);
      showLevel(42,20,batLevel);
    }while(u8g2.nextPage());
    batTime = new_time; // Reset Frame rate ticks
  }

}
void drawPageFour(int rate){
  if(buttons != old_buttons){
    switch(buttons){
      case 0x01: screen = (screen+1)%screenCount; break;
      case 0x02: break; //Don't care presses
      case 0x04: if(screen > 0){ screen--; }else{ screen = screenCount-1; } break;
      case 0x08: break; //Don't care presses
      case 0x10: break; //Don't care presses
      case 0x20: state = (state+1)%stateCount; break;
      case 0x40: break; //Don't care presses
      case 0x80: if(state > 0) state--;else state = stateCount-1; break;
    }
    old_buttons = buttons;
  } 

  if((new_time - scrollTime) >= rate){ // Update on cycle or screen change
    //Update Data
    raw_batVoltage = analogRead(BAT_SENSE);
    batVoltage = raw_batVoltage*voltageOffset;
    double pre = batVoltage/3.7;
    double pre1 = 1 + pow(pre,80);
    double pre2 = pow(pre1,0.165);
    batPercent = 123-(123/pre2);
    batLevel = getLevel(batPercent);

    //Update Time
    if(timeReceived){
      if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return;
      }
      strftime(chBuffer, sizeof(chBuffer), "%I:%M%p", &timeinfo);
    }

    // Update Screen
    u8g2.firstPage();
    do{
      drawInfoBar(0,0,batLevel,chBuffer); // Draw info bar

      //Scroll Name if it exist
      u8g2.setFont(u8g2_font_courB24_tf);
      u8g2.setFontDirection(0);
      if(strlen(name.c_str()) > 1){        
        textScroll(50, name.c_str(), scrollIndex, u8g2.getUTF8Width(name.c_str()));
        scrollIndex-=1;							// scroll by one pixel
        if ( (u8g2_uint_t)scrollIndex < (u8g2_uint_t)-u8g2.getUTF8Width(name.c_str())){	
          scrollIndex = 0;							// start over again
        }
      }
    }while(u8g2.nextPage());
    scrollTime = new_time; // Reset Frame rate ticks
  }
}
int checkBLE(void) {
  pBLEScan = BLEDevice::getScan();
  //pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
  BLEScanResults *results = pBLEScan->start(1, false);
  int best = -100;
  int dev = 0;
  int count = results->getCount();
  for (int i = 0; i < count; i++) {
    BLEAdvertisedDevice device = results->getDevice(i);
    int rssi = device.getRSSI();
    if (rssi > best) {
      best = rssi;
      dev = i;
    }
  }
  BLEAdvertisedDevice device = results->getDevice(dev);
  Serial.print(device.getRSSI());
  Serial.print(" : ");
  Serial.println(device.getAddress().toString());
  BT_Dev = device.getAddress().toString();
  pBLEScan->setActiveScan(false);
  pBLEScan->clearResults();  // delete results fromBLEScan buffer to release memory
  if(best > cutOff){
    setStatus(5,10);
  }else{
    setStatus(0,10);
  }
  return count; 
}
