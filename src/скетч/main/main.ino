/**
  Интервалы:
   - время пауз между затоплентями k*60 мин = k*3600 сек
   - время затопления 10 мин = 600 сек

  k коэффициент, зависит от:
  - времени суток (1 днем, 3 ночью)
  - влажности воздуха и почвы ( значения определить )
*/
#include "VirtuinoEsp8266_WebServer.h"
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <Wire.h>
#include <DS3231.h>

#define WATER_LEVEL_PIN     2

#define COMPRESSOR_PIN      3
#define COMPRESSOR_PIN_TWO  4
#define CLAPAN_PIN          5
#define LIGHT_PIN           6

#define SENSOR_TEMP         8
#define LIGHT_SENSOR_PIN    A0
#define SOIL_PIN            A1
#define SENSOR_PH           A2

#define MIN_LUM           180
#define MAX_LUM           300
#define OFFSET            0.00

/** 
 * arduino RX pin=9  arduino TX pin=10 
 * connect the arduino RX pin to esp8266 module TX pin     
 * connect the arduino TX pin to esp8266 module RX pin
 */    
SoftwareSerial espSerial =  SoftwareSerial(9,10);
/**                                                       
 * Your esp8266 device's speed is probably at 115200. 
 * For this reason use the test code to change the baud rate to 9600                                                       
 * SoftwareSerial doesn't work at 115200 
 */
VirtuinoEsp8266_WebServer virtuino(espSerial, 9600);                                                      
OneWire ds(SENSOR_TEMP);
DallasTemperature sensors(&ds);

enum STATE
{
  COMPRESSOR_ONE=0,  // включен компрессор, вода накачивается в трубу  t=80
  SUBMERSION_ONE=1,  // все выключено вода в верхней трубе             t=600
  COMPRESSOR_TWO=2,  // включен клапан, вода стекает в нижнюю трубу    t=600
  SUBMERSION_TWO=3,  // все выключено вода в нижней трубе              t=600
  CLAPAN=4,          // включен клапан 2, вода стекает в бак           t=600
  PAUSE=5            // все выключено вода в баке                      t=k*3000
};

static inline uint16_t timeFromState(enum STATE state)
{
  static const uint16_t times[] = { 80, 600, 80, 600, 1200, 3000 };
  return times[state];
}

static inline STATE nextState(enum STATE state)
{
  static const STATE states[] = { SUBMERSION_ONE, COMPRESSOR_TWO, SUBMERSION_TWO, CLAPAN, PAUSE, COMPRESSOR_ONE };
  return states[state];
}

//static inline char *stringFromState(enum STATE state)
//{
//    static const char *strings[] = { "Компрессор 1", "Затопление трубы 1", "Компрессор 2", "Затопление трубы 2", "Клапан", "Тайм-Аут"  };
//
//    return strings[state];
//}

enum LED_STATE
{
  LED_ON=1,
  LED_OFF=0  
};

RTClib RTC;
DS3231 Clock;

// коэффициент расчета временных интервалов затоплений
byte k;
// время следующего действия
uint32_t nextActionTime;
STATE state;
LED_STATE ledState;

//int phVol;
//float phVal;

void setup () {
  virtuino.DEBUG=true;
  Serial.begin(9600);
  espSerial.begin(9600);
  /**
   * Set your home wifi router SSID and PASSWORD. ESP8266 will connect to Internet. Port=8000
   */
  virtuino.connectESP8266_toInternet("ASUS_60_2G","kir12bo8",8000);
  /**  
   * Set a local ip. Forward port 80 to this IP on your router
   */
  virtuino.esp8266_setIP(192,168,1,135);                          
  /**
   * Enable this line to create a wifi local netrork using ESP8266 as access point
   */
  //virtuino.createLocalESP8266_wifiServer("ESP8266","1234",8000,2);
  virtuino.password="1234";
  k=1;
  sensors.begin();
//  Wire.begin();
  // pin setup
  pinMode(COMPRESSOR_PIN,OUTPUT);
  pinMode(CLAPAN_PIN,OUTPUT);
  pinMode(COMPRESSOR_PIN_TWO,OUTPUT);
  pinMode(LIGHT_PIN,OUTPUT);
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  
  state = COMPRESSOR_ONE;
  digitalWrite(COMPRESSOR_PIN, LOW);
  digitalWrite(COMPRESSOR_PIN_TWO, HIGH);
  digitalWrite(CLAPAN_PIN,HIGH);
  digitalWrite(LIGHT_PIN,HIGH);
  
  
  nextActionTime=RTC.now().unixtime()+timeFromState(COMPRESSOR_ONE);
  
  ledState=LED_OFF;
  
//        Clock.setYear(18);
//        Clock.setMonth(1);
//      Clock.setDate(22);
//      Clock.setHour(6);
//      Clock.setMinute(55);
//      Clock.setDoW(1);
//      Clock.setClockMode(false);
//  printTime(RTC.now());
//  Serial.println((long)timeFromState(COMPRESSOR_ONE));
//  Serial.print("Current unix time: ");
//  Serial.println(RTC.now().unixtime());
  Serial.print("Next action time - ");
  Serial.println(nextActionTime);
//  Serial.println(stringFromState(state));
}

void loop () {
  virtuino.run();
  DateTime now = RTC.now();
  getK();
  compressorClapanProcess(now);
  lightProcess(now);
  getTemperature();
  virtuino.vDelay(10000);
}

/**
 * Включать доп освещение утром и вечером в зависимости от текущего месяца (и дня)
 */
void lightProcess(DateTime now)
{
  byte hourMorningStart = 6;
  byte hourMorningEnd = 7;
  byte hourEveningStart = 20;
  byte hourEveningEnd = 22;
  switch(now.month())
  {
    case 1 ... 3:
      hourMorningStart = 5;  
      hourMorningEnd = 7;
      hourEveningStart = 17;
      hourEveningEnd = 21;
    break;
    case 4 ... 5:
      hourMorningStart = 5;  
      hourMorningEnd = 7;
      hourEveningStart = 18;
      hourEveningEnd = 22;
    break;
    case 6:
      hourMorningStart = 5;  
      hourMorningEnd = 5;
      hourEveningStart = 20;
      hourEveningEnd = 21;
    break;
    case 7 ... 8:
      hourMorningStart = 5;  
      hourMorningEnd = 5;
      hourEveningStart = 21;
      hourEveningEnd = 21;
    break;
    case 9 ... 10:
      hourMorningStart = 5;  
      hourMorningEnd = 6;
      hourEveningStart = 20;
      hourEveningEnd = 22;
    break;
    case 11 ... 12:
      hourMorningStart = 5;  
      hourMorningEnd = 7;
      hourEveningStart = 18;
      hourEveningEnd = 21;
    break;
  }
  if(hourMorningStart != hourMorningEnd && now.hour() >= hourMorningStart && now.hour() < hourMorningEnd)
  {
    if(ledState == LED_OFF)
    {
      Serial.println("Morning light ON");
      digitalWrite(LIGHT_PIN,LOW);
      ledState = LED_ON;
    }
  }
  else if(hourEveningStart != hourEveningEnd && now.hour() >= hourEveningStart && now.hour() < hourEveningEnd )
  {
    if(ledState == LED_OFF)
    {
      Serial.println("Evening light ON");
      digitalWrite(LIGHT_PIN,LOW);
      ledState = LED_ON;
    }
  }
  else
  {
    if(ledState == LED_ON)
    {
      Serial.println("Light OFF");
      digitalWrite(LIGHT_PIN,HIGH);
      ledState = LED_OFF;
    }
  }
}

void compressorClapanProcess(DateTime now)
{
  uint32_t currTime = now.unixtime();
  if(currTime >= nextActionTime) 
  {
//    printTime(now);
    state = nextState(state);
//    Serial.println(stringFromState(state));
    nextActionTime = currTime + timeFromState(state);
    processState(state);
  }
}

void processState(STATE state)
{
  switch(state)
  {
    case COMPRESSOR_ONE:     
        digitalWrite(COMPRESSOR_PIN, LOW);
      break;
    case SUBMERSION_ONE:
        digitalWrite(COMPRESSOR_PIN, HIGH);
      break;
    case COMPRESSOR_TWO:
        digitalWrite(COMPRESSOR_PIN_TWO, LOW);
      break;
    case SUBMERSION_TWO:
        digitalWrite(COMPRESSOR_PIN_TWO, HIGH);
      break;
    case CLAPAN:
        digitalWrite(CLAPAN_PIN, LOW);
        break;  
    case PAUSE:
        digitalWrite(CLAPAN_PIN, HIGH);
        break;
  }
}

void getK()
{
  byte tmp = k;
  uint16_t lum = analogRead(LIGHT_SENSOR_PIN);
  if (lum <= MIN_LUM)
  {
    k = 3;
  }
  else
  {
    k = 1;
  }
  int soilVal = analogRead(SOIL_PIN);
  //Serial.println(soilVal);
  if(soilVal<=200)
  {
    k=k+2;
  }
  else if(soilVal<=500 && soilVal>200)
  {
    k=k+1;
  }
  
  if (k != tmp)
  {
    Serial.print("k = ");
    Serial.println(k);
  }
}

void getTemperature()
{
 // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
 // Serial.println(" Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  //Serial.println("DONE");

  // You can have more than one IC on the same bus. 
  // 0 refers to the first IC on the wire
  float temperature1=sensors.getTempCByIndex(0);
 // Serial.println("Temperature for Device 1 is: "+String(temperature1));
  virtuino.vMemoryWrite(0,temperature1);    // write temperature 1 to virtual pin V0. On Virtuino panel add a value display or an analog instrument to pin V0
}


//void printTime(DateTime now)
//{
//  Serial.print(now.year(), DEC);
//  Serial.print('/');
//  Serial.print(now.month(), DEC);
//  Serial.print('/');
//  Serial.print(now.day(), DEC);
//  Serial.print(' ');
//  Serial.print(now.hour(), DEC);
//  Serial.print(':');
//  Serial.print(now.minute(), DEC);
//  Serial.print(':');
//  Serial.print(now.second(), DEC);
//  Serial.println();
//
//  Serial.print(" since midnight 1/1/1970 = ");
//  Serial.print(now.unixtime());
//  Serial.print("s = ");
//  Serial.print(now.unixtime() / 86400L);
//  Serial.println("d");
  //Serial.println("Temp="+Clock.getTemperature());
//}
