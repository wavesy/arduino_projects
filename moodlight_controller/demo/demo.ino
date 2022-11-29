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
void setColor(const byte r, const byte g, const byte b, bool save){
  analogWrite(RPIN, r);
  RSTATE = r;
  analogWrite(GPIN, g);
  GSTATE = g;
  analogWrite(BPIN, b);
  BSTATE = b;

  // save to EEPROM (optional)
  if (save){
      byte address = 0;      
      EEPROM.write(address, r);
      address++;
      EEPROM.write(address, g);
      address++;
      EEPROM.write(address, b);
      address++;
  }

  return;
}

void saveColor();

void partyMode(){
  setColor(255,50,50,false);
  delayMillis(100);
  setColor(50,255,50,false);
  delayMillis(100);
  setColor(50,50,255,false);
  delayMillis(100);
  setColor(255,255,255,false);
  delayMillis(100);
  setColor(50,50,50,false);
  delayMillis(100);
  return;
}

void loop() {
  while(!SLEEP){
    // check if new ir transmission has been received
    if (IrReceiver.decode()){ 
      if (IrReceiver.decodedIRData.command != PREV_CMD && IrReceiver.decodedIRData.command != 0){       
        int ir_cmd = IrReceiver.decodedIRData.command;
        Serial.println(ir_cmd);  // debug ir codes
        IrReceiver.resume();

        // define profiles here
        if (ir_cmd == 64) setColor(0,0,0,false); // lights off, implement sleep mode later
        else if(ir_cmd == 18){
          while(!IrReceiver.decode()){
            partyMode(); 
          }
        }
        else if(ir_cmd == 69) setColor(255,0,0,true);     // red
        else if(ir_cmd == 88) setColor(0,255,0,true);     // green
        else if(ir_cmd == 89) setColor(0,0,255,true);     // blue
        else if(ir_cmd == 68) setColor(255,255,255,true); // white
        /*
        else if(ir_cmd == 5) setColor();
        else if(ir_cmd == 6) setColor();
        else if(ir_cmd == 7) setColor();
        else if(ir_cmd == 8) setColor();
        else if(ir_cmd == 9) setColor();
        */
        PREV_CMD = ir_cmd;
      }
      else{
        IrReceiver.resume();
      }
    }    
  }
}
