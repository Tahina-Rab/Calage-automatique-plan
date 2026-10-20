**🇫🇷 Français** · [🇬🇧 English](README.md)

# Système de calage automatique de l’angle d’inclinaison d’un plan (site et azimut)

> Projet de fin d’études à l’**EAMAC** (École Africaine de la Météorologie et de l’Aviation Civile) — Division Électronique Informatique, promotion EAC-SEI 2023-A, soutenu le 11 juillet 2025.

Un dispositif à base d’**ESP32** qui mesure l’orientation d’un plan avec un capteur inertiel et la corrige automatiquement, **en site (élévation)** et **en azimut**, jusqu’à atteindre l’angle de consigne fixé par l’utilisateur, avec des potentiomètres ou **à distance par Bluetooth**.

<p align="center">
  <img src="docs/images/prototype-front.jpg" alt="Prototype, vue de face avec l’écran LCD 20x4" width="48%">
  <img src="docs/images/prototype-side.jpg" alt="Prototype, vue de côté" width="48%">
</p>

<!-- À FAIRE : ajouter ici le lien de la démo vidéo, par exemple :  🎥 **Démo vidéo :** https://... -->

## Pourquoi ce projet ?

Le réglage des unités **PAPI** (indicateur de pente d’approche de précision) lors des opérations de calibration se fait à la main avec des instruments optiques, ce qui est long et peu précis. Les systèmes de suivi solaire ont aussi besoin d’un contrôle en temps réel de l’inclinaison des panneaux. Ce prototype montre une solution automatisée et peu coûteuse pour régler et maintenir un angle sur deux axes.

## Fonctionnement

| Axe | Mesure | Actionneur | Méthode |
|-----|--------|------------|---------|
| **Site (élévation)**, 0–75° | Accéléromètre + gyroscope du MPU6050 | Servomoteur SG90 | Un filtre complémentaire (α = 0,98) fusionne les deux capteurs ; le servomoteur avance par pas jusqu’à ce que l’écart soit inférieur à ±1° |
| **Azimut**, 0–60° | Gyroscope du MPU6050 (axe Z) | Moteur pas à pas 28BYJ-48 + driver ULN2003 | La vitesse de rotation en Z est intégrée dans le temps ; le moteur tourne jusqu’à ce que l’écart soit inférieur à ±2° |

1. Au démarrage, le MPU6050 est testé, le gyroscope est calibré et un message de bienvenue s’affiche sur l’écran LCD.
2. Les **angles cibles** sont lus sur deux potentiomètres. Si **les deux sont à 0**, le système passe en **mode Bluetooth** et attend les angles envoyés depuis un téléphone.
3. Le site est calé en premier, puis l’azimut. Une **LED rouge / orange / verte** indique le sens de la correction et la réussite.
4. La progression s’affiche sur un **écran LCD 20×4 I2C** et est envoyée au téléphone par Bluetooth, jusqu’au message `CALAGE REUSSIE.`

## Matériel

| Composant | Modèle | Qté |
|-----------|--------|-----|
| Microcontrôleur | ESP32-WROOM | 1 |
| Capteur inertiel | MPU6050 (accéléromètre 3 axes + gyroscope 3 axes) | 1 |
| Potentiomètres | 10 kΩ | 2 |
| Servomoteur | SG90 | 1 |
| Moteur pas à pas + driver | 28BYJ-48 + ULN2003 | 1 |
| Afficheur | LCD 20×4 avec module I2C | 1 |
| LED de signalisation | rouge, orange, verte (+ résistances 220 Ω) | 3 |
| Divers | breadboard, câbles Dupont, alimentation 5 V, boîtier en contreplaqué | – |

Coût estimé du prototype : environ **60 800 FCFA** (voir le rapport, section 6.3).

### Brochage (tel qu’utilisé dans le programme)

| Fonction | Broche ESP32 |
|----------|--------------|
| Signal du servomoteur | GPIO 13 |
| Potentiomètre – site | GPIO 34 |
| Potentiomètre – azimut | GPIO 35 |
| LED rouge / orange / verte | GPIO 4 / 32 / 14 |
| Moteur pas à pas (IN1–IN4 de l’ULN2003, dans l’ordre passé à `Stepper`) | GPIO 18, 23, 19, 5 |
| LCD (I2C, adresse `0x27`) et MPU6050 | bus I2C par défaut (`Wire.begin()` ; GPIO 21 SDA, GPIO 22 SCL sur la plupart des cartes ESP32) |

Schéma de câblage (réalisé avec Fritzing) :

![Schéma de câblage](docs/images/wiring-diagram-fritzing.jpg)

<details>
<summary>Intérieur du boîtier</summary>

![Intérieur du prototype](docs/images/prototype-inside.jpg)

</details>

## Démarrage

### Prérequis
- [Arduino IDE](https://www.arduino.cc/en/software) avec le **paquet de cartes ESP32** (Espressif).
- Bibliothèques (Gestionnaire de bibliothèques) : `ESP32Servo`, `LiquidCrystal_I2C`, et une bibliothèque `MPU6050` qui fournit `getMotion6()` et `CalibrateGyro()` (par exemple *MPU6050 par Electronic Cats*, basée sur I2Cdevlib). `Stepper`, `Wire` et `BluetoothSerial` sont fournies avec le cœur Arduino / ESP32.

### Téléversement
1. Ouvre `firmware/esp32_calage_site_azimut/esp32_calage_site_azimut.ino`.
2. Choisis ta carte ESP32 et le port, puis téléverse.
3. Ouvre le moniteur série à 115200 bauds si tu veux voir les messages de débogage.

### Utilisation
- **Avec les potentiomètres :** tourne-les pour régler le site cible (0–75°) et l’azimut cible (0–60°) ; le système corrige les deux axes.
- **Avec le Bluetooth :** mets les deux potentiomètres à **0**, associe ton téléphone au périphérique **`ESP32-calage`**, ouvre une application de terminal Bluetooth série, puis envoie chaque angle sous forme de nombre suivi d’un retour à la ligne quand il est demandé. Les valeurs hors plage sont refusées.

## Simulation (Proteus)

Avant la réalisation, la logique de commande a été validée sous **Proteus 8** avec une **carte Arduino Uno**, un potentiomètre, un servomoteur, un écran LCD et trois LED. Proteus ne sait pas simuler une inclinaison : l’angle « mesuré » est donc simulé dans le code et corrigé vers la cible par pas de 2°. Voir `firmware/simulation_arduino_uno/`.

![Schéma Proteus](docs/images/proteus-simulation.jpg)

## Limites et perspectives

- L’**azimut est estimé en intégrant le gyroscope**, ce qui dérive avec le temps (le MPU6050 n’a pas de magnétomètre). Un magnétomètre ou un capteur d’orientation absolue comme le **BNO055**, avec un **filtre de Kalman**, améliorerait la précision.
- Le servomoteur SG90 a un couple limité ; un **vérin électrique** conviendrait mieux à de vraies unités PAPI ou à des panneaux solaires.
- Ajouter une **batterie rechargeable** (Li-Ion avec gestion de charge) pour rendre le système portable.
- Applications possibles : réglage automatique des unités PAPI et des feux d’approche, et **suiveur solaire à deux axes** (avec un algorithme de trajectoire du soleil ou des capteurs LDR).

## Structure du dépôt

```
.
├── firmware/
│   ├── esp32_calage_site_azimut/     # programme final (ESP32)
│   └── simulation_arduino_uno/       # programme de simulation Proteus (Arduino Uno)
├── docs/
│   ├── Rapport_PFE_EAMAC_2025.pdf    # rapport complet (français)
│   └── images/
├── README.md                         # version anglaise
└── README.fr.md                      # version française
```

## Auteurs

- **RABARY Haja Tahina**
- **AVOCEGAMOU Lionel Noukpo**

Encadrant : **DAKO Komlan Dominique Tavio**, EAMAC. La théorie, les détails et les coûts sont dans le [rapport](docs/Rapport_PFE_EAMAC_2025.pdf).

## Licence

Code publié sous [licence MIT](LICENSE).
