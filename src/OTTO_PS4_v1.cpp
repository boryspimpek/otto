#include <ESP32Servo.h>
#include <PS4Controller.h>

static const int servoLeftFootPin = 13;   // 360° continuous rotation servo
static const int servoLeftLegPin = 14;    // Standard 180° servo
static const int servoRightFootPin = 12;  // 360° continuous rotation servo 
static const int servoRightLegPin = 27;   // Standard 180° servo

Servo servoLeftFoot;
Servo servoLeftLeg;
Servo servoRightFoot;
Servo servoRightLeg;

bool wasSquarePressed = false;
bool wasCirclePressed = false;
bool wasTrianglePressed = false;
bool wasCrossPressed = false;
bool wasL1Pressed = false;
bool wasR1Pressed = false;
bool SquareStateActive = false;
bool manualOverride = false;

const int JOYSTICK_DEADZONE = 10;
const int MAX_SERVO_SPEED = 180;

int LNP = 65;     // Left leg neutral position
int RNP = 117;    // Right leg neutral position

void onConnect() {
  Serial.println("Connected!");
}

void onDisConnect() {
  Serial.println("Disconnected!");
}

void moveServosSmooth(Servo &servo1, int start1, int end1, Servo &servo2, int start2, int end2, int steps, int delayTime) {
  int diff1 = end1 - start1;
  int diff2 = end2 - start2;
  for (int i = 0; i <= steps; i++) {
    int pos1 = start1 + (diff1 * i / steps);
    int pos2 = start2 + (diff2 * i / steps);
    servo1.write(pos1);
    servo2.write(pos2);
    delay(delayTime);
  }
}

void moveServoSmooth(Servo &servo1, int start1, int end1, int steps, int delayTime) {
  int diff = end1 - start1;

  for (int i = 0; i <= steps; i++) {
    int pos = start1 + (diff * i / steps);
    servo1.write(pos);
    delay(delayTime);
  }
}

void returnToNeutral() {
  int currentLeftLeg = servoLeftLeg.read();
  int currentRightLeg = servoRightLeg.read();
  
  moveServosSmooth(servoLeftLeg, currentLeftLeg, LNP, servoRightLeg, currentRightLeg, RNP, 20, 15);
  
  servoLeftFoot.write(90);
  servoRightFoot.write(90);
}

int mapJoystickToSpeed(int value) {
  if (abs(value) < JOYSTICK_DEADZONE) {
    return 90;  // Center position - stop for 360° servos
  }
  
  int mappedSpeed = map(value, -128, 127, 0, 180);  // Maps to full speed range for 360° servos
  return mappedSpeed;
  }

void rightLegSwing() {
  moveServosSmooth(servoLeftLeg, servoLeftLeg.read(), 100, servoRightLeg, servoRightLeg.read(), 175, 20, 15); // Pozycja lewa
  delay(150);
  servoLeftFoot.write(90+20);  // Obrót lewej stopy serwo 360

  moveServoSmooth(servoRightLeg, 175, 60, 20, 10);
  delay(100);

  for (int i = 0; i < 4; i++) {  
    moveServoSmooth(servoRightLeg, 60, 120, 20, 10);  
    delay(100);
    moveServoSmooth(servoRightLeg, 120, 60, 20, 10);  
    delay(100);
  }

  moveServoSmooth(servoRightLeg, 60, 175, 20, 10);  
  delay(100);
}

void moonWalk() {
  for (int i = 0; i < 2; i++) { 
    moveServosSmooth(servoLeftLeg, servoLeftLeg.read(), 25, servoRightLeg, servoRightLeg.read(), 93, 30, 20);
    moveServoSmooth(servoLeftLeg, servoLeftLeg.read(), 100, 30, 20);
    moveServoSmooth(servoLeftLeg, servoLeftLeg.read(), 25, 30, 20);
    moveServosSmooth(servoLeftLeg, servoLeftLeg.read(), 95, servoRightLeg, servoRightLeg.read(), 162, 30, 20);
    moveServoSmooth(servoRightLeg, servoRightLeg.read(), 80, 30, 20);
    moveServoSmooth(servoRightLeg, servoRightLeg.read(), 162, 30, 20);
  }
}

void setup() {
  servoLeftFoot.attach(servoLeftFootPin, 544, 2400);
  servoRightFoot.attach(servoRightFootPin, 544, 2400);
  
  servoLeftLeg.attach(servoLeftLegPin, 544, 2400);
  servoRightLeg.attach(servoRightLegPin, 544, 2400);

  servoLeftFoot.write(90);    // Stop - no rotation
  servoRightFoot.write(90);   // Stop - no rotation
  
  servoLeftLeg.write(LNP);     // Position in degrees
  servoRightLeg.write(RNP);   // Position in degrees

  Serial.begin(115200);
  PS4.attachOnConnect(onConnect);
  PS4.attachOnDisconnect(onDisConnect);
  PS4.begin();
  Serial.println("Ready.");

  delay(300);
}

void loop() {
  if (PS4.isConnected()) {
    // Sterowanie serwami 360 za pomocą joysticków (gdy nie ma manualOverride)
    if (!manualOverride) {
      int leftFootSpeed = mapJoystickToSpeed(PS4.LStickY());
      int rightFootSpeed = mapJoystickToSpeed(PS4.RStickY());

      servoLeftFoot.write(leftFootSpeed);
      servoRightFoot.write(180 - rightFootSpeed);
    }

    // Square button handling
    if (PS4.Square()) {
      if (!wasSquarePressed) {
        wasSquarePressed = true;
        if (!SquareStateActive) {
          moveServosSmooth(servoLeftLeg, servoLeftLeg.read(), 170, servoRightLeg, servoRightLeg.read(), 10, 20, 15);
          SquareStateActive = true;
        } else {
          returnToNeutral();
          SquareStateActive = false;
        }
      }
    } else {
      wasSquarePressed = false;
    }

    // L1 button handling
    if (PS4.L1()) {
      if (!wasL1Pressed) {
        manualOverride = true;  // Blokowanie joysticka
        moveServosSmooth(servoLeftLeg, servoLeftLeg.read(), 100, servoRightLeg, servoRightLeg.read(), 175, 20, 15); // Pozycja idąca lewa
        delay(150);
        servoLeftFoot.write(90 + 25); // Obrót w prawo, ustawienie prędkości serwa 360
        wasL1Pressed = true;
      }
    } else if (wasL1Pressed) {
      servoLeftFoot.write(90); // Natychmiastowe zatrzymanie obrotu
      delay(150);
      returnToNeutral();
      manualOverride = false; // Odblokowanie joysticka
      wasL1Pressed = false;
    }

    // R1 button handling
    if (PS4.R1()) {
      if (!wasR1Pressed) {
        manualOverride = true;  // Blokowanie joysticka
        moveServosSmooth(servoRightLeg, servoRightLeg.read(), 80, servoLeftLeg, servoLeftLeg.read(), 5, 20, 15);  // Pozycja idąca prawo
        delay(150);
        servoRightFoot.write(90 - 20); // Obrót w lewo, ustawienie prędkości serwa 360
        wasR1Pressed = true;
      }
    } else if (wasR1Pressed) {
      servoRightFoot.write(90); // Natychmiastowe zatrzymanie obrotu
      delay(150);
      returnToNeutral();
      manualOverride = false; // Odblokowanie joysticka
      wasR1Pressed = false;
    }

    // Triangle button handling
    if (PS4.Triangle()) {
      if (!wasTrianglePressed) {
        manualOverride = true;  // Blokowanie joysticka
        rightLegSwing();
        wasTrianglePressed = true;
      }
    } else if (wasTrianglePressed) {
      returnToNeutral();  // Powrót do neutralnej pozycji po zakończeniu ruchów
      manualOverride = false; // Odblokowanie joysticka
      wasTrianglePressed = false;
    }

    // Cross butto handling
    if (PS4.Cross()) {
      if (!wasCrossPressed) {
        manualOverride = true;
        moonWalk();
        wasCrossPressed = true;
      }
    } else if (wasCrossPressed) {
      returnToNeutral();  // Powrót do neutralnej pozycji po zakończeniu ruchów
      manualOverride = false; // Odblokowanie joysticka
      wasCrossPressed = false;
    }

  }
}
