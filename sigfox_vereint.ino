#include <RTClib.h>

#include <Arduino.h>
#include <SigFox.h>
#include <ArduinoLowPower.h>
#include "DHT.h"
#include "Wire.h"
#include <SPI.h>
#include <SD.h>

const int chipSelect = 7;

#define WIRE Wire1
#define DHTPIN 1   
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

volatile bool israining = 0;
int rain_count_day = 0;
int rain_count_interval = 0;
int message_to_save = 4;
int rain = 0;
bool forced_send = 1;

float temp;
float hum;

RTC_DS3231 rtc;
DateTime jetzt;

typedef struct __attribute__ ((packed)) sigfox_message {
  uint8_t voltage;
  uint16_t temp1;
  uint16_t temp2;
  uint8_t hum1;
  uint8_t hum2;
  uint8_t rain1;
  uint8_t rain2;
  uint16_t rain_day;
} SigfoxMessage;

SigfoxMessage msg;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(0, INPUT_PULLDOWN);
   
    rtc.begin();
    if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
    }
    LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, alarm, CHANGE);
    LowPower.attachInterruptWakeup(0, itsraining, FALLING);

    if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }

    analogReadResolution(12);
    analogReference(AR_INTERNAL);
  
    dht.begin();
    msg.hum1 = msg.hum2 = msg.rain1 = msg.rain2 = msg.rain_day = msg.temp1 = msg.temp2 = msg.voltage = 0;
    blink(5,200);
    LowPower.sleep(sleeptime());
}

void loop() {
    jetzt = rtc.now();   
        
    if (israining == 1) {
        blink(1, 1000);
        Serial.println("rain");
        // count rain, save rain to SD
        rain_count_day++;
        rain_count_interval++;
        log_rain(jetzt, rain_count_day);
        israining = 0;        
    }
    else {
        // no rain --> read sensors, save data
        delay(2000);
        temp = dht.readTemperature();
        hum = dht.readHumidity();
        log_Sensordata(jetzt, temp, hum);
        
        Serial.println(temp);
        Serial.println(hum);        
        
        if (jetzt.hour() == 0 && jetzt.minute() == 0) {
            message_to_save = 4;
            forced_send = 1;
        }
                
        if (jetzt.minute() % 10 == 0) {
            blink(2, 500);
            // Minute mit Teiler 10 --> Senden der Daten
            int sensorValue = analogRead(ADC_BATTERY);
            float voltage = sensorValue * (6.8 / 4095.0);
            msg.voltage = (voltage - 2.4) * 255;
            msg.temp2 = ((temp + 40) / 100) * 65535;
            msg.hum2 = (hum / 100) * 255;
            msg.rain2 = rain_count_interval;
            msg.rain_day = rain_count_day;
            
            rain_count_interval = 0;

            rain = msg.rain1 + msg.rain2;

            if (message_to_save > 0) {
                if (forced_send == 1) {
                    send_data(msg);
                    forced_send = 0;
                }
                else if (rain > 0) {
                    send_data(msg);
                }
                else {
                    message_to_save--;
                    forced_send = 1;
                }
            }
            else {
                send_data(msg);
            }        
        }

        else {
            blink(4, 500);
            // Minute mit Teiler 5 --> nur setzen der msg.Teile
            msg.temp1 = ((temp + 40) / 100) * 65535;
            msg.hum1 = (hum / 100) * 255;
            msg.rain1 = rain_count_interval;
           
            rain_count_interval = 0;
        }

        if (jetzt.hour() == 1 && jetzt.minute() == 0) {
            rain_count_day = 0;
        }
        
    }
    
    LowPower.sleep(sleeptime());
    //delay(10000);
}


void alarm() {
}

void itsraining() { 
    israining = 1;
}

void log_rain(DateTime timestamp, int count) {
  // save rain to SD
  File dataFile = SD.open("rain.txt", FILE_WRITE);
  dataFile.print(timestamp.day());
  dataFile.print(".");
  dataFile.print(timestamp.month());
  dataFile.print(".");
  dataFile.print(timestamp.year());
  dataFile.print(" ");
  dataFile.print(timestamp.hour());
  dataFile.print(":");
  dataFile.print(timestamp.minute());
  dataFile.print(":");
  dataFile.print(timestamp.second());
  dataFile.print(";");
  dataFile.println(count);
  dataFile.close();
  
}

void log_Sensordata(DateTime timestamp, float temp, float hum) {
  File dataFile = SD.open("weather.txt", FILE_WRITE);
  dataFile.print(timestamp.day());
  dataFile.print(".");
  dataFile.print(timestamp.month());
  dataFile.print(".");
  dataFile.print(timestamp.year());
  dataFile.print(" ");
  dataFile.print(timestamp.hour());
  dataFile.print(":");
  dataFile.print(timestamp.minute());
  dataFile.print(":");
  dataFile.print(timestamp.second());
  dataFile.print(";");
  dataFile.print(temp);
  dataFile.print(";");
  dataFile.println(hum);  
  dataFile.close();
}

int sleeptime() {
    jetzt = rtc.now();
    int x = 1000 * (60 * (4 - (jetzt.minute() % 5)) + (60 - jetzt.second()));
    //int x = 1000 * (60 - jetzt.second());
    return x;
}

void blink(int x, int y) {
  for (int i=0; i < x; i++){
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(y);                       // wait for a second
    
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(y);    
  }// wait for a second
}

void send_data(SigfoxMessage message) {
    SigFox.begin();
    // Wait at least 30ms after first configuration (100ms before)
    delay(50);
    SigFox.debug();
          
    SigFox.status();
    delay(1);
    SigFox.beginPacket();
    SigFox.write((uint8_t*)&message, 12);
    SigFox.endPacket();
    SigFox.end();
}
