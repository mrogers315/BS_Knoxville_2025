


//Global headers
#include <Arduino.h>
#include <U8g2lib.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>

//Local headers
#include "BSK_2025.h"
#include "BSK_Logo.h"
#include "FP_Logo.h"
#include "Battery_Image.h"
#include "Oompa.h"
#include "Small_Battery.h"


#define TIMEOUT 30000 //screen time out 30s
#define BATTERY_READ 2000 //Battery read interval 10s
#define FRAME_TIME 500 //Animation frames speed


U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ CS, /* dc=*/ DC, /* reset=*/ RES); //OLED initializer
Adafruit_NeoPixel statusLED(1, NP_LED, NEO_GRB + NEO_KHZ400); //Neopixel intializer

uint8_t buttons = 0x00; //Holds button presses
uint8_t screen = 0; //screen var
uint8_t oompa = 1; //Number of Oompas
uint8_t logo = 0; //Logo to dispay
uint8_t old_buttons = 0x03; //Current press var
bool screenOn, flag = true; //change flag
uint8_t state, oldState = 0; //NP LED status vr
int old_time, new_time = 0; //black light time out var
uint8_t batLevel, old_batLevel = 0; //Battery level change dectector
uint8_t onState, old_onState = 0;
float batVoltage = 0;
int raw_batVoltage = 0;
int old_batReadTime = 0;
int old_frame = 0;
uint8_t batteryFrame = 0;
float voltageOffset = 0.001711011052;
double batPercent = 0;
uint8_t screenCount = 4;
uint8_t logoCount = 2;
uint8_t stateCount = 8;

void setup() {
  Serial.begin(115200);

  pinMode(L_UP, INPUT);
  pinMode(L_RIGHT, INPUT);
  pinMode(L_DOWN, INPUT);
  pinMode(L_LEFT, INPUT);
  pinMode(R_UP, INPUT);
  pinMode(R_RIGHT, INPUT);
  pinMode(R_DOWN, INPUT);
  pinMode(R_LEFT, INPUT);

  pinMode(BAT_SENSE, INPUT);
  pinMode(CHARGING, INPUT);
  pinMode(ON_SENSE, INPUT);

  pinMode(BL_EN, OUTPUT);
  digitalWrite(BL_EN, HIGH);

  u8g2.begin(); //Display Initializer
  statusLED.begin(); // INITIALIZE Status LED (REQUIRED)
  new_time = millis(); //Initialize loop time
  raw_batVoltage = analogRead(BAT_SENSE);
  batVoltage = raw_batVoltage*voltageOffset;

  Serial.println("Setup Complete");
}

void loop() {

  if((new_time - old_time) >= TIMEOUT){
      u8g2.firstPage();
      if(screenOn){
        do{
          u8g2.clearDisplay();
        }while(u8g2.nextPage());
      }
      digitalWrite(BL_EN, LOW);
      screenOn = false;
    }else{
      if(!screenOn){ 
        digitalWrite(BL_EN, HIGH);
        screenOn = true;
      }
    }
  if ((new_time - old_batReadTime) >= BATTERY_READ){
      
      raw_batVoltage = analogRead(BAT_SENSE);
      batVoltage = raw_batVoltage*voltageOffset;
      double pre = batVoltage/3.7;
      double pre1 = 1 + pow(pre,80);
      double pre2 = pow(pre1,0.165);
      batPercent = 123-(123/pre2);
      Serial.println(batPercent);
      old_batReadTime = new_time;
  }
  onState = digitalRead(ON_SENSE);

 
  batLevel = getLevel(batPercent);

  buttons = getButtons();
  if(buttons != old_buttons || onState != old_onState){
    switch(buttons){
      case 0x01: screen = (screen+1)%screenCount; break;
      case 0x02: logo = (logo+1)%logoCount; break;
      case 0x04: if(screen > 0) screen--; else screen = screenCount-1; break;
      case 0x08: if(logo > 0) logo--; else logo = logoCount-1; break;
      case 0x10: if(screen == 1){if(oompa < 14){oompa++;}else{oompa = 14;}} break;
      case 0x20: state = (state+1)%stateCount; break;
      case 0x40: if(screen == 1){if(oompa > 1){oompa--;}else{oompa = 1;}} break;
      case 0x80: if(state > 0) state--;else state = stateCount-1; break;
    }
    old_buttons = buttons;
    if(buttons != 0 || onState != old_onState){
      flag = true;
      old_time = new_time;
      old_onState = onState;
    }
    Serial.print("Buttons: ");
    Serial.println(buttons);
    Serial.print("State: ");
    Serial.println(state);
    Serial.print("Screen: ");
    Serial.println(screen);
    Serial.print("Oompa: ");
    Serial.println(oompa);
    Serial.print("Logo: ");
    Serial.println(logo);
  } 

  
  if(onState && screenOn){
    u8g2.firstPage();
    if(!digitalRead(CHARGING)){
      if((new_time - old_frame) >= FRAME_TIME){
        old_frame = new_time;
        u8g2.firstPage();
        do{
          if(batteryFrame == 0){
              u8g2.clearDisplay();
            }
          showBattery(batteryFrame,1);
        }while(u8g2.nextPage());
        
        batteryFrame = (batteryFrame + 1)%5;       
      }
    }else{
      if(flag){
        u8g2.firstPage();
        do{
          u8g2.clearDisplay();
          showBattery(4,1);
        }while(u8g2.nextPage());
        flag = false;
      }
    }
  }else{
    if((flag || (screen == 2 && batLevel != old_batLevel)) && screenOn){
      u8g2.firstPage();
      do{
        u8g2.clearDisplay();
        switch(screen){
          case 0: showLogo(logo, 1);break;
          case 1: drawOompa(oompa, 0);break;
          case 2: showLevel(batLevel);break;
          case 3: drawHome();break;
        }
      }while( u8g2.nextPage() );
      flag = false;
      old_batLevel = batLevel;
    }
    if(state != oldState){
      setStatus(state, 20);
      oldState = state;
      Serial.println("Status Update");
    }
    if(screen == 2 && screenOn){
      //u8g2.firstPage();
      char result[8]; // Buffer big enough for 7-character float
      const char *combined = "";
      dtostrf(batVoltage, 4, 2, result); // Leave room for too large numbers!
      String str;
      str += result;
      str += " Volts";
      combined = str.c_str();

      do{
        if(digitalRead(CHARGING)){
          u8g2.setFont(u8g2_font_8x13B_mf);
          u8g2.setColorIndex(1);
          u8g2.drawStr(5,10,combined);
        }else{
          u8g2.setFont(u8g2_font_8x13B_mf);
          u8g2.setColorIndex(1);
          u8g2.drawStr(5,10,"Charging");
        }

      }while(u8g2.nextPage());
    }
  } 
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
  Serial.println("Show Logo");
  u8g2.setDrawColor(color);
  switch(img){
    case 0 : u8g2.drawXBM(0,1, bsk_logo_width,bsk_logo_height,bsk_logo_bits); break;
    default : u8g2.drawXBM(0,1, fp_logo_width,fp_logo_height,fp_logo_bits);
  }

}

float getBattery(void) {
  //Serial.println("Get Battery");
  float level = analogRead(BAT_SENSE)*0.0018;
  return level;
}

void setStatus(uint8_t stat, uint8_t lvl) {
  //Serial.println("Set Status");
  switch(stat){
    case 0 : statusLED.setPixelColor(0, statusLED.Color(lvl, 0, 0)); break;
    case 1 : statusLED.setPixelColor(0, statusLED.Color(lvl, lvl, 0)); break;
    case 2 : statusLED.setPixelColor(0, statusLED.Color(lvl, lvl, lvl)); break;
    case 3 : statusLED.setPixelColor(0, statusLED.Color(0, lvl, 0)); break;
    case 4 : statusLED.setPixelColor(0, statusLED.Color(0, lvl, lvl)); break;
    case 5 : statusLED.setPixelColor(0, statusLED.Color(0, 0, lvl)); break;
    case 6 : statusLED.setPixelColor(0, statusLED.Color(lvl, 0, lvl)); break;
    default: statusLED.clear();
  }
  statusLED.show();   // Send the updated pixel colors to the hardware.
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
  Serial.println("Draw Oompa");
  uint8_t x = 0;
  uint8_t y = 0;
  u8g2.clearDisplay();
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
void showLevel(uint8_t lvl){
  Serial.println("Show Level");
  switch(lvl){
    case 0: showBatterySmall(0,1);break;
    case 1: showBatterySmall(1,1);break;
    case 2: showBatterySmall(2,1);break;
    case 3: showBatterySmall(3,1);break;
    case 4: showBatterySmall(4,1);break;
  }
}
void showBatterySmall(uint8_t lvl, uint8_t color) {
  Serial.println("Show Small Battery");
  u8g2.setDrawColor(color);
  switch(lvl){
    case 0 : u8g2.drawXBM(42,20, bat_42x24_width,bat_42x24_height,bat_00_42x24_bits); break;
    case 1 : u8g2.drawXBM(42,20, bat_42x24_width,bat_42x24_height,bat_25_42x24_bits); break;
    case 2 : u8g2.drawXBM(42,20, bat_42x24_width,bat_42x24_height,bat_50_42x24_bits); break;
    case 3 : u8g2.drawXBM(42,20, bat_42x24_width,bat_42x24_height,bat_75_42x24_bits); break;
    case 4 : u8g2.drawXBM(42,20, bat_42x24_width,bat_42x24_height,bat_100_42x24_bits); break;
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
void drawHome(void){
  //info bar u8g2_font_battery24_tr u8g2_font_battery19_tn
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_battery24_tr);
  u8g2.setFontDirection(1);
  u8g2.drawStr(104,1, "\u0034");
  u8g2.setFont(u8g2_font_streamline_interface_essential_wifi_t);
  u8g2.setFontDirection(3);
  u8g2.drawStr(102,21, "\u0030");
  u8g2.setFont(u8g2_font_fub20_tf);
  u8g2.setFontDirection(0);
  u8g2.drawStr(0,20,"8:28a");
  

  //Main screen
}
