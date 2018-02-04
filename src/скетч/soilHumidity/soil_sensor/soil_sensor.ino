// контакт подключения аналогового выхода датчика
int aPin=A2;
// контакты подключения светодиодов индикации
int ledPins=13;
// переменная для сохранения значения датчика
int avalue=0;
// значение полного полива
int minvalue=220;
// значение критической сухости
int maxvalue=600;

void setup(){
// инициализация последовательного порта
Serial.begin(9600);

pinMode(ledPins,OUTPUT);
}

void loop(){
// получение значения с аналогового вывода датчика
avalue=analogRead(aPin);
// вывод значения в монитор последовательного порта Arduino
Serial.print("avalue=");
Serial.println(avalue);
// масштабируем значение 
// индикация уровня влажности
if(avalue<=maxvalue){
digitalWrite(ledPins,HIGH); //зажигаем светодиод
}else{
digitalWrite(ledPins,LOW); // гасим светодиод
}
// пауза перед следующим получением значения 1000 мс
delay(1000);
}
