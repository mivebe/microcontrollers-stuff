const int compressorPin = 9;
const int valvePin1 = 10;
const int valvePin2 = 12;

const int thermistorPin1 = A0;
const int thermistorPin2 = A5;

void setup() {
  // put your setup code here, to run once:

  pinMode(thermistorPin1, INPUT);
  pinMode(thermistorPin2, INPUT);

  pinMode(compressorPin, OUTPUT);
  pinMode(valvePin1, OUTPUT);
  pinMode(valvePin2, OUTPUT);

  digitalWrite(compressorPin, LOW);
  digitalWrite(valvePin1, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:

  // fill circle 1 for 10 mins
  digitalWrite(valvePin1, HIGH);
  digitalWrite(compressorPin, HIGH);
  delay(600000);
  digitalWrite(compressorPin, LOW);

  // wait 5 min
  delay(300000);

  // fill circle 2 for 10 mins
  digitalWrite(valvePin2, HIGH);
  digitalWrite(compressorPin, HIGH);
  delay(600000);
  digitalWrite(compressorPin, LOW);

  // wait 5 min
  delay(300000);
}
