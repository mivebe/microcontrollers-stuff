#include <Board_Identify.h>

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:

  Serial.print(F("Board Type: "));
  Serial.println(BoardIdentify::type);
  Serial.print(F("Board Make: "));
  Serial.println(BoardIdentify::make);
  Serial.print(F("Board Model: "));
  Serial.println(BoardIdentify::model);
  Serial.print(F("Board MCU: "));
  Serial.println(BoardIdentify::mcu);

  const boolean asd = BoardIdentify::model == "Arduino Nano";
  const boolean asd2 = BoardIdentify::model == "Arduino Uno";
  Serial.println(asd);
  Serial.println(asd2);

    digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(2000);

  // sensorValue = analogRead(thermistorPin);
  // temperature = (sensorValue - 300) * 0.116;

  // Serial.println(sensorValue);
  // Serial.println(temperature);

  // if (temperature < 24) {
  //   digitalWrite(indicatorPin, HIGH);
  // } else if (temperature > 27) {
  //   digitalWrite(LED_BUILTIN, HIGH);
  // }

  // delay(2000);

  // digitalWrite(indicatorPin, LOW);
  // digitalWrite(LED_BUILTIN, LOW);

  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(2000);
}
