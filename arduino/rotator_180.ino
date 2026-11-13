#include <Servo.h>

Servo altServo; // Motor 1: Altitude
Servo azServo;  // Motor 2: Azimuth (2:1 geared)

// Pin assignments
const int MOTOR2_PIN = 10; // Azimuth
const int MOTOR1_PIN = 9;  // Altitude

const int SERVO_MIN_US = 800;
const int SERVO_MAX_US = 2200;

// Current servo positions (used for the deadband check)
int currentServoAz = 90;
int currentServoAlt = 90;

// Current real-world positions (reported back when polled)
float currentRealAz = 180.0;
float currentRealAlt = 0.0;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(10); // Stops serial reads from slowing the loop

  altServo.attach(MOTOR1_PIN, SERVO_MIN_US, SERVO_MAX_US);
  azServo.attach(MOTOR2_PIN, SERVO_MIN_US, SERVO_MAX_US);

  // Starting position: azimuth 180 (south), altitude 0 (horizon)
  moveToRealAngles(180, 0);
}

void loop() {
  if (Serial.available() > 0) {
    String inputString = Serial.readStringUntil('\n');
    inputString.trim();

    int azIndex = inputString.indexOf("AZ");
    int elIndex = inputString.indexOf("EL");

    if (azIndex != -1 && elIndex != -1) {
      // A movement command contains numbers after AZ and EL
      if (elIndex - azIndex > 3) {
        float targetRealAz = inputString.substring(azIndex + 2, elIndex).toFloat();
        float targetRealAlt = inputString.substring(elIndex + 2).toFloat();
        moveToRealAngles((int)targetRealAz, (int)targetRealAlt);
      }
      // An empty poll ("AZ EL") gets the current position back
      else {
        Serial.print("AZ");
        Serial.print(currentRealAz);
        Serial.print(" EL");
        Serial.println(currentRealAlt);
      }
    }
  }
}

void moveToRealAngles(int targetAz, int targetAlt) {
  // Altitude: real 0 (horizon) = servo 90; real 70 (highest) = servo 20
  int constrainedAlt = constrain(targetAlt, 0, 70);
  int servoAlt = 90 - constrainedAlt;

  // Azimuth: 360 real degrees = 180 servo degrees (2:1 gearing)
  int servoAz = targetAz / 2;

  // Deadband: ignore moves smaller than 2 degrees to stop jitter
  if (abs(servoAz - currentServoAz) <= 2 && abs(servoAlt - currentServoAlt) <= 2) {
    return;
  }

  currentServoAz = constrain(servoAz, 0, 180);
  currentServoAlt = constrain(servoAlt, 20, 90);
  currentRealAz = targetAz;
  currentRealAlt = targetAlt;

  int targetAzPulse = map(currentServoAz, 0, 180, SERVO_MIN_US, SERVO_MAX_US);
  int targetAltPulse = map(currentServoAlt, 0, 180, SERVO_MIN_US, SERVO_MAX_US);

  azServo.writeMicroseconds(targetAzPulse);
  altServo.writeMicroseconds(targetAltPulse);
}
