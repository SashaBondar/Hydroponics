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

#define INTERVAL 	1600
#define PERIOD		60
#define SIGNAL_PIN	13

RTClib RTC;
DS3231 Clock;

// коэффициент расчета временных интервалов затоплений
int k;
// время следующего затопления
uint32_t nextTimeStartSubmersion;
// время следующего слива
uint32_t nextTimeDraining;

// текущий процесс - затопление true, слив false
boolean isSubmersion;

void setup () {
  k = 1;
  isSubmersion = false;
    Wire.begin();
  nextTimeStartSubmersion = RTC.now().unixtime()+10;
  nextTimeDraining = nextTimeStartSubmersion + PERIOD;
  
  Serial.begin(19200);
  Serial.println("======Setup started====");
  printTime(RTC.now());
        Serial.print(RTC.now().unixtime());
      Serial.print(" - "); 
      Serial.println(nextTimeStartSubmersion);
            Serial.println(nextTimeDraining);
//  Serial.flush();
  Wire.begin();
  //		Clock.setDate(11);
  //		Clock.setHour(7);
  //		Clock.setMinute(31);
  //		Clock.setDoW(4);
  //		Clock.setClockMode(false);
  pinMode(SIGNAL_PIN, OUTPUT);
}

void loop () {
//  Serial.println("=====Loop started====");
  DateTime now = RTC.now();
  uint32_t currTime = now.unixtime();
  if (isSubmersion) // в процессе затопления
  {
    if (currTime >= nextTimeDraining) // пора переключать
    {
//      printTime(now);
      Serial.print(currTime);
      Serial.print(" - "); 
      Serial.println(nextTimeDraining);
      Serial.println(" Состояние слив включено.");
      nextTimeStartSubmersion = nextTimeDraining + k * INTERVAL;
      isSubmersion = false;
      digitalWrite(SIGNAL_PIN, LOW);
    }
  }
  else
  {
    if (currTime >= nextTimeStartSubmersion)
    {
//      printTime(now);
      Serial.print(currTime);
      Serial.print(" - "); 
      Serial.println(nextTimeStartSubmersion);
      Serial.println("Состояние затопление включено");
      nextTimeDraining = nextTimeStartSubmersion + PERIOD;
      
      isSubmersion = true;
      digitalWrite(SIGNAL_PIN, HIGH);
    }
  }
  delay(10000);




//
//  Serial.print(" since midnight 1/1/1970 = ");
//  Serial.print(now.unixtime());
//  Serial.print("s = ");
//  Serial.print(now.unixtime() / 86400L);
//  Serial.println("d");
//  Serial.print("Temp=");
//  Serial.println(Clock.getTemperature());
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
}


