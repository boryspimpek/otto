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
bool wasL1Pressed = false;
bool L1StateActive = false;
bool manualOverride = false;

const int JOYSTICK_DEADZONE = 10;
const int MAX_SERVO_SPEED = 180;

int LNP = 65;     // Left leg neutral position
int RNP = 120;    // Right leg neutral position

void onConnect() {
  Serial.println("Connected!");
}

void onDisConnect() {
  Serial.println("Disconnected!");
}

int mapJoystickToSpeed(int value) {
  if (abs(value) < JOYSTICK_DEADZONE) {
    return 90;  // Center position - stop for 360° servos
  }
  
  int mappedSpeed = map(value, -128, 127, 0, 180);  // Maps to full speed range for 360° servos
  return mappedSpeed;
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

void returnToNeutral() {
  int currentLeftLeg = servoLeftLeg.read();
  int currentRightLeg = servoRightLeg.read();
  
  moveServosSmooth(servoLeftLeg, currentLeftLeg, LNP, servoRightLeg, currentRightLeg, RNP, 20, 15);
  
  servoLeftFoot.write(90);
  servoRightFoot.write(90);
}

void setup() {
  servoLeftFoot.attach(servoLeftFootPin, 544, 2400);
  servoRightFoot.attach(servoRightFootPin, 544, 2400);
  
  servoLeftLeg.attach(servoLeftLegPin, 544, 2400);
  servoRightLeg.attach(servoRightLegPin, 544, 2400);

  servoLeftFoot.write(90);    // Stop - no rotation
  servoRightFoot.write(90);   // Stop - no rotation
  
  servoLeftLeg.write(60);     // Position in degrees
  servoRightLeg.write(120);   // Position in degrees

  Serial.begin(115200);
  PS4.attachOnConnect(onConnect);
  PS4.attachOnDisconnect(onDisConnect);
  PS4.begin();
  Serial.println("Ready.");

  delay(300);
}

void loop() {
  if (PS4.isConnected()) {

    // Sprawdzenie czy Square lub Circle są aktywne
    if (!manualOverride) {
      int leftFootSpeed = mapJoystickToSpeed(PS4.LStickY());
      int rightFootSpeed = mapJoystickToSpeed(PS4.RStickY());

      servoLeftFoot.write(leftFootSpeed);
      servoRightFoot.write(180 - rightFootSpeed);
    }

    // L1 button handling
    if (PS4.L1()) {
      if (!wasL1Pressed) {
        wasL1Pressed = true;
        if (!L1StateActive) {
          moveServosSmooth(servoLeftLeg, servoLeftLeg.read(), 180, servoRightLeg, servoRightLeg.read(), 0, 20, 15);
          L1StateActive = true;
        } else {
          returnToNeutral();
          L1StateActive = false;
        }
      }
    } else {
      wasL1Pressed = false;
    }

    // Square button handling
    if (PS4.Square()) {
      if (!wasSquarePressed) {
        manualOverride = true;  // Blokowanie joysticka
        moveServosSmooth(servoLeftLeg, servoLeftLeg.read(), 100, servoRightLeg, servoRightLeg.read(), 175, 20, 15);
        delay(150);
        servoLeftFoot.write(90 + 25); // Obrót w prawo, ustawienie prędkości serwa 360
        wasSquarePressed = true;
      }
    } else if (wasSquarePressed) {
      servoLeftFoot.write(90); // Natychmiastowe zatrzymanie obrotu
      delay(150);
      returnToNeutral();
      manualOverride = false; // Odblokowanie joysticka
      wasSquarePressed = false;
    }

    // Circle button handling
    if (PS4.Circle()) {
      if (!wasCirclePressed) {
        manualOverride = true;  // Blokowanie joysticka
        moveServosSmooth(servoRightLeg, servoRightLeg.read(), 80, servoLeftLeg, servoLeftLeg.read(), 5, 20, 15);
        delay(150);
        servoRightFoot.write(90 - 20); // Obrót w lewo, ustawienie prędkości serwa 360
        wasCirclePressed = true;
      }
    } else if (wasCirclePressed) {
      servoRightFoot.write(90); // Natychmiastowe zatrzymanie obrotu
      delay(150);
      returnToNeutral();
      manualOverride = false; // Odblokowanie joysticka
      wasCirclePressed = false;
    }
  }
}
