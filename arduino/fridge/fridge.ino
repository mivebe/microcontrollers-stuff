// DIGITAL PINS
const int fillingPin = 2;
const int doorPin = 4;
const int zoomerPin = 5;
const int lampPin = 6;
const int noFrostPin = 8;
const int compressorPin = 9;
const int valvePin1 = 10;
const int valvePin2 = 12;

// ANALOG PINS
const int thermistorPin1 = A0;
const int thermistorPin2 = A5;

// INTERVALS
const long doorOpenPingInterval = 10000;  // 1 min
// const long doorOpenPingInterval = 60000;  // 1 min
const long noFrostInterval = 28800000;    // 8 h
const long noFrostDuration = 600000;      // 10 min
const long zoomInterval = 3000;           // 3 sec
const long zoomDuration = 1000;           // 1 sec

// STATES & TIMERS
unsigned long currentTime = 0;
unsigned long doorOpenTime = 0;
bool isDoorOpen = false;

bool isPingingActive = false;
bool isZooming = false;
unsigned long lastZoomTime = 0;

unsigned long lastNoFrostTime = 0;
unsigned long noFrostPeriod = 0;
bool isInNoFrost = false;
bool justSwitched = false;

bool isCompressorRunning = 0;
bool activeCircle = false;

int asd;
int asd2;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(doorPin, INPUT);
  pinMode(fillingPin, INPUT);
  pinMode(thermistorPin1, INPUT);
  pinMode(thermistorPin2, INPUT);

  pinMode(lampPin, OUTPUT);
  pinMode(zoomerPin, OUTPUT);
  pinMode(noFrostPin, OUTPUT);
  pinMode(compressorPin, OUTPUT);
  pinMode(valvePin1, OUTPUT);
  pinMode(valvePin2, OUTPUT);

  digitalWrite(lampPin, LOW);
  digitalWrite(zoomerPin, LOW);
  digitalWrite(noFrostPin, LOW);
  digitalWrite(compressorPin, HIGH);
  digitalWrite(valvePin1, HIGH);
  digitalWrite(valvePin2, HIGH);
}

void handlePing() {

  bool isTimeToPing = (currentTime - lastZoomTime) > zoomInterval;
  bool isTimeToStop = (currentTime - lastZoomTime) > (zoomInterval + zoomDuration);

  if (isTimeToPing && !isZooming && isPingingActive) {
    digitalWrite(zoomerPin, HIGH);
  }

  if (isTimeToStop && isZooming) {
    digitalWrite(zoomerPin, LOW);
  }
}

void togglePing(bool toggle) {

  isPingingActive == toggle ?: isPingingActive = toggle;
}

void handleDoor() {
  // is door opened on this loop;
  bool doorPinVal = digitalRead(doorPin);
  // Serial.println("doorPinVal");
  // Serial.println(doorPinVal);

  // ON DOOR CLOSE
  if (isDoorOpen && !doorPinVal) {
    isDoorOpen = false;
    doorOpenTime = 0;
    digitalWrite(lampPin, LOW);
    togglePing(false);
    Serial.println("door   CLOSE");
  }

  // ON DOOR OPEN
  if (!isDoorOpen && doorPinVal) {
    isDoorOpen = true;
    doorOpenTime = currentTime;
    digitalWrite(lampPin, HIGH);
    Serial.println("door  OPEN");
  }

  // IF DOOR IS OPEN
  if (doorOpenTime) {
    // doorOpenTime = currentTime;
    Serial.println("door opem time");
    Serial.println(doorOpenTime);

    if ((currentTime - doorOpenTime) >= doorOpenPingInterval) {
      togglePing(true);
    }
  }
}

void handleNoFrost() {

  bool isTimeToDefrost = (currentTime - lastNoFrostTime) > noFrostInterval;
  bool isDefrostComplete = (currentTime - noFrostPeriod) > noFrostDuration;

  if (justSwitched) justSwitched = false;

  // ON NO FROST END
  if (isDefrostComplete && isInNoFrost) {
    isInNoFrost = false;
    justSwitched = true;
    lastNoFrostTime = currentTime;
    noFrostPeriod = 0;
    digitalWrite(noFrostPin, LOW);
    Serial.println("no frost   OFF");
  }

  // ON NO FROST START
  if (isTimeToDefrost && !isInNoFrost && !justSwitched) {
    isInNoFrost = true;
    noFrostPeriod = currentTime;
    digitalWrite(noFrostPin, HIGH);
    Serial.println("no frost   ON");
  }
}

void handleTempChange() {

  int thermistorVal1 = analogRead(thermistorPin1);
  int thermistorVal2 = analogRead(thermistorPin2);
  float temperature1;
  float temperature2;

  Serial.println("");
  Serial.println("termistor 1");
  Serial.println(thermistorVal1);

  Serial.println("");
  Serial.println("termistor 2");
  Serial.println(thermistorVal2);

  // CIRCLE 1 START
  if (thermistorVal1 > 512.0 && !isCompressorRunning) {
    activeCircle = false;
    digitalWrite(valvePin1, LOW);

    isCompressorRunning = true;
    digitalWrite(compressorPin, LOW);
    Serial.println("circle 1 START");
    asd = digitalRead(valvePin1);
    asd2 = digitalRead(compressorPin);
    Serial.print("pin ");
    Serial.print(valvePin1);
    Serial.print(" ");
    Serial.print(asd);

    Serial.println("");

    Serial.print("pin ");
    Serial.print(compressorPin);
    Serial.print(" ");
    Serial.print(asd2);
  }

  // CIRCLE 2 START
  if (thermistorVal2 > 512.0 && !isCompressorRunning) {
    activeCircle = true;
    digitalWrite(valvePin2, LOW);

    isCompressorRunning = true;
    digitalWrite(compressorPin, LOW);
    Serial.println("circle 2 START");
    asd = digitalRead(valvePin2);
    asd2 = digitalRead(compressorPin);
    Serial.print("pin ");
    Serial.print(valvePin2);
    Serial.print(" ");
    Serial.print(asd);

    Serial.println("");

    Serial.print("pin ");
    Serial.print(compressorPin);
    Serial.print(" ");
    Serial.print(asd2);
  }

  // CIRCLE 1 STOP
  if (thermistorVal1 < 512.0 && isCompressorRunning && !activeCircle) {

    isCompressorRunning = false;
    digitalWrite(valvePin1, HIGH);
    digitalWrite(compressorPin, HIGH);
    Serial.println("circle 1 STOP");
    asd = digitalRead(valvePin1);
    asd2 = digitalRead(compressorPin);
    Serial.print("pin ");
    Serial.print(valvePin1);
    Serial.print(" ");
    Serial.print(asd);

    Serial.println("");

    Serial.print("pin ");
    Serial.print(compressorPin);
    Serial.print(" ");
    Serial.print(asd2);
  }

  // CIRCLE 2 STOP
  if (thermistorVal2 < 512.0 && isCompressorRunning && activeCircle) {

    isCompressorRunning = false;
    digitalWrite(valvePin2, HIGH);
    digitalWrite(compressorPin, HIGH);
    Serial.println("circle 2 STOP");
    asd = digitalRead(valvePin2);
    asd2 = digitalRead(compressorPin);
    Serial.print("pin ");
    Serial.print(valvePin2);
    Serial.print(" ");
    Serial.print(asd);

    Serial.println("");

    Serial.print("pin ");
    Serial.print(compressorPin);
    Serial.print(" ");
    Serial.print(asd2);
  }
}

void fillingProgram() {

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

void loop() {
  // put your main code here, to run repeatedly:
  currentTime = millis();

  bool isFillingMode = digitalRead(fillingPin);

  if (isFillingMode) {
    fillingProgram();
  } else {
    handleDoor();
    handleNoFrost();
    handleTempChange();
    handlePing();

    delay(1000);
    Serial.println("");
  }
}
