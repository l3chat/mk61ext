/*
 * ADD TO PCB
 * ----------
 * + pull up resistors I2C SDA, SCL
 * + on/off switch: GND to 3V3_EN
 * + buttons RESET, BOOTSELECT
 * - SWD connector
 * + battery charger with MOSFET ON/OFF switch
 * 
 * 
 * DISPLAY
 * -------
 * GMG12864-06D Ver.2.2
 * ST7565R
 * https://www.lcd-module.de/eng/pdf/zubehoer/st7565r.pdf
 * https://www.youtube.com/watch?v=YwYkXSt6HfA
 * https://rcl-radio.ru/?p=83094
 * http://arduino.ru/forum/apparatnye-voprosy/podklyuchenie-displeya-gmg12864-06d-na-st7565r
 * https://www.sparkfun.com/datasheets/LCD/GDM12864H.pdf
 * 
 * 1 CS  -                            - 17 //17
 * 2 RSE - reset RST                  - 18 //20
 * 3 RS  - Data/Command select DC A0  - 19 //21
 * 4 SCL -                            - 20 //18
 * 5 SI  - MOSI                       - 21 //19
 * 6 VDD - 3v3
 * 7 VSS - GND
 * 8 A   - (3v3)                      - D-pin of the P-channel MOSFET
 * 9 K   - GND
 * 10 IC_SCL
 * 11 IC_CS
 * 12 IC_SO
 * 13 IC_SI
 * 
 * 
 * 
 * P-channel MOSFET for backlight
 * ----------------
 * G - 16
 * D - DISPLAY 8 A
 * S - 3v3
 * 
 * 
 * 
 * external EEPROM
 * ---------------
 * AT24C256C-SSHL-T - 256 kbits
 * jlcpcb - C6482
 * https://datasheet.lcsc.com/szlcsc/Microchip-Tech-AT24C256C-SSHL-T_C6482.pdf
 * 
 * 1 - 
 * 2 - 
 * 3 - 
 * 4 - 
 * 5 - 
 * 6 - 
 * 7 - 
 * 8 - 
 * 
 * 
 * 
 * LIPO charger
 * ------------
 * https://shop.pimoroni.com/products/pico-lipo-shim
 * https://cdn.shopify.com/s/files/1/0174/1800/files/lipo_shim_for_pico_schematic.pdf?v=1618573454
 * https://www.best-microcontroller-projects.com/tp4056.html
 * https://datasheet.lcsc.com/lcsc/1809261820_TOPPOWER-Nanjing-Extension-Microelectronics-TP4056-42-ESOP8_C16581.pdf
 * 
 * TP4056-42-ESOP8
 * C16581
 * https://datasheet.lcsc.com/lcsc/1809261820_TOPPOWER-Nanjing-Extension-Microelectronics-TP4056-42-ESOP8_C16581.pdf
 * 
 * DW01-G
 * C14213
 * https://datasheet.lcsc.com/lcsc/1810101017_Fortune-Semicon-DW01-G_C14213.pdf
 * 
 * https://datasheet.lcsc.com/lcsc/1809050110_Fortune-Semicon-FS8205A_C16052.pdf
 */
 
#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

#include "SparkFun_External_EEPROM.h" // Click here to get the library: http://librarymanager/All#SparkFun_External_EEPROM
//#include <Keypad.h>
#include "Keypad.h"

const byte ROWS = 8; //four rows
const byte COLS = 5; //three columns
char keys[ROWS][COLS] = {
  {'a','b','c','d','e'},
  {'f','g','h','i','j'},
  {'k','l','m','n','o'},
  {'p','q','r','s','t'},
  {'7','8','9','-','/'},
  {'4','5','6','+','*'},
  {'1','2','3','u','v'},
  {'0','.','x','y','z'}
};
byte rowPins[ROWS] = {7,6,5,4,3,2,1,0}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {8,9,10,11,12}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
ExternalEEPROM eep;
U8G2_ST7565_ERC12864_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 20, /* data=*/ 21, /* cs=*/ 17, /* dc=*/ 19, /* reset=*/ 18);

int backLight = 0, brightness=256;
unsigned long loop1_timer;

void setup_eeprom(){
    Wire1.setSDA(14);
    Wire1.setSCL(15);
    Wire1.setClock(400000);
    Wire1.begin();

  if (eep.begin(0x50, Wire1) == false){
    u8g2.drawStr(0,0,"no memory found");
    while (1)
      ;
  } else {
    u8g2.drawStr(0,0,"  memory found");
  }

  eep.setMemorySize(262144 / 8); //EEPROM is the AT24C256 (256 Kbit = 32768 bytes)
  eep.setPageSize(64);
  eep.setPageWriteTime(3);
  eep.disablePollForWriteComplete();
}


void setupDisplay(){
  u8g2.begin();
  u8g2.setPowerSave(0);
  u8g2.setFlipMode(1);
  u8g2.setContrast(20);
  u8g2.setFont(u8g2_font_t0_15b_mr );
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);

  pinMode(16, OUTPUT);
  applyBrightness();
}


unsigned long st;
void displayCurrentTime(){
  unsigned long t = millis()/1000;
  t -= st;

  u8g2.setFont(u8g2_font_t0_15b_mr );
  if(t>1800){
    u8g2.drawStr(0,0,"   take break");
  } else {
    u8g2.drawStr(0,0,"12345678901234567890");
  }

  int s1 = t % 10; t /= 10;
  int s2 = t % 6; t /= 6;
  int m1 = t % 10; t /= 10;
  int m2 = t % 6; t /= 6;
  t %= 24;
  int h1 = t % 10; t /= 10;
  int h2 = t % 10;

  char dot = s1%2==0 ? '.' : ' ';

  char b[8];
  sprintf(b, "%d%d%c%d%d%c%d%d", h2,h1,dot,m2,m1,dot,s2,s1);
  u8g2.setFont(u8g2_font_inb16_mr );
  u8g2.drawStr(0,16,b);
  u8g2.setFont(u8g2_font_t0_15b_mr );
}


void setup(){
  setupDisplay();

  u8g2.clearBuffer();
  setup_eeprom();
  u8g2.sendBuffer();
  delay(3000);

  st = millis()/1000;
}

void loop(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_t0_15b_mr );

  displayCurrentTime();

  char key[2] = {keypad.getKey(),'\0'};

  u8g2.setFont(u8g2_font_6x10_mr );
  u8g2.setCursor(0,38);
//  u8g2.print(u8g2.getBufferSize());
  u8g2.print(8 * u8g2.getBufferTileHeight() * u8g2.getBufferTileWidth());
  u8g2.setCursor(64,38);
  u8g2.print(brightness);
  u8g2.drawStr(0,46,"12345678901234567890123456789012");
  u8g2.setFont(u8g2_font_t0_12_mr );
  u8g2.drawStr(0,54,"12345678901234567890123456789012");
  u8g2.setFont(u8g2_font_t0_15b_mr );
  u8g2.drawStr(0,48,key);

//  u8g2.drawLine(0, 0, 127, 63);

  if(key[0]=='a'){increaseBrightness();}
  if(key[0]=='b'){decreaseBrightness();}
  if(key[0]=='e'){}
  if(key[0]=='d'){st = millis()/1000;}

  u8g2.sendBuffer();
  delay(100);
}


void setup1(){
  loop1_timer = micros();
}


void loop1(){
  loop1_timer = micros();
}

void increaseBrightness(){
  brightness *= 2;
  brightness = brightness>256 ? 256 : brightness;
  brightness = brightness==0 ? 1 : brightness;
  applyBrightness();
}

void decreaseBrightness(){
  brightness /= 2;
  applyBrightness();
}

void applyBrightness(){
  if(brightness==256){
    digitalWrite(16, 0); //on
  } else if(brightness==0) {
    digitalWrite(16, 1); //off
  } else {
    analogWrite(16, 255-brightness);
  }
}
