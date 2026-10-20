/*
  Automatic elevation (site) and azimuth positioning of a plane
  Board   : ESP32 (ESP32-WROOM)
  Sensor  : MPU6050 (accelerometer + gyroscope, I2C)
  Outputs : SG90 servo (site), 28BYJ-48 + ULN2003 stepper (azimuth),
            20x4 I2C LCD, 3 status LEDs, Bluetooth (BluetoothSerial)
  Target angles: two potentiometers, or Bluetooth if both are at 0.

  End-of-study project - EAMAC, EAC-SEI 2023-A (July 2025)
  Authors: RABARY Haja Tahina, AVOCEGAMOU Lionel Noukpo
*/

#include <ESP32Servo.h>
#include <MPU6050.h>
#include "BluetoothSerial.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Stepper.h>

// === Constants ===
#define SERVO_PIN 13
#define PIN_POT 34            // potentiometer: target site angle
#define PIN_POT1 35           // potentiometer: target azimuth angle
#define PIN_LEDR 4            // red LED
#define PIN_LEDO 32           // orange LED
#define PIN_LEDV 14           // green LED
#define LCD_ADDR 0x27
#define MAX_SITE_ANGLE 75
#define MAX_AZIMUT_ANGLE 60
#define ANGLE_MARGIN_SITE 1
#define ANGLE_MARGIN_AZIMUT 2
#define MAX_STEPPER_STEP 2048
#define MIN_STEPPER_STEP 28
#define RPM 10

// === Global objects ===
LiquidCrystal_I2C lcd(LCD_ADDR, 20, 4);
Stepper myStepper(MAX_STEPPER_STEP, 18, 23, 19, 5);
BluetoothSerial SerialBT;
Servo myServo;
MPU6050 mpu;

// === Sensor variables ===
float currentAngle = 0, commande = 0, angleZ = 0;
float axg, ayg, azg, gyg, gzg, accAngle, dt;
const float PAS_CORRECTION = 5;
int16_t ax, ay, az, gx, gy, gz;
unsigned long lastTime = 0;
const float alpha = 0.98;     // complementary filter coefficient

// === Initial setup ===
void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32-calage");

  pinMode(PIN_LEDR, OUTPUT);
  pinMode(PIN_LEDO, OUTPUT);
  pinMode(PIN_LEDV, OUTPUT);

  myServo.attach(SERVO_PIN);
  myStepper.setSpeed(RPM);
  myServo.write(commande);

  Wire.begin();
  lcd.init();
  lcd.backlight();

  // MPU6050 initialisation
  mpu.initialize();
  if (mpu.testConnection()) {
    lcd.setCursor(0, 0);
    lcd.print("MPU6050 OK");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Erreur MPU");
    while (1);
  }
  delay(500);

  lcd.setCursor(0, 1);
  lcd.print("CALIBRAGE GYROSCOPE.....");
  mpu.CalibrateGyro();
  delay(1000);

  // Welcome message
  lcd.clear();
  lcd.setCursor(0, 2);
  lcd.print("     BIENVENUE");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" CALAGE AUTOMATIQUE");
  lcd.setCursor(0, 1);
  lcd.print(" ANGLE D'INCLINAISON");
  lcd.setCursor(0, 2);
  lcd.print("       PROJET ");
  lcd.setCursor(0, 3);
  lcd.print("    FIN D'ETUDE");
  delay(3000);

  lastTime = millis();
}

// === Read an angle sent over Bluetooth ===
float lireBluetoothAngle(String message, int minVal, int maxVal) {
  float angle = -1;
  while (angle < minVal || angle > maxVal) {
    SerialBT.println(message);
    while (!SerialBT.available()) {
      delay(10);
    }
    String input = SerialBT.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) continue;
    angle = input.toFloat();
    if (angle < minVal || angle > maxVal) {
      SerialBT.println("VALEUR INVALIDE. REESSAYEZ.");
    }
  }
  return angle;
}

// === Read tilt angle (site) : complementary filter ===
float lire_site() {
  for (int i = 0; i < 100; i++) {
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    axg = ax / 16384.0;
    ayg = ay / 16384.0;
    azg = az / 16384.0;
    gyg = gy / 131.0;
    accAngle = atan2(axg, sqrt(ayg * ayg + azg * azg)) * 180.0 / PI;

    unsigned long currentTime = millis();
    dt = (currentTime - lastTime) / 1000.0;
    lastTime = currentTime;

    if (dt > 0)
      currentAngle = alpha * (currentAngle + gyg * dt) + (1 - alpha) * accAngle;

    currentAngle = constrain(currentAngle, 0, 180);
  }
  return currentAngle;
}

// === Read azimuth angle (Z axis, gyroscope integration) ===
float lire_azimut() {
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  gzg = gz / 131.0;
  unsigned long currentTime = millis();
  dt = (currentTime - lastTime) / 1000.0;
  lastTime = currentTime;
  angleZ += gzg * dt;
  angleZ = constrain(angleZ, 0, MAX_AZIMUT_ANGLE);
  return angleZ;
}

// === 4-line LCD display ===
void afficherLCD(String ligne1, String ligne2 = "", String ligne3 = "", String ligne4 = "") {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(ligne1);
  lcd.setCursor(0, 1); lcd.print(ligne2);
  lcd.setCursor(0, 2); lcd.print(ligne3);
  lcd.setCursor(0, 3); lcd.print(ligne4);
  delay(2000);
}

// === Main loop ===
void loop() {
  int potSite = analogRead(PIN_POT);
  int potAzimut = analogRead(PIN_POT1);

  float targetSite = map(potSite, 0, 4095, 0, MAX_SITE_ANGLE);
  float targetAzimut = map(potAzimut, 0, 4095, 0, MAX_AZIMUT_ANGLE);

  // If both potentiometers are at 0 -> Bluetooth mode
  if (potSite == 0 && potAzimut == 0) {
    afficherLCD("MODE BLUETOOTH ACTIF", "CONNECTEZ VOUS...", "EN ATTENTE DE VALEUR", "ANGLE DE SITE");
    while (!SerialBT.hasClient()) {
      SerialBT.println("EN ATTENTE DE CONNEXION BLUETOOTH");
      delay(1000);
    }
    SerialBT.println("CONNECTE! BIENVENUE");
    SerialBT.println("VEUILLEZ ENTRER L'ANGLE CIBLE EN SITE(0,75)");
    targetSite = lireBluetoothAngle("Entrez l'angle SITE (0-75)", 0, MAX_SITE_ANGLE);
  }

  currentAngle = lire_site();

  // === Site correction (servo) ===
  float angleError = targetSite - currentAngle;
  while (abs(angleError) > ANGLE_MARGIN_SITE) {
    if (angleError < 0) {
      commande -= min(abs(angleError), PAS_CORRECTION);
      digitalWrite(PIN_LEDR, HIGH);
    } else {
      commande += min(PAS_CORRECTION, angleError);
      digitalWrite(PIN_LEDO, HIGH);
    }
    digitalWrite(PIN_LEDV, LOW);
    commande = constrain(commande, 0, 180);
    myServo.write(commande);
    delay(200);

    currentAngle = lire_site();
    potSite = analogRead(PIN_POT);
    if (potSite != 0) targetSite = map(potSite, 0, 4095, 0, MAX_SITE_ANGLE);
    angleError = targetSite - currentAngle;

    afficherLCD("Angle Actuel: " + String(currentAngle, 1) + (char)223,
                "Angle Cible: " + String(targetSite, 1) + (char)223,
                "CORRECTION DE SITE", " EN COURS");
  }
  digitalWrite(PIN_LEDV, HIGH);
  digitalWrite(PIN_LEDO, LOW);
  digitalWrite(PIN_LEDR, LOW);
  afficherLCD(" CALAGE SITE OK");
  SerialBT.println("CALAGE SITE OK");

  // === Azimuth input via Bluetooth ===
  if (potSite == 0 && potAzimut == 0) {
    afficherLCD("SAISIE D'AZIMUT");
    targetAzimut = lireBluetoothAngle("Entrez l'angle AZIMUT (0-60)", 0, MAX_AZIMUT_ANGLE);
  }
  angleZ = lire_azimut();

  // === Azimuth correction (stepper) ===
  float angleError1 = targetAzimut - angleZ;
  while (abs(angleError1) > ANGLE_MARGIN_AZIMUT) {
    int sens = (angleError1 > 0) ? 1 : -1;
    digitalWrite(PIN_LEDO, sens == 1);
    digitalWrite(PIN_LEDR, sens != 1);
    digitalWrite(PIN_LEDV, LOW);

    int errorStep = map(abs(angleError1), 0, 360, 0, MAX_STEPPER_STEP);
    int pas = (errorStep > MIN_STEPPER_STEP) ? min(204, errorStep) : MIN_STEPPER_STEP;
    myStepper.step(sens * pas);
    delay(200);

    angleZ = lire_azimut();
    potAzimut = analogRead(PIN_POT1);
    if (potAzimut != 0) targetAzimut = map(potAzimut, 0, 4095, 0, MAX_AZIMUT_ANGLE);
    angleError1 = targetAzimut - angleZ;

    afficherLCD("Azimut actuel: " + String(angleZ, 1) + (char)223,
                "Azimut cible : " + String(targetAzimut, 1) + (char)223,
                "CORRECTION D'AZIMUT", " EN COURS");
  }
  digitalWrite(PIN_LEDV, HIGH);
  digitalWrite(PIN_LEDO, LOW);
  digitalWrite(PIN_LEDR, LOW);
  afficherLCD(" CALAGE AZIMUT OK");
  SerialBT.println("CALAGE AZIMUT OK");

  // === Final display ===
  delay(500);
  afficherLCD("Site Actuel : " + String(currentAngle, 1) + (char)223,
              "Site Cible : " + String(targetSite, 1) + (char)223,
              "Azimut Actuel: " + String(angleZ, 1) + (char)223,
              "Azimut Cible : " + String(targetAzimut, 1) + (char)223);

  delay(3000);
  afficherLCD("Ecart Site : " + String(angleError, 1),
              "Ecart Azimut: " + String(angleError1, 1),
              "AUCUNE ERREUR", " DETECTEE");
  SerialBT.println("CALAGE REUSSIE.");
  delay(6000);
}
