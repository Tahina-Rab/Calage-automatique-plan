/*
  Proteus simulation of the automatic angle positioning (1 axis: site)
  Board : Arduino Uno
  The "measured" angle is simulated in software (Proteus has no tilt model),
  the target angle comes from a potentiometer on A1.
  A servo corrects the simulated angle step by step; LEDs show the error level.
*/

#include <Servo.h>      // servo library
#include <Wire.h>       // I2C (not used here but often included)
#include <LiquidCrystal.h>

#define SERVO_PIN 9     // servo control pin

Servo myServo;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);   // LCD pins

int currentAngle;       // simulated current angle

void setup() {
  // LEDs
  pinMode(13, OUTPUT);  // red LED (large error)
  pinMode(8, OUTPUT);   // green LED (no error)
  pinMode(7, OUTPUT);   // orange LED (moderate error)

  Serial.begin(9600);
  randomSeed(analogRead(0));       // random initial angle
  currentAngle = random(0, 180);

  myServo.attach(SERVO_PIN);
  lcd.begin(16, 2);

  // Welcome sequence
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("    WELCOME   ");
  delay(600);
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("    CALAGE");
  lcd.setCursor(0, 1); lcd.print("  AUTOMATIQUE ");
  delay(600);
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("ENTRER L'ANGLE");
  lcd.setCursor(0, 1); lcd.print("    CIBLE");
  delay(2000);

  // Initial target angle reading
  int potValue = analogRead(A1);
  int targetAngle = map(potValue, 0, 1023, 0, 180);
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("ANGLE CIBLE: ");
  lcd.setCursor(13, 0); lcd.print(targetAngle);
  delay(3000);
}

void loop() {
  int vitesse = 2;                                  // correction speed
  int potValue = analogRead(A1);                    // target potentiometer
  int targetAngle = map(potValue, 0, 1023, 0, 180);
  int angleError = currentAngle - targetAngle;

  // Move the simulated angle towards the target
  if (currentAngle < targetAngle) {
    currentAngle += vitesse;
    if (currentAngle > targetAngle) currentAngle = targetAngle;
  } else if (currentAngle > targetAngle) {
    currentAngle -= vitesse;
    if (currentAngle < targetAngle) currentAngle = targetAngle;
  }

  // Turn all LEDs off
  digitalWrite(13, LOW);
  digitalWrite(8, LOW);
  digitalWrite(7, LOW);

  lcd.clear();

  // An error is detected
  if (abs(angleError) > 1) {
    myServo.write(currentAngle);                    // update servo

    // Signal according to error size
    if (abs(angleError) > 20) {
      digitalWrite(13, HIGH);                       // large error -> red LED
    } else {
      digitalWrite(7, HIGH);                        // moderate error -> orange LED
    }

    // Display information on the LCD
    lcd.setCursor(0, 0); lcd.print("Actuel: "); lcd.print(currentAngle);
    lcd.setCursor(0, 1); lcd.print("Cible: "); lcd.print(targetAngle);
    delay(600);

    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Ecart: "); lcd.print(angleError);
    delay(500);

    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("CORRECTION");
    lcd.setCursor(0, 1); lcd.print("  EN COURS ");
    delay(700);
  } else {
    // No significant error
    myServo.write(currentAngle);                    // hold position
    digitalWrite(8, HIGH);                          // green LED on
    lcd.setCursor(0, 0); lcd.print("AUCUNE ERREUR");
    lcd.setCursor(0, 1); lcd.print("  DETECTEE ");
    delay(600);
  }
}
