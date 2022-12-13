//#define DEBUG
#define IR_USE_AVR_TIMER* 1
#include <IRremote.hpp>;
#include <EEPROM.h>;


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

  Serial.begin(9600);
}

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

  // break cmd into bytes, save to EEPROM
  byte byte1 = (cmd >> 8) & 0xFF;
  byte byte2 = cmd & 0xFF;
  EEPROM.write(0,byte1);
  EEPROM.write(1,byte2);

  return;
}

// sets color profile (either static or a sequence)
void setProfile(int cmd){
  // DEFINE PROFILES HERE
        if (cmd == 64){
          setColor(0,0,0,cmd); // lights off, implement sleep mode later
          screenSetup();
        }
        
        else if(cmd == 18){
          /*screenSetup();
          display.println("PARTY");
          display.display();*/           
          while(!IrReceiver.decode()){
            setColor(255,50,50,cmd);
            //delayMillis(1000);
            setColor(50,255,50,cmd);
            //delayMillis(1000);
            setColor(50,50,255,cmd);
            //delayMillis(1000);
            setColor(255,255,255,cmd);
            //delayMillis(1000);
            setColor(50,50,50,cmd);
            //delayMillis(1000);
          }
        }
        
        else if(cmd == 69) setColor(255,0,0,cmd);     // red
        
        else if(cmd == 88) setColor(0,255,0,cmd);     // green
        
        else if(cmd == 89) setColor(0,0,255,cmd);     // blue
        
        else if(cmd == 68) setColor(255,255,255,cmd); // white
        return;
}

// reset screen
void screenSetup(){
  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(0, 10);
  display.display();
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

  // init ir decoder with led feedback on
  IrReceiver.begin(IRPIN, 1);

  // init display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
  }
  screenSetup();
  display.setTextColor(WHITE);
  display.println("GO NUTS");
  display.fillCircle(64, 50, 8, WHITE); // (x,y,radius, color)
  display.display();

  // init serial monitor
  Serial.begin(9600);

  // load saved profile from EEPROM
  // read two bytes, since cmd is saved as an int
  int stored_cmd = (EEPROM.read(0) << 8) + EEPROM.read(1);
  Serial.print("loaded: ");
  Serial.println(stored_cmd);
  delay(2000);
  setProfile(stored_cmd);
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
