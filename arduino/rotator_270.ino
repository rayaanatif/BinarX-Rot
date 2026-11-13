#include <Servo.h>

Servo altServo; // Motor 1: Altitude (DS3225 270 degree)
Servo azServo;  // Motor 2: Azimuth (DS3225 270 degree, 2:1 geared)

// Pins
const int MOTOR2_PIN = 10; // Azimuth
const int MOTOR1_PIN = 9;  // Altitude

// DS3225 270 degree servo settings
const float SERVO_RANGE_DEG = 270.0; // Real travel of the servo
const int   PULSE_MIN_US    = 500;   // Pulse for 0 degrees (calibrate, section 6)
const int   PULSE_MAX_US    = 2500;  // Pulse for 270 degrees (calibrate, section 6)

// Mechanical limits
const float AZ_MAX_PHYS   = 180.0;   // 180 servo degrees = 360 antenna degrees (2:1 gear)
const float ALT_HOME_PHYS = 135.0;   // Servo angle for the horizon (calibrate)
const float ALT_MAX_REAL  = 70.0;    // Highest elevation the mechanism allows

// Last positions (used by the deadband check and for polling replies)
float lastAzPhys  = -100.0;
float lastAltPhys = -100.0;
float currentRealAz  = 180.0;
float currentRealAlt = 0.0;

int usFromDeg(float deg) {
  deg = constrain(deg, 0.0, SERVO_RANGE_DEG);
  return PULSE_MIN_US + (int)((PULSE_MAX_US - PULSE_MIN_US) * deg / SERVO_RANGE_DEG);
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(10);

  altServo.attach(MOTOR1_PIN);
  azServo.attach(MOTOR2_PIN);

  // Start at azimuth 180 (south), altitude 0 (horizon)
  moveToRealAngles(180.0, 0.0);
}

void loop() {
  if (Serial.available() > 0) {
    String inputString = Serial.readStringUntil('\n');
    inputString.trim();

    int azIndex = inputString.indexOf("AZ");
    int elIndex = inputString.indexOf("EL");

    if (azIndex != -1 && elIndex != -1) {
      if (elIndex - azIndex > 3) {
        // Movement command: "AZ<deg> EL<deg>"
        float targetRealAz  = inputString.substring(azIndex + 2, elIndex).toFloat();
        float targetRealAlt = inputString.substring(elIndex + 2).toFloat();
        moveToRealAngles(targetRealAz, targetRealAlt);
      } else {
        // Poll ("AZ EL"): report the current position
        Serial.print("AZ");
        Serial.print(currentRealAz);
        Serial.print(" EL");
        Serial.println(currentRealAlt);
      }
    }
  }
}

void moveToRealAngles(float targetAz, float targetAlt) {
  // Clamp to the real range the mechanism allows
  float realAz  = constrain(targetAz, 0.0, 360.0);
  float realAlt = constrain(targetAlt, 0.0, ALT_MAX_REAL);

  // Convert to physical servo angles
  float azPhys  = realAz / 2.0;            // 2:1 gearing; never above AZ_MAX_PHYS
  azPhys = constrain(azPhys, 0.0, AZ_MAX_PHYS);
  float altPhys = ALT_HOME_PHYS - realAlt; // horizon at ALT_HOME_PHYS, rising downward in servo angle

  // Deadband: ignore moves smaller than 2 servo degrees
  if (fabs(azPhys - lastAzPhys) <= 2.0 && fabs(altPhys - lastAltPhys) <= 2.0) {
    return;
  }

  lastAzPhys  = azPhys;
  lastAltPhys = altPhys;
  currentRealAz  = realAz;
  currentRealAlt = realAlt;

  azServo.writeMicroseconds(usFromDeg(azPhys));
  altServo.writeMicroseconds(usFromDeg(altPhys));
}
