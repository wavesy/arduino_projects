// TODO: add screen, mq-135 later

#include <DHT.h>

#define DHTPIN 5        // temp/humidity sensor
#define DHTTYPE DHT11
#define SAMPLERATE 2000 // sampling rate in ms

float tempoffset = 0.0;
float humidityoffset = 0.0;

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  dht.begin();
  Serial.begin(9600);
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
  Serial.println();
  Serial.print("H: ");
  Serial.println(humidity);
  Serial.print("T: ");
  Serial.println(temp);
  delayMillis(2000);
}
