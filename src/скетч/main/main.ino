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

#define COMPRESSOR_PIN      3
#define CLAPAN_PIN          5
#define COMPRESSOR_PIN_TWO  4
#define WATER_LEVEL_PIN	    2
#define LIGHT_PIN           6
#define LIGHT_SENSOR_PIN    A0
#define SOIL_PIN            A1

#define MIN_LUM           180
#define MAX_LUM           300

enum STATE
{
  COMPRESSOR_ONE=0,  // включен компрессор, вода накачивается в трубу  t=80
  SUBMERSION_ONE=1,     // все выключено вода в верхней трубе             t=600
  COMPRESSOR_TWO=2,      // включен клапан, вода стекает в нижнюю трубу    t=600
  SUBMERSION_TWO=3, // все выключено вода в нижней трубе              t=600
  CLAPAN=4,         // включен клапан 2, вода стекает в бак           t=600
  PAUSE=5           // все выключено вода в баке                      t=k*3000
};

static inline int timeFromState(enum STATE state)
{
//  static const int times[] = { 7, 60, 7, 60, 180, 50 };
  static const int times[] = { 80, 600, 80, 600, 1200, 1100 };

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

enum LED_STATE
{
  LED_ON=1,
  LED_OFF=0  
};

RTClib RTC;
DS3231 Clock;

// коэффициент расчета временных интервалов затоплений
float k;
// время следующего действия
uint32_t nextActionTime;
// напряжение соответствующее освещенности на датчике света
int lum;
STATE state;
LED_STATE ledState;

void setup () {
  k=1;
  lum = 0;
  Serial.begin(19200);
  Wire.begin();
  // pin setup
  pinMode(COMPRESSOR_PIN,OUTPUT);
  pinMode(CLAPAN_PIN,OUTPUT);
  pinMode(COMPRESSOR_PIN_TWO,OUTPUT);
  pinMode(LIGHT_PIN,OUTPUT);
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  
  state = COMPRESSOR_ONE;
  digitalWrite(COMPRESSOR_PIN, HIGH);
  digitalWrite(CLAPAN_PIN,HIGH);

  digitalWrite(COMPRESSOR_PIN_TWO, LOW);
  
  nextActionTime=RTC.now().unixtime()+timeFromState(COMPRESSOR_ONE);
  digitalWrite(LIGHT_PIN,HIGH);
  ledState=LED_OFF;
  
//        Clock.setYear(18);
//        Clock.setMonth(1);
//      Clock.setDate(22);
//      Clock.setHour(6);
//      Clock.setMinute(55);
//      Clock.setDoW(1);
//      Clock.setClockMode(false);
  printTime(RTC.now());
  Serial.println((long)timeFromState(COMPRESSOR_ONE));
  Serial.print("Current unix time: ");
  Serial.println(RTC.now().unixtime());
  Serial.print("Next action time - ");
  Serial.println(nextActionTime);
  Serial.println(stringFromState(state));
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
        digitalWrite(COMPRESSOR_PIN, HIGH);
      break;
    case SUBMERSION_ONE:
        digitalWrite(COMPRESSOR_PIN, LOW);
      break;
    case COMPRESSOR_TWO:
        digitalWrite(COMPRESSOR_PIN_TWO, HIGH);
      break;
    case SUBMERSION_TWO:
        digitalWrite(COMPRESSOR_PIN_TWO, LOW);
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
  float tmp = k;
  lum = analogRead(LIGHT_SENSOR_PIN);
  if (lum <= MIN_LUM)
  {
    k = 3;
  }
  else
  {
    k = 1;
  }
  int soilVal = analogRead(SOIL_PIN);
  Serial.println(soilVal);
//  k = k*((1168-0,8*soilVal)/640);
//  if (k != tmp)
//  {
//    Serial.print("k = ");
//    Serial.println(k);
//  }
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
