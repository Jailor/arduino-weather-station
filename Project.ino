#include "DHT.h"
#include <LiquidCrystal.h>
#include <IRremote.h>
#include <TimerOne.h>

#include <ThreeWire.h>  
#include <RtcDS1302.h>

#include <SoftwareSerial.h>

#define RECV_PIN 53 // the pin where you connect the output pin of IR sensor     
IRrecv irrecv(RECV_PIN);
decode_results results;

SoftwareSerial bluetooth(19, 18); // RX and TX

#define DHT_PIN 51     // Digital pin connected to the DHT sensor
#define DHT_TYPE DHT11


#define RS 8
#define EN 9
//#define CONTRAST_PIN 4 //pin 10 PWM not working
#define D4 4
#define D5 5
#define D6 6
#define D7 7

#define CLK 43
#define CLK_DAT 41
#define CLK_RST 39

#define GAS_PIN A8
#define LIGHT_PIN A9
#define FIRE_PIN A10

#define BUZZER_PIN 45
#define LED_PIN 44

DHT dht(DHT_PIN, DHT_TYPE);

LiquidCrystal lcd(8,9,4,5,6,7);

ThreeWire myWire(CLK_DAT,CLK,CLK_RST); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);


#define PRESSED_ONE 4077715200 // temperature and humidity
#define PRESSED_TWO 3877175040
#define PRESSED_THREE 2707357440
#define PRESSED_FOUR 4144561920
#define PRESSED_FIVE 3810328320
#define FALSE_PRESS 0

#define countof(a) (sizeof(a) / sizeof(a[0]))

int CURRENT_STATE = 1;
volatile bool state_changed = false;
float h;
float t;

void setup() {   
  Serial.begin(9600);
  bluetooth.begin(9600);
  
  irrecv.enableIRIn();
  
  lcd.begin(16, 2);
  
  lcd.setCursor(0, 0);
  
  dht.begin();
  
  Timer1.initialize(3000000); // init the timing interval
  Timer1.attachInterrupt(changeBool); 
  
  Rtc.Begin();
  //rtc_setup();

  pinMode(BUZZER_PIN, OUTPUT);
  lcd.print("Setup complete!");
}



void loop() {
  checkFire();
  checkIRreceiver();
  
  if(state_changed == true){
    state_changed = false;
    checkTempAndHumidity();
    
    lcd.clear();
    lcd.setCursor(0, 0);
    if(CURRENT_STATE == 1) printTempAndHumidity(); 
    if(CURRENT_STATE == 2)  {
      RtcDateTime now = Rtc.GetDateTime();
      printDateTime(now);
    }
    if(CURRENT_STATE == 3){
      //lcd.print("Pressed 3");
      int sensorValue = analogRead(GAS_PIN);
      Serial.print("Read from gas pin:");
      Serial.println(sensorValue);
      lcd.print("Gas value:");
      lcd.print(sensorValue);
      lcd.setCursor(0,1);
      if(sensorValue < 200)lcd.print("Good quality!");
      else if(sensorValue < 500) lcd.print("Medium quality!");
      else lcd.print("DANGER!Bad air!");
    }
    if(CURRENT_STATE == 4){
      int sensorValue = analogRead(LIGHT_PIN);
      Serial.println(sensorValue);

      lcd.print("Light value:");
      lcd.print(sensorValue);
      lcd.setCursor(0,1);
      if(sensorValue < 50) lcd.print("Pitch dark");
      else if(sensorValue <150) lcd.print("Dark");
      else if(sensorValue < 300) lcd.print("Dim");
      else if(sensorValue < 700) lcd.print("Normal light");
      else if (sensorValue <900) lcd.print("Bright");
      else lcd.print("Blinding light");
    }
    if(CURRENT_STATE == 5){
      int sensorValue = analogRead(FIRE_PIN);
      Serial.println(sensorValue);
      lcd.print("IR value:");
      lcd.print(sensorValue);
      lcd.setCursor(0,1);
      if(sensorValue > 500) lcd.print("No fire!");
    }
  }
  //small delay between measurements
  delay(100);

}

void checkTempAndHumidity(){
    h = dht.readHumidity();
    t = dht.readTemperature();
}

void printTempAndHumidity(){
   if (isnan(h) || isnan(t)) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Failed to read");
      lcd.setCursor(0,1);
      lcd.print("from DHT sensor");
      // for debug
      Serial.println("Failed to read from DHT sensor!");
      return;
    }else
    {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Hum: ");
        lcd.print(h);
        lcd.print("%");
  
        lcd.setCursor(0,1);
        lcd.print("Temp: ");
        lcd.print(t);
        lcd.print((char)223);
        lcd.print("C");
        // for debug
        Serial.print(" Humidity: ");
        Serial.print(h);
        Serial.print("%  Temperature: ");
        Serial.println(t);
    }
}
void checkIRreceiver(){
  if (irrecv.decode())// Returns 0 if no data ready, 1 if data ready.
  {
    long long incoming = irrecv.decodedIRData.decodedRawData;
    if(incoming == FALSE_PRESS) goto CONT;
    lcd.clear();
    lcd.setCursor(0, 0);
    if(incoming == PRESSED_ONE) {
      CURRENT_STATE = 1;
    }
    else if(incoming == PRESSED_TWO) {
      CURRENT_STATE = 2;
    }
    else if(incoming == PRESSED_THREE) {
      CURRENT_STATE = 3;
    }
    else if(incoming == PRESSED_FOUR) {
      CURRENT_STATE = 4;
    }
    else if(incoming == PRESSED_FIVE) {
      CURRENT_STATE = 5;
    }
    else {
      Serial.println("Unknown code from remote:");
      Serial.println(irrecv.decodedIRData.decodedRawData, DEC);
    }
    state_changed = true;
    CONT:
    irrecv.resume(); // Restart the ISR state machine and Receive the next value
  }
}

void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];
    char timestring[20];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("Date: %02u/%02u/%04u"),
            dt.Day(),
            dt.Month(),
            dt.Year());
            
    snprintf_P(timestring, 
        countof(timestring),
        PSTR("Time: %02u:%02u:%02u"),
        dt.Hour(),
        dt.Minute(),
        dt.Second() );
    Serial.println(datestring);
    Serial.println(timestring);
    lcd.print(datestring);
    lcd.setCursor(0,1);
    lcd.print(timestring);
}

void rtc_setup(){
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();
    if (!Rtc.IsDateTimeValid()) 
    {
        // Common Causes:
        //    1) first time you ran and the device wasn't running yet
        //    2) the battery on the device is low or even missing

        Serial.println("RTC lost confidence in the DateTime!");
        Rtc.SetDateTime(compiled);
    }

    if (Rtc.GetIsWriteProtected())
    {
        Serial.println("RTC was write protected, enabling writing now");
        Rtc.SetIsWriteProtected(false);
    }

    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }
}

void checkFire(){
  int fireSensorValue = analogRead(FIRE_PIN);
  while(fireSensorValue < 400){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("IR value:");
    lcd.print(fireSensorValue);
    lcd.setCursor(0,1);
    lcd.print("DANGER! FIRE!");
    analogWrite(LED_PIN,30);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(3000);
    digitalWrite(BUZZER_PIN, LOW);
    analogWrite(LED_PIN, 0);
    fireSensorValue = analogRead(FIRE_PIN);
  }
}

void changeBool(void)
{
  state_changed = true;
  char messageToSend[200];
     
  if (isnan(h) || isnan(t)) {
    h = 0.0;
    t = 0.0;
  }
  char str_temp[6];
  char hum_temp[6];
  
  dtostrf(t, 4, 2, str_temp);
  dtostrf(h, 4, 2, hum_temp);
  
  RtcDateTime dt = Rtc.GetDateTime();

  int gasSensorValue = analogRead(GAS_PIN);
  char gasString[20];

  if(gasSensorValue < 200)strcpy(gasString,"Good quality!");
  else if(gasSensorValue < 500) strcpy(gasString,"Medium quality!");
  else strcpy(gasString,"DANGER!Bad air!");
  
  int lightSensorValue = analogRead(LIGHT_PIN);
  char lightString[20];
  
  if(lightSensorValue < 50) strcpy(lightString, "Pitch dark");
  else if(lightSensorValue <150) strcpy(lightString, "Dark");
  else if(lightSensorValue < 300) strcpy(lightString, "Dim");
  else if(lightSensorValue < 700) strcpy(lightString, "Normal Light");
  else if (lightSensorValue <900) strcpy(lightString, "Bright");
  else strcpy(lightString, "Blidning light");
  
  int fireSensorValue = analogRead(FIRE_PIN); 
  char fireString[15];
  if(fireSensorValue < 500){
    strcpy(fireString,"DANGER! FIRE!");
  }
  else strcpy(fireString, "No fire!");

  snprintf_P(messageToSend, 
            countof(messageToSend),
PSTR("Humidity=%s%% \nTemperature=%s°C \nDate: %02u/%02u/%04u \nTime: %02u:%02u:%02u \nGas sensor value: %d -> %s\nLight sensor value: %d -> %s\nFire sensor value: %d -> %s\n\n"),
            hum_temp,
            str_temp,
            dt.Day(),
            dt.Month(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second(),
            gasSensorValue,
            gasString,
            lightSensorValue,
            lightString,
            fireSensorValue,
            fireString);
            
  bluetooth.print(messageToSend);
}



 
