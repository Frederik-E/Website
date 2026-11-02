---
titel: Scientific Paper - haptic feedback shared control
kurz: Nutzerstudie zu haptischen Kräften an einem Joystick, die beim Steuern einer Drohne helfen
datum: 2026-08
rolle: Software-Entwicklung, Datenintegrität, Organisation
tags: [ROS 2, Python, Haptik, Nutzerstudie, Statistik, Software-Architektur]
code: https://github.com/Frederik-E/Human-Machine-interaction-shared-control-
titelbild: ./joystick.png
titelbild_alt: "Zeichnung eines Probanden von hinten an einem Tisch. Mit der rechten Hand hält er einen Joystick mit Kraftrückkopplung. Auf dem Bildschirm vor ihm fliegt eine Drohne zwischen kreisförmigen Hindernissen auf ein Ziel zu. Ein blauer Pfeil am Joystick und einer an der Drohne zeigen die Kraft, die vom nächsten Hindernis wegdrückt."
startseite: true
reihenfolge: 4
entwurf: false
---

## Worum es ging

Im Forschungsseminar „Cooperative and Haptic Human-Machine Interactions“ ging
es um einen Joystick, der selbst Kräfte erzeugt. Beim Steuern einer Drohne
drückt er die Hand zum Beispiel von einem Hindernis weg. Mensch und Assistenz
steuern so gemeinsam, das nennt sich Haptic Shared Control. Die Kräfte sollen
den Piloten sicherer ans Ziel bringen, ohne ihn zu bevormunden.

## Meine Arbeit

Im Team habe ich das Kraftmodell mitentwickelt, die Nutzerstudie durchgeführt
und ausgewertet. Gemeinsam haben wir darüber das Paper „Phased Haptic Shared
Control for Safe and Autonomy-Preserving UAV Teleoperation“ geschrieben. Es
ist noch nicht veröffentlicht.

Den Versuch habe ich in ROS 2 so aufgebaut, dass die Versuchsleitung den
ganzen Ablauf über ein Dashboard steuert. Dazu gehören Recovery-Methoden:
Stürzt ein Teil des Systems ab, gehen keine Daten verloren, und der Proband
macht an derselben Stelle weiter.
