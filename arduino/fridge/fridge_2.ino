const int compressorPin = 9;
const int valvePin1 = 10;
const int fanPin = 12;

// ANALOG PINS
const int thermistorPin1 = A0;
const int thermistorPin2 = A5;

int thermistorVal1 = analogRead(thermistorPin1);
int thermistorVal2 = analogRead(thermistorPin2);
float temperature1;
float temperature2;
bool isCompressorRunning = false;
bool activeCircle1 = false;
bool activeCircle2 = false;
unsigned long timestop = 0;
unsigned long timeout = 0;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(thermistorPin1, INPUT);
  pinMode(thermistorPin2, INPUT);

  pinMode(valvePin1, OUTPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(compressorPin, OUTPUT);


  digitalWrite(compressorPin, HIGH);
  digitalWrite(valvePin1, HIGH);
  digitalWrite(fanPin, HIGH);
}

void tempstop() {
  digitalWrite(valvePin1, HIGH);
  digitalWrite(compressorPin, HIGH);
  timestop = millis();
  Serial.println("stop");
  activeCircle1 = false;
  activeCircle2 = false;


  //delay(60000);
}


void tempstart1() {
  timeout = millis();
  if (timeout - timestop >= 60000) {
    activeCircle1 = true;
    digitalWrite(valvePin1, LOW);
    Serial.println("start1");
    digitalWrite(compressorPin, LOW);
  }
}
void tempstart2() {
  timeout = millis();
  if (timeout - timestop >= 60000) {
    activeCircle2 = true;
    digitalWrite(valvePin1, HIGH);
    Serial.println("start2");
    digitalWrite(compressorPin, LOW);
  }
}




void loop() {
  delay(2000);
  thermistorVal1 = analogRead(thermistorPin1);
  thermistorVal2 = analogRead(thermistorPin2);

  Serial.println("");
  Serial.println("termistor 1");
  Serial.println(thermistorVal1);

  Serial.println("");
  Serial.println("termistor 2");
  Serial.println(thermistorVal2);

  if (thermistorVal1 < 470.0 && !activeCircle2) {
    tempstart1();
  } else if (thermistorVal1 >= 480.0 && activeCircle1) {
    tempstop();
  }
  if (thermistorVal2 < 780.0 && !activeCircle1) {
    tempstart2();
  } else if (thermistorVal2 >= 790.0 && activeCircle2) {
    tempstop();
  }
  if (activeCircle1) {
    digitalWrite(fanPin, LOW);
  } else {
    digitalWrite(fanPin, HIGH);
  }
}