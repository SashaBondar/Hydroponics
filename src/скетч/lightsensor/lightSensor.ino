#define MIN_LUM 1
#define MAX_LUM 3
#define pinPhoto A0
#define pinLamp 13

int raw;


void setup() {
	raw=0;
  Serial.begin(9600);
  pinMode( pinPhoto, INPUT );
	pinMode( pinLamp, INPUT );
}

void loop() {
  raw = analogRead( pinPhoto );
    if ( raw <= MIN_LUM )
    {
	// ???????? ?????
			digitalWrite(pinLamp, HIGH);
			delay(1000);
    }
		else if ( raw > MAX_LUM )
		{
	// ????????? ?????
			digitalWrite(pinLamp, LOW);
			deplay(1000);
		}
  Serial.println( raw );
  delay(200);
}
