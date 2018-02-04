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
  COMPRESSOR_ONE=0,  // включен компрессор, вода накачивается в трубу  t=80
  SUBMERSION_ONE=1,     // все выключено вода в верхней трубе             t=600
  COMPRESSOR_TWO=2,      // включен клапан, вода стекает в нижнюю трубу    t=600
  SUBMERSION_TWO=3, // все выключено вода в нижней трубе              t=600
  CLAPAN=4,         // включен клапан 2, вода стекает в бак           t=600
  PAUSE=5           // все выключено вода в баке                      t=k*3000
};

static inline int *timeFromState(enum STATE state)
{
//  static const int *times[] = { 7, 60, 7, 60, 180, 50 };
  static const int *times[] = { 70, 600, 70, 600, 1800, 500 };

    return times[state];
}

static inline STATE nextState(enum STATE state)
{
  static const STATE states[] = { SUBMERSION_ONE, COMPRESSOR_TWO, SUBMERSION_TWO, CLAPAN, PAUSE, COMPRESSOR_ONE };

    return states[state];
}

static inline char *stringFromState(enum STATE state)
{
    static const char *strings[] = { "Компрессор 1", "Затопление трубы 1", "Компрессор 2", "Затопление трубы 2", "Клапан", "Тайм-Аут"  };

    return strings[state];
}

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
  
  state = COMPRESSOR_ONE;
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
  nextActionTime=RTC.now().unixtime()+timeFromState(COMPRESSOR_ONE);
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
  if(currTime >= nextActionTime)
  {
    printTime(now);
    state = nextState(state);
    Serial.println(stringFromState(state));
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
        digitalWrite(CLAPAN_PIN, HIGH);
      break;
    case COMPRESSOR_TWO:
        digitalWrite(CLAPAN_PIN, LOW);
      break;
    case SUBMERSION_TWO:
        digitalWrite(CLAPAN_PIN_SEC, HIGH);
      break;
    case CLAPAN:
        digitalWrite(CLAPAN_PIN_SEC, LOW);
        break;  
    case PAUSE:
        digitalWrite(COMPRESSOR_PIN, HIGH);
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
