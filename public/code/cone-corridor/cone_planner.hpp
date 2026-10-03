#pragma once
// ============================================================================
//  cone_planner.hpp — Ablaufsteuerung des Pylonen-Korridors
//
//  Zwei Klassen, zwei Ebenen:
//
//    CorridorPipeline   (namespace corridor)
//        Die Zustandsmaschine. Entscheidet, WANN geplant wird und WELCHER
//        Algorithmus zum Einsatz kommt. Enthält selbst keine Geometrie —
//        gerechnet wird ausschließlich in cone_corridor.hpp.
//
//    ConeCorridorPlanner (namespace planning)
//        Der Adapter nach außen. Übersetzt zwischen der Trajektorien-Welt
//        des Planners (PointGlobal) und der Geometrie-Welt des Korridors
//        (Vec2/Pose/Cone) und entscheidet, was am Ende zurückgegeben wird.
//
//  ── Grundprinzip: jeder Zyklus plant komplett neu ──
//
//  Pylonen bewegen sich nicht. Derselbe Pylonensatz führt deterministisch zum
//  selben Ergebnis, also gibt es keinen Grund, einen Plan über mehrere Zyklen
//  zu "merken". runPipeline() bewertet die Situation jedes Mal von vorn.
//
//  Eine frühere Version prüfte die Trigger-Bedingung nur einmal beim Verlassen
//  von IDLE. Folge: nach dem allerersten je erkannten Trigger blieb der
//  Korridor für den Rest der Fahrt aktiv und plante beliebige später
//  auftauchende Pylonen ungeprüft mit ein.
//
//  Ausnahmen von "jedes Mal neu" gibt es nur zwei, beide gut begründet:
//    - identisches Cone-Array  -> Zyklus übersprungen (reine Wiederholung)
//    - cached_transition_      -> der Abzweigpunkt wird eingefroren (s.u.)
//
//  ── Die fünf Planungszweige ──
//
//  runPipeline() baut jeden Zyklus zuerst die Reihenzuordnung auf
//  (classifyRows(), feste Nachbarschaftsdistanz) und wählt dann:
//
//    Schritt 0.5  Nur die VORDERE Reihe ist als Reihe sichtbar, die
//                 Trigger-Pylone steht noch isoliert.
//                 -> planCorridorFrontRowOnly()
//                 Muss zuerst geprüft werden: Schritt 1.5 würde den Fall
//                 sonst anziehen, dort aber entarten.
//
//    Schritt 1    Genug Pylonen insgesamt UND genug außerhalb der
//                 Trigger-Kette -> echte Voronoi/Delaunay-Mittellinie.
//                 -> planCorridorVoronoi()
//
//    Schritt 1.5  Genau EINE Gegenpylone bekannt. Zu wenig für eine echte
//                 Triangulation, aber genug, um per mirrorChainThrough()
//                 eine virtuelle Gegenreihe zu konstruieren und darauf
//                 dieselbe Mittellinie zu berechnen.
//                 -> planCorridorVirtualMirror()
//
//    Schritt 2    Eine Reihe erkennbar -> die ganze Reihe wird als
//                 Fahrvektorkette abgefahren.
//                 -> planCorridorChain()
//
//    Schritt 2b   Nichts davon -> Dreieck-Heuristik als Bootstrap für die
//                 allererste Situation. Inline in runPipeline().
//
//  Kein Zweig fällt auf einen anderen zurück. Die Zweigwahl ist eine
//  Situationsbewertung; scheitert der gewählte Zweig, ist das ein echter
//  Planungsfehler und der Zyklus wird FAILED.
// ============================================================================

#include "kal_trajectory_planner/types.hpp"
#include "kal_trajectory_planner/cost_functions_global.hpp"
#include "kal_trajectory_planner/cone_corridor.hpp"

#include <optional>
#include <string>
#include <vector>

namespace corridor {

// ════════════════════════════════════════════════════════════════════
//  1. CorridorPipeline — Zustandsmaschine und Ablaufsteuerung
// ════════════════════════════════════════════════════════════════════
//
//  Reine Orchestrierung. Ruft die Funktionen aus cone_corridor.hpp in der
//  richtigen Reihenfolge auf, rechnet aber selbst nichts:
//
//    onConesReceived():  filterCones() -> detectTrigger() -> Zustand setzen
//    runPipeline():      Zielpunkt -> Übergang -> Kollisionsprüfung
//
//  filterCones() und detectTrigger() sind virtuelle Policy-Hooks — sie legen
//  fest, WAS als Ausreißer bzw. als Trigger gilt. Die Geometrie dahinter
//  steckt in cone_corridor.cpp.

class CorridorPipeline {
public:
    explicit CorridorPipeline(PipelineConfig cfg = {});

    // ────────────────────────────────────────────────────────────────
    //  Schnittstelle zur äußeren Planungsschleife
    // ────────────────────────────────────────────────────────────────

    /// Nimmt ein neues Cone-Array von der Perception entgegen und bewertet
    /// die Situation komplett neu.
    ///
    /// Ablauf: filtern -> ins Pylonen-Gedächtnis einpflegen -> Trigger prüfen.
    ///
    /// Bei gültigem Trigger: current_cones_ aktualisiert, replan_needed_ und
    /// trigger_latched_ gesetzt.
    ///
    /// Ohne Trigger hängt das Verhalten davon ab, ob in diesem Lauf schon
    /// einmal getriggert wurde:
    ///   - noch nie  -> reset(), zurück nach IDLE
    ///   - schon mal -> Zustand bleibt unverändert stehen
    ///
    /// Der zweite Fall ist wichtig: lane1_pts_ wird jeden Zyklus aus dem
    /// aktuellen, ungefilterten Wahrnehmungsframe neu bestimmt und kann
    /// kurzzeitig schrumpfen, auch wenn das Pylonen-Gedächtnis unverändert
    /// bleibt. Der Trigger fällt dann fälschlich negativ aus. Ohne das Latch
    /// verwarf plan() dadurch gelegentlich einen bereits gefundenen, gültigen
    /// Plan und fiel auf die rohe Kartenlinie zurück.
    ///
    /// Einzige Abkürzung: ist `cones` byteweise identisch zum letzten Aufruf,
    /// wird der ganze Zyklus übersprungen. Das ist keine Politik, sondern
    /// reine Wiederholungsvermeidung.
    void onConesReceived(const std::vector<Cone>& cones);

    /// Setzt die aktuelle Fahrspur (die Basistrajektorie um den Korridor herum).
    void setLane1(const std::vector<Vec2>& lane1_pts);

    /// Soll die Pipeline in diesem Zyklus laufen?
    bool needsReplan() const;

    /// Führt einen vollständigen Planungszyklus aus (Zweigwahl siehe
    /// Dateikommentar oben).
    PlanResult runPipeline();

    /// Ergebnis des letzten Zyklus.
    const std::optional<PlanResult>& getCurrentResult() const;

    /// Aktueller Zustand der Zustandsmaschine.
    CorridorState getState() const;

    /// Die in diesem Zyklus erkannte Trigger-Pylone, nullopt wenn IDLE.
    ///
    /// GlobalPlannerNode braucht ihre Weltposition, um beim ersten Trigger
    /// den zu publizierenden Kartenausschnitt einzufrieren — unabhängig
    /// davon, welcher Zweig anschließend greift.
    std::optional<Cone> triggerCone() const;

    /// Korridor verlassen: Plan, Zustand und Caches weg.
    ///
    /// Das Pylonen-Gedächtnis und die Durchfahrt-Latches bleiben bewusst
    /// erhalten — Pylonen bewegen sich nicht, und eine begonnene Durchfahrt
    /// wird durch vorübergehend fehlende Wahrnehmung nicht ungeschehen.
    /// Für einen echten Neustart siehe resetRun().
    void reset();

    /// Kompletter Neustart des Laufs (bei init_request): reset() plus
    /// Pylonen-Gedächtnis, Durchfahrt-Latches und Trigger-Latch.
    void resetRun();

    /// Aktuelle Fahrzeugposition einspeisen, einmal pro Request.
    ///
    /// Verfolgt rein geometrisch, ob das Fahrzeug die konvexe Hülle aller
    /// gemerkten Pylonen betreten (corridor_entered_) und danach wieder um
    /// mindestens hull_exit_margin_m verlassen hat (corridor_finished_).
    void updateVehiclePosition(const Vec2& car_xy);

    /// Fahrzeug hat die Pylonen-Hülle betreten (gelatcht).
    bool corridorEntered() const { return corridor_entered_; }

    /// Fahrzeug hat die Hülle betreten UND wieder verlassen (gelatcht) —
    /// die Durchfahrt gilt als abgeschlossen.
    bool corridorFinished() const { return corridor_finished_; }

    /// Anzahl der insgesamt gemerkten Pylonen. Für Logging von außen, ohne
    /// die interne Struktur nach außen zu reichen.
    size_t memoryConeCount() const { return cone_memory_.size(); }

    /// Lesezugriff auf das Pylonen-Gedächtnis.
    ///
    /// TrajectoryManagerGlobal nutzt es, um schon VOR dem Trigger vor jeder
    /// bekannten Pylone abzubremsen. Über das Gedächtnis statt über den
    /// aktuellen Frame, damit ein kurzer Wahrnehmungsausfall die Bremszone
    /// nicht flackern lässt.
    const std::vector<Cone>& memoryCones() const { return cone_memory_; }

    /// Konfiguration, veränderbar für Laufzeit-Tuning über ROS-Parameter.
    PipelineConfig& config();
    const PipelineConfig& config() const;

    /// Die aktuellen Pylonen als Kreishindernisse.
    ///
    /// Öffentlich, weil ConeCorridorPlanner::plan() damit prüft, ob ein
    /// zuvor gültiger Plan gegen den JETZIGEN Pylonenstand noch
    /// kollisionsfrei ist.
    std::vector<Obstacle> buildObstacles() const;

    // ────────────────────────────────────────────────────────────────
    //  Erweiterungspunkte: Policy, keine Geometrie
    // ────────────────────────────────────────────────────────────────

    /// Was gilt als Ausreißer? Default: Isolations- und Abstandsfilter aus
    /// cone_corridor.hpp.
    virtual std::vector<Cone> filterCones(const std::vector<Cone>& raw) const;

    /// Was gilt als Trigger-Pylone? Default: Abstand zur Fahrspur.
    virtual bool detectTrigger(const std::vector<Cone>& cones) const;

    virtual ~CorridorPipeline() = default;

private:
    PipelineConfig         cfg_;
    CorridorState          state_ = CorridorState::IDLE;

    std::vector<Cone>      current_cones_;
    std::vector<Vec2>      lane1_pts_;

    // Zuletzt ROH (vor filterCones()) gesehenes Array. Ist das nächste
    // identisch, wird der Zyklus übersprungen — siehe onConesReceived().
    std::vector<Cone>      last_raw_cones_;
    std::optional<PlanResult> current_result_;
    std::optional<Cone>    current_trigger_cone_;

    bool replan_needed_ = false;

    // Hysterese für die Voronoi-Zweigwahl: merkt, ob der letzte Zyklus
    // Voronoi gewählt hat, damit die Schwelle als Schmitt-Trigger statt als
    // flackernde Einzelschwelle wirkt.
    bool voronoi_active_ = false;

    // ── Pylonen-Gedächtnis ──────────────────────────────────────────
    // Pylonen bewegen sich nicht: einmal gesehen bleiben sie für den Rest
    // des Laufs bekannt, auch wenn sie längst aus dem Sichtfeld sind.
    //
    // Ohne diese Persistenz verlor die Pipeline mitten im Korridor die
    // Trigger-Pylone aus dem aktuellen Frame. Folge: reset(), und es wurde
    // wieder die rohe Kartenlinie mitten durch die Pylonen ausgegeben. Oder
    // die Kette schrumpfte auf das Sichtfeld zusammen und das Fahrzeug
    // bremste auf einen vermeintlichen Endpunkt mitten im Korridor ab.
    //
    // Zuordnung neuer Messungen über cone_memory_match_dist_m. Die ids
    // werden hier stabil vergeben, weil Perception-Indizes je Frame wechseln.
    std::vector<Cone>      cone_memory_;
    int                    next_memory_id_ = 0;
    bool                   corridor_entered_  = false;
    bool                   corridor_finished_ = false;

    // Einmal ausgelöst, bleibt der Korridor für den Rest des Laufs aktiv:
    // ab dem ersten erfolgreichen Trigger verhindert dieses Latch jeden
    // späteren Rückfall nach IDLE. Nur resetRun() löscht es.
    // Begründung siehe onConesReceived().
    bool                   trigger_latched_ = false;

    // Der Übergang von der Fahrspur in den Korridor wird EINMAL pro Lauf
    // geplant und bleibt danach unverändert — auch dann, wenn eine neu
    // erkannte Pylone ihn inzwischen schneiden würde.
    //
    // Warum eingefroren: die Mittellinie verändert sich mit jeder neuen
    // Pylone leicht. Würde der Übergang mitgeplant, wanderte der Abzweig-
    // punkt auf der Fahrspur unter dem Fahrzeug hin und her.
    //
    // Ein kollidierender Übergang wird trotzdem nicht blind gefahren: die
    // abschließende Kollisionsprüfung des Gesamtpfads schlägt weiterhin an
    // und ConeCorridorPlanner::plan() fährt dann den letzten guten Plan
    // weiter. Nur der Übergang selbst wird nicht mehr neu berechnet.
    //
    // Von allen Zweigen über acquireTransition() genutzt sowie direkt vom
    // Dreieck-Bootstrap: ein Zweigwechsel darf den gefundenen Übergang nicht
    // verwerfen, nur der nachfolgende Teil darf sich weiterentwickeln.
    // In reset() geleert — ein neuer Lauf darf keinen Übergang eines längst
    // verlassenen Korridors wiederverwenden.
    std::optional<TransitionResult> cached_transition_;

    // Nur für den Dreieck-Bootstrap: dort fällt die Zielpose mit dem
    // Übergangsziel zusammen (bei allen anderen Zweigen ist target_pose nur
    // noch Diagnose-Metadatum). Würde nur der Übergang eingefroren, liefe er
    // auf eine jeden Zyklus neu berechnete Zielpose zu. Deshalb wird beides
    // gemeinsam gesetzt und gemeinsam geleert.
    std::optional<TargetResult> cached_bootstrap_target_;

    // Die zuletzt publizierte Korridor-Mittellinie. Wird jeden Zyklus mit
    // der frisch berechneten überblendet, damit ein durch neue Pylonen
    // anders gruppiertes Ergebnis nicht hart unter dem Fahrzeug springt —
    // während runPipeline() weiterhin jeden Zyklus komplett neu plant.
    // Leer = erster Zyklus dieses Laufs, kein Blend.
    std::vector<Vec2> last_centerline_body_;

    /// Überblendet `raw` mit der Mittellinie des Vorzyklus, sofern eine
    /// existiert und das Blending nicht per Konfiguration abgeschaltet ist.
    std::vector<Vec2> blendBodyIfAvailable(const std::vector<Vec2>& raw) const;

    /// Wie oben für Posen. Die Headings werden aus der geblendeten
    /// xy-Folge neu bestimmt, weil sich die Positionen verschieben.
    std::vector<Pose> blendBodyIfAvailable(const std::vector<Pose>& raw) const;

    /// Nach einem erfolgreichen Zyklus aufzurufen: legt die Mittellinie
    /// dieses Zyklus als Blend-Grundlage für den nächsten ab.
    void commitCenterlineBody(const std::vector<Vec2>& xy);
    void commitCenterlineBody(const std::vector<Pose>& path);

    /// Liefert den Übergang von der Fahrspur zur Mittellinie.
    ///
    /// Cache-Treffer: der gespeicherte Übergang kommt unverändert zurück
    /// (used_cache = true), nur `entry_index` wird auf die nächstgelegene
    /// Stelle der AKTUELLEN Mittellinie neu verankert. Der Übergang endet
    /// dadurch nicht mehr zwingend exakt auf lane_poses[entry_index] — eine
    /// kleine Naht ist möglich und wird von der Gesamtpfad-Kollisionsprüfung
    /// mit abgedeckt.
    ///
    /// Kein Cache: erstmalige Suche per planTransitionShiftedEntry(), das
    /// Ergebnis wird gecacht und ist ab dann für den Rest des Laufs bindend.
    TransitionResult acquireTransition(const std::vector<Vec2>& p1,
                                        const std::vector<double>& t1,
                                        const std::vector<Pose>& lane_poses,
                                        const std::vector<Obstacle>& obstacles,
                                        size_t& entry_index,
                                        bool& used_cache);

    /// Pflegt die gefilterten Pylonen dieses Frames ins Gedächtnis ein:
    /// Position bei Wiedererkennung aktualisieren, sonst neu aufnehmen.
    void mergeConesIntoMemory(const std::vector<Cone>& filtered);

    /// Schritt 2 — Fahrvektorkette entlang der erkannten Reihe.
    ///
    /// Einstiegspunkt (Schnitt der Reihengeraden mit der Fahrspur) ->
    /// Fahrvektoren -> Hermite-Pfad mit lokaler Reparatur -> Verlängerung
    /// über das Kettenende hinaus -> Übergang -> Gesamtpfad +
    /// Kollisionsprüfung.
    ///
    /// Schreibt `pr` und committet bei Erfolg an current_result_.
    /// false = gescheitert; runPipeline() markiert dann FAILED, und
    /// ConeCorridorPlanner::plan() fährt den letzten gültigen Plan weiter
    /// oder lässt die Basislinie unverändert.
    bool planCorridorChain(const RowClassification& rc, const std::vector<Obstacle>& obstacles,
                            PlanResult& pr);

    /// Schritt 1 — Voronoi/Delaunay-Mittellinie über alle bekannten Pylonen.
    ///
    /// buildCenterlinePath() -> Eintrittspose -> Übergang -> Gesamtpfad +
    /// Kollisionsprüfung. Verhalten bei Fehlschlag wie planCorridorChain().
    bool planCorridorVoronoi(const Cone& trigger, const std::vector<Obstacle>& obstacles,
                              PlanResult& pr);

    /// Schritt 1.5 — Voronoi auf einer virtuellen Gegenreihe.
    ///
    /// Ausgangslage: außer der Trigger-Kette ist genau EINE Gegenpylone
    /// bekannt. Zu wenig für Schritt 1, aber genug, um per
    /// mirrorChainThrough() eine vollständige virtuelle Gegenreihe zu
    /// konstruieren.
    ///
    /// In die Voronoi-Eingabe gehen nur die virtuelle Reihe und die
    /// Trigger-Kette ein, NICHT die reale Gegenpylone selbst: sie fällt
    /// geometrisch mit der virtuellen Trigger-Kopie zusammen und würde die
    /// Triangulation destabilisieren.
    ///
    /// Die Kollisionsprüfung läuft trotzdem gegen die ECHTEN Pylonen — die
    /// virtuelle Reihe ist reine Rechengrundlage, kein Hindernis.
    bool planCorridorVirtualMirror(const Cone& trigger, const RowClassification& rc,
                                    const std::vector<Obstacle>& obstacles,
                                    PlanResult& pr);

    /// Schritt 0.5 — nur die vordere Reihe ist als Reihe sichtbar.
    ///
    /// Spiegelfall zu planCorridorChain(): die hintere Reihe ist bloß durch
    /// die noch isolierte Trigger-Pylone vertreten. classifyRows() hat die
    /// Rollen dann vertauscht (siehe RowClassification::trigger_row_isolated).
    ///
    /// Die Fahrvektoren kommen aus der vorderen Reihe, versetzt um den halben
    /// Abstand C <-> Trigger. Ab da identisch zu planCorridorChain().
    ///
    /// Muss VOR Schritt 1.5 geprüft werden: dort ist die einzelne
    /// "Gegenpylone" die Trigger-Pylone selbst, mirrorChainThrough() misst
    /// dann lateral == 0 und legt die virtuelle Reihe exakt auf die vordere —
    /// die Voronoi-Eingabe entartet zu doppelten Punkten.
    bool planCorridorFrontRowOnly(const Cone& trigger, const RowClassification& rc,
                                   const std::vector<Obstacle>& obstacles,
                                   PlanResult& pr);
};

}  // namespace corridor

// ════════════════════════════════════════════════════════════════════
//  2. ConeCorridorPlanner — Adapter zum globalen Planner
// ════════════════════════════════════════════════════════════════════

namespace planning {

// Die beiden Korridor-Geschwindigkeiten, an genau einer Stelle definiert.
//
// mapResultToBase() stempelt kCorridorExitCreepSpeed auf den Auslauf hinter
// dem Korridor. applyCorridorFixedSpeed() (trajectory_manager_global.cpp)
// erkennt genau diesen Wert wieder und weiß dadurch, wo der Auslauf beginnt —
// alles davor bekommt kCorridorSpeed.
//
// Beide Stellen hatten früher eigene lokale Kopien, die unbemerkt
// auseinanderliefen. Eine gemeinsame Definition macht das unmöglich.
inline constexpr double kCorridorSpeed = 0.5;           // m/s
inline constexpr double kCorridorExitCreepSpeed = 0.35;  // m/s

class ConeCorridorPlanner {
public:
    corridor::CorridorPipeline& pipeline() { return pipeline_; }
    const corridor::CorridorPipeline& pipeline() const { return pipeline_; }

    /// Grobe Vorauswahl: ist überhaupt eine Pylone sichtbar?
    ///
    /// Bewusst weit gefasst. Ob eine davon eine echte TRIGGER-Pylone ist,
    /// entscheidet erst die Pipeline in plan() — siehe triggerActive().
    bool isApplicable(const PlannerRequestGlobal& request) const;

    /// Wurde in diesem Lauf eine echte Trigger-Pylone erkannt?
    ///
    /// TrajectoryManagerGlobal entscheidet damit NACH plan(), ob der
    /// Korridor-Modus wirklich übernimmt. Normale Pylonen im Sichtfeld
    /// dürfen ein laufendes Überholmanöver nicht kapern.
    bool triggerActive() const;

    /// Ein Planungszyklus. Gibt die Basislinie mit ersetztem Korridorstück
    /// zurück — oder unverändert, wenn kein Plan zustande kommt.
    std::vector<PointGlobal> plan(
        const std::vector<PointGlobal>& base,
        const std::vector<PointGlobal>& cones) const;

    /// Klartext-Erklärung, wie plan() in diesem Zyklus zu seinem Ergebnis
    /// gekommen ist: frischer Plan, letzter gültiger Plan weiterverwendet,
    /// oder Basislinie unverändert gelassen — jeweils mit Grund.
    ///
    /// Rein diagnostisch, fürs Debug-Dashboard. Beantwortet die Frage
    /// "warum wurde dieser Pfad nicht publiziert", die PlanResult::fail_reason
    /// allein nicht beantwortet: fail_reason beschreibt nur den frischen
    /// Versuch, nicht was plan() daraus gemacht hat.
    const std::string& lastDecisionNote() const { return last_decision_note_; }

private:
    // mutable, weil plan() const ist (Vorgabe von TrajectoryManagerGlobal),
    // die Pipeline ihren Zustand aber über mehrere Aufrufe pflegt.
    mutable corridor::CorridorPipeline pipeline_;

    // Der letzte erfolgreiche Plan dieses Laufs.
    //
    // Scheitert ein späterer Zyklus, wird dieser Plan weitergefahren —
    // sofern er gegen die aktuellen Pylonen noch kollisionsfrei ist.
    //
    // Gibt es keinen brauchbaren alten Plan, bleibt die Basislinie diesen
    // Zyklus unverändert. Es gibt bewusst KEINEN Brems- oder Halt-Fallback:
    // die Trigger-Pylone liegt per Definition nah an dieser Linie, ein
    // Anhalten darauf führte selbst in die Pylone.
    //
    // Geleert, sobald die Pipeline wieder IDLE wird — ein neuer Korridor
    // darf keinen Plan eines längst verlassenen wiederverwenden.
    mutable std::optional<corridor::PlanResult> last_good_result_;

    // Fortschrittsindex der Fahrspur: der zum Pylonen-Schwerpunkt nächste
    // Punkt auf `base` darf pro Lauf nur vorwärts laufen.
    //
    // Ohne diese Sperre kann eine schwächer besetzte Wahrnehmung die
    // Fahrspur auf ein kurzes Präfix zurückschrumpfen lassen — die längst
    // erkannte Trigger-Pylone fällt dann wieder aus trigger_lane_distance
    // heraus und die Trigger-Erkennung schlägt fehl.
    //
    // last_base_size_ erkennt den Wechsel auf eine andere `base`-Indexierung
    // (der eingefrorene Kartenausschnitt) und setzt den Fortschritt zurück.
    mutable size_t max_lane1_ci_ = 0;
    mutable size_t last_base_size_ = 0;

    // Siehe lastDecisionNote(). Bei jedem plan()-Aufruf neu gesetzt.
    mutable std::string last_decision_note_;

    /// Stempelt den berechneten Korridorpfad zurück in die Basislinie.
    /// Die Punktzahl bleibt dabei erhalten.
    static std::vector<PointGlobal> mapResultToBase(
        const std::vector<PointGlobal>& base,
        const corridor::PlanResult& result);
};

}  // namespace planning
