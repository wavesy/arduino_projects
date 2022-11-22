// interrupt flags
volatile bool SLEEP = 0;  // on/off
volatile bool STOP = 0;   // for responsive color changes

// IO pins
const int RPIN = 9, 
          GPIN = 10, 
          BPIN = 11,
          GATEPIN = 2,
          IRPIN = 4,
          AUDIOPIN = A5;


void setup() {
  const int in_pins [] = {GATEPIN, IRPIN, AUDIOPIN};
  const int out_pins [] = {RPIN, GPIN, BPIN};
  for (auto pin : in_pins){
    pinMode(pin, INPUT);
  } 
  for (auto pin : out_pins){
    pinMode(pin, OUTPUT);
  }

  Serial.begin(9600);
}

// non-blocking delay function using millis()
void delayMillis(const unsigned long t){
  const unsigned long start = millis();
  while(millis() - start < t){
    // check interrupt flag
    if(STOP) break;
  }
  return;
}

// function for controlling led brightness, param values 0-255
void setColor(const int r, const int g, const int b){
  analogWrite(RPIN, r);
  analogWrite(GPIN, g);
  analogWrite(BPIN, b);
}

void partyMode(){
  setColor(255,50,50);
  Serial.println("red");
  delayMillis(1000);
  setColor(50,255,50);
  Serial.println("green");
  delayMillis(1000);
  setColor(50,50,255);
  Serial.println("blue");
  delayMillis(1000);
  setColor(255,255,255);
  Serial.println("all");
  delayMillis(1000);
  setColor(50,50,50);
  Serial.println("none");
  delayMillis(1000);
}

void loop() {
  while(!SLEEP){
    while(!STOP){
      partyMode();
    }
  }
}
