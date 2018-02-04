/**
  Интервалы:
   - время пауз между затоплентями k*30 мин = k*1600 сек
   - время затопления 1 мин = 60 сек

  k коэффициент, зависит от:
  - времени суток (1 днем, 3 ночью)
  - влажности воздуха и почвы ( значения определить )
*/
#include <Wire.h>
#include <DS3231.h>

#define INTERVAL 3600
#define PERIOD            600
#define COMPRESSOR_TIME   60
#define CLAPAN_TIME       600

#define COMPRESSOR_PIN    3
#define CLAPAN_PIN        4
#define WATER_LEVEL_PIN   2
#define LIGHT_PIN         6
#define LIGHT_SENSOR_PIN  A0
#define SOIL_PIN          A1

#define MIN_LUM           180
#define MAX_LUM           300

enum STATE
{
  COMPRESSOR_ON=0,  // включен компрессор, вода накачивается в трубу  t=60
  SUBMERSION=1,     // все выключено вода в трубе                     t=60
  CLAPAN_ON=2,      // включен клапан, вода стекает в бак             t=180
  DRAINING=3        // все выключено вода в баке                      t=k*1600
};

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
  pinMode(LIGHT_PIN,OUTPUT);
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  
  state = COMPRESSOR_ON;
  digitalWrite(COMPRESSOR_PIN, HIGH);
  digitalWrite(CLAPAN_PIN,LOW);
  digitalWrite(LIGHT_PIN,HIGH);
  ledState=LED_OFF;
  
//        Clock.setYear(18);
//        Clock.setMonth(1);
//      Clock.setDate(22);
//      Clock.setHour(6);
//      Clock.setMinute(55);
//      Clock.setDoW(1);
//      Clock.setClockMode(false);
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
  k = k*((1168-0,8*soilVal)/640);
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
