const int MIN_LUM = 1;
const int MAX_LUM = 3;
const int pinPhoto = A0;
const int pinLamp = 5;
int raw = 0;
int lamp = 0;

void setup() {
  Serial.begin(9600);
  pinMode( pinPhoto, INPUT );
	pinMode( pinLamp, INPUT );
}

void loop() {
  raw = analogRead( pinPhoto );
    if ( raw <= MIN_LUM )
    {
	// ???????? ?????
			digitalWrite(5, HIGH);
			delay(1000);
    }
		else if ( raw > MAX_LUM )
		{
	// ????????? ?????
			digitalWrite(5, LOW);
			deplay(1000);
		}
  Serial.println( raw );
  delay(200);
}