/**
  Интервалы:
   - время пауз между затоплентями k*60 мин = k*3600 сек
   - время затопления 10 мин = 600 сек

  k коэффициент, зависит от:
  - времени суток (1 днем, 3 ночью)
  - влажности воздуха и почвы ( значения определить )
*/
#include <Wire.h>
#include <DS3231.h>

#define INTERVAL          3000
#define PERIOD		        600
#define COMPRESSOR_TIME   80
#define CLAPAN_TIME       600
#define CLAPAN_TIME_SEC   600

#define COMPRESSOR_PIN    3
#define CLAPAN_PIN        4
#define CLAPAN_PIN_SEC    6
#define WATER_LEVEL_PIN	  2
#define LIGHT_PIN         5
#define LIGHT_SENSOR_PIN  A0

#define MIN_LUM           180
#define MAX_LUM           300
#define SUBMERSION_TIME   120
#define DRAINING_TIME     180

enum STATE
{
  COMPRESSOR_ON=0,  // включен компрессор, вода накачивается в трубу  t=80
  SUBMERSION=1,     // все выключено вода в верхней трубе             t=600
  CLAPAN_ON=2,      // включен клапан, вода стекает в нижнюю трубу    t=600
  SUBMERSION_SEC=3, // все выключено вода в нижней трубе              t=600
  CLAPAN_SEC_ON=4,  // включен клапан 2, вода стекает в бак           t=600
  DRAINING=5        // все выключено вода в баке                      t=k*1600
};

RTClib RTC;
DS3231 Clock;

// коэффициент расчета временных интервалов затоплений
int k;
// время следующего действия
uint32_t nextActionTime;
// напряжение соответствующее освещенности на датчике света
int lum;
STATE state;

void setup () {
  k=1;
  lum = 0;
  Serial.begin(19200);
  Wire.begin();
  // pin setup
  pinMode(COMPRESSOR_PIN,OUTPUT);
  pinMode(CLAPAN_PIN,OUTPUT);
  pinMode(CLAPAN_PIN_SEC,OUTPUT);
  pinMode(LIGHT_PIN,OUTPUT);
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  
  state = COMPRESSOR_ON;
  digitalWrite(COMPRESSOR_PIN, HIGH);
  digitalWrite(CLAPAN_PIN,LOW);
  digitalWrite(CLAPAN_PIN_SEC, LOW);
//        Clock.setYear(18);
//        Clock.setMonth(1);
//  		Clock.setDate(22);
//  		Clock.setHour(6);
//  		Clock.setMinute(55);
//  		Clock.setDoW(1);
//  		Clock.setClockMode(false);
  nextActionTime=RTC.now().unixtime()+COMPRESSOR_TIME;
  printTime(RTC.now());
  Serial.println("Compressor ON");
}

void loop () {
  DateTime now = RTC.now();
  getK();
  compressorClapanProcess(now);
  lightProcess(now);
  delay(1000);
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
    case 1:
      hourMorningStart = 5;  
      hourMorningEnd = 7;
      hourEveningStart = 17;
      hourEveningEnd = 21;
    break;
    case 2:
      hourMorningStart = 5;  
      hourMorningEnd = 7;
      hourEveningStart = 17;
      hourEveningEnd = 22;
    break;
    case 3:
      hourMorningStart = 5;  
      hourMorningEnd = 7;
      hourEveningStart = 18;
      hourEveningEnd = 22;
    break;
    case 4:
      hourMorningStart = 5;  
      hourMorningEnd = 7;
      hourEveningStart = 18;
      hourEveningEnd = 22;
    break;
    case 5:
      hourMorningStart = 5;  
      hourMorningEnd = 6;
      hourEveningStart = 19;
      hourEveningEnd = 21;
    break;
    case 6:
      hourMorningStart = 5;  
      hourMorningEnd = 5;
      hourEveningStart = 20;
      hourEveningEnd = 21;
    break;
    case 7:
      hourMorningStart = 5;  
      hourMorningEnd = 5;
      hourEveningStart = 21;
      hourEveningEnd = 21;
    break;
    case 8:
      hourMorningStart = 5;  
      hourMorningEnd = 5;
      hourEveningStart = 21;
      hourEveningEnd = 21;
    break;
    case 9:
      hourMorningStart = 5;  
      hourMorningEnd = 6;
      hourEveningStart = 20;
      hourEveningEnd = 22;
    break;
    case 10:
      hourMorningStart = 5;  
      hourMorningEnd = 6;
      hourEveningStart = 20;
      hourEveningEnd = 22;
    break;
    case 11:
      hourMorningStart = 5;  
      hourMorningEnd = 7;
      hourEveningStart = 18;
      hourEveningEnd = 21;
    break;
    case 12:
      hourMorningStart = 5;  
      hourMorningEnd = 7;
      hourEveningStart = 17;
      hourEveningEnd = 21;
    break;
  }
  if(hourMorningStart != hourMorningEnd && now.hour() >= hourMorningStart && now.hour() < hourMorningEnd)
  {
    if(digitalRead(LIGHT_PIN) == LOW)
    {
      digitalWrite(LIGHT_PIN,HIGH);
    }
  }
  else if(hourEveningStart != hourEveningEnd && now.hour() >= hourEveningStart && now.hour() < hourEveningEnd )
  {
    if(digitalRead(LIGHT_PIN) == LOW)
    {
      digitalWrite(LIGHT_PIN,HIGH);
    }
  }
  else
  {
    if(digitalRead(LIGHT_PIN) == HIGH)
    {
      digitalWrite(LIGHT_PIN,LOW);
    }
  }
}

void compressorClapanProcess(DateTime now)
{
  uint32_t currTime = now.unixtime();
  switch(state)
  {
    case COMPRESSOR_ON:     
      // Проверить пора ли перейти в режим затопление
      if (currTime >= nextActionTime)
      {
        printTime(now);
        Serial.println("Submersion started");
        // выключить компрессор и пересчитать время событий
        digitalWrite(COMPRESSOR_PIN, LOW);
        nextActionTime = currTime+PERIOD;
        state = SUBMERSION;
      }
      break;
    case SUBMERSION:
      if (currTime >= nextActionTime)
      {
        printTime(now);
        Serial.println("Clapan ON");
        // включить клапан, пересчитать время
        digitalWrite(CLAPAN_PIN, HIGH);
        nextActionTime = currTime+CLAPAN_TIME;
        state = CLAPAN_ON;
      }
      break;
    case CLAPAN_ON:     
      if (currTime >= nextActionTime)
      {
        printTime(now);
        Serial.println("Draining on");
        // выключить клапан и пересчитать время
        digitalWrite(CLAPAN_PIN, LOW);
        nextActionTime = currTime + PERIOD;
        state = SUBMERSION_SEC;
      }
      break;
    case SUBMERSION_SEC:
      if (currTime >= nextActionTime)
      {
        printTime(now);
        Serial.println("Draining on");
        // включить клапан 2 и пересчитать время
        digitalWrite(CLAPAN_PIN_SEC, HIGH);
        nextActionTime = currTime + CLAPAN_TIME_SEC;
//        nextActionTime = currTime + k*INTERVAL;
        state = CLAPAN_SEC_ON;
      }
      break;
    case CLAPAN_SEC_ON:
      if (currTime >= nextActionTime)
      {
        printTime(now);
        Serial.println("Draining on");
        // выключить клапан 2 и пересчитать время
        digitalWrite(CLAPAN_PIN_SEC, LOW);
        nextActionTime = currTime + k*INTERVAL;
        state = DRAINING;
      }
      break;  
    case DRAINING:
      if (currTime >= nextActionTime)
      {
        printTime(now);
        Serial.println("Compressor ON");
        // включить компрессор и пересчитать время
        digitalWrite(COMPRESSOR_PIN, HIGH);
        nextActionTime = currTime+COMPRESSOR_TIME;
        state = COMPRESSOR_ON;
      }
      break;
  }
}

void getK()
{
  int tmp = k;
  lum = analogRead(LIGHT_SENSOR_PIN);
  if (lum <= MIN_LUM)
  {
    k = 3;
  }
  else
  {
    k = 1;
  }
  if (k != tmp)
  {
    Serial.print("k = ");
    Serial.println(k);
  }
}

void printTime(DateTime now)
{
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
//
//  Serial.print(" since midnight 1/1/1970 = ");
//  Serial.print(now.unixtime());
//  Serial.print("s = ");
//  Serial.print(now.unixtime() / 86400L);
//  Serial.println("d");
  //Serial.println("Temp="+Clock.getTemperature());
}
