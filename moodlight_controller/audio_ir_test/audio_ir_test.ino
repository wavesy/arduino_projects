#include <IRremote.hpp>;
#include <LiquidCrystal.h>

// io pins
const byte audioPin = A5;
const byte irPin = 10;

// lcd
const int rs=2, en=3, d4=4, d5=5, d6=6, d7=7;
const int rows=2, cols=16; 
LiquidCrystal lcd(rs,en,d4,d5,d6,d7);

// constants to fiddle with
const int audioDelay = 5; // audio sample delay in ms
const int audioSamplesMax = 1000/audioDelay; // 1 sec of audio samples before updating screen

int audio, ir;
int sampleCount = 0;
unsigned long last = 0;
int maxAudio = 0;

void setup() {
  IrReceiver.begin(irPin, 1);

  lcd.begin(cols, rows);
  lcd.print("ir: ");  
  lcd.setCursor(0,1);
  lcd.print("audio: ");

  pinMode(audioPin, INPUT);
  pinMode(irPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  if (millis() - last > audioDelay){
    last = millis();
    audio = analogRead(audioPin)-500;

    // update audio sample max if needed
    if (sampleCount < audioSamplesMax){
      if (audio > maxAudio) maxAudio = audio;
      sampleCount++;
    }
    // max samples reached, print max
    else{
      lcd.setCursor(7, 1);
      lcd.print("    ");
      lcd.setCursor(7, 1);
      lcd.print(maxAudio);
      
      // reset
      sampleCount = 0;
      maxAudio = 0;
    }
  }

  if (IrReceiver.decode()){
    Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);

    lcd.setCursor(4, 0);
    lcd.print(IrReceiver.decodedIRData.decodedRawData, HEX);
    
    IrReceiver.resume();
  }

  //delay(100);
}
