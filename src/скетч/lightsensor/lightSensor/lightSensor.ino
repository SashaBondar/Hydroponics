

#define MIN_LUM 1
#define MAX_LUM 3
#define pinPhoto 1
#define pinLamp 13

int raw;


void setup() {
	raw=0;
  Serial.begin(9600);
  pinMode( pinPhoto, INPUT );
	pinMode( pinLamp, OUTPUT );
}

void loop() {
  raw = analogRead( pinPhoto );
    if ( raw < 300 )
    {
	// включаем ломпу
			digitalWrite(pinLamp, HIGH);
			delay(1000);
    }
		else 
		{
	// лампа выключается
			digitalWrite(pinLamp, LOW);
			delay(1000);
		}
  Serial.println( raw );
  delay(2000);
}
