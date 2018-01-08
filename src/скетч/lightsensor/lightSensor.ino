const int pinPhoto = A0;
int raw = 0;

void setup() {
  Serial.begin(9600);
  pinMode( pinPhoto, INPUT );
}

void loop() {
  raw = analogRead( pinPhoto );
  Serial.println( raw );
  delay(200);
}