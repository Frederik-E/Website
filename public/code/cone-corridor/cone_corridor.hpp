#pragma once
// ============================================================================
//  cone_corridor.hpp — Geometrie-Werkzeugkasten für den Pylonen-Korridor
//
//  Reine Rechenfunktionen: kein Zustand, keine Ablauflogik, keine ROS-
//  Abhängigkeit. Wer wann was aufruft, entscheidet CorridorPipeline
//  (cone_planner.hpp).
//
//  Fünf Abschnitte:
//    1. ZIELVEKTOR   — Polylinien-Werkzeuge, Cone-Filter, Dreieck-Heuristik
//    2. KOLLISION    — Fahrzeug-Rechteck gegen Pylonen-Kreise
//    3. ÜBERGANG     — Einfädeln von der Fahrspur in den Korridor (Biarc/Dubins)
//    4. KETTE        — Fahrpfad entlang einer erkannten Pylonenreihe
//    5. VORONOI      — Mittellinie aus einer Delaunay-Triangulation
//
//  Die Abschnitte 1, 4 und 5 sind ALTERNATIVEN: sie berechnen denselben
//  Korridorpfad auf drei Wegen, je nachdem wie viele Pylonen bekannt sind.
//  Abschnitt 2 und 3 werden von allen dreien geteilt.
//
//  Faustregel zur Auswahl (Details: CorridorPipeline::runPipeline()):
//    wenige Pylonen, noch keine Reihe erkennbar   → Abschnitt 1
//    eine Reihe erkennbar                         → Abschnitt 4
//    beide Reihen erkennbar (>= 4 Pylonen)        → Abschnitt 5
//
//  Herkunft: portiert aus den Python-Prototypen cone_corridor_phase_planner.py
//  (Abschnitt 1), cone_corridor_lattice.py + cone_corridor_vector_testbed.py
//  (Abschnitt 4) und dem Branch feature/global_planner_v1 (Abschnitt 5).
// ============================================================================

#include "kal_trajectory_planner/types.hpp"

#include <array>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace corridor {

// ════════════════════════════════════════════════════════════════════
//  1. ZIELVEKTOR — Dreieck-Heuristik für die ersten Pylonen
// ════════════════════════════════════════════════════════════════════

// ────────────────────────────────────────────────────────────────────
//  Polylinien-Werkzeuge (auch von Abschnitt 2/3 genutzt)
// ────────────────────────────────────────────────────────────────────

std::vector<Vec2>   resample(const std::vector<Vec2>& pts, double ds = 0.05);
std::vector<double> tangents(const std::vector<Vec2>& poly, int smooth = 6);
std::vector<Vec2>   convexHull(std::vector<Vec2> pts);
bool                pointInConvexHull(const Vec2& point, const std::vector<Vec2>& hull);
double              minDistToPolyline(const Vec2& pt, const std::vector<Vec2>& polyline);

/// Polylinie -> Posen. Heading aus dem zentralen Differenzenquotienten.
std::vector<Pose> pathToPoses(const std::vector<Vec2>& path);

// ────────────────────────────────────────────────────────────────────
//  Cone-Vorverarbeitung: Fehldetektionen aussortieren, Trigger finden
// ────────────────────────────────────────────────────────────────────

/// Verwirft Pylonen, deren nächster Nachbar deutlich weiter weg ist als der
/// mediane Pylonenabstand im Cluster (Faktor `spacing_factor`).
/// Typischer Fall: Fehldetektion der Perception. -> stellt sicher, dass nur relevante cones behalten werden 
std::vector<Cone> filterOutlierCones(const std::vector<Cone>& raw, double spacing_factor);

/// Verwirft echte Einzelpylonen: eine Pylone muss BEIDES erfüllen, um zu
/// fliegen — (a) sie bildet im Nachbarschaftsgraphen (decomposeChains(),
/// Abschnitt 4) eine Kette der Länge 1 und (b) die nächste andere Pylone ist
/// weiter als `max_isolation_dist` entfernt.
///
/// Die UND-Verknüpfung ist wichtig: eine dünn besetzte, aber zusammenhängende
/// Reihe scheitert an (a) und bleibt erhalten, auch wenn ihr Reihenabstand
/// über `max_isolation_dist` liegt.
///
/// Bei < 2 Pylonen wird `raw` unverändert durchgereicht — eine einzelne
/// Ersterkennung soll nicht verworfen werden.
std::vector<Cone> filterIsolatedCones(const std::vector<Cone>& raw, const VehicleParams& vehicle,
                                       double max_isolation_dist);

/// Steht mindestens eine Pylone näher als `max_dist` an der Fahrspur?
/// Das ist die Trigger-Bedingung des gesamten Korridor-Verhaltens.
bool anyConeNearPolyline(const std::vector<Cone>& cones, const std::vector<Vec2>& lane1_pts,
                          double max_dist);

/// Die zur Fahrspur nächstgelegene Pylone = die Trigger-Pylone.
/// `cones` darf nicht leer sein.
const Cone& findTriggerCone(const std::vector<Cone>& cones, const std::vector<Vec2>& lane1_pts);

// ────────────────────────────────────────────────────────────────────
//  Dreieck-Zielvektor: Trigger + bis zu 2 weitere Pylonen (B, C)
// ────────────────────────────────────────────────────────────────────

/// Bestimmt EINEN Zielpunkt vor dem Korridor — kein Pfad, nur wohin.
/// Bootstrap für die allererste Situation, in der noch keine Pylonenreihe
/// erkennbar ist.
///
/// Konstruktion:
///   B := nächste Pylone zum Trigger, aus DERSELBEN Reihe
///   C := nächste Pylone zum Trigger, aus der GEGENÜBERLIEGENDEN Reihe
///   Zielrichtung := Richtung(B - trigger)
///   Zielposition := Mitte(B, C)            wenn C existiert
///                   B + halbe Fahrzeugbreite Richtung Auto   sonst
///
/// Warum die Reihenunterscheidung (über decomposeChains(), Abschnitt 4):
/// reine Abstandsminimierung wählt sonst gelegentlich zwei Pylonen DERSELBEN
/// Reihe als B/C — der Zielpunkt läge dann neben statt in der Lücke.
///
/// `forward_dir` schränkt die Kandidaten auf Pylonen VOR dem Trigger ein.
/// Ohne diese Einschränkung kann eine bereits passierte Pylone gewinnen und
/// der Zielvektor zeigt rückwärts. Der Nullvektor schaltet die Einschränkung
/// ab (= keine verlässliche Fahrtrichtung bekannt).
///
/// ok=false, wenn `others` leer ist oder nach der forward_dir-Einschränkung
/// kein Kandidat übrig bleibt — bewusst ein Fehlschlag statt eines
/// rückwärts gerichteten Ziels.
TargetResult computeTriangleTarget(
    const Cone& trigger,
    const std::vector<Cone>& others,
    const Vec2& car_xy,
    const VehicleParams& vehicle,
    const Vec2& forward_dir,
    double extra_clearance = 0.05);

// ────────────────────────────────────────────────────────────────────
//  Fahrspur für die Übergangssuche aufbereiten
// ────────────────────────────────────────────────────────────────────

struct LaneForTransition {
    std::vector<Vec2>   points;
    std::vector<double> tangents;
};

/// Bereitet die Fahrspur als Suchraum für Abzweigpunkte auf: resampeln,
/// geradeaus über das bekannte Ende hinaus verlängern, dann an der ERSTEN
/// Pylone abschneiden, die das geradeaus weiterfahrende Fahrzeug treffen
/// würde.
///
/// Der Schnitt ist entscheidend: die Trigger-Pylone steht per Definition nah
/// an der Spur. Ohne Schnitt könnte der Abzweigpunkt hinter ihr liegen — das
/// Fahrzeug würde erst nach der Pylone einlenken.
LaneForTransition laneForTransition(
    const std::vector<Vec2>& lane1_pts,
    const VehicleParams& vehicle,
    const std::vector<Obstacle>& obstacles,
    double resample_ds);

// ════════════════════════════════════════════════════════════════════
//  2. KOLLISION — Fahrzeug-Rechteck gegen Pylonen-Kreise
// ════════════════════════════════════════════════════════════════════

/// Ist der gesamte Posenpfad kollisionsfrei?
bool bodyClear(const std::vector<Pose>& poses,
               const std::vector<Obstacle>& obstacles,
               const VehicleParams& veh);

/// Index der ersten Kollision, -1 wenn keine.
int firstCollisionIndex(const std::vector<Pose>& poses,
                        const std::vector<Obstacle>& obstacles,
                        const VehicleParams& veh);

/// Prüft einen bereits berechneten Pfad gegen ein aktualisiertes
/// Hindernisfeld. Damit entscheidet ConeCorridorPlanner::plan(), ob der
/// letzte gültige Plan noch weitergefahren werden darf.
bool pathStillClear(const std::vector<Pose>& existing_path,
                    const std::vector<Obstacle>& new_obstacles,
                    const VehicleParams& veh);

/// Klassifiziert eine gefundene Kollision: echte Engstelle oder nur eine
/// schlechte Pfadwahl?
///
/// Sucht bei Pose `p` das nächste Hindernis links UND rechts der
/// Fahrtrichtung und vergleicht die Lücke mit width + 2*clearance.
/// Fehlt auf einer Seite ein Hindernis, gilt die Stelle konservativ NICHT
/// als Engstelle — ohne Gegenwand ist es kein Breitenproblem.
bool isPassageTooNarrow(const Pose& p, const std::vector<Obstacle>& obstacles,
                         const VehicleParams& vehicle, double search_radius = 1.0);

// ════════════════════════════════════════════════════════════════════
//  3. ÜBERGANG — von der Fahrspur in den Korridor einfädeln
// ════════════════════════════════════════════════════════════════════

struct DubinsCandidate {
    double segs[3];     // Segmentlängen [m]
    char   mode[4];     // z.B. "LSL\0"
    double length;      // Gesamtlänge [m]
    double R;           // verwendeter Radius [m]
    // true: der erste Bogen lenkt zu der Seite ein, auf der das Ziel liegt.
    // false markiert Kandidaten, die erst wegschwenken und dann einscheren --
    // die will man nur als letzte Wahl.
    bool natural = true;
};

/// Alle gültigen Dubins-Kandidaten, sortiert: erst alle "natürlichen"
/// (natural == true) nach Länge, danach der Rest nach Länge.
///
/// Warum nicht rein nach Länge: die kürzeste Kurve schwenkt gelegentlich
/// stark in die falsche Richtung aus, obwohl eine kaum längere direkt
/// zur Zielseite einschert.
std::vector<DubinsCandidate> dubinsCandidates(const Pose& start, const Pose& goal, double R);

/// Kürzester natürlicher Kandidat; nur falls es keinen gibt, der kürzeste
/// überhaupt.
std::optional<DubinsCandidate> dubinsShortest(const Pose& start, const Pose& goal, double R);

/// Tastet einen Dubins-Pfad in Posen ab.
std::vector<Pose> samplePoses(const Pose& start, const DubinsCandidate& c,
                              double step = 0.08);

/// Verbindet die Fahrspur mit `entry_pose` (dem Korridor-Eintritt).
///
/// Drei Stufen, alle in geschlossener Form — keine Iteration, keine
/// Zufallssuche:
///   1. Biarc  — zwei tangentenstetige Kreisbögen, Radius so groß wie
///               möglich. Gültig nur, wenn beide Bögen >= R_min bleiben.
///   2. Dubins — alle Worttypen, Radienfächer R_min .. 12*R_min.
///               Existiert konstruktionsbedingt immer.
///   3. Eng    — Radius auf transition_tight_radius_factor * R_min
///               abgesenkt, Länge hart gedeckelt. NUR diese Stufe
///               unterschreitet R_min.
///
/// Auswahl in allen Stufen: kürzester kollisionsfreier Kandidat, wobei
/// Kandidaten unter transition_max_length_m bevorzugt werden.
///
/// Harte Verwerfungsgründe, unabhängig von der Länge:
///   - kumulierter Drehwinkel > transition_max_turn_rad (verwirft Schleifen;
///     ein enger Vollkreis wäre sonst kurz genug, um zu gewinnen)
///   - Abzweigpunkt weiter als transition_max_departure_distance_m vom Ziel
///
/// `entry_heading_tolerance_rad` > 0 erlaubt zusätzlich, die Zielausrichtung
/// symmetrisch zu variieren; der genutzte Wert steht danach in
/// TransitionResult::heading_offset_used_rad.
///
/// Die Nahtstellen sind krümmungs-, nicht richtungsunstetig — fahrdynamisch
/// zulässig, es wird nicht nachgeglättet.
TransitionResult planTransition(
    const std::vector<Vec2>&     p1,
    const std::vector<double>&   t1,
    const Pose&                  entry_pose,
    const std::vector<Obstacle>& obstacles,
    const PipelineConfig&        cfg,
    double                       entry_heading_tolerance_rad = 0.0);

struct ShiftedTransition {
    TransitionResult transition;
    // Ab diesem Index hängt der Aufrufer die Mittellinie an den Übergang an.
    // lane[entry_index] IST die Zielpose; alles davor wird übersprungen und
    // darf nicht mehr gefahren werden.
    size_t entry_index   = 0;
    double entry_shift_m = 0.0;   // Bogenlänge lane[0] -> lane[entry_index]
};

/// planTransition() mit Suche über den Eintrittspunkt.
///
/// Richtungsbedingung (gilt IMMER): das Heading an einem akzeptierten
/// Eintrittspunkt muss innerhalb ±30° zur Richtung "Kandidat -> Ende der
/// Mittellinie" liegen. lane[0] ist also kein automatischer Rückfall —
/// erfüllt es die Bedingung nicht, wird der erste Punkt gesucht, der sie
/// erfüllt.
///
/// Warum: ein lokal tangentenstetiger, global aber falscher Eintrittspunkt
/// (Vorzeichenfehler in der Seitenwahl, instabile Tangente am Kettenanfang)
/// erzeugt sonst einen Übergang, der von der falschen Seite einfädelt.
///
/// Ab diesem Basispunkt werden weitere Eintrittspunkte in Schritten von
/// transition_entry_shift_step_m bis transition_entry_shift_max_m entlang
/// der Mittellinie durchprobiert (nur vorwärts). Der Eintrittspunkt bleibt
/// damit immer exakt auf der Mittellinie, nur weiter vorn.
///
/// Bewertet wird pro Kandidat der GESAMTPFAD (Übergang + Rest der
/// Mittellinie), nicht nur der Übergang: ein kurzer Übergang, der das
/// übersprungene Stück in eine Pylone hinein abkürzt, ist wertlos.
///
/// Ist kein Kandidat kollisionsfrei, kommt das Ergebnis für den Basispunkt
/// zurück. Erfüllt kein einziger Punkt die Richtungsbedingung, scheitert der
/// Übergang komplett.
ShiftedTransition planTransitionShiftedEntry(
    const std::vector<Vec2>&     p1,
    const std::vector<double>&   t1,
    const std::vector<Pose>&     lane,
    const std::vector<Obstacle>& obstacles,
    const PipelineConfig&        cfg);

// ════════════════════════════════════════════════════════════════════
//  4. KETTE — Fahrpfad entlang einer erkannten Pylonenreihe
// ════════════════════════════════════════════════════════════════════
//
//  Greift, sobald eine Pylonenreihe erkennbar ist. Statt auf einen einzelnen
//  Zielpunkt zu zielen (Abschnitt 1) wird die ganze bekannte Reihe abgefahren.
//
//  Gliederung:
//    4.0  Nachbarschaftsgraph + Zerlegung in Ketten
//    4.0b Reihen-Classifier — welche Pylone gehört zu welcher Reihe
//    4.1  Referenzlinie + Glättungswerkzeuge
//    4.2  Kandidaten (Frenet-Lattice)   ← ALTERNATIVE, NICHT im Einsatz
//    4.3  Kosten und Auswahl            ← ALTERNATIVE, NICHT im Einsatz
//    4.5  Fahrvektoren                  ← das tatsächlich Gefahrene
//
//  Achtung: 4.2 und 4.3 sind eine vollständig portierte, aber ungenutzte
//  Alternative. Nur LatticePlanner::plan() ruft sie auf, und den ruft
//  niemand. Die live gefahrene Pipeline nutzt 4.5.

using Chain = std::vector<Cone>;

// ────────────────────────────────────────────────────────────────────
//  4.0 Nachbarschaftsgraph + Ketten-Zerlegung
// ────────────────────────────────────────────────────────────────────

/// Nachbarschaftsgraph über alle Pylonen (adaptive Distanz, Reduktion auf
/// Grad <= 2, Ringe aufgelöst).
std::unordered_map<int, std::vector<int>> buildChainGraph(
    const std::vector<Cone>& cones, const VehicleParams& vehicle);

/// Läuft vom Startknoten über unbesuchte Nachbarn bis zur Sackgasse.
std::vector<int> walkChain(const std::unordered_map<int, std::vector<int>>& adj,
                            const std::vector<Cone>& cones, int start);

/// Zerlegt alle Pylonen über den Nachbarschaftsgraphen in disjunkte Ketten.
std::vector<Chain> decomposeChains(const std::vector<Cone>& cones, const VehicleParams& vehicle);

/// Mittlerer Punktabstand entlang der Kette (dicht = Wand, weit = offene Seite).
double chainMeanSpacing(const Chain& chain);

/// Dichteste Kette mit mindestens `min_len` Pylonen.
///
/// Wählt rein nach Pylonenabstand, ohne jeden Bezug zu Fahrzeug oder Spur.
/// Nur für den ungenutzten LatticePlanner gedacht. Die live gefahrene
/// Pipeline nutzt pickWallChainContaining() — Begründung siehe dort.
std::optional<Chain> pickWallChain(const std::vector<Chain>& chains, int min_len);

/// Wählt die Kette, die `cone_id` enthält — typischerweise die Trigger-Pylone.
///
/// Warum nicht pickWallChain(): ohne Verankerung an der auslösenden Pylone
/// könnte eine völlig unabhängige, aber dichtere Pylonengruppe im Sichtfeld
/// gewinnen (anderer Streckenabschnitt, Perception-Rauschen).
///
/// nullopt, wenn die Pylone in keiner Kette der Länge >= min_len vorkommt
/// (z.B. noch isoliert) — dann greift der Dreieck-Bootstrap.
std::optional<Chain> pickWallChainContaining(const std::vector<Chain>& chains, int min_len, int cone_id);

/// Dreht die Kette so, dass chain.front() das autonahe Ende ist.
Chain orientChainTowardCar(const Chain& chain, const Vec2& car_xy);

/// Schneidet Kettenenden mit Knickwinkel > max_kink_deg ab (bis Länge 3).
/// Der Nachbarschaftsgraph fängt an einem Ende sonst gelegentlich eine
/// Pylone der anderen Reihe ein.
Chain trimChainOutliers(const Chain& chain, double max_kink_deg = 45.0);

/// Konstruiert eine virtuelle Gegenreihe aus einer einzigen bekannten
/// Gegenpylone.
///
/// Ausgangslage: die Trigger-Kette ist bekannt, gegenüber steht genau EINE
/// Pylone (`mirror_target`). Für eine Delaunay-Triangulation ist das zu
/// wenig — also wird die Trigger-Kette kopiert und versetzt.
///
/// Der Versatz wird pro Kettenpunkt EINZELN entlang seiner lokalen Normale
/// angewandt (nicht als ein starrer Verschiebungsvektor für die ganze
/// Kette). Bei gekrümmten Ketten dreht sich die Korridor-Normale mit — ein
/// fester Vektor täte das nicht und würde mit wachsendem Abstand vom Anker
/// immer stärker von einer echten Parallelkurve abweichen.
///
/// Betrag: die Projektion von (mirror_target - anchor.pos) auf die Normale
/// am ankernächsten Kettenpunkt = die gemessene VOLLE Korridorbreite. Der
/// längsseitige Anteil wird bewusst weggelassen, er drückt keine Breite aus.
///
/// Das Ergebnis ist die gegenüberliegende WAND, keine Mittellinie — es wird
/// hier NICHT halbiert. Die Halbierung passiert automatisch in
/// buildCenterlinePath(): Voronoi-Kanten liegen per Definition gleich weit
/// von beiden Punktmengen entfernt.
///
/// Annahme: die Gegenreihe verläuft lokal parallel zur Trigger-Kette.
/// `anchor` muss kein Element von `chain` sein (Vergleich über Position,
/// nicht über Cone::id).
///
/// WICHTIG: rein virtuell. Das Ergebnis darf nicht in die Kollisionsprüfung
/// eingehen, nur als Voronoi-Eingabe.
std::vector<Vec2> mirrorChainThrough(const Chain& chain, const Cone& anchor,
                                      const Vec2& mirror_target);

/// Baut eine Kette ab `start` durch wiederholtes Anhängen der jeweils
/// nächstgelegenen unbesuchten Pylone aus `pool` — direkte Verkettung ohne
/// Nachbarschaftsgraph. `start` muss nicht in `pool` liegen.
///
/// Abbruch je Schritt:
///   - `fixed_threshold` gesetzt: jeder Schritt muss <= Schwelle sein.
///   - sonst: erster Schritt frei, jeder weitere <=
///     chainMeanSpacing(bisherige Kette) * growth_factor.
///
/// NICHT MEHR IM EINSATZ. Der mitwachsende Schwellwert war die vermutete
/// Ursache springender Trajektorien: eine falsch zugeordnete Pylone
/// verschiebt den Mittelwert und kippt die Kette auf die andere Reihe.
/// Ersetzt durch classifyRows() (feste Schwelle). Bleibt als generisches
/// Werkzeug erhalten.
Chain growChainFromCone(const Cone& start, const std::vector<Cone>& pool,
                         double growth_factor = 1.3,
                         std::optional<double> fixed_threshold = std::nullopt);

// ────────────────────────────────────────────────────────────────────
//  4.0b Reihen-Classifier — feste Distanzschwelle
// ────────────────────────────────────────────────────────────────────
//
//  Ordnet jede Pylone einer der beiden Reihen zu. Grundidee: Pylonen
//  derselben Reihe stehen nah beieinander, Pylonen verschiedener Reihen
//  deutlich weiter auseinander. Eine Zusammenhangskomponenten-Zerlegung über
//  eine FESTE Kantenlänge ist daher robuster als iteratives Kettenwachstum
//  (siehe growChainFromCone()).

struct Line2 {
    Vec2 point;   // Aufpunkt (Schwerpunkt der Fitpunkte)
    Vec2 dir;     // Richtung, normiert
};

/// Geradenfit durch `pts` per totaler kleinster Quadrate (Hauptachse der
/// Kovarianzmatrix). Mindestens 2 Punkte nötig.
Line2 fitLineTLS(const std::vector<Vec2>& pts);

/// Skalare Koordinate von `p` entlang `line`.
double projectOntoLine(const Line2& line, const Vec2& p);

/// Nächster Schnittpunkt von `line` mit `polyline`, gemessen ab `car_xy`.
/// nullopt, falls kein Schnitt existiert.
std::optional<Vec2> intersectLineWithPolyline(const Line2& line,
                                               const std::vector<Vec2>& polyline,
                                               const Vec2& car_xy);

/// Die `ref` nächstgelegene Pylone aus `pool`, ohne Distanzschranke.
/// nullptr bei leerem Pool.
///
/// Bewusst zentral: mehrere Stellen brauchen exakt diese Regel (die
/// B-Auswahl in computeTriangleTarget(), die Torbreiten-Messung in
/// computeDrivingVectorsFromFrontChain()) und dürfen nicht auseinanderlaufen.
const Cone* nearestConeTo(const std::vector<Cone>& pool, const Vec2& ref);

struct RowClassification {
    bool  ok = false;
    Chain rear_chain;    // Trigger-Seite, sortiert entlang rear_line
    Chain front_chain;   // gegenüberliegende Reihe, unsortiert
    Line2 rear_line;     // Geradenfit durch rear_chain
    // ACHTUNG, Rollentausch: true bedeutet, dass die Trigger-Pylone noch
    // isoliert ist. `rear_chain` ist dann NICHT die Trigger-Reihe, sondern
    // die gegenüberliegende, und `front_chain` enthält nur die Trigger-
    // Pylone selbst. Ohne dieses Flag wäre der Tausch für den Aufrufer
    // unsichtbar. Behandelt wird der Fall in
    // CorridorPipeline::planCorridorFrontRowOnly().
    bool  trigger_row_isolated = false;
};

/// Teilt `cones` in Trigger-Reihe (`rear_chain`) und Gegenreihe
/// (`front_chain`) auf.
///
/// Verfahren: Zusammenhangskomponenten mit Kante <=> Abstand <=
/// `same_side_max_dist`. Die Komponente mit dem Trigger ist die hintere
/// Reihe (braucht >= 2 Pylonen, sonst ok=false). Die ihr nächstgelegene
/// andere Komponente ist die vordere Reihe.
///
/// `rear_chain` wird nach Projektion auf `rear_line` sortiert — die beiden
/// Enden sind damit die Extrempunkte entlang der Reihe.
///
/// `cross_side_min_dist` > 0 aktiviert zwei Robustheits-Erweiterungen:
///   (a) Kollineare Fremd-Komponenten (alle Punkte näher als
///       cross_side_min_dist/2 an rear_line) werden in die hintere Reihe
///       absorbiert — überbrückt Lücken durch ausgefallene Einzeldetektionen.
///   (b) Die vordere Reihe muss tatsächlich seitlich liegen (mittlerer
///       senkrechter Abstand >= cross_side_min_dist/2).
RowClassification classifyRows(const std::vector<Cone>& cones, const Cone& trigger,
                                double same_side_max_dist, double cross_side_min_dist = 0.0);

// ────────────────────────────────────────────────────────────────────
//  4.1 Referenzlinie
// ────────────────────────────────────────────────────────────────────

struct ReferenceLine {
    std::vector<Vec2>   xy;
    std::vector<double> theta;
    std::vector<double> s;
    double half_width_m = 0.0;
    std::vector<Vec2>   centerline_xy;
    double sign = 1.0;             // +1/-1: Seite der befahrbaren Fläche
    std::vector<int>    wall_chain_ids;
    bool extrapolated = false;     // true: keine frische Kette diesen Zyklus
};

/// Glatter Fit durch die Kette: natürlicher kubischer Spline über die
/// Bogenlänge.
std::vector<Vec2> resampleChainSpline(const Chain& chain, int n_samples);

/// Abstand jeder Nicht-Wand-Pylone zur Referenzlinie, halbiert.
std::vector<double> measureHalfWidthSamples(const std::vector<Vec2>& wall_xy,
                                             const std::vector<Cone>& other_cones);

/// +1/-1: auf welcher Seite der lokalen Tangente von `side_pts` liegt `point`.
double signTowardPoint(const std::vector<Vec2>& side_pts, const Vec2& point);

/// Verschiebt jeden Punkt senkrecht zur lokalen Tangente um half_width * sign.
std::vector<Vec2> offsetPolyline(const std::vector<Vec2>& side_pts, double half_width, double sign);

/// Blendet den ANFANG von `new_cl` über `blend_distance` in `old_cl` ein
/// (Smoothstep) — verhindert, dass die Mittellinie zwischen Planungszyklen
/// unter dem Fahrzeug springt.
///
/// Sicherheitsabbruch: sind die Eingangstangenten entgegengesetzt oder
/// entstünde eine >= 90°-Ecke, kommt `new_cl` unverändert zurück.
std::vector<Vec2> blendCenterlines(const std::vector<Vec2>& old_cl, const std::vector<Vec2>& new_cl,
                                    double blend_distance);

// ────────────────────────────────────────────────────────────────────
//  4.2 Kandidaten (Quintik-Lattice) — ALTERNATIVE, nicht im Einsatz
// ────────────────────────────────────────────────────────────────────
//
//  n_lateral_candidates x horizons_m.size() Kandidatenkurven um die
//  Referenzlinie: je eine Quintik d(s) im Frenet-Frame von (s0,d0,d0') nach
//  (d_target,0,0). Krümmungskonformität prüft erst 4.3.

struct LatticeCandidate {
    double d_target = 0.0;
    double s_horizon = 0.0;
    std::vector<Vec2>   xy;
    std::vector<double> theta;
    std::vector<double> s;
    std::array<double, 6> d_coeffs{};   // Quintik-Koeffizienten in s' = s - s0
};

/// Zustandsbehafteter Alternativplaner: hält Referenzlinie (4.1) und das
/// zuletzt gewählte laterale Offset für die Hysterese (4.3).
///
/// NICHT IM EINSATZ — CorridorPipeline nutzt stattdessen die Fahrvektoren
/// aus 4.5. Vollständig portiert und lauffähig, falls die Lattice-Variante
/// wieder gebraucht wird.
class LatticePlanner {
public:
    explicit LatticePlanner(VehicleParams vehicle, LatticeConfig config = {});

    void reset();

    /// Ein Planungszyklus. Gibt die (ggf. extrapolierte) ReferenceLine
    /// zurück, oder nullopt, falls noch nie eine möglich war.
    std::optional<ReferenceLine> update(const std::vector<Cone>& known_cones, const Pose& car_pose);

    /// Referenzlinie -> Kandidaten -> Filter/Kosten -> Hysterese fortschreiben.
    std::optional<LatticeCandidate> plan(const std::vector<Cone>& known_cones, const Pose& car_pose);

    const std::optional<ReferenceLine>& reference() const { return reference_; }

private:
    VehicleParams vehicle_;
    LatticeConfig config_;
    std::optional<ReferenceLine> reference_;
    std::optional<double> last_chosen_d_;
};

/// Löst die 6 Randbedingungen per 6x6-System.
std::array<double, 6> solveQuintic(double d0, double d0p, double d0pp,
                                    double dT, double dTp, double dTpp, double S);

/// Referenzpunkt + -tangente bei Bogenlänge s_query (linear interpoliert;
/// Winkel über cos/sin, um ±pi-Sprünge zu vermeiden).
void sampleReferenceAt(const ReferenceLine& ref, const std::vector<double>& s_query,
                        std::vector<Vec2>& xy_out, std::vector<double>& theta_out);

/// Projiziert car_pose auf die Referenzlinie -> (s0, d0, d0').
std::tuple<double, double, double> projectToFrenet(const ReferenceLine& ref, const Pose& car_pose);

/// n gleichmäßig verteilte End-Offsets von der Wand bis zur offenen Seite.
std::vector<double> lateralOffsets(const ReferenceLine& ref, int n, double margin_m);

/// Erzeugt die Kandidaten als Quintiken um die Referenzlinie.
std::vector<LatticeCandidate> generateCandidates(const ReferenceLine& ref, const Pose& car_pose,
                                                  const LatticeConfig& config);

// ────────────────────────────────────────────────────────────────────
//  4.3 Kosten, Filter, Auswahl — ALTERNATIVE, nicht im Einsatz
// ────────────────────────────────────────────────────────────────────
//
//  Hart: Kollision oder Durchstoß der Wand-Referenzlinie.
//  Weich: Wandabstand, Krümmung, Hysterese, optional Exploration.

struct CostTerms {
    double wall = 0.0;
    double curvature = 0.0;
    double hysteresis = 0.0;
    double exploration = 0.0;
};

/// d(s') an den Abtastpunkten, aus den gelösten Quintik-Koeffizienten.
std::vector<double> candidateDValues(const LatticeCandidate& candidate);

/// Minimaler signierter Abstand zur Wand-Referenzlinie (<= 0: Durchstoß).
double minWallClearance(const ReferenceLine& ref, const LatticeCandidate& candidate);

/// Harter Filter: Wand-Clearance + Kollision mit bekannten Pylonen.
bool isFeasible(const ReferenceLine& ref, const LatticeCandidate& candidate,
                const std::vector<Cone>& known_cones, const VehicleParams& vehicle,
                const LatticeConfig& config);

/// Gewichtete weiche Kosten, normiert auf half_width bzw. 1/r_min.
double candidateCost(const ReferenceLine& ref, const LatticeCandidate& candidate,
                      const VehicleParams& vehicle, std::optional<double> last_chosen_d,
                      const LatticeConfig& config, CostTerms& terms_out);

/// Harter Filter -> weiche Kosten -> Minimum. nullopt, wenn kein Kandidat
/// den harten Filter besteht.
std::optional<std::tuple<LatticeCandidate, double, CostTerms>> selectCandidate(
    const ReferenceLine& ref, const std::vector<LatticeCandidate>& candidates,
    const std::vector<Cone>& known_cones, const VehicleParams& vehicle,
    std::optional<double> last_chosen_d, const LatticeConfig& config);

// ────────────────────────────────────────────────────────────────────
//  4.5 Fahrvektoren aus der Pylonenreihe  ← das tatsächlich Gefahrene
// ────────────────────────────────────────────────────────────────────
//
//  Jede Pylone der Reihe bekommt genau einen Fahrvektor: Orientierung aus
//  dem Mittel der Nachbarrichtungen, Position seitlich versetzt. Die
//  Fahrvektoren werden anschließend per Hermite verbunden.

/// Orientierung des Fahrvektors an Index i: Mittel aus der Richtung zum
/// nächsten und vom vorherigen Nachbarn; am Kettenende dessen Richtung direkt.
double neighborOrientation(const std::vector<Vec2>& chain_pts, int i);

struct DrivingVectorResult {
    std::vector<Pose> vectors;
    Chain front_chain;   // leer: keine Pylone auf der befahrbaren Seite
    // Nur von computeDrivingVectorsFromFrontChain() gesetzt: die Pylone, an
    // der die Torbreite gemessen wurde. Wird als TargetResult::cone_c
    // weitergemeldet — sonst müsste der Aufrufer die Auswahlregel nachbauen
    // und könnte eine andere Pylone ausweisen als die tatsächlich vermessene.
    std::optional<Cone> cone_c;
};

/// Fahrvektoren aus der hinteren (Trigger-)Kette. Zwei Fälle, je nachdem wie
/// viel von der Gegenreihe bekannt ist.
///
/// ── Fall A: front_chain.size() < 2 — fester Abstand ──
/// Jede Pylone der Kette bekommt einen Fahrvektor im Abstand
/// `single_row_offset_m`, senkrecht zur lokalen Kettentangente, auf der
/// durch `sign` gewählten Seite, ohne Längsversatz.
///
/// Bildlich: um jede Pylone ein Kreis mit diesem Radius; der Fahrvektor liegt
/// exakt darauf. Die überlappenden Kreise ergeben zusammen die Fahrspur. Das
/// Fahrzeug hält damit zu JEDER Pylone denselben Abstand — nie näher, nie
/// weiter.
///
/// Der Fahrvektor der letzten Kettenpylone definiert automatisch das
/// Korridorende (siehe planCorridorChain(), stop_pose).
///
/// `other_cones` wird hier nur ausgewertet, um `front_chain` für den
/// Rückgabewert zu füllen — auf die Fahrvektoren hat es keinen Einfluss.
///
/// ── Fall B: front_chain.size() >= 2 — echte Mittellinie ──
/// Beide Ketten werden per resampleChainSpline() zu glatten Kurven f und h
/// mit je `n_samples` Punkten; die Fahrspur ist der elementweise Mittelpunkt
/// (f[k]+h[k])/2.
///
/// Warum nicht der Abstand zur jeweils nächsten Einzelpylone: der schwankt
/// sprunghaft, sobald die nächstgelegene vordere Pylone wechselt — das war
/// die Ursache des Schlingerns beim Geradeausfahren durch den Korridor.
///
/// Die Reihenfolge der front_chain wird über fitLineTLS() + Projektion
/// bestimmt (nicht über growChainFromCone(), aus demselben Grund wie bei
/// classifyRows()) und an rear_chain.front() ausgerichtet — die autonahen
/// Enden beider Ketten müssen zusammenpassen, sonst läuft die
/// Mittelpunktbildung verkehrt herum.
///
/// ── Sicherheitsklemmung, NUR in Fall B ──
/// half_width wird nach unten geklemmt auf
///   max(width/2 + Pylonenradius + clearance, rear_wall_clearance_m)
/// damit die Karosserie die Kettenpylone nie überlappt.
///
/// Symmetrisch dazu nach oben auf
///   front_dist - (width/2 + Radius der vorderen Pylone + clearance)
/// Ohne diese Gegenschranke schöbe die untere Klemmung in einem engen Tor
/// den Fahrvektor über die Mitte hinaus Richtung vorderer Reihe, ohne zu
/// prüfen, ob dort Platz ist — das Fahrzeug schleifte an der vorderen Reihe.
///
/// Passen beide Mindestabstände nicht zusammen, wird symmetrisch gemittelt.
/// Ein dann immer noch unbefahrbarer Rest ist Sache der nachgelagerten
/// Kollisionsprüfung beim Aufrufer.
///
/// Fall A bleibt bewusst ungeklemmt: `single_row_offset_m` ist dort bereits
/// der exakte Zielabstand, keine zu korrigierende Schätzung.
DrivingVectorResult computeDrivingVectorsFromChain(
    const Chain& rear_chain,
    const std::vector<Cone>& other_cones,
    double sign,
    double single_row_offset_m,
    const VehicleParams& vehicle,
    int n_samples = 200,
    double rear_wall_clearance_m = 0.40);

/// Spiegelfall zu computeDrivingVectorsFromChain(): nur die VORDERE Reihe ist
/// als Reihe sichtbar, die hintere ist nur durch die Trigger-Pylone vertreten
/// (siehe RowClassification::trigger_row_isolated).
///
/// computeDrivingVectorsFromChain() kann das nicht übernehmen: dessen fester
/// `single_row_offset_m` ignoriert die tatsächliche Korridorbreite — die ist
/// hier aber bekannt.
///
/// Zwei Schritte:
///   1. Breite messen: C := die Pylone aus `front_chain` mit dem kleinsten
///      Abstand zur Trigger-Pylone. Der Korridor ist dort |C - trigger| breit,
///      die Fahrspur gehört auf die Hälfte davon.
///   2. Parallelverschiebung: die Vektorkette entlang `front_chain` wird um
///      diesen halben Abstand Richtung Trigger versetzt — pro Kettenpunkt
///      entlang seiner lokalen Normale, damit der Versatz in Kurven mitdreht.
///
/// `sign` wählt die Seite von `front_chain`, auf der die Trigger-Pylone liegt.
/// Der Versatz wird nach unten auf Pylonenradius + clearance + halbe
/// Fahrzeugbreite geklemmt, damit die Fahrspur bei einem sehr engen Tor nicht
/// unter die Kollisionsgrenze rutscht.
DrivingVectorResult computeDrivingVectorsFromFrontChain(
    const Chain& front_chain,
    const Cone& trigger,
    double sign,
    const VehicleParams& vehicle);

/// Kubisches Hermite-Segment zwischen zwei Posen.
///
/// Warum nicht Dubins: bei nahe beieinanderliegenden Fahrvektoren baucht
/// Hermite sanft aus, während Dubins zu großen Schleifen neigt. Dafür ist
/// |kappa| <= 1/R_min nicht garantiert — das prüft und repariert
/// buildAndRepairDrivingPath().
std::vector<Pose> hermiteSegment(const Pose& start, const Pose& goal, double step = 0.05);

/// Verbindet die Fahrvektoren Segment für Segment per Hermite.
std::optional<std::vector<Pose>> buildDrivingPath(const std::vector<Pose>& vectors, double step = 0.05);

struct RepairedPath {
    std::optional<std::vector<Pose>> path;
    bool ok = false;   // true: kollisions- und krümmungsfrei (ggf. nach Reparatur)
};

/// Baut den Hermite-Pfad und repariert ihn bei Bedarf lokal (localRepair()).
RepairedPath buildAndRepairDrivingPath(const std::vector<Pose>& vectors,
                                        const std::vector<Cone>& obstacles,
                                        const VehicleParams& vehicle);

// ────────────────────────────────────────────────────────────────────
//  Krümmungs- und Reparaturwerkzeuge (von 4.5 genutzt)
// ────────────────────────────────────────────────────────────────────

/// Maximale Krümmung über den umschriebenen Kreis dreier aufeinander-
/// folgender Punkte. true, falls überall <= 1/R_min.
bool pathCurvatureOk(const std::vector<Vec2>& pts, double R_min, double& kmax_out);

/// Laplace-Glättung, endpunkterhaltend.
std::vector<Vec2> smoothPolyline(const std::vector<Vec2>& pts, int iters, double alpha = 0.5);

/// Krümmung + Kollision gegen die aktuell bekannten Pylonen.
bool centerlineFeasible(const std::vector<Vec2>& centerline, bool curv_ok,
                         const std::vector<Cone>& cones, const VehicleParams& vehicle);

/// Lokale Reparatur statt globaler Neuplanung: schiebt kollidierende
/// Abschnitte von der Pylone weg und glättet rein krümmungsdefekte
/// Abschnitte. Arbeitet fensterweise, in maximal `max_passes` Durchläufen.
std::vector<Vec2> localRepair(const std::vector<Vec2>& centerline, const std::vector<Cone>& cones,
                               const VehicleParams& vehicle, int max_passes = 6);

// ════════════════════════════════════════════════════════════════════
//  5. VORONOI-MITTELLINIE — wenn beide Reihen bekannt sind
// ════════════════════════════════════════════════════════════════════
//
//  Der genaueste der drei Wege, aber auch der anspruchsvollste: braucht
//  mindestens 4 Pylonen (für die Triangulation) UND erkennbare Struktur auf
//  BEIDEN Seiten.
//
//  Ablauf:
//    Delaunay-Triangulation (Bowyer-Watson)
//      -> dualer Voronoi-Graph
//      -> Pruning auf eine einzelne durchgehende Kette
//      -> Gate-Ausgang am Kettenende bestimmen
//      -> Recentering + Laplace-Glättung + Krümmungsprüfung
//
//  Die Voronoi-Kanten liegen per Definition gleich weit von den Pylonen
//  beider Seiten entfernt — sie SIND die Mittellinie, ohne dass irgendwo
//  explizit halbiert werden müsste.
//
//  Wiederverwendet aus 1-3: resample(), pathToPoses(), convexHull(),
//  pointInConvexHull(), minDistToPolyline(), smoothPolyline(),
//  pathCurvatureOk(), planTransition().

struct VoronoiVertex { Vec2 pos; };

struct VoronoiEdge {
    int v1, v2;         // Indizes in vertices[]
    int gen1, gen2;     // Generator-(Pylonen-)Indizes
};

struct VoronoiGraph {
    std::vector<VoronoiVertex> vertices;
    std::vector<VoronoiEdge>   edges;
};

using AdjList = std::unordered_map<int, std::vector<int>>;

/// Delaunay-Triangulation -> dualer Voronoi-Graph, beschnitten auf die
/// konvexe Hülle der Pylonen (unbeschränkte Randzellen fallen weg).
/// Leerer Graph bei < 4 Pylonen.
VoronoiGraph buildVoronoiGraph(const std::vector<Vec2>& cones);

/// Adjazenzliste aus den Kanten, ungerichtet und dedupliziert.
AdjList buildAdj(const VoronoiGraph& vg);

/// Reduziert den Graphen auf eine einzelne Kette: jeder Knoten mit Grad > 2
/// verliert iterativ seine längste Kante, danach bleibt nur die größte
/// Zusammenhangskomponente. Der rohe Voronoi-Graph hat an Verzweigungen
/// Grad > 2 — ohne diesen Schritt gäbe es keinen eindeutigen Pfad.
void pruneHighDegree(AdjList& adj, const std::vector<VoronoiVertex>& verts,
                     int max_iter = 200);

/// Alle Knoten mit Grad 1 = die Kettenenden nach dem Pruning.
std::vector<int> degree1Vertices(const AdjList& adj);

/// Läuft von `start` über unbesuchte Nachbarn bis `end`.
/// nullopt, falls kein einfacher Pfad existiert (z.B. Ring ohne Grad-1-Enden).
std::optional<std::vector<int>> traverseChain(const AdjList& adj, int start, int end);

/// Wählt das Grad-1-Knotenpaar mit der längsten Kette dazwischen = die
/// Hauptkette durch den Korridor statt eines kurzen Seitenasts.
/// Deterministischer Fallback bei Gleichstand: minimales/maximales x.
/// `lane1_pts` orientiert das Ergebnis so, dass das erste Element näher an
/// der Fahrspur liegt.
std::pair<int, int> longestEndpointPair(
    const AdjList& adj,
    const std::vector<VoronoiVertex>& verts,
    const std::vector<int>& degree1,
    const std::vector<Vec2>& lane1_pts);

/// Ersetzt den letzten Kettenknoten durch die Mitte des Ausgangs-Tors:
/// die letzte Kettenrichtung wird bis zur Hülle verlängert, dort werden die
/// nächsten Pylonen links und rechts des Strahls gesucht, ihr Mittelpunkt
/// ist das Ergebnis.
/// nullopt bei fehlendem Schnittpunkt oder fehlenden Tor-Pylonen — der
/// Aufrufer behält dann den ursprünglichen Knoten.
std::optional<Vec2> computeExitPoint(const std::vector<Vec2>& raw_chain,
                                      const std::vector<Vec2>& cones,
                                      const std::vector<Vec2>& hull);

/// Eintrittspose in eine Mittellinie: Position von path[start], Heading
/// entlang der Sehne zu path[start + lookahead].
///
/// Die Sehne statt der lokalen Tangente, weil letztere deutlich
/// empfindlicher auf Rauschen in der Mittellinie reagiert.
Pose computeEntryPose(const std::vector<Vec2>& path, size_t lookahead = 10, size_t start = 0);

/// Die ganze Mittellinie als Posenfolge nach der computeEntryPose()-Regel.
/// Eingabe für planTransitionShiftedEntry() bei den Zweigen, deren
/// Mittellinie als reine Punktfolge vorliegt (Voronoi, virtueller Spiegel).
std::vector<Pose> laneEntryPoses(const std::vector<Vec2>& path, size_t lookahead = 10);

/// Baut die vollständige Mittellinie (siehe Abschnitts-Kommentar oben).
/// `cfg.voronoi` steuert Glättung und Recentering, `cfg.vehicle.R_min` die
/// Krümmungsschranke, `cfg.resample_ds` die Abtastschrittweite.
CenterlineResult buildCenterlinePath(
    const std::vector<Vec2>& cones,
    const std::vector<Vec2>& lane1_pts,
    const PipelineConfig& cfg);

}  // namespace corridor
