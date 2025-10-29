
int mode = 0;
int pinBase = 9;

void setup() {

  Serial.begin(9600);

  for (int i = 0; i < 3; i++) {
    pinMode(pinBase + i, OUTPUT);
  }

}

void writeAllLED(int base, int count, int lux) {

  for (int i = 0; i < count; i++) {
    analogWrite(base + i, lux);
  }

  return;
}

void mode1(int lux) {

  int H = 255;
  int M = 120;
  int L = 30;

  if (lux < 100) {
    writeAllLED(pinBase, 3, H);
  } else if (lux < 250) {
    writeAllLED(pinBase, 3, M);
  } else if (lux < 500) {
    writeAllLED(pinBase, 3, L);
  }

  return;
}

void mode2(int lux) {

  int brightness = map(lux, 255, 0, 0, 255);

  writeAllLED(pinBase, 3, brightness);
  Serial.println(brightness);

  return;
}

void mode3() {
  int luxPins[3] = {A0, A1, A2};
  int ledPins[3] = {pinBase, pinBase + 1, pinBase + 2};

  int activeCount = 0;
  int lux[3];
  bool ledOn[3];

  for (int i = 0; i < 3; i++) {
    lux[i] = analogRead(luxPins[i]);
    if (lux[i] < 50) {
      ledOn[i] = true;
      activeCount++;
    } else {
      ledOn[i] = false;
    }
  }

  int brightness = 0;
  if (activeCount == 1) brightness = 255;
  else if (activeCount == 2) brightness = 128;
  else if (activeCount == 3) brightness = 85;

  for (int i = 0; i < 3; i++) {
    if (ledOn[i]) {
      analogWrite(ledPins[i], brightness);
    } else {
      analogWrite(ledPins[i], 0);
    }
  }

  Serial.print("Mode3 Active Count : ");
  Serial.println(activeCount);

  return;
}

void loop() {

  Serial.println("=============================");
  Serial.println("Please Enter Mode : 1 / 2 / 3");

  int tmp = Serial.read() - '0';

  if (tmp >= 1 && tmp <= 3) mode = tmp;

  Serial.print("Mode : ");
  Serial.println(mode);

  int lux = analogRead(A0);
  Serial.print("Light : ");
  Serial.println(lux);

  switch (mode) {
    case 1 : mode1(lux); break;
    case 2 : mode2(lux); break;
    case 3 : mode3(); break;
  }

  delay(300);

}