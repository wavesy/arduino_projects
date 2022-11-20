#include <LiquidCrystal.h>    // LCD library
#include <Ethernet.h>         // Ethernet library W5100
#include <PubSubClient.h>     // MQTT library      
#include <TimerOne.h>         // timer library





////////////////////////////////// GLOBALS //////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

// SAMPLING SETTINGS
const unsigned long int sampleInterval = 1000000;   // sample interval in µs
const int n_samples = 20;

// IO PINS
const byte inputPin = B00000000; // wind dir signal input A0
const int keyInterruptPin = 2;
const byte buttonPins [3] = {A1,A2,A3};
const int buttonPinsSize = 3;
const int bgledPin = 3;


// VOLATILES
volatile int dispButtonPress = 1;     // 0 = off, 1,2 = modes
volatile bool dispOn = true;          // display power on/off
volatile int dispMode = 1;            // stores dispButtonPress 1,2
volatile bool sampleInQueue = false;  // if true, a sample is taken within loop function, flag is then reset


// LCD SETTINGS
const int cols = 20;
const int rows = 4;  
const int rs = 4, en = 5, d4 = 6, d5 = 7, d6 = 8, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);              


// ETHERNET SETTINGS
EthernetClient ethClient;                                                                                                                                                        
static uint8_t mymac[6] = { 0x44,0x76,0x58,0x10,0x00,0x73 };                          


// MQTT SETTINGS
unsigned int Port = 1883;             //  MQTT port number
byte server[] = { 10,6,0,20 };        // TAMK IP

char* deviceId     = "GoNuts";        // * set your device id (will be the MQTT client username) *yksilöllinen*
char* clientId     = "gonutsclient";  // * set a random string (max 23 chars, will be the MQTT client id) *yksilöllinen*
char* deviceSecret = "tamk";          // * set your device secret (will be the MQTT client password) *kaikille yhteinen*


//  MQTT Server settings  
void callback(char* topic, byte* payload, unsigned int length); // subscription callback for received MQTTT messages   
PubSubClient client(server, Port, callback, ethClient);   // mqtt client 


//  MQTT topic names 
#define inTopic    "ICT1A_in_2020"     // * MQTT channel where data are received 
#define outTopic   "ICT1A_out_2020"    // * MQTT channel where data is send 

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////





// Get corresponding value for degrees (N = 1, NE = 2 etc.)
int deg_to_val(int& deg){  
  int dir_ranges[8] = {22,67,112,157,202,247,292,337};
  for(int i=0; i<7; i++){
    if((dir_ranges[i] < deg) && (deg <= dir_ranges[i+1])){
      return i+2;
    }     
  }
  return 1;
}


// Read voltage from inputPin, convert to degrees
int read_v_to_deg(){
  // fine tuning variables
  float coef = 1;
  float offset = -7;

  // read voltage on inputPin, convert to degrees
  float deg = 0.0;
  float a = (float(AD_convert(inputPin))*(1023.0/850.0)*coef)+offset; // fix Vref 5 -> ~3.8 (1023*850)
  deg = (a * (360.0/1023.0));
  return deg;
}


int AD_convert(byte ch){
  //        76543210
  DIDR0  = B11111111;
  ADMUX  = B01000000 | ch; // reference voltage 5V, input ch
  ADCSRA = B11000111; // enable converter, start conversion
  
  while((ADCSRA & B01000000) != 0); // hold until conversion is ready
  
  return ADC;
}


void fetch_IP(void){
  byte rev=1;
  
  lcd.setCursor(0,3);
  //         01234567890123456789  
  lcd.print("Waiting for IP");
  rev=Ethernet.begin(mymac); // get IP number   
  Serial.print( F("\nW5100 Revision ") );
    
  if ( rev == 0){
    Serial.println( F( "Failed to access Ethernet controller" ) );         
                                // 0123456789012345689 
    lcd.setCursor(0,3); lcd.print("  Ethernet failed  ");
  }    
                              
  Serial.println( F( "Setting up DHCP" ));
  Serial.print("Connected with IP: "); 
  Serial.println(Ethernet.localIP()); 

  lcd.setCursor(0,3);
  //         012345678901234567890
  lcd.print("                     ");
  
  lcd.setCursor(0,3);
  lcd.print("myIP=");
  lcd.print(Ethernet.localIP());
  delay(1500); 
}


//  MQTT Routines                                                         
void send_MQTT_message(int val){                     // Send MQTT message
  char bufa[50];                                     //  Print message to serial monitor
  if (client.connected()){ 
    //"IOTJS={\"S_name\":\"Wind_dir1\",\"S_value\": %d}"
    sprintf(bufa,"IOTJS={\"S_name\":\"Wind_dir1\",\"S_value\": %d}", val);  // create message with header and data
    Serial.println( bufa ); 
    client.publish(outTopic,bufa);                   // send message to MQTT server        
  }
  else{                                              // Reconnect if connection is lost
    delay(500);
    Serial.println("No, re-connecting" );
    client.connect(clientId, deviceId, deviceSecret);
    delay(1000);                                     // wait for reconnecting
  }
}


//  MQTT server connection                                             
void Connect_MQTT_server(){ 
  Serial.println(" Connecting to MQTT" );
  Serial.print(server[0]); Serial.print(".");              // Print MQTT server IP number to Serial monitor
  Serial.print(server[1]); Serial.print(".");
  Serial.print(server[2]); Serial.print(".");
  Serial.println(server[3]); 
  delay(500);
  
  if (!client.connected()){                                // check if allready connected  
    if (client.connect(clientId, deviceId, deviceSecret)){ // connection to MQTT server 
      Serial.println(" Connection OK " );
      client.subscribe(inTopic);                           // subscribe to in-topic        
    } 
    else{
       Serial.println(client.state());
    }    
  } 
}


//  Receive incoming MQTT message    
void callback(char* topic, byte* payload, unsigned int length){ 
  char* receiv_string;                               // copy the payload content into a char* 
  receiv_string = (char*) malloc(length + 1); 
  memcpy(receiv_string, payload, length);            // copy received message to receiv_string 
  receiv_string[length] = '\0';           
  Serial.println( receiv_string );
  free(receiv_string); 
} 


// Find the mode from an array of wind dir samples
// Expected wind dir values are 1-8
int find_mode(const int A [], const int len){
  // wind_dir values   1 2 3 4 5 6 7 8
  int dircounts [8] = {0,0,0,0,0,0,0,0};

  // count occurences for each direction
  for(int i=0; i<len; i++){
    int dir = A[i];
    dircounts[dir-1] = dircounts[dir-1]+1;
  }

  // find mode from counts (var named moode cause shadowing is scary)
  int c_max = 0;
  int moode;
  for(int j=0; j<8; j++){
    if(dircounts[j] > c_max){
      moode = j+1;
      c_max = dircounts[j];
    }
  }
  
  return moode;
}


void IS_sample(){
  sampleInQueue = true;
}


void IS_keyscan(){
  for(int i=0; i<buttonPinsSize; i++){
    if(analogRead(buttonPins[i]) > 900){  // digitalRead didn't work for some reason
       dispButtonPress = i;
    }
  }
}





////////////////////////////////// SETUP, LOOP //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////


void setup() {
  Serial.begin(9600);
  // io pins
  pinMode(inputPin, INPUT);               // wind dir signal pin
  pinMode(keyInterruptPin, INPUT);        // keypad common interrupt pin
  for(auto pin : buttonPins){
    pinMode(pin, INPUT);
  }
  pinMode(bgledPin, OUTPUT);              // LCD bgled control

  // interrupts
  Timer1.initialize(sampleInterval);      // use scheduled interrupts for sample invoking
  Timer1.attachInterrupt(IS_sample);
  attachInterrupt(digitalPinToInterrupt(keyInterruptPin), IS_keyscan, RISING);
  

  // lcd, connections init
  lcd.begin(cols,rows);
  digitalWrite(bgledPin, HIGH);
  lcd.setCursor(0,0);
  lcd.print("      GO NUTS");
  fetch_IP();
  Connect_MQTT_server();
  lcd.clear();
}



// declare here to avoid unwanted resetting
int sample_count = 0;
int val_hist [n_samples] = {};

void loop() {
  const char* dirs [8] = {"N","NE","E","SE","S","SW","W","NW"};
  int deg;
  int val;
  char* dir;
  int moode;

  // get sample, update deg, val, dir
  if(sampleInQueue){
    deg = read_v_to_deg();
    Serial.print("deg: ");
    Serial.println(deg);
    
    val = deg_to_val(deg);
    Serial.print("val: ");
    Serial.println(val);
    
    dir = dirs[val-1];
    Serial.print("dir: ");
    Serial.println(dir);
    Serial.println();

    Serial.print("sample_count: ");
    Serial.println(sample_count);

    val_hist[sample_count] = val;
    
    sample_count++;
    sampleInQueue = false;      // reset sample flag
  }

  if(sample_count == n_samples){
    moode = find_mode(val_hist, n_samples);
    for(auto val : val_hist){   // clear val_hist
      val = 0;
    }
    
    sample_count = 0;           // reset sample counter
    send_MQTT_message(moode);
  }

  // TODO: LCD PRINTS
  
  if(dispButtonPress != 0){
    dispMode = dispButtonPress;
  }
  else{
    dispOn = !dispOn;
    dispButtonPress = dispMode; // set dispButtonPress back to avoid flicker
  }

  if(dispOn){
    digitalWrite(bgledPin, HIGH);
  }
  else{
    digitalWrite(bgledPin, LOW);
  }

  lcd.setCursor(0,0);
  if(dispOn){
    switch(dispMode){
      case 1:
        //           01234567890123456789 
        lcd.println("      GO NUTS       ");
        lcd.setCursor(0,2);
        lcd.print("Wind direction: ");
        lcd.print("  ");
        lcd.setCursor(16,2);
        lcd.print(dir); 
        break;
      
      case 2: 
        //           01234567890123456789 
        lcd.println("      GO NUTS       ");
        lcd.setCursor(0,2);
        lcd.print("My IP: ");
        lcd.print(Ethernet.localIP());
        lcd.print("   ");  // clear wind dir off the screen
        break; 
    }
  }
  else lcd.clear();
}
