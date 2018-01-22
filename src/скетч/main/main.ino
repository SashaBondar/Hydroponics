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

#define INTERVAL 1600
#define PERIOD		        60
#define COMPRESSOR_TIME   90
#define CLAPAN_TIME       180

#define COMPRESSOR_PIN    3
#define CLAPAN_PIN        4
#define WATER_LEVEL_PIN	  2
#define LIGHT_SENSOR_PIN  A0

#define MIN_LUM           1
#define MAX_LUM           3
#define SUBMERSION_TIME   120
#define DRAINING_TIME     180

enum STATE
{
  COMPRESSOR_ON=0,  // включен компрессор, вода накачивается в трубу  t=90
  SUBMERSION=1,     // все выключено вода в трубе                     t=60
  CLAPAN_ON=2,      // включен клапан, вода стекает в бак             t=180
  DRAINING=3        // все выключено вода в баке                      t=k*1600
};

RTClib RTC;
DS3231 Clock;

// коэффициент расчета временных интервалов затоплений
int k;
uint32_t compressorOnTime;
// время следующего затопления
uint32_t submersionOnTime;
// время следующего слива
uint32_t clapanOnTime;
uint32_t drainingOnTime;
// текущий процесс - затопление true, слив false
boolean isSubmersion;
// напряжение соответствующее освещенности на датчике света
int lum;
STATE state;
void setup () {
  k=1;
  lum = 0;
  isSubmersion = false; 
  Serial.begin(19200);
  Wire.begin();
  pinMode(COMPRESSOR_PIN,OUTPUT);
  pinMode(CLAPAN_PIN,OUTPUT);
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  state = COMPRESSOR_ON;
  digitalWrite(COMPRESSOR_PIN, HIGH);
  digitalWrite(CLAPAN_PIN,LOW);

//        Clock.setYear(18);
//        Clock.setMonth(1);
//  		Clock.setDate(22);
//  		Clock.setHour(6);
//  		Clock.setMinute(55);
//  		Clock.setDoW(1);
//  		Clock.setClockMode(false);
  compressorOnTime=RTC.now().unixtime();
  submersionOnTime=compressorOnTime + COMPRESSOR_TIME;
  clapanOnTime=submersionOnTime+PERIOD;
  drainingOnTime=clapanOnTime+CLAPAN_TIME;
 // pinMode(SIGNAL_PIN, OUTPUT);
  printTime(RTC.now());
  Serial.println("Compressor ON");
}

void loop () {
  DateTime now = RTC.now();
  //printTime(now);
  uint32_t currTime = now.unixtime();
  k = getK();
  switch(state)
  {
    case COMPRESSOR_ON:     
      // Проверить пора ли перейти в режим затопление
      if (currTime >= submersionOnTime)
      {
        printTime(now);
        Serial.println("Submersion started");
        // выключить компрессор и пересчитать время событий
        digitalWrite(COMPRESSOR_PIN, LOW);
        compressorOnTime = currTime+PERIOD+CLAPAN_TIME+k*INTERVAL;
        state = SUBMERSION;
      }
      break;
    case SUBMERSION:
      
      if (currTime >= clapanOnTime)
      {
        printTime(now);
        Serial.println("Clapan ON");
        // включить клапан, пересчитать время
        digitalWrite(CLAPAN_PIN, HIGH);
        submersionOnTime = currTime+COMPRESSOR_TIME+CLAPAN_TIME +k*INTERVAL;
        state = CLAPAN_ON;
      }
      break;
    case CLAPAN_ON:
      
      if (currTime >= drainingOnTime)
      {
        printTime(now);
        Serial.println("Draining on");
        // выключить клапан и пересчитать время
        digitalWrite(CLAPAN_PIN, LOW);
        clapanOnTime = currTime+PERIOD+COMPRESSOR_TIME+k*INTERVAL;
        state = DRAINING;
      }
      break;
    case DRAINING:
      
      if (currTime >= compressorOnTime)
      {
        printTime(now);
        Serial.println("Compressor ON");
        // включить компрессор и пересчитать время
        digitalWrite(COMPRESSOR_PIN, HIGH);
        drainingOnTime = currTime+PERIOD+COMPRESSOR_TIME+CLAPAN_TIME;
        state = COMPRESSOR_ON;
      }
      break;
  }
  delay(1000);
}

int getK()
{
  int ret = 1;
  lum = analogRead(LIGHT_SENSOR_PIN);
  if (lum <= MIN_LUM)
  {
    ret = 3;
  }
  return ret;
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
