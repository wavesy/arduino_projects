// TODO: add mq-135 later, reading updates only with change
// TODO: figure out setCursor

#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DHTPIN 5        // temp/humidity sensor
#define DHTTYPE DHT11
#define SAMPLERATE 2000 // sampling rate in ms
#define OLEDADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// fine tuning variables
const float tempoffset = 0.0;
const float humidityoffset = 0.0;

// declaration for temp/humidity sensor object
DHT dht(DHTPIN, DHTTYPE);

// declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


void setup() {
  // init serial monitor
  Serial.begin(9600);

  // init temp / humidity sensor
  dht.begin();
  
  // init display
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLEDADDRESS)) { 
    Serial.println(F("SSD1306 allocation failed"));
    while(1); // stay here
  }
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.print("Setting up");
  delayMillis(500);
  display.print(".");
  display.display();
  delayMillis(500);
  display.print(".");
  display.display();
  delayMillis(500);
  display.print(".");
  display.display();

  display.clearDisplay();
  /*
  display.setCursor(0,5);
  display.print("C");
  display.setCursor(32,5);
  display.print("RH%");
  */
  display.display();
}

void delayMillis(unsigned long t){
// non-blocking delay function using millis()
// param t time to wait in ms
  unsigned long now = millis();
  while (millis() - now < t);
  return;
}

void loop() {
  // dht sensor reading ~250 ms
  float humidity = dht.readHumidity() + humidityoffset;
  float temp = dht.readTemperature() + tempoffset;
  
  // debug
  Serial.print("T: ");
  Serial.println(temp);
  delayMillis(2000);
  //
  display.setCursor(0,0);
  display.println(temp);
  display.display();

  // debug
  Serial.println();
  Serial.print("H: ");
  Serial.println(humidity);
  //
  display.setCursor(0,32);
  display.println(humidity);
  display.display();
}
