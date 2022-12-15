// TODO: wake up from ir

//#define DEBUG
#define IR_USE_AVR_TIMER* 1
#include <IRremote.hpp>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/sleep.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// interrupt flags
volatile bool SLEEP = 0;  // on/off
volatile bool INT_BY_SOUND = 0;  // sound detected
volatile bool INT_BY_IR = 0; // ir receive detected

// sleep
bool SLEEP_STATUS = 0;

// store previous ir transmission
byte PREV_CMD = 0;

// store detected sound to recognize second clap
long CURRENT_NOISE_TIME = 0;
long LAST_POWER_TOGGLE = 0;


// IO pins
const byte RPIN = 9, 
          GPIN = 10, 
          BPIN = 11,
          GATEPIN = 5,
          IRPIN = 4,
          AUDIOPIN = A5,
          INTERRUPTPIN = 2;

const byte in_pins [] = {GATEPIN, IRPIN, AUDIOPIN, INTERRUPTPIN};
const byte out_pins [] = {RPIN, GPIN, BPIN};


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
  analogWrite(GPIN, g);
  analogWrite(BPIN, b);

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
          goToSleep();
        }
        
        // HERE BE DRAGONS
        // dont comment this out, it breaks the code for some fucking reason
        else if(cmd == 666){      
          while(!IrReceiver.decode()){
            // this can be commented out
            /*
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
            */
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
  display.display();
  return;
}

void pinISR(){
  if(digitalRead(GATEPIN)) INT_BY_SOUND = 1;
  else INT_BY_IR = 1;
  return;
}

// call this to reset (wake up)
void(* resetFunc)(void) = 0;


void goToSleep(){
  // turn off leds
  for (auto pin : out_pins){
    analogWrite(pin, 0);
  }
  // turn off screen
  screenSetup();
  display.print("BYE");
  display.display();
  delay(2000);
  screenSetup();

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  // go to sleep
  sleep_mode();

  // wake up
  SLEEP_STATUS = 0;
  sleep_disable();
  resetFunc();
  return;
}


void setup() {
  DDRD &= B11110101; // pins 2 and 4 as input pins
  DDRB |= B00001110; // pins 9,10,11, as output pins

  // init ir decoder with led feedback on
  IrReceiver.begin(IRPIN, 1);

  // init interrupt routine for sound detection
  attachInterrupt(digitalPinToInterrupt(INTERRUPTPIN), pinISR, RISING);

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

  // reset sleep flag if reset by waking up (might be unneccessary)
  SLEEP_STATUS = 0;
}


void loop() {  
  // check if interrupted by sound
  if (INT_BY_SOUND){
    Serial.println("int by sound");
    CURRENT_NOISE_TIME = millis();
    if (CURRENT_NOISE_TIME > LAST_POWER_TOGGLE + 200){ // debounce
      SLEEP_STATUS = !SLEEP_STATUS; // toggle power state variable
      // go to sleep if required, otherwise continue as normal
      if (SLEEP_STATUS){          
        LAST_POWER_TOGGLE = millis();
        Serial.println("sleep");
        delay(1000);        
        goToSleep();
      }
    }
    INT_BY_SOUND = 0;
  }
  

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

