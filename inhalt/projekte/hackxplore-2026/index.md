---
titel: "HackXplore 2026: Smart IoT Platform für VEGA"
kurz: "Konzept einer IoT-Plattform, mit der Karlsruhe seine Stadtbäume nach Bedarf bewässern kann. Aufgabe von VEGA beim Hackathon HackXplore 2026."
datum: "2026-06"
tags: [IoT, MQTT, Arduino, Sensornetz, Hackathon]
titelbild: ./baumsensor.png
titelbild_alt: "Zeichnung: Zwei Stadtbäume am Gehweg, darunter im Schnitt die Erde mit den Wurzeln. Neben jedem Baum steckt ein Feuchtigkeitssensor im Boden, um die Sonde des ersten liegen Wassertropfen. Eine gestrichelte Funkstrecke verbindet die Sensoren von Nachbar zu Nachbar und führt zu einer Station an einer Ampel. Von dort gehen die Daten zur IoT-Plattform: ein Bildschirm mit Zählern und einer Karte, auf der grüne, orange und rote Punkte für die Bäume stehen und eine blaue Route zu den roten führt."
code: https://github.com/jan-sltze/HackXplore/tree/feat/freddy_MQTT_Backend
linkedin: https://www.linkedin.com/feed/update/urn:li:ugcPost:7477290419605864448/
linkedin_text: "Beitrag von Jan Schultze aus meinem Team, mit Fotos vom Arbeitstisch, vom Pitch und vom Team."
startseite: true
reihenfolge: 3
---

## Die Aufgabe

Beim Hackathon HackXplore 2026 habe ich mit meinem Team eine Aufgabe der VEGA KG
bearbeitet: Innerstädtische Bäume sollen mit einem Feuchtigkeitssensor ausgestattet werden, um intelligente Bewässerungsstrategien in heißen Sommern zu ermöglichen. Sie soll der Stadt Karlsruhe helfen, die Bewässerung zu optimieren, sei es in Hinsicht auf Routenplanung, Wasserbedarfsplanung o. Ä.

Zur Verfügung stand ein Sensorkit mit einem Arduino und Feuchtigkeitssensoren.

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

![Screenshot des Dashboards TreePulse in dunklem Design. Oben das Wetter in Karlsruhe, darunter vier Zähler: 3 Bäume brauchen Wasser, 3 sollen bald geprüft werden, 6 sind gesund, 12 Sensoren. Darunter eine Karte der Karlsruher Innenstadt mit roten, gelben und grünen Punkten für die Sensoren.](./bilder/dashboard.png)
*Abb. 01 — Das Dashboard: oben das Wetter, darunter die Zähler für den Zustand der Bäume und die Karte der Sensoren.*

## Ausblick

Die Baumbewässerung ist nur ein Problem, das eine solche IoT-Plattform lösen
kann. Mit derselben Plattform lassen sich weitere Aufgaben in der Stadt
angehen, zum Beispiel volle Mülleimer erkennen.

## Pitch und Teamarbeit

Während des Hackathons hat mein Team Schulungen zum Projekt-Pitch und zur
Kommunikation im Team bekommen. Am Ende haben wir unsere Lösung einer Jury
vorgestellt.

![Beide Seiten des Teilnahmezertifikats von HackXplore 2026 für Frederik Eichlohn. Links die Urkunde mit der Challenge Smart Watering for Urban Trees von VEGA, rechts die Details mit Ausstellungsdatum und QR-Code zur Prüfung.](./bilder/zertifikat.png)
*Abb. 02 — Mein Teilnahmezertifikat: HackXplore 2026 vom 26. bis 28. Juni, Challenge „Smart Watering for Urban Trees“ von VEGA. [Zertifikat als PDF öffnen](/zertifikate/hackxplore-2026.pdf)*
