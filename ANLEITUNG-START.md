# Von der Domain bis zur veröffentlichten Website

Der ganze Weg in der Reihenfolge, in der du ihn gehen solltest. Einmal durcharbeiten, danach brauchst du nur noch [ANLEITUNG-VEROEFFENTLICHEN.md](ANLEITUNG-VEROEFFENTLICHEN.md).

**Zeit:** etwa zwei bis drei Stunden aktive Arbeit, verteilt über ein bis zwei Tage. Das meiste davon ist Text schreiben.

**Kosten:** Das Hosting bei Cloudflare ist für diese Seite kostenlos. Nur die Domain kostet Geld, meist 10 bis 15 Euro im Jahr.

**Wichtig für die Reihenfolge:** Die Umstellung der Nameserver in Schritt 5 kann bis zu 24 Stunden dauern. Starte sie früh und schreibe deine Texte in der Wartezeit. Deshalb steht sie hier vor den Inhalten.

---

## Schritt 1: Werkzeuge einrichten

1. **Node.js** installieren, Version 22.12 oder neuer, von <https://nodejs.org>. Nimm die Fassung, die als „LTS" angeboten wird.
2. Terminal öffnen und `node -v` eingeben. Es muss eine Zahl ab `v22.12` erscheinen.
3. Einen Editor installieren, zum Beispiel **Visual Studio Code** von <https://code.visualstudio.com>.
4. Den Projektordner im Editor öffnen.
5. Im Terminal `npm install` eingeben. Das lädt einmalig die benötigten Pakete.

---

## Schritt 2: Domain kaufen

1. Einen Anbieter wählen. Üblich sind INWX, Netcup, Namecheap oder Porkbun. Cloudflare selbst verkauft auch Domains, dann entfällt Schritt 5 fast komplett.
2. Einen Namen wählen. `vorname-nachname.de` ist unaufgeregt und gut zu diktieren.
3. **Nur die Domain kaufen.** Du brauchst kein Hosting-Paket, keinen Webspace und kein E-Mail-Postfach dazu. Das übernimmt alles Cloudflare kostenlos.
4. Beim Kauf den Whois-Schutz aktivieren, wenn er angeboten wird. Sonst steht deine Adresse in einer öffentlichen Datenbank.
5. Die Zugangsdaten beim Anbieter notieren. Du brauchst sie in Schritt 5.

---

## Schritt 3: Die Seite lokal ansehen

1. Im Projektordner `npm run dev` eingeben.
2. <http://localhost:4321> im Browser öffnen.
3. Du siehst die Seite mit Platzhaltern und zwei Beispielprojekten.
4. Das Terminal offen lassen. Jede Änderung, die du speicherst, erscheint sofort im Browser.

---

## Schritt 4: Einstellungen ausfüllen

1. `inhalt/einstellungen.yaml` im Editor öffnen.
2. Jede Zeile mit `[TODO: ...]` durch deine Angabe ersetzen. Die Anführungszeichen stehen lassen.
3. Bei `domain` deine gekaufte Domain eintragen, mit `https://` und ohne Schrägstrich am Ende.
4. Bei `sprache` `en` oder `de` wählen. Das ändert alle Beschriftungen der Seite.
5. Speichern und im Browser nachsehen.

`einleitung` ist der erste Satz auf der Startseite, groß gesetzt. Schreib dort, woran du arbeitest.

`initialen` sind bis zu drei Buchstaben, die das bewegte Zeichen auf der Startseite alle 40 Sekunden zeichnet. Leer lassen (`initialen: ""`) blendet sie aus.

Dein Name erscheint nur klein in der Fußzeile, im Browser-Tab und im Lebenslauf. `untertitel` steht gar nicht auf der Seite, sondern im Browser-Tab, in Suchergebnissen und über dem Lebenslauf im PDF.

---

## Schritt 5: Cloudflare-Konto anlegen und die Domain umziehen

Das dauert am längsten, deshalb jetzt.

1. <https://dash.cloudflare.com/sign-up> öffnen, Konto kostenlos anlegen, Bestätigungsmail anklicken.
2. Im Dashboard auf „Add a domain", deine Domain eingeben, Tarif **Free** wählen.
3. Cloudflare liest die vorhandenen DNS-Einträge ein und zeigt dir am Ende **zwei Nameserver** an, zum Beispiel `bob.ns.cloudflare.com`. Diese beiden brauchst du gleich.
4. Bei deinem Domain-Anbieter einloggen, die Einstellung für Nameserver suchen (heißt oft „Nameserver", „DNS" oder „NS-Einträge") und die vorhandenen durch die beiden von Cloudflare ersetzen. Speichern.
5. Warten. Cloudflare schickt eine E-Mail, sobald die Domain „Active" ist. Meist unter einer Stunde, manchmal bis zu 24 Stunden.

**Bevor du umstellst:** Ab jetzt verwaltet Cloudflare deine Domain. Weiterleitungen und E-Mail-Einstellungen, die du beim bisherigen Anbieter eingerichtet hast, funktionieren danach nicht mehr. Läuft dort schon eine E-Mail-Adresse, notiere dir vorher alle DNS-Einträge.

Während du wartest, mach mit Schritt 6 weiter.

---

## Schritt 6: Inhalte schreiben

Das ist der eigentliche Aufwand. Wie es geht, steht in [ANLEITUNG-INHALTE.md](ANLEITUNG-INHALTE.md).

In dieser Reihenfolge:

1. Die zwei Beispielprojekte löschen: die Ordner `inhalt/projekte/beispiel-modellauto/` und `inhalt/projekte/beispiel-slam-vergleich/`. Auch `inhalt/seiten/beispiel-seite.md` kannst du löschen.
2. Zwei bis vier eigene Projekte anlegen. Lieber wenige gute als viele halbe.
3. `inhalt/ueber-mich.md` schreiben, 60 bis 120 Wörter.
4. `inhalt/lebenslauf.yaml` ausfüllen.
5. Zwischendurch `npm run check` laufen lassen. Es zeigt, was noch fehlt.

---

## Schritt 7: Impressum und Datenschutz

In Deutschland brauchst du beides, auch für eine private Seite mit Projekten.

1. `inhalt/impressum.md` öffnen. Nötig sind mindestens: vollständiger Name, ladungsfähige Anschrift, E-Mail-Adresse.
2. Willst du deine Wohnadresse nicht öffentlich zeigen, prüfe eine ladungsfähige Adresse über einen Anbieter.
3. `inhalt/datenschutz.md` öffnen. Im Platzhalter stehen die Punkte, die für diese Seite zutreffen: Hosting bei Cloudflare, Server-Logs, E-Mail-Weiterleitung, keine Cookies, kein Tracking, Schriften und Videos selbst gehostet.
4. Einen Generator im Netz benutzen und den Text hier einsetzen.
5. Speichern.

---

## Schritt 8: Zum ersten Mal hochladen

1. `npm install -D wrangler` eingeben. Wrangler ist das Programm, das die Seite hochlädt.
2. `npx wrangler login` eingeben. Der Browser öffnet sich, auf „Allow" klicken.
3. `wrangler.jsonc` öffnen und bei `"name"` einen Namen eintragen, zum Beispiel `"anna-meier"`. Erlaubt sind Kleinbuchstaben, Zahlen und Bindestriche.
4. `npm run deploy -- --dry-run` als Testlauf. Meldet die Prüfung fehlende Inhalte, zuerst diese ausfüllen.
5. `npm run deploy` eingeben. Am Ende zeigt Wrangler eine Adresse wie `https://anna-meier.deinkonto.workers.dev`. Diese öffnen und die Seite ansehen.

Ab hier ist die Seite öffentlich erreichbar, nur noch nicht unter deiner Domain.

---

## Schritt 9: Domain mit der Seite verbinden

Erst möglich, wenn Cloudflare die Domain als „Active" meldet.

1. Im Cloudflare-Dashboard links auf „Compute (Workers)" klicken, in älteren Ansichten heißt der Punkt „Workers & Pages". Dann deinen Worker anklicken.
2. Auf „Settings", dann „Domains & Routes".
3. „Add", dann „Custom domain" wählen.
4. `anna-meier.de` eintragen und bestätigen. Cloudflare legt den DNS-Eintrag selbst an.
5. Den Vorgang wiederholen und `www.anna-meier.de` eintragen.

Nach ein paar Minuten ist die Seite unter deiner Domain erreichbar, mit HTTPS.

---

## Schritt 10: www auf die Hauptadresse umleiten

Damit es die Seite nur unter einer Adresse gibt.

1. Im Dashboard deine Domain anklicken, dann links „Rules", dann „Redirect Rules".
2. „Create rule" anklicken, Name zum Beispiel `www umleiten`.
3. Bedingung: Field `Hostname`, Operator `equals`, Value `www.anna-meier.de`.
4. Aktion „Dynamic" wählen, Ausdruck `concat("https://anna-meier.de", http.request.uri.path)`, Status `301`, „Preserve query string" anhaken.
5. Speichern.

---

## Schritt 11: E-Mail-Adresse der Domain einrichten

Eine Adresse wie `kontakt@anna-meier.de`, die an dein normales Postfach weitergeleitet wird. Kostenlos.

1. Im Dashboard deine Domain anklicken, dann links „Email", dann „Email Routing", dann „Get started".
2. Cloudflare trägt die nötigen DNS-Einträge selbst ein, wenn du zustimmst.
3. Unter „Destination addresses" deine private E-Mail-Adresse eintragen.
4. In deinem Postfach die Bestätigungsmail von Cloudflare öffnen und den Link anklicken.
5. Unter „Routing rules" eine Regel anlegen: `kontakt` als Adresse, dein Postfach als Ziel.

Danach eine Testmail an `kontakt@anna-meier.de` schicken. Die Adresse gehört anschließend in `inhalt/einstellungen.yaml` und ins Impressum.

Senden kannst du über diese Adresse nicht, Cloudflare leitet nur weiter.

---

## Schritt 12: Cloudflare-Schalter prüfen

Cloudflare kann Skripte in die Seite einbauen, die du nicht willst. Vier Schalter gehören aus. Klicke dazu im Dashboard auf deine Domain.

1. „Scrape Shield", dann „Email Address Obfuscation" ausschalten.
2. „Speed", dann „Optimization", dann „Content Optimization": „Rocket Loader" ausschalten.
3. „Security", dann „Bots": „Bot Fight Mode" ausschalten.
4. „Analytics & Logs", dann „Web Analytics": nicht aktivieren.

Danach prüfen, ob die Live-Seite sauber ist:

1. Die Seite im Browser öffnen, Entwicklerwerkzeuge mit `F12` öffnen.
2. Reiter „Network", Seite neu laden. In der Spalte „Domain" darf nur deine eigene Domain stehen.
3. Reiter „Application", links „Cookies". Die Liste muss leer sein.

---

## Schritt 13: Das Projekt sichern

Wenn der Ordner verloren geht, ist die Seite weg. Cloudflare speichert nur das Ergebnis, nicht deine Dateien.

**Variante A, privates Repository bei GitHub:**

1. Auf GitHub ein neues Repository anlegen und auf „Private" stellen.
2. `git add -A` und `git commit -m "Erster Stand"`.
3. Die von GitHub angezeigten Befehle für `git remote add` und `git push` ausführen.
4. Nach jeder größeren Änderung wieder committen und pushen.

**Variante B, einfache Kopie:**

1. Den Ordner `inhalt/` kopieren.
2. Die Kopie in deine Cloud oder auf eine externe Festplatte legen.
3. Nach jeder größeren Änderung wiederholen.

`inhalt/` enthält alles, was nicht wiederherstellbar ist. Der Rest lässt sich neu bauen.

---

## Checkliste

- [ ] Node.js installiert, `npm install` gelaufen
- [ ] Domain gekauft
- [ ] `inhalt/einstellungen.yaml` ausgefüllt, `domain` eingetragen
- [ ] Cloudflare-Konto, Domain hinzugefügt, Nameserver umgestellt, Status „Active"
- [ ] Beispielprojekte und Beispielseite gelöscht
- [ ] Eigene Projekte, Über mich, Lebenslauf geschrieben
- [ ] Impressum und Datenschutz geschrieben
- [ ] `npm run check` meldet nichts mehr
- [ ] `npm run deploy` gelaufen, workers.dev-Adresse geprüft
- [ ] Domain und `www` als Custom Domain eingetragen
- [ ] `www` leitet auf die Hauptadresse um
- [ ] E-Mail-Weiterleitung eingerichtet und getestet
- [ ] Vier Cloudflare-Schalter aus, keine Cookies, keine fremden Anfragen
- [ ] Projekt gesichert
