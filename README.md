**🇬🇧 English** · [🇫🇷 Français](README.fr.md)

# Automatic Tilt-Angle Positioning System (Elevation & Azimuth)

> End-of-study project at **EAMAC** (African School of Meteorology and Civil Aviation) — Electronics & Computer Science division, class EAC-SEI 2023-A, defended on 11 July 2025.

An **ESP32-based device** that measures the orientation of a plane with an inertial sensor and automatically corrects it, in **elevation (site)** and **azimuth**, until it matches a target angle set by the user — with a potentiometer or **remotely over Bluetooth**.

<p align="center">
  <img src="docs/images/prototype-front.jpg" alt="Prototype, front view with the 20x4 LCD" width="48%">
  <img src="docs/images/prototype-side.jpg" alt="Prototype, side view" width="48%">
</p>

<!-- TODO: add the video demo link here, e.g.:  🎥 **Video demo:** https://... -->

## Why this project?

Aligning units such as **PAPI** (Precision Approach Path Indicator) lights during calibration is done by hand with optical tools, which is slow and imprecise. Solar-tracking systems also need real-time control of panel inclination. This prototype shows a low-cost, automated way to set and hold an angle on two axes.

## How it works

| Axis | Sensing | Actuator | Method |
|------|---------|----------|--------|
| **Elevation (site)**, 0–75° | MPU6050 accelerometer + gyroscope | SG90 servo motor | Complementary filter (α = 0.98) fuses both sensors; the servo moves in steps until the error is within ±1° |
| **Azimuth**, 0–60° | MPU6050 gyroscope (Z axis) | 28BYJ-48 stepper motor + ULN2003 driver | Z rate is integrated over time; the stepper turns until the error is within ±2° |

1. At power-up the MPU6050 is checked, the gyroscope is calibrated and a welcome message is shown on the LCD.
2. **Target angles** are read from two potentiometers. If **both are at 0**, the system switches to **Bluetooth mode** and waits for the angles from a phone.
3. The elevation is set first, then the azimuth. A **red / orange / green LED** shows the correction direction and success.
4. Progress is displayed on a **20×4 I2C LCD** and sent to the phone over Bluetooth, ending with `CALAGE REUSSIE.`

## Hardware

| Component | Model | Qty |
|-----------|-------|-----|
| Microcontroller | ESP32-WROOM | 1 |
| Inertial sensor | MPU6050 (3-axis accelerometer + 3-axis gyroscope) | 1 |
| Potentiometers | 10 kΩ | 2 |
| Servo motor | SG90 | 1 |
| Stepper motor + driver | 28BYJ-48 + ULN2003 | 1 |
| Display | LCD 20×4 with I2C backpack | 1 |
| Status LEDs | red, orange, green (+ 220 Ω resistors) | 3 |
| Others | breadboard, Dupont wires, 5 V supply, plywood enclosure | – |

Estimated prototype cost: about **60,800 FCFA** (see the report, section 6.3).

### Pinout (as used in the firmware)

| Function | ESP32 pin |
|----------|-----------|
| Servo signal | GPIO 13 |
| Potentiometer – site | GPIO 34 |
| Potentiometer – azimuth | GPIO 35 |
| LED red / orange / green | GPIO 4 / 32 / 14 |
| Stepper (ULN2003 IN1–IN4, as passed to `Stepper`) | GPIO 18, 23, 19, 5 |
| LCD (I2C, address `0x27`) and MPU6050 | default I2C bus (`Wire.begin()`; GPIO 21 SDA, GPIO 22 SCL on most ESP32 boards) |

Wiring diagram (made with Fritzing):

![Wiring diagram](docs/images/wiring-diagram-fritzing.jpg)

<details>
<summary>Inside the enclosure</summary>

![Inside the prototype](docs/images/prototype-inside.jpg)

</details>

## Getting started

### Requirements
- [Arduino IDE](https://www.arduino.cc/en/software) with the **ESP32 board package** (Espressif).
- Libraries (Library Manager): `ESP32Servo`, `LiquidCrystal_I2C`, and an `MPU6050` library that provides `getMotion6()` and `CalibrateGyro()` (for example *MPU6050 by Electronic Cats*, based on I2Cdevlib). `Stepper`, `Wire` and `BluetoothSerial` come with the Arduino / ESP32 core.

### Upload
1. Open `firmware/esp32_calage_site_azimut/esp32_calage_site_azimut.ino`.
2. Select your ESP32 board and port, then upload.
3. Open the Serial Monitor at 115200 baud if you want debug output.

### Use it
- **With potentiometers:** turn them to set the target site (0–75°) and azimuth (0–60°); the system corrects both axes.
- **With Bluetooth:** set both potentiometers to **0**, pair your phone with the device **`ESP32-calage`**, open a Bluetooth serial terminal app, then send each angle as a number followed by a newline when asked. Out-of-range values are rejected.

## Simulation (Proteus)

Before building the hardware, the control logic was validated in **Proteus 8** with an **Arduino Uno**, a potentiometer, a servo, an LCD and three LEDs. Proteus has no tilt model, so the "measured" angle is simulated in the code and corrected towards the target in steps of 2°. See `firmware/simulation_arduino_uno/`.

![Proteus schematic](docs/images/proteus-simulation.jpg)

## Limitations and future work

- The **azimuth is estimated by integrating the gyroscope**, which drifts over time (the MPU6050 has no magnetometer). A magnetometer or an absolute-orientation sensor such as the **BNO055**, plus a **Kalman filter**, would improve accuracy.
- The SG90 servo has limited torque; an **electric linear actuator (vérin)** would suit real PAPI units or solar panels.
- Add a **rechargeable battery** (Li-Ion with charge management) to make the system portable.
- Possible applications: automatic aiming of PAPI units and approach lights, and **two-axis solar tracking** (with a sun-path algorithm or LDR sensors).

## Repository structure

```
.
├── firmware/
│   ├── esp32_calage_site_azimut/     # final firmware (ESP32)
│   └── simulation_arduino_uno/       # Proteus simulation sketch (Arduino Uno)
├── docs/
│   ├── Rapport_PFE_EAMAC_2025.pdf    # full report (French)
│   └── images/
├── README.md                         # English version
└── README.fr.md                      # French version
```

## Authors

- **RABARY Haja Tahina**
- **AVOCEGAMOU Lionel Noukpo**

Supervisor: **DAKO Komlan Dominique Tavio**, EAMAC. Full details, theory and costs are in the [report](docs/Rapport_PFE_EAMAC_2025.pdf) (in French). A French version of this README is available: [README.fr.md](README.fr.md).

## License

Code released under the [MIT License](LICENSE).
