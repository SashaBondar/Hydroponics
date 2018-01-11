/**
* Интервалы:
*  - время пауз между затоплентями k*30 мин = k*1600 сек
*  - время затопления 1 мин = 60 сек
* 
* k коэффициент, зависит от: 
* - времени суток (1 днем, 3 ночью)
* - влажности воздуха и почвы ( значения определить )
*/
#include <Wire.h>
#include "DS3231.h"

#define INTERVAL 	1600
#define PERIOD		60
#define SIGNAL_PIN	13

RTClib RTC;
DS3231 Clock;

// коэффициент расчета временных интервалов затоплений
int k = 1;
// время следующего затопления
int nextTimeStartSubmersion = RTC.now().unixtime();
// время следующего слива
int nextTimeDraining = nextTimeStartSubmersion + PERIOD;

// текущий процесс - затопление true, слив false
boolean isSubmersion = true;

void setup () {
    Serial.begin(9600);
    Wire.begin();
//		Clock.setDate(11);
//		Clock.setHour(7);
//		Clock.setMinute(31);
//		Clock.setDoW(4);
//		Clock.setClockMode(false);
		pinMode(SIGNAL_PIN, OUTPUT);
}

void loop () {
  	
		DateTime now = RTC.now();
		int currTime = now.unixtime();
		if(isSubmersion) // в процессе затопления
		{
				if(currTime>=nextTimeDraining) // пора переключать
				{
						nextTimeDraining = nextTimeStartSubmersion+PERIOD;
            isSubmersion = false;
						digitalWrite(SIGNAL_PIN, LOW);
				}
		}
		else
		{
				if(currTime>=nextTimeStartSubmersion)
				{
						nextTimeStartSubmersion = nextTimeDraining+k*INTERVAL;
            isSubmersion = true;
						digitalWrite(SIGNAL_PIN, HIGH);
				}
		}
    delay(1000);
  
    
    
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
    
    Serial.print(" since midnight 1/1/1970 = ");
    Serial.print(now.unixtime());
    Serial.print("s = ");
    Serial.print(now.unixtime() / 86400L);
    Serial.println("d");
		//Serial.println("Temp="+Clock.getTemperature());
}
