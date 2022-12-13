//#define DEBUG
#define IR_USE_AVR_TIMER* 1
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#include <IRremote.hpp>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
//*** Screen setup end


// interrupt flags
volatile bool SLEEP = 0;  // on/off

// store previous ir transmission
byte PREV_CMD = 0;

// store led states
byte RSTATE = 0;
byte GSTATE = 0;
byte BSTATE = 0;

// IO pins
const byte RPIN = 9, 
          GPIN = 10, 
          BPIN = 11,
          GATEPIN = 2,
          IRPIN = 4,
          AUDIOPIN = A5;


// non-blocking delay function using millis()
void delayMillis(const unsigned long t){
  const unsigned long start = millis();
  while(millis() - start < t){
    // check for incoming transmission
    if (IrReceiver.decode()){
      Serial.println("Party's over");
      break;
    }
  }
  return;
}

// function for controlling led brightness, led param values 0-255
// last param for saving to EEPROM
void setColor(const byte r, const byte g, const byte b, int cmd){
  analogWrite(RPIN, r);
  RSTATE = r;
  analogWrite(GPIN, g);
  GSTATE = g;
  analogWrite(BPIN, b);
  BSTATE = b;

  // save to EEPROM
  EEPROM.write(0,cmd);

  return;
}

// sets color profile (either static or a sequence)
void setProfile(int cmd){
  // DEFINE PROFILES HERE
        if (cmd == 64) setColor(0,0,0,cmd); // lights off, implement sleep mode later
        
        else if(cmd == 18){
          while(!IrReceiver.decode()){
            setColor(255,50,50,cmd);
            delayMillis(100);
            setColor(50,255,50,cmd);
            delayMillis(100);
            setColor(50,50,255,cmd);
            delayMillis(100);
            setColor(255,255,255,cmd);
            delayMillis(100);
            setColor(50,50,50,cmd);
            delayMillis(100);
          }
        }
        
        else if(cmd == 88){
          setColor(255,0,0,cmd);     // red
          screenSetup();
          display.println("RED");
          display.display();          
        } 
        
        else if(cmd == 89){
          setColor(0,255,0,cmd);     // green
          screenSetup();
          display.println("GREEN");
          display.display();
        }  

        else if(cmd == 69){
          setColor(0,0,255,cmd);     // blue
          screenSetup();
          display.println("BLUE");
          display.display();
        }

        else if(cmd == 68){
          setColor(255,255,255,cmd); // white
          screenSetup();
          display.println("WHITE");
          display.display();
        }
        
        return;
}

// reset screen
void screenSetup(){
  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(0, 10);
  display.setTextColor(WHITE);
  return;
}

void setup() {
  const byte in_pins [] = {GATEPIN, IRPIN, AUDIOPIN};
  const byte out_pins [] = {RPIN, GPIN, BPIN};
  for (auto pin : in_pins){
    pinMode(pin, INPUT);
  } 
  for (auto pin : out_pins){
    pinMode(pin, OUTPUT);
  }

  // load saved profile from EEPROM

  // init ir decoder with led feedback on
  IrReceiver.begin(IRPIN, 1);

  // init display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
  }
  screenSetup();
  display.println("GO NUTS");
  display.fillCircle(64, 50, 8, WHITE); // (x,y,radius, color)
  display.display();

  // init serial monitor
  Serial.begin(9600);
}

void loop() {
  while(!SLEEP){
    // check if new ir transmission has been received
    if (IrReceiver.decode()){ 
      if (IrReceiver.decodedIRData.command != PREV_CMD && IrReceiver.decodedIRData.command != 0){       
        int ir_cmd = IrReceiver.decodedIRData.command;
        Serial.println(ir_cmd);  // debug ir codes
        IrReceiver.resume();
        setProfile(ir_cmd);
        PREV_CMD = ir_cmd;
      }
      else{
        IrReceiver.resume();
      }
    }    
  }
}
