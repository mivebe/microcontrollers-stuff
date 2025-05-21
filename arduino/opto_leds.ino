
const int sensorPin = A0;
const int optoPin = 2;

int sensorValue;
float temperature;
bool ledState = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(optoPin, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:

  sensorValue = analogRead(sensorPin);
  temperature = ((sensorValue * 5 / 1023.0) - 0.5) * 100;

  // if (temperature >= 25 && ledState == 0) {
  digitalWrite(optoPin, HIGH);
  ledState = 1;
  delay(2000);
  // } else if (temperature < 25 && ledState == 1) {
  digitalWrite(optoPin, LOW);
  ledState = 0;
  // }
}
