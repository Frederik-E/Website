#pragma once

#include <chrono>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <Eigen/Dense>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>



namespace kal_trajectory_planner {

using Duration = std::chrono::duration<double, std::ratio<1>>;
using Time = std::chrono::time_point<std::chrono::steady_clock, Duration>;

using Position = Eigen::Vector2d;             // 2-D Kartesischer Punkt [m, m]
using Path = std::vector<Position>;           // Positionsfolge ohne Zeitstempel
struct StampedPosition {
    Position position;
    Time stamp;                               // monotoner Zeitstempel (steady_clock)
};
using Trajectory = std::vector<StampedPosition>;  // zeitgestempelte Positionsfolge

using Pose = Eigen::Isometry2d;              // SE(2)-Pose: Rotation + Translation in der Ebene

} // namespace kal_trajectory_planner


// ============================================================================
namespace corridor {

using Vec2 = Eigen::Vector2d;  // 2-D Vektor [m, m]

// ────────────────────────────────────────────────────────────────────
//  Geometrie-Primitive
// ────────────────────────────────────────────────────────────────────

struct Pose {
    double x  = 0.0;  // [m]
    double y  = 0.0;  // [m]
    double th = 0.0;  // Heading [rad]
};

// Kreisfoermiger Fussabdruck fuer die Kollisionspruefung.
struct Obstacle {
    double cx = 0.0;  // Mittelpunkt X [m]
    double cy = 0.0;  // Mittelpunkt Y [m]
    double r  = 0.0;  // Radius [m]
};

struct Cone {
    Vec2   pos{0.0, 0.0};
    double radius = 0.15;   // physischer Pylonenradius [m]
    int    id     = -1;     // Tracking-ID von Perception; -1 = unbekannt
};

// Vehicle geometry. Tuning reference: README.md.
struct VehicleParams {
    double R_min      = 0.6;   // minimaler Wenderadius [m]
    double length     = 0.4;   // Karosserie-Laenge [m]
    double width      = 0.3;   // Karosserie-Breite [m]
    double rear_overhang_frac = 0.05;  // Hinterueberhang als Anteil der Fahrzeuglänge [0..1]
    double clearance  = 0.05;  // seitlicher Sicherheitsabstand zu Hindernissen [m]
};

// Wall-following chain planner + Frenet-lattice alternative. Tuning reference: README.md.
struct LatticeConfig {
    double default_half_width_m       = 0.5;   // angenommene halbe Spurbreite ohne Wandinfo [m]
    double half_width_smoothing       = 0.15;  // Glaettungsfaktor fuer Spurbreite [m]
    double reference_blend_distance_m = 0.6;   // Ueberblendabstand zur Referenzlinie [m]
    int    min_wall_chain_len         = 2;      // min. Wandpunkte fuer Chain-Planner
    int    n_samples                  = 200;   // Anzahl Abtastpunkte auf der Referenzlinie

    double single_row_offset_m  = 0.45;        // seitlicher Versatz bei einseitiger Wand [m]
    double rear_wall_clearance_m = 0.45;       // Mindestabstand zur Hinterwand [m]

    int    n_lateral_candidates = 7;           // Anzahl lateraler Kandidatenpfade
    double lateral_margin_m     = 0.12;        // laterale Schrittweite zwischen Kandidaten [m]
    std::vector<double> horizons_m{2.0};       // Planungshorizonte [m]
    double candidate_ds         = 0.05;        // Abtastschrittweite der Kandidaten [m]

    double wall_clearance_min_m = 0.05;   // absoluter Mindestabstand zur Wand [m]
    double w_wall_offset  = 1.0;          // Kostengewicht: Wandabstand
    double w_curvature    = 1.0;          // Kostengewicht: Kruemmung
    double w_hysteresis   = 2.0;          // Kostengewicht: Pfadstabilitaet (Hysterese)
    double w_exploration  = 0.0;          // Kostengewicht: Erkundung (normalerweise 0)
};

// Fixed-distance row classifier. Tuning reference: README.md.
struct RowClassifierConfig {
    double same_side_max_dist_m  = 0.50;  // max. Abstand zweier Cones auf gleicher Seite [m]
    double cross_side_min_dist_m = 0.60;  // min. Abstand fuer Zuordnung zur gegenueberliegenden Seite [m]
};

// Voronoi/Delaunay centerline dispatch + smoothing. Tuning reference: README.md.
struct VoronoiConfig {
    int min_total_cones                      = 4;   // min. Cones fuer Voronoi-Berechnung
    int min_cones_outside_trigger_chain      = 2;   // min. Cones ausserhalb der Trigger-Kette (Einfahrt)
    int min_cones_outside_trigger_chain_exit = 1;   // min. Cones ausserhalb der Trigger-Kette (Ausfahrt)

    double min_front_chain_ratio_enter = 0.7;  // min. Anteil vorderer Cones zum Aktivieren
    double min_front_chain_ratio_exit  = 0.5;  // min. Anteil vorderer Cones zum Beibehalten

    int    smooth_outer_iters    = 6;      // Laplacian-Glaettung: aeussere Iterationen
    int    smooth_inner_iters    = 4;      // Laplacian-Glaettung: innere Iterationen
    double smooth_alpha          = 0.5;    // Laplacian-Glaettung: Schrittweite [0..1]
    double recenter_pull         = 0.5;    // Zurueckzieh-Staerke zur Mittellinie [0..1]
    int    smooth_curv_extra_max = 40;     // extra Glaettungsschritte bei hoher Kruemmung
};

// Cone-corridor pipeline configuration. Tuning reference: README.md.
struct PipelineConfig {
    VehicleParams vehicle;

    double cone_default_radius = 0.10;  // Fallback-Pylonenradius falls Perception keinen liefert [m]
    double resample_ds         = 0.05;  // Abtastschrittweite fuer alle Pfade [m]

    double phase1_extra_clearance = 0.05;                      // zusaetzlicher Abstand in Phase 1 [m]
    double phase1_forward_dir_min_agreement = 0.0;             // min. cos(angle) zur Vorwaertsrichtung
    double phase1_transition_heading_tolerance_rad = 0.5;      // max. Heading-Fehler beim Uebergang [rad]

    LatticeConfig lattice;
    RowClassifierConfig row_classifier;
    VoronoiConfig voronoi;

    double transition_radius_growth     = 1.35;   // Wachstumsfaktor fuer Kurvenradius im Uebergang
    double transition_max_radius_factor = 12.0;   // max. Vielfaches des Minimalradius
    double transition_max_length_m = 2.0;         // Soft-Laengenlimit des Uebergangspfads [m]
    double transition_max_departure_distance_m = 3.0;  // Hard-Limit: max. Abstand vom Abfahrtpunkt [m]
    int    transition_heading_sweep_steps = 3;    // Schritte beim Heading-Sweep fuer Pfadsuche
    double transition_obstacle_padding = 0.06;    // Padding bei Hindernispruefung im Uebergang [m]
    double transition_min_clearance_m = 0.02;     // min. Abstand zur Wand nach Uebergang [m]
    double transition_entry_shift_max_m  = 0.0;   // max. laterale Verschiebung des Einstiegspunkts [m]
    double transition_entry_shift_step_m = 0.05;  // Schrittweite der Einstiegspunkt-Verschiebung [m]
    double centerline_blend_distance_m = 1.5;     // Ueberblendabstand zur Mittellinie am Eingang [m]

    double transition_max_turn_rad = 3.9;          // Hard-Limit: max. kumulierter |Drehwinkel| eines
                                                    // Uebergangskandidaten [rad] (~225 Grad) -- verwirft
                                                    // schleifenartige Kandidaten unabhaengig von ihrer Laenge
    double transition_tight_radius_factor = 0.4;  // letzte Rueckfallstufe: Radius-Faktor < 1.0 fuer einen
                                                    // kleinen, eng gefahrenen Bereich statt einer Schleife --
                                                    // nur versucht, wenn keine R_min-Loesung existiert
    double transition_tight_max_length_m = 2.0;    // Laengendeckel fuer die abgesenkte-Radius-Stufe [m] --
                                                    // haelt sie auf einen kleinen Bereich beschraenkt

    bool allow_colliding_path = true;  // Pfad trotz erkannter Kollision publizieren (Fallback)

    double corridor_end_extension_m  = 0.2;   // Verlängerung des Korridors am Ende [m]
    double hull_exit_margin_m        = 0.15;  // Randabstand zur konvexen Huelle beim Austritt [m]
    double cone_memory_match_dist_m  = 0.20;  // max. Abstand zum Matchen von Cones im Gedaechtnis [m]
    double trigger_lane_distance     = 0.2;   // max. Abstand eines Cones zur Spur, um als Trigger zu gelten [m]
    double outlier_spacing_factor    = 4.0;   // Vielfaches des typ. Pylonenabstands -> Ausreisser
    double isolation_max_dist_factor = 5.0;   // Vielfaches des Pylonenradius -> isolierter Cone
};

// Ergebnis der Trigger-Zielpunkt-Bestimmung (TargetSelector).
struct TargetResult {
    bool ok = false;               // Zielpunkt erfolgreich bestimmt
    Cone trigger;                  // der ausloesende Trigger-Cone
    std::optional<Cone> cone_b;    // zweiter Begrenzungs-Cone (falls vorhanden)
    std::optional<Cone> cone_c;    // dritter Begrenzungs-Cone (falls vorhanden)
    Pose target_pose;              // Zielpose am Korridoreingang
    std::string method;            // verwendete Methode (fuer Logging)
    bool gap_too_narrow = false;   // true, wenn Luecke zu schmal fuer das Fahrzeug
};

// Ergebnis der Mittellinienberechnung (Voronoi/Delaunay).
struct CenterlineResult {
    bool ok = false;
    std::vector<Vec2> raw_chain;              // rohe Voronoi-Kette vor der Glaettung
    std::vector<Vec2> path;                   // geglaettete, zentrierte Mittellinie
    double kappa_max = 0.0;                   // maximale Kruemmung entlang der Mittellinie [1/m]
    bool   curv_ok   = false;                 // true, wenn kappa_max das Fahrzeuglimit einhält
    bool used_raw_vertex_fallback = false;    // true, wenn rohe Vertices statt Voronoi verwendet wurden
};

// Ergebnis des Uebergangspfad-Planers (Einfahrt in den Korridor).
struct TransitionResult {
    bool ok = false;
    int  departure_index = 0;          // Index auf der resampled Spur 1, ab dem abgebogen wird
    std::vector<Pose> poses;           // abgetasteter Uebergangspfad (x, y, theta)
    std::string method;                // Bezeichnung der genutzten Planerstufe (fuer Logging)
    std::string fail_reason;           // failure cause on unsuccessful attempt (for logging, like PlanResult)
    double heading_offset_used_rad = 0.0;  // tatsaechlich verwendeter Heading-Offset [rad]
    double clearance_used_m = 0.0;         // tatsaechlich eingehaltener Wandabstand [m]
};

// Gesamtergebnis der Cone-Corridor-Pipeline.
struct PlanResult {
    bool ok             = false;
    bool collision_free = false;       // true, wenn der Pfad alle Hindernisse freilaesst
    int  hit_idx        = -1;          // erster Kollisionsindex; -1 = keine Kollision
    bool corridor_too_narrow = false;  // true, wenn die Luecke zu schmal fuer das Fahrzeug ist

    TargetResult      target;
    CenterlineResult  centerline;
    TransitionResult  transition;
    Pose              entry_pose;      // Eintrittspunkt in die Korridor-Mittellinie

    std::vector<Vec2>  full_xy;        // Gesamtpfad als (x, y)-Punkte
    std::vector<Pose>  full_poses;     // Gesamtpfad als (x, y, theta)-Posen

    std::string fail_reason;           // Fehlerursache im Misserfolgsfall (fuer Logging)
};

// Zustand der Korridor-State-Machine (CorridorPipeline).
enum class CorridorState {
    IDLE,              // kein Korridor aktiv
    TRIGGER_DETECTED,  // Trigger-Pylone erkannt, Pfad noch nicht geplant
    PLANNING,          // Pipeline wird ausgefuehrt
    ACTIVE,            // gueltiger Pfad vorhanden und aktiv
    REPLAN_NEEDED,     // neues Cone-Array macht Neuplanung erforderlich
    FAILED,            // Pipeline gescheitert — Fallback aktiv
};

}  // namespace corridor
