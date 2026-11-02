---
titel: "HackXplore 2026: Smart IoT Platform für VEGA"
kurz: "Konzept einer IoT-Plattform, mit der Karlsruhe seine Stadtbäume nach Bedarf bewässern kann. Aufgabe von VEGA beim Hackathon HackXplore 2026."
datum: "2026-06"
tags: [IoT, MQTT, Arduino, Sensornetz, Hackathon]
titelbild: ./bilder/dashboard.png
titelbild_alt: "Screenshot des Dashboards TreePulse in dunklem Design. Oben das Wetter in Karlsruhe, darunter vier Zähler: 3 Bäume brauchen Wasser, 3 sollen bald geprüft werden, 6 sind gesund, 12 Sensoren. Darunter eine Karte der Karlsruher Innenstadt mit roten, gelben und grünen Punkten für die Sensoren."
code: https://github.com/jan-sltze/HackXplore/tree/feat/freddy_MQTT_Backend
startseite: true
reihenfolge: 3
---

## Die Aufgabe

Beim Hackathon HackXplore 2026 habe ich mit meinem Team eine Aufgabe der VEGA KG
bearbeitet: Innerstädtische Bäume sollen mit einem Feuchtigkeitssensor ausgestattet werden, um intelligente Bewässerungsstrategien in heißen Sommern zu ermöglichen. Sie soll der Stadt Karlsruhe helfen, die Bewässerung zu optimieren, sei es in Hinsicht auf Routenplanung, Wasserbedarfsplanung o. Ä.

Zur Verfügung stand ein Sensorkit mit einem Arduino und Feuchtigkeitssensoren.

![Fünf Personen in schwarzen T-Shirts und mit Namensschildern stehen in einem Raum mit Backsteinwänden, zwei T-Shirts tragen den Aufdruck HackXplore 2026](./bilder/IMG_1764.jpg)
*Abb. 01 — Team beim HackXplore 2026.*

## Unser Konzept

Das Konzept hat drei Teile: ein Sensornetz an den Bäumen, eine App für den
Wasserbedarf und die IoT-Plattform.

### Ein Sensor an jedem Baum

An jedem Baum sitzt ein günstiger Feuchtigkeitssensor. Auf LoRa an jedem Sensor haben wir bewusst verzichtet. Stattdessen kommuniziert jeder Sensor nur mit seinem
direkten Nachbarn links und rechts.

Wie in einer typischen Sterntopologie laufen die Informationen an zentralen
Stationen zusammen. Diese Stationen sitzen an Ampeln und hängen an der
elektrischen Leitung, die dort schon liegt. Erst an ihnen findet die
aufwendigere Kommunikation statt.


### Wasser nach Bedarf

Nicht jeder Baum braucht dieselbe Menge Wasser. Deshalb sollen sich die Daten
eines Baums einfach per App einscannen lassen. Aus Baumart und
Stammdurchmesser schätzt das System, wie viel Wasser der Baum braucht. Der
Bewässerer bekommt diese Menge als Bewässerungszeit angezeigt.

### Die Plattform

Die IoT-Plattform zeigt den Zustand jedes Baums an. Außerdem optimiert sie die
Routen für die Bewässerung.

Im Team habe ich mich um das MQTT-Backend gekümmert.

![Fünf Personen stehen vor einer großen Leinwand. Darauf ein Dashboard mit der Temperatur, einem roten Hinweis auf Bäume, die heute Wasser brauchen, Zählern für den Zustand der Bäume und einer Karte der Sensoren. Vorne sitzen zwei Zuhörer mit Laptops.](./bilder/IMG_1763.jpg)
*Abb. 03 — Beim Pitch. Auf der Leinwand das Dashboard: oben die Bäume, die heute Wasser brauchen, darunter die Karte der Sensoren.*

## Ausblick

Die Baumbewässerung ist nur ein Problem, das eine solche IoT-Plattform lösen
kann. Mit derselben Plattform lassen sich weitere Aufgaben in der Stadt
angehen, zum Beispiel volle Mülleimer erkennen.

## Pitch und Teamarbeit

Während des Hackathons hat mein Team Schulungen zum Projekt-Pitch und zur
Kommunikation im Team bekommen. Am Ende haben wir unsere Lösung einer Jury
vorgestellt.

![Selfie am Arbeitstisch: sechs Personen mit Laptops, Kaffeebechern und Sensorkabeln, im Hintergrund ein Banner der KIT-Gründerschmiede](./bilder/IMG_1762.jpg)
*Abb. 04 — Am Arbeitstisch während des Hackathons.*

![Beide Seiten des Teilnahmezertifikats von HackXplore 2026 für Frederik Eichlohn. Links die Urkunde mit der Challenge Smart Watering for Urban Trees von VEGA, rechts die Details mit Ausstellungsdatum und QR-Code zur Prüfung.](./bilder/zertifikat.png)
*Abb. 05 — Mein Teilnahmezertifikat: HackXplore 2026 vom 26. bis 28. Juni, Challenge „Smart Watering for Urban Trees“ von VEGA. [Zertifikat als PDF öffnen](/zertifikate/hackxplore-2026.pdf)*
