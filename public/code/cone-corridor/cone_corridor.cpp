// ============================================================================
//  Fassung fuer die Projektseite.
//
//  Inhaltlich identisch mit dem Stand aus dem Fahrzeug-Repository, aber auf
//  Lesbarkeit hin ueberarbeitet:
//    - classifyRows() und computeDrivingVectorsFromChain() sind in benannte
//      Schritte bzw. in die beiden Faelle "beide Reihen" und "eine Reihe"
//      zerlegt, statt als 160 bzw. 205 Zeilen am Stueck zu stehen.
//    - Kuerzel wie cr, vg, P, km, cok, si tragen jetzt sprechende Namen.
//    - Die Begruendungen aus dem Original sind vollstaendig erhalten; sie sind
//      der eigentliche Wert dieser Datei.
//
//  Geprueft mit: g++ -fsyntax-only -std=c++17 -I include -I /usr/include/eigen3
// ============================================================================

// ============================================================================
//  cone_corridor.cpp — Korridor-Algorithmik (Zielvektor + Kollision + Uebergang)
//
//  Inhalt:
//    1. TARGET-VEKTOR
//       1.1 Polylinien-Werkzeuge (resample, tangents, convexHull, ...)
//       1.2 Trigger-Erkennung + Dreieck-Zielvektor (Phase 1: <=3 Pylonen)
//       1.3 Spur 1 fuer die Uebergangssuche (Verlaengerung + Kollisions-Schnitt)
//    2. COLLISION — bodyClear(), firstCollisionIndex(), pathStillClear()
//    3. TRANSITION
//       3.1 Dubins-Worttypen (LSL, RSR, LSR, RSL, RLR, LRL)
//       3.2 Kandidaten-Erzeugung + Sampling
//       3.3 planTransition() — Stufe 1 (Biarc) + Stufe 2 (Dubins-Rueckfallebene)
//
//  Abschnitt 1 ersetzt die vormalige Voronoi/Delaunay-Mittellinie durch die
//  Dreieck-Heuristik aus cone_corridor_phase_planner.py (Phase 1: Trigger-
//  Pylone + bis zu 2 weitere sichtbare Pylonen -- siehe computeTriangleTarget()
//  weiter unten). Abschnitte 2+3 sind UNVERAENDERT: der Dubins-/Kollisions-
//  Kern wird von der neuen Zielvektor-Berechnung genauso wiederverwendet wie
//  vorher von der Mittellinie.
// ============================================================================

#include "kal_trajectory_planner/cone_corridor.hpp"

#include <Eigen/Dense>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <queue>
#include <set>
#include <unordered_set>

namespace corridor {

// ════════════════════════════════════════════════════════════════════
//  1. TARGET-VEKTOR
// ════════════════════════════════════════════════════════════════════

// ────────────────────────────────────────────────────────────────────
//  1.1 Polylinien-Werkzeuge
// ────────────────────────────────────────────────────────────────────

//ensuring, that all points of the curvature have the same distance
std::vector<Vec2> resample(const std::vector<Vec2>& pts, double ds) {
    if (pts.size() < 2) return pts;
    std::vector<double> s(pts.size(), 0.0);
    for (size_t i = 1; i < pts.size(); ++i)
        s[i] = s[i - 1] + (pts[i] - pts[i - 1]).norm();
    double total = s.back();
    if (total < 1e-9) return pts;
    int n = std::max(2, static_cast<int>(total / ds));
    std::vector<Vec2> out(n);
    for (int k = 0; k < n; ++k) {
        double sk = total * k / (n - 1);
        auto it = std::upper_bound(s.begin(), s.end(), sk);
        size_t j = static_cast<size_t>(std::distance(s.begin(), it));
        if (j == 0) j = 1;
        if (j >= pts.size()) j = pts.size() - 1;
        double seg = s[j] - s[j - 1];
        double t = (seg > 1e-12) ? (sk - s[j - 1]) / seg : 0.0;
        out[k] = (1.0 - t) * pts[j - 1] + t * pts[j];
    }
    return out;
}
//calculating the tangent angle at each point of the curvature, using a smoothing window of size "smooth"
std::vector<double> tangents(const std::vector<Vec2>& poly, int smooth) {
    int n = static_cast<int>(poly.size());
    std::vector<double> th(n, 0.0);
    for (int i = 0; i < n; ++i) {
        int i0 = std::max(0, i - smooth);
        int i1 = std::min(n - 1, i + smooth);
        Vec2 d = poly[i1] - poly[i0];
        th[i] = std::atan2(d.y(), d.x());
    }
    return th;
}

std::vector<Vec2> convexHull(std::vector<Vec2> pts) {
    size_t n = pts.size();
    if (n < 3) return pts;
    std::sort(pts.begin(), pts.end(), [](const Vec2& a, const Vec2& b) {
        return a.x() < b.x() || (a.x() == b.x() && a.y() < b.y());
    });
    auto cross = [](const Vec2& O, const Vec2& A, const Vec2& B) -> double {
        return (A.x() - O.x()) * (B.y() - O.y()) -
               (A.y() - O.y()) * (B.x() - O.x());
    };
    std::vector<Vec2> hull(2 * n);
    int k = 0;
    for (size_t i = 0; i < n; ++i) {
        while (k >= 2 && cross(hull[k - 2], hull[k - 1], pts[i]) <= 0) --k;
        hull[k++] = pts[i];
    }
    for (int i = static_cast<int>(n) - 2, t = k + 1; i >= 0; --i) {
        while (k >= t && cross(hull[k - 2], hull[k - 1], pts[i]) <= 0) --k;
        hull[k++] = pts[i];
    }
    hull.resize(k - 1);
    return hull;
}

bool pointInConvexHull(const Vec2& p, const std::vector<Vec2>& hull) {
    size_t n = hull.size();
    if (n < 3) return false;
    for (size_t i = 0; i < n; ++i) {
        const Vec2& a = hull[i];
        const Vec2& b = hull[(i + 1) % n];
        double c = (b.x() - a.x()) * (p.y() - a.y()) -
                   (b.y() - a.y()) * (p.x() - a.x());
        if (c < -1e-10) return false;
    }
    return true;
}

double minDistToPolyline(const Vec2& pt, const std::vector<Vec2>& polyline) {
    double best = 1e18;
    for (size_t i = 0; i + 1 < polyline.size(); ++i) {
        Vec2 ab = polyline[i + 1] - polyline[i];
        double len2 = ab.squaredNorm();
        double t = (len2 > 1e-12)
            ? std::clamp((pt - polyline[i]).dot(ab) / len2, 0.0, 1.0)
            : 0.0;
        double d = (pt - (polyline[i] + t * ab)).norm();
        best = std::min(best, d);
    }
    return best;
}

std::vector<Pose> pathToPoses(const std::vector<Vec2>& path) {
    std::vector<Pose> poses(path.size());
    for (size_t i = 0; i < path.size(); ++i) {
        size_t i0 = (i > 0) ? i - 1 : 0;
        size_t i1 = (i + 1 < path.size()) ? i + 1 : i;
        Vec2 d = path[i1] - path[i0];
        poses[i] = {path[i].x(), path[i].y(), std::atan2(d.y(), d.x())};
    }
    return poses;
}

std::vector<Cone> filterOutlierCones(const std::vector<Cone>& raw, double spacing_factor) {
    if (raw.empty()) return raw;

    std::vector<double> nearest(raw.size(), std::numeric_limits<double>::max());
    for (size_t i = 0; i < raw.size(); ++i) {
        for (size_t j = 0; j < raw.size(); ++j) {
            if (i == j) continue;
            nearest[i] = std::min(nearest[i], (raw[i].pos - raw[j].pos).norm());
        }
    }

    std::vector<double> sorted_nearest = nearest;
    std::sort(sorted_nearest.begin(), sorted_nearest.end());
    const double median_spacing = sorted_nearest[sorted_nearest.size() / 2];
    const double max_allowed = median_spacing * spacing_factor;

    std::vector<Cone> filtered;
    filtered.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        if (nearest[i] <= max_allowed) {
            filtered.push_back(raw[i]);
        }
    }
    return filtered;
}

std::vector<Cone> filterIsolatedCones(const std::vector<Cone>& raw, const VehicleParams& vehicle,
                                       double max_isolation_dist) {
    if (raw.size() < 2) return raw;

    // decomposeChains() gruppiert ueber eine dichteadaptive Schwelle (siehe
    // Abschnitt 4.0) statt einer festen Distanz -- eine sparsame, aber
    // zusammenhaengende Reihe bildet dort i.d.R. weiterhin eine Kette der
    // Laenge >= 2 und bleibt so von diesem Filter verschont, auch wenn ihr
    // Reihenabstand max_isolation_dist einzeln uebersteigt. Nur wer WEDER
    // eine Kette bildet NOCH irgendeine andere Pylone in max_isolation_dist
    // hat, gilt als Fehldetektion.
    const auto chains = decomposeChains(raw, vehicle);

    std::vector<Cone> filtered;
    filtered.reserve(raw.size());
    for (const auto& chain : chains) {
        if (chain.size() > 1) {
            filtered.insert(filtered.end(), chain.begin(), chain.end());
            continue;
        }
        const Cone& lone = chain.front();
        double nearest = std::numeric_limits<double>::max();
        for (const auto& c : raw) {
            if (c.id == lone.id) continue;
            nearest = std::min(nearest, (lone.pos - c.pos).norm());
        }
        if (nearest <= max_isolation_dist) {
            filtered.push_back(lone);
        }
    }
    return filtered;
}

bool anyConeNearPolyline(const std::vector<Cone>& cones, const std::vector<Vec2>& lane1_pts,
                          double max_dist) {
    if (lane1_pts.size() < 2) return false;
    for (const auto& cone : cones) {
        if (minDistToPolyline(cone.pos, lane1_pts) <= max_dist) {
            return true;
        }
    }
    return false;
}

// ────────────────────────────────────────────────────────────────────
//  1.2 Trigger-Erkennung + Dreieck-Zielvektor (Phase 1: <=3 Pylonen)
// ────────────────────────────────────────────────────────────────────
//
//  Portierung von compute_triangle_target() aus cone_corridor_phase_planner.py.
//  Ersetzt die vormalige Voronoi/Delaunay-Mittellinie: statt einer Ketten-
//  Traversierung durch beliebig viele Pylonen wird aus GENAU der Trigger-
//  Pylone plus bis zu 2 weiteren ein einzelner Zielvektor bestimmt --
//  ausreichend fuer die allererste Cone-Corridor-Situation (2-3 sichtbare
//  Pylonen), in der noch keine Nachbarschaftsstruktur fuer eine Mittellinie
//  existiert.

const Cone* nearestConeTo(const std::vector<Cone>& pool, const Vec2& ref) {
    const Cone* best = nullptr;
    double best_d2 = std::numeric_limits<double>::infinity();
    for (const auto& c : pool) {
        const double d2 = (c.pos - ref).squaredNorm();
        if (d2 < best_d2) { best_d2 = d2; best = &c; }
    }
    return best;
}

const Cone& findTriggerCone(const std::vector<Cone>& cones, const std::vector<Vec2>& lane1_pts) {
    size_t best = 0;
    double bestDist = minDistToPolyline(cones[0].pos, lane1_pts);
    for (size_t i = 1; i < cones.size(); ++i) {
        double d = minDistToPolyline(cones[i].pos, lane1_pts);
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return cones[best];
}

TargetResult computeTriangleTarget(
    const Cone& trigger,
    const std::vector<Cone>& others,
    const Vec2& car_xy,
    const VehicleParams& vehicle,
    const Vec2& forward_dir,
    double extra_clearance)
{
    TargetResult r;
    r.trigger = trigger;
    if (others.empty()) {
        r.method = "no further cone visible -- no target point determinable";
        return r;
    }

    // B/C-Kandidaten auf die Fahrtrichtung einschraenken: nur Pylonen, die --
    // auf `forward_dir` projiziert -- nicht hinter der Trigger-Pylone liegen,
    // sind ueberhaupt B/C-Kandidaten. Sonst kann die reine Naechster-Nachbar-
    // Suche eine Pylone der gegenueberliegenden Reihe oder eine bereits
    // passierte, isolierte Pylone waehlen, die geometrisch hinter dem Trigger
    // liegt -- der Zielvektor (B.pos - trigger.pos) zeigt dann rueckwaerts.
    // forward_dir == 0 (keine verlaessliche Fahrtrichtung bekannt) deaktiviert
    // die Einschraenkung -- ALLE `others` bleiben Kandidaten (altes Verhalten).
    std::vector<Cone> candidates;
    const double fwd_norm2 = forward_dir.squaredNorm();
    if (fwd_norm2 > 1e-12) {
        const Vec2 fwd = forward_dir / std::sqrt(fwd_norm2);
        candidates.reserve(others.size());
        for (const auto& c : others) {
            const Vec2 rel = c.pos - trigger.pos;
            // Nur Pylonen verwerfen, die KLAR hinter der Trigger-Pylone
            // liegen (> ~110 Grad zur Fahrtrichtung). Die fruehere harte
            // Schwelle "Projektion > 0" verwarf auch die QUER
            // gegenueberliegende Pylone der anderen Reihe (Tor-Fall,
            // Projektion ~0, bei angewinkeltem Korridor leicht negativ) --
            // in der 2-Pylonen-Bootstrap-Situation blieb dann gar kein
            // Kandidat uebrig und es wurde nichts/falsch geplant.
            if (rel.dot(fwd) > -0.35 * rel.norm()) {
                candidates.push_back(c);
            }
        }
    } else {
        candidates = others;
    }
    if (candidates.empty()) {
        r.method = "no cone visible in driving direction -- no target point determinable";
        return r;
    }

    auto sameCone = [](const Cone& a, const Cone& b) {
        return (a.pos - b.pos).squaredNorm() < 1e-12;
    };

    // Reihenzugehoerigkeit ueber die RICHTUNG statt ueber Ketten: die beiden
    // Pylonenreihen verlaufen laengs des Korridors, die gegenueberliegende
    // Reihe liegt QUER vor der Trigger-Pylone. Eine Pylone gilt daher als
    // "quer" (= Gegenreihe), wenn die Richtung Trigger->Pylone mehr als 60 Grad
    // von der Fahrtrichtung abweicht, sonst als "laengs" (= eigene Reihe).
    //
    // Vorher entschied das decomposeChains()/adaptiveNeighborDist() -- also
    // eine ABSTANDSSCHWELLE (bei <= 3 Pylonen fix vehicle.width + 0.05 =
    // 0.45 m). Eine Pylone der eigenen Reihe 0.90 m voraus fiel damit aus der
    // Trigger-Kette, die Funktion hielt die eigene Reihe faelschlich fuer
    // unsichtbar und baute ein Tor zur Gegenpylone, statt die naechstgelegene
    // Pylone als B zu nehmen. Der Richtungstest kennt keine Distanz und kann
    // deshalb auch keine weit entfernte Reihenpylone "verlieren" -- genau die
    // Vorgabe der B-Heuristik unten.
    const Vec2 fwd_eff = (fwd_norm2 > 1e-12)
        ? (forward_dir / std::sqrt(fwd_norm2))
        : ((trigger.pos - car_xy).norm() > 1e-9
               ? (trigger.pos - car_xy).normalized()
               : Vec2(1.0, 0.0));
    constexpr double kAcrossCos = 0.5;   // 60 Grad
    auto isAcross = [&](const Cone& c) {
        const Vec2 rel = c.pos - trigger.pos;
        const double n = rel.norm();
        if (n < 1e-9) return false;
        return std::abs(rel.dot(fwd_eff) / n) < kAcrossCos;
    };

    // B := die der Trigger-Pylone am naechsten liegende Pylone ueberhaupt
    // (Heuristik-Vorgabe, keine Distanzschranke) -- siehe nearestConeTo().
    // `candidates` ist nur ueber die FAHRTRICHTUNG gefiltert (Pylonen klar
    // hinter dem Trigger scheiden aus, sonst zeigt der Zielvektor rueckwaerts);
    // das ist eine Richtungs-, keine Distanzeinschraenkung.
    const Cone cone_b = *nearestConeTo(candidates, trigger.pos);

    // ── Tor-Fall: die dichteste Pylone liegt QUER -- also ist von der eigenen
    // Reihe nichts sichtbar (typisch die allererste 2-Pylonen-Situation:
    // Trigger + eine Pylone quer gegenueber). Sie einfach als B zu nehmen waere
    // falsch: der Zielvektor (B - Trigger) zeigte dann quer durch den Korridor,
    // der Uebergang plante eine absurde Kurve und das Fahrzeug crashte in die
    // Trigger-Pylone. Richtig ist: die Luecke zwischen Trigger und Gegenpylone
    // ist das EINFAHRTSTOR -- Zielpunkt = Tor-Mittelpunkt, Zielausrichtung =
    // senkrecht zur Torlinie, in Fahrtrichtung orientiert. cone_b bleibt
    // bewusst leer: planTransition() soll hier direkt mit den Dubins-Stufen
    // (natuerliche Anfangsrichtung bevorzugt) einscheren, nicht um eine
    // Pivot-Pylone herumfahren.
    if (isAcross(cone_b)) {
        const Cone& gate_c = cone_b;
        const Vec2 across = gate_c.pos - trigger.pos;
        const double across_norm = across.norm();
        if (across_norm < 1e-6) {
            r.method = "trigger and opposite cone coincide";
            return r;
        }
        Vec2 gate_dir{-across.y() / across_norm, across.x() / across_norm};
        const Vec2 gate_mid = 0.5 * (trigger.pos + gate_c.pos);
        if (fwd_norm2 > 1e-12) {
            if (gate_dir.dot(forward_dir) < 0.0) gate_dir = -gate_dir;
        } else if (gate_dir.dot(gate_mid - car_xy) < 0.0) {
            gate_dir = -gate_dir;
        }

        r.ok = true;
        r.cone_c = gate_c;
        r.target_pose = {gate_mid.x(), gate_mid.y(), std::atan2(gate_dir.y(), gate_dir.x())};
        r.method = "target point = gate midpoint(trigger, C) -- no cone of own row visible";
        const double physical_gap = across_norm - trigger.radius - gate_c.radius;
        if (physical_gap < vehicle.width + 2.0 * vehicle.clearance) {
            r.method += "  [WARNING: gate is narrower than vehicle width+clearance]";
            r.gap_too_narrow = true;
        }
        return r;
    }

    // Ab hier steht fest: cone_b liegt LAENGS, ist also eine Pylone der eigenen
    // Reihe -- der Tor-Fall oben hat den Quer-Fall bereits abgefangen.

    // C := die dichteste QUER liegende Pylone (die gegenueberliegende Reihe),
    // cone_b ausgenommen. Derselbe Richtungstest wie beim Tor-Fall, damit B und
    // C nach EINER konsistenten Regel bestimmt werden; frueher entschied auch
    // hier die Kettenzugehoerigkeit, sodass eine weiter entfernte Gegenpylone
    // als C ausfallen konnte. Keine quer liegende Pylone sichtbar -> kein C
    // (faellt in den "nur B sichtbar"-Fall weiter unten, statt faelschlich eine
    // zweite Pylone DERSELBEN Reihe als C zu waehlen).
    std::vector<Cone> across_cones;
    across_cones.reserve(candidates.size());
    for (const auto& c : candidates) {
        if (!sameCone(c, cone_b) && isAcross(c)) across_cones.push_back(c);
    }
    const Cone* cone_c = nearestConeTo(across_cones, trigger.pos);

    Vec2 vec = cone_b.pos - trigger.pos;
    double vnorm = vec.norm();
    if (vnorm < 1e-6) {
        r.method = "trigger and B cone coincide";
        return r;
    }
    Vec2 direction = vec / vnorm;
    double theta = std::atan2(direction.y(), direction.x());

    Vec2 target_pos;
    std::string method;
    if (cone_c) {
        target_pos = 0.5 * (cone_b.pos + cone_c->pos);
        method = "target point = midpoint(B, C)";
        // Torbreite pruefen: ist B-C schmaler als das Fahrzeug, ist das Ziel
        // geometrisch nicht durchfahrbar -- nur eine Warnung, keine Korrektur.
        double bc_norm = (cone_c->pos - cone_b.pos).norm();
        double physical_gap = bc_norm - cone_b.radius - cone_c->radius;
        if (physical_gap < vehicle.width + 2.0 * vehicle.clearance) {
            method += "  [WARNING: B-C gap is narrower than vehicle width+clearance]";
            r.gap_too_narrow = true;
        }
    } else {
        // Nur B sichtbar: seitlich um eine halbe Fahrzeugbreite ausweichen, auf
        // der Seite, auf der sich das Fahrzeug gerade befindet. Der Versatz
        // wird ab der PYLONEN-OBERFLAECHE (nicht ihrem Mittelpunkt) gemessen,
        // um vehicle.clearance ergaenzt und zusaetzlich um extra_clearance
        // vergroessert, damit der Uebergang bei der engen T-B-Geometrie in der
        // Praxis eher kollisionsfrei ausfaellt.
        Vec2 perp{-direction.y(), direction.x()};
        if (perp.dot(car_xy - cone_b.pos) < 0.0) perp = -perp;
        double offset = cone_b.radius + vehicle.clearance + vehicle.width / 2.0 + extra_clearance;
        target_pos = cone_b.pos + perp * offset;
        method = "target point = B + half vehicle width (from cone surface) toward car";
    }

    r.ok = true;
    r.cone_b = cone_b;
    if (cone_c) r.cone_c = *cone_c;
    r.target_pose = {target_pos.x(), target_pos.y(), theta};
    r.method = method;
    return r;
}

// ────────────────────────────────────────────────────────────────────
//  1.3 Spur 1 fuer die Uebergangssuche (Verlaengerung + Kollisions-Schnitt)
// ────────────────────────────────────────────────────────────────────
//
//  Portierung von _lane_for_transition() aus cone_corridor_phase_planner.py.

LaneForTransition laneForTransition(
    const std::vector<Vec2>& lane1_pts,
    const VehicleParams& vehicle,
    const std::vector<Obstacle>& obstacles,
    double resample_ds)
{
    LaneForTransition out;
    auto p1 = resample(lane1_pts, resample_ds);
    if (p1.size() < 2) {
        out.points = std::move(p1);
        return out;
    }

    // Spur geradeaus ueber den letzten bekannten Punkt hinaus verlaengern,
    // damit der Abzweigpunkt-Suchraum in planTransition() nicht auf das kurze,
    // bekannte Stueck Spur 1 begrenzt ist.
    Vec2 last_dir = p1.back() - p1[p1.size() - 2];
    double last_dir_norm = last_dir.norm();
    if (last_dir_norm > 1e-9) {
        last_dir /= last_dir_norm;
        double extend_len = std::max(5.0, 10.0 * vehicle.R_min);
        int n_extra = std::max(1, static_cast<int>(extend_len / resample_ds));
        Vec2 base = p1.back();
        for (int k = 1; k <= n_extra; ++k) {
            p1.push_back(base + (k * resample_ds) * last_dir);
        }
    }

    auto t1 = tangents(p1);

    // An der ERSTEN Pylone abschneiden, die die geradeaus weiterfahrende
    // Karosserie treffen wuerde -- allen voran die Trigger-Pylone selbst: der
    // Uebergang darf niemals erst HINTER der Trigger-Pylone abzweigen.
    if (!obstacles.empty()) {
        std::vector<Pose> straight_poses(p1.size());
        for (size_t i = 0; i < p1.size(); ++i)
            straight_poses[i] = {p1[i].x(), p1[i].y(), t1[i]};
        int hit = firstCollisionIndex(straight_poses, obstacles, vehicle);
        if (hit >= 0) {
            size_t cut = std::min(p1.size(), static_cast<size_t>(std::max(2, hit)));
            p1.resize(cut);
            t1.resize(cut);
        }
    }

    out.points   = std::move(p1);
    out.tangents = std::move(t1);
    return out;
}

// ════════════════════════════════════════════════════════════════════
//  2. COLLISION — Karosserie-Kollisionspruefung
// ════════════════════════════════════════════════════════════════════

bool bodyClear(const std::vector<Pose>& poses,
               const std::vector<Obstacle>& obstacles,
               const VehicleParams& veh) {
    if (poses.empty()) return false;
    if (obstacles.empty()) return true;
    double rear  = veh.rear_overhang_frac * veh.length;
    double front = veh.length - rear;
    double hw    = veh.width / 2.0;
    double cl    = veh.clearance;
    for (auto& o : obstacles) {
        for (auto& p : poses) {
            double c = std::cos(p.th), s = std::sin(p.th);
            double dx = o.cx - p.x, dy = o.cy - p.y;
            double lx = dx * c + dy * s;
            double ly = -dx * s + dy * c;
            double qx = std::clamp(lx, -rear, front);
            double qy = std::clamp(ly, -hw, hw);
            double d2 = (lx - qx) * (lx - qx) + (ly - qy) * (ly - qy);
            if (d2 <= (o.r + cl) * (o.r + cl))
                return false;
        }
    }
    return true;
}

int firstCollisionIndex(const std::vector<Pose>& poses,
                        const std::vector<Obstacle>& obstacles,
                        const VehicleParams& veh) {
    if (poses.empty() || obstacles.empty()) return -1;
    double rear  = veh.rear_overhang_frac * veh.length;
    double front = veh.length - rear;
    double hw    = veh.width / 2.0;
    double cl    = veh.clearance;
    for (size_t i = 0; i < poses.size(); ++i) {
        auto& p = poses[i];
        double c = std::cos(p.th), s = std::sin(p.th);
        for (auto& o : obstacles) {
            double dx = o.cx - p.x, dy = o.cy - p.y;
            double lx = dx * c + dy * s;
            double ly = -dx * s + dy * c;
            double qx = std::clamp(lx, -rear, front);
            double qy = std::clamp(ly, -hw, hw);
            double d2 = (lx - qx) * (lx - qx) + (ly - qy) * (ly - qy);
            if (d2 <= (o.r + cl) * (o.r + cl))
                return static_cast<int>(i);
        }
    }
    return -1;
}

bool pathStillClear(const std::vector<Pose>& existing_path,
                    const std::vector<Obstacle>& new_obstacles,
                    const VehicleParams& veh) {
    return bodyClear(existing_path, new_obstacles, veh);
}

bool isPassageTooNarrow(const Pose& p, const std::vector<Obstacle>& obstacles,
                         const VehicleParams& vehicle, double search_radius) {
    double best_left  = std::numeric_limits<double>::infinity();
    double best_right = std::numeric_limits<double>::infinity();
    const double c = std::cos(p.th), s = std::sin(p.th);
    for (const auto& o : obstacles) {
        const double dx = o.cx - p.x, dy = o.cy - p.y;
        const double lx = dx * c + dy * s;
        if (std::abs(lx) > search_radius) continue;
        const double ly = -dx * s + dy * c;
        const double edge_dist = std::abs(ly) - o.r;
        if (ly >= 0.0) best_left  = std::min(best_left,  edge_dist);
        else            best_right = std::min(best_right, edge_dist);
    }
    if (!std::isfinite(best_left) || !std::isfinite(best_right)) return false;
    return (best_left + best_right) < (vehicle.width + 2.0 * vehicle.clearance);
}

// ════════════════════════════════════════════════════════════════════
//  3. TRANSITION — Biarc-Uebergang + Dubins-Rueckfallebene
// ════════════════════════════════════════════════════════════════════

// ────────────────────────────────────────────────────────────────────
//  3.1 Dubins-Worttypen
// ────────────────────────────────────────────────────────────────────

namespace {

constexpr double kPi  = 3.14159265358979323846;
constexpr double k2Pi = 2.0 * kPi;

double mod2pi(double t) {
    t = std::fmod(t, k2Pi);
    if (t < 0.0) t += k2Pi;
    return t;
}

std::optional<DubinsCandidate> make(double t, double p, double q,
                                     const char* mode, double R) {
    DubinsCandidate c;
    c.segs[0] = t * R; c.segs[1] = p * R; c.segs[2] = q * R;
    std::copy(mode, mode + 4, c.mode);
    c.length = (t + p + q) * R;
    c.R = R;
    return c;
}

std::optional<DubinsCandidate> dub_LSL(double a, double b, double d, double R) {
    double sa = std::sin(a), sb = std::sin(b), ca = std::cos(a), cb = std::cos(b);
    double p2 = 2 + d*d - 2*std::cos(a-b) + 2*d*(sa-sb);
    if (p2 < 0) return std::nullopt;
    double tmp = std::atan2(cb-ca, d+sa-sb);
    return make(mod2pi(-a+tmp), std::sqrt(p2), mod2pi(b-tmp), "LSL", R);
}

std::optional<DubinsCandidate> dub_RSR(double a, double b, double d, double R) {
    double sa = std::sin(a), sb = std::sin(b), ca = std::cos(a), cb = std::cos(b);
    double p2 = 2 + d*d - 2*std::cos(a-b) + 2*d*(sb-sa);
    if (p2 < 0) return std::nullopt;
    double tmp = std::atan2(ca-cb, d-sa+sb);
    return make(mod2pi(a-tmp), std::sqrt(p2), mod2pi(-b+tmp), "RSR", R);
}

std::optional<DubinsCandidate> dub_LSR(double a, double b, double d, double R) {
    double sa = std::sin(a), sb = std::sin(b), ca = std::cos(a), cb = std::cos(b);
    double p2 = -2 + d*d + 2*std::cos(a-b) + 2*d*(sa+sb);
    if (p2 < 0) return std::nullopt;
    double ps = std::sqrt(p2);
    double tmp = std::atan2(-ca-cb, d+sa+sb) - std::atan2(-2.0, ps);
    return make(mod2pi(-a+tmp), ps, mod2pi(-mod2pi(b)+tmp), "LSR", R);
}

std::optional<DubinsCandidate> dub_RSL(double a, double b, double d, double R) {
    double sa = std::sin(a), sb = std::sin(b), ca = std::cos(a), cb = std::cos(b);
    double p2 = d*d - 2 + 2*std::cos(a-b) - 2*d*(sa+sb);
    if (p2 < 0) return std::nullopt;
    double ps = std::sqrt(p2);
    double tmp = std::atan2(ca+cb, d-sa-sb) - std::atan2(2.0, ps);
    return make(mod2pi(a-tmp), ps, mod2pi(mod2pi(b)-tmp), "RSL", R);
}

std::optional<DubinsCandidate> dub_RLR(double a, double b, double d, double R) {
    double sa = std::sin(a), sb = std::sin(b), ca = std::cos(a), cb = std::cos(b);
    double val = (6.0 - d*d + 2*std::cos(a-b) + 2*d*(sa-sb)) / 8.0;
    if (std::abs(val) > 1.0) return std::nullopt;
    double p = mod2pi(k2Pi - std::acos(val));
    double t = mod2pi(a - std::atan2(ca-cb, d-sa+sb) + p/2.0);
    return make(t, p, mod2pi(a-b-t+p), "RLR", R);
}

std::optional<DubinsCandidate> dub_LRL(double a, double b, double d, double R) {
    double sa = std::sin(a), sb = std::sin(b), ca = std::cos(a), cb = std::cos(b);
    double val = (6.0 - d*d + 2*std::cos(a-b) + 2*d*(sb-sa)) / 8.0;
    if (std::abs(val) > 1.0) return std::nullopt;
    double p = mod2pi(k2Pi - std::acos(val));
    double t = mod2pi(-a - std::atan2(ca-cb, d+sa-sb) + p/2.0);
    return make(t, p, mod2pi(mod2pi(b)-a-t+mod2pi(p)), "LRL", R);
}

using DubFunc = std::optional<DubinsCandidate>(*)(double, double, double, double);
constexpr DubFunc kFuncs[] = {dub_LSL, dub_RSR, dub_LSR, dub_RSL, dub_RLR, dub_LRL};

// sin-Toleranz um den Geradeaus-Grenzfall (Ziel ~genau voraus/dahinter): dort
// ist keine Anfangsrichtung "falscher" als die andere, also gelten beide als
// natural, statt bei jedem Rauschen zwischen ihnen zu kippen.
constexpr double kNaturalEps = 0.05;
// Prohibitiv-Strafe fuer einen Kandidaten mit LANGEM "falsche Richtung
// zuerst"-Bogen -- so gross, dass sie jeden realistischen Laengenunterschied
// dominiert (Uebergaenge in dieser Anwendung bleiben deutlich unter 1000m),
// aber endlich: bleibt kein einziger anderer Kandidat kollisionsfrei, wird
// trotzdem der beste unter den unnatuerlichen zurueckgegeben, statt komplett
// aufzugeben.
constexpr double kUnnaturalPenalty = 1e4;
// Weiche Malus-Staffel fuer KURZES anfaengliches Gegenschwenken: ein leichtes
// Ausscheren (erster Bogen in die "falsche" Richtung) ist zulaessig, kostet
// aber pro Meter Erst-Bogen deutlich -- gewaehlt wird es nur, wenn es
// gegenueber dem besten natuerlichen Kandidaten wirklich viel Gesamtlaenge
// spart. Erst oberhalb von kUnnaturalMaxFirstArc greift der alte
// Prohibitiv-Malus (kein kraeftiges Ausscheren in die falsche Richtung).
constexpr double kUnnaturalPenaltyPerMeter = 12.0;
constexpr double kUnnaturalMaxFirstArc     = 0.8;   // [m]

// True, falls der erste Bogen (mode[0]) zu der Seite einschlaegt, auf der das
// Ziel tatsaechlich liegt. `a = mod2pi(s.th - theta)` (theta = Peilung
// Start->Ziel) ist bereits in dubinsCandidates() bekannt: crossSign > 0
// heisst, das Ziel liegt links vom aktuellen Heading (Kreuzprodukt Heading x
// Richtung-zum-Ziel), crossSign < 0 rechts davon.
bool naturalFirstTurn(double a, char firstTurn) {
    if (firstTurn == 'S') return true;
    double crossSign = -std::sin(a);
    if (firstTurn == 'L') return crossSign >= -kNaturalEps;
    return crossSign <= kNaturalEps;  // firstTurn == 'R'
}

// Sortier-/Auswahlkriterium ueber alle Kandidaten hinweg (siehe
// dubinsCandidates()/planTransition()): kleiner ist besser. Natuerliche
// Kandidaten kosten nur ihre Laenge; unnatuerliche mit KURZEM Erst-Bogen
// (leichtes Ausscheren) bekommen einen laengenproportionalen weichen Malus,
// unnatuerliche mit langem Erst-Bogen den Prohibitiv-Malus.
double dubinsScore(const DubinsCandidate& c) {
    if (c.natural) return c.length;
    if (c.mode[0] != 'S' && c.segs[0] > kUnnaturalMaxFirstArc)
        return c.length + kUnnaturalPenalty;
    return c.length + kUnnaturalPenaltyPerMeter * c.segs[0];
}

// Kumulierter |Drehwinkel| ueber alle drei Segmente [rad]: fuer 'L'/'R' ist
// segs[i] eine Bogenlaenge (= Winkel * R), fuer 'S' ein Geradenstueck (0
// Drehung). Anders als dubinsScore()/c.length (reine Weglaenge) erfasst das
// auch einen KURZEN Kandidaten, der trotzdem fast einmal komplett herumdreht
// (z.B. RLR/LRL mit kleinem R und grossem Mittelbogen) -- genau der Fall, den
// die Laengen-Auswahl allein nicht ausschliesst (siehe Kommentar bei
// arcFromTangent() oben: ein langer Bogen "verliert die Laengen-Auswahl von
// selbst", was fuer einen kurzradiusigen Vollkreis nicht gilt).
double dubinsTotalTurn(const DubinsCandidate& c) {
    double turn = 0.0;
    for (int i = 0; i < 3; ++i) {
        if (c.mode[i] == 'S') continue;
        turn += c.segs[i] / c.R;
    }
    return turn;
}

}  // anonymous namespace

// ────────────────────────────────────────────────────────────────────
//  3.2 Kandidaten-Erzeugung + Sampling
// ────────────────────────────────────────────────────────────────────

std::vector<DubinsCandidate> dubinsCandidates(const Pose& s, const Pose& g, double R) {
    double D = std::hypot(g.x - s.x, g.y - s.y);
    if (D < 1e-9) return {};
    double d     = D / R;
    double theta = mod2pi(std::atan2(g.y - s.y, g.x - s.x));
    double a     = mod2pi(s.th - theta);
    double b     = mod2pi(g.th - theta);
    std::vector<DubinsCandidate> out;
    for (auto fn : kFuncs) {
        auto c = fn(a, b, d, R);
        if (!c) continue;
        c->natural = naturalFirstTurn(a, c->mode[0]);
        out.push_back(*c);
    }
    std::sort(out.begin(), out.end(),
        [](auto& x, auto& y) { return dubinsScore(x) < dubinsScore(y); });
    return out;
}

std::optional<DubinsCandidate> dubinsShortest(const Pose& s, const Pose& g, double R) {
    auto cs = dubinsCandidates(s, g, R);
    return cs.empty() ? std::nullopt : std::optional{cs[0]};
}

std::vector<Pose> samplePoses(const Pose& start, const DubinsCandidate& c, double step) {
    double x = start.x, y = start.y, th = start.th;
    std::vector<Pose> out = {{x, y, th}};
    for (int seg = 0; seg < 3; ++seg) {
        double segLen = c.segs[seg];
        if (segLen < 1e-9) continue;
        char m = c.mode[seg];
        double k = (m == 'L') ? (1.0 / c.R) : (m == 'R') ? (-1.0 / c.R) : 0.0;
        int n = std::max(1, static_cast<int>(std::ceil(segLen / step)));
        double ds = segLen / n;
        for (int j = 0; j < n; ++j) {
            if (std::abs(k) < 1e-9) {
                x += ds * std::cos(th);
                y += ds * std::sin(th);
            } else {
                double tho = th;
                th += k * ds;
                x += (std::sin(th) - std::sin(tho)) / k;
                y += (std::cos(tho) - std::cos(th)) / k;
            }
            out.push_back({x, y, th});
        }
    }
    return out;
}

// ────────────────────────────────────────────────────────────────────
//  3.3 planTransition — Biarc-Uebergang + Dubins-Rueckfallebene
// ────────────────────────────────────────────────────────────────────
//
//  Ersetzt die vormalige 3-Stufen-Suche (Dubins-Bestfit, Dubins-Radienfaecher,
//  RRT mit Dubins-Lenkung -- siehe Git-Historie). Der Uebergang ist ein rein
//  LOKALES Problem: das Ziel liegt typischerweise nur wenige Dezimeter neben
//  Spur 1, die Hindernisse sind wenige statische Kreise --
//  ein Biarc (zwei tangentenstetige Kreisboegen) in geschlossener Form plus
//  Kollisionspruefung genuegt und ist deterministisch mit nahezu konstanter
//  Laufzeit. Der Biarc-Radius ergibt sich aus der Geometrie von Abzweigpunkt
//  und Zielpose und ist damit automatisch so gross, wie der Korridor es
//  zulaesst (kein Radienfaecher noetig); ein Kandidat ist nur zulaessig, wenn
//  beide Boegen >= vehicle.R_min bleiben. Nur wenn KEIN Abzweigpunkt einen
//  fahrbaren, kollisionsfreien Biarc liefert (Zielpose sehr nah und stark
//  verdreht), greift die Dubins-Rueckfallebene, die konstruktionsbedingt fuer
//  jede Posenkombination existiert.
//
//  Die RRT-Stufe entfiel ersatzlos: sie war der mit Abstand teuerste Pfad
//  (bis zu 6000 Iterationen, je bis zu 3x in der Polsterungs-Leiter von
//  planTransition()), lieferte die qualitativ schlechtesten Pfade
//  (Zufallsschleifen) und wurde nur erreicht, wenn Biarc UND Dubins
//  scheitern -- in diesem lokalen Setting existiert dann praktisch nie ein
//  sinnvoller Uebergang, den ausgerechnet ein RRT noch faende; die
//  Polsterungs-/Clearance-Leiter unten ist der wirksamere (und billigere)
//  Rueckfall. Ebenfalls entfiel die endpunkterhaltende Nachglaettung
//  (finalize()/transition_smooth_*): Biarc-/Dubins-Nahtstellen sind
//  kruemmungs-, nicht richtungsunstetig und damit fahrdynamisch bereits
//  ohne weitere Glaettung zulaessig; eine zusaetzliche innere Glaettung
//  dieses Teilstuecks war unnoetig.

namespace {

// Ein Segment eines Biarcs: Kreisbogen oder (radius = inf) Geradenstueck.
struct ArcSeg {
    Vec2   p0{0.0, 0.0}, p1{0.0, 0.0};
    Vec2   center{0.0, 0.0};                                  // nur falls !straight
    double radius = std::numeric_limits<double>::infinity();  // |r| [m]
    double sweep  = 0.0;                                      // signiert, + = links
    bool   straight = true;
    double length = 0.0;
};

// Der eindeutige Kreisbogen (bzw. das Geradenstueck), der in P mit Heading
// `th` startet und durch Q laeuft. nullopt nur, wenn Q exakt hinter P auf
// der Tangente liegt (Rueckwaertsfahrt). Ein Ziel seitlich-hinten ergibt
// einen gueltigen, aber langen Bogen (> pi Drehung) -- solche Kandidaten
// verlieren die Laengen-Auswahl von selbst.
std::optional<ArcSeg> arcFromTangent(const Vec2& P, double th, const Vec2& Q) {
    ArcSeg s;
    s.p0 = P;
    s.p1 = Q;
    const Vec2 T(std::cos(th), std::sin(th));
    const Vec2 n(-T.y(), T.x());
    const Vec2 chord = Q - P;
    const double L2 = chord.squaredNorm();
    if (L2 < 1e-18) {
        s.length = 0.0;
        return s;
    }
    const double d = chord.dot(T);   // Laengsanteil des Ziels
    const double h = chord.dot(n);   // Queranteil des Ziels
    if (std::abs(h) < 1e-9) {
        if (d <= 0.0) return std::nullopt;
        s.length = d;
        return s;
    }
    s.straight = false;
    const double r_signed = L2 / (2.0 * h);
    s.center = P + n * r_signed;
    s.radius = std::abs(r_signed);
    s.sweep  = 2.0 * std::atan2(h, d);   // Umfangswinkelsatz: Bogen = 2x Sehnenwinkel
    s.length = s.radius * std::abs(s.sweep);
    return s;
}

// Posen entlang eines ArcSeg anhaengen (Startpose exklusive). `th0` ist das
// Heading am Segmentanfang; entlang des Bogens dreht es mit dem Drehwinkel mit.
void appendArcPoses(const ArcSeg& s, double th0, double step, std::vector<Pose>& out) {
    if (s.length < 1e-9) return;
    const int n = std::max(1, static_cast<int>(std::ceil(s.length / step)));
    if (s.straight) {
        const Vec2 T(std::cos(th0), std::sin(th0));
        for (int k = 1; k <= n; ++k) {
            const Vec2 p = s.p0 + (s.length * k / n) * T;
            out.push_back({p.x(), p.y(), th0});
        }
        return;
    }
    const double a0 = std::atan2(s.p0.y() - s.center.y(), s.p0.x() - s.center.x());
    for (int k = 1; k <= n; ++k) {
        const double t = static_cast<double>(k) / n;
        const double a = a0 + s.sweep * t;
        out.push_back({s.center.x() + s.radius * std::cos(a),
                       s.center.y() + s.radius * std::sin(a),
                       th0 + s.sweep * t});
    }
}

struct Biarc {
    ArcSeg a, b;
    double length     = 0.0;
    double min_radius = std::numeric_limits<double>::infinity();
};

// Klassischer "equal d"-Biarc zwischen zwei Posen: Verbindungspunkt
//   J = (P1 + P2 + d*(T1 - T2)) / 2,
// wobei d >= 0 die gemeinsame Tangentenlaenge ist (Loesung der Quadrik aus
// der Gleichabstands-Bedingung). Beide Teilboegen sind in J tangentenstetig;
// Radien und Drehsinn ergeben sich vollstaendig aus der Geometrie.
std::optional<Biarc> makeBiarc(const Pose& s, const Pose& g) {
    const Vec2 P1(s.x, s.y), P2(g.x, g.y);
    const Vec2 T1(std::cos(s.th), std::sin(s.th));
    const Vec2 T2(std::cos(g.th), std::sin(g.th));
    const Vec2 v = P2 - P1;
    if (v.squaredNorm() < 1e-12) return std::nullopt;

    const double denom = 2.0 * (1.0 - T1.dot(T2));
    double d = 0.0;
    if (denom > 1e-9) {
        const double vt = v.dot(T1 + T2);
        d = (-vt + std::sqrt(vt * vt + denom * v.squaredNorm())) / denom;
    } else {
        // Parallele Tangenten. Ziel exakt seitlich (v.dot(T2) ~ 0) entartet
        // zu zwei Halbkreisen -- d = 0 fuehrt genau darauf.
        const double vT2 = v.dot(T2);
        if (std::abs(vT2) > 1e-9) {
            d = v.squaredNorm() / (4.0 * vT2);
            if (d < 0.0) return std::nullopt;  // Ziel klar hinter der Startpose
        }
    }

    const Vec2 J = 0.5 * (P1 + P2 + d * (T1 - T2));
    auto a1 = arcFromTangent(P1, s.th, J);
    if (!a1) return std::nullopt;
    const double th_j = s.th + (a1->straight ? 0.0 : a1->sweep);
    auto a2 = arcFromTangent(J, th_j, P2);
    if (!a2) return std::nullopt;
    // Konsistenz: das Endheading des zweiten Bogens muss die Zielausrichtung
    // treffen (bei korrektem J konstruktionsbedingt erfuellt; faengt
    // numerische Grenzfaelle der Spezialzweige oben ab).
    const double th_end = th_j + (a2->straight ? 0.0 : a2->sweep);
    if (std::abs(std::remainder(th_end - g.th, k2Pi)) > 1e-3) return std::nullopt;

    Biarc bi;
    bi.a = *a1;
    bi.b = *a2;
    bi.length = a1->length + a2->length;
    bi.min_radius = std::min(a1->radius, a2->radius);
    if (bi.length < 1e-9) return std::nullopt;
    return bi;
}

// Kumulierter |Drehwinkel| beider Teilboegen [rad] -- siehe dubinsTotalTurn()
// fuer die Motivation (Laenge allein schliesst einen kurzradiusigen
// Schleifenkandidaten nicht aus). ArcSeg::sweep ist bereits 0 fuer ein
// Geradenstueck (straight == true).
double biarcTotalTurn(const Biarc& bi) {
    return std::abs(bi.a.sweep) + std::abs(bi.b.sweep);
}

std::vector<Pose> sampleBiarc(const Pose& start, const Biarc& bi, double step = 0.08) {
    std::vector<Pose> out{start};
    appendArcPoses(bi.a, start.th, step, out);
    const double th_j = start.th + (bi.a.straight ? 0.0 : bi.a.sweep);
    appendArcPoses(bi.b, th_j, step, out);
    return out;
}

// Kern von planTransition() -- parametrisiert ueber `padding` statt fest
// cfg.transition_obstacle_padding zu lesen, damit die oeffentliche
// planTransition() unten bei einem gescheiterten gepolsterten Versuch
// GENAU DIESELBE Logik mit padding=0 wiederholen kann (siehe dortiger
// Kommentar: ein geometrisch exakt an der harten Kollisionsgrenze liegender,
// aber gueltiger Korridor -- z.B. ein Tor mit Nullspielraum nach Abzug der
// Polsterung -- darf nicht an der reinen Sicherheitsmarge scheitern).
TransitionResult planTransitionImpl(
    const std::vector<Vec2>&     p1,
    const std::vector<double>&   t1,
    const Pose&                  entry_pose,
    const std::vector<Obstacle>& obstacles,
    const PipelineConfig&        cfg,
    double                       padding,
    double                       heading_tol_rad)
{
    TransitionResult tr;
    const int N = static_cast<int>(p1.size());
    if (N < 2) {
        tr.fail_reason = "lane 1 has < 2 points";
        return tr;
    }

    const double R = cfg.vehicle.R_min;

    // Diagnostic counters for the failure case (see tr.fail_reason at the end):
    // why did NO candidate survive? Only terminal rejections are counted, not the
    // selection prunings (best_short/best_long comparisons) -- those only discard
    // against an already-found better candidate, so they are not a reason for the
    // whole stage to fail.
    int n_tested     = 0;  // candidates that reached the expensive bodyClear() check
    int rej_collision = 0; // of those, collided with a cone
    int rej_radius   = 0;  // biarc radius < R_min (not drivable) or over length_cap
    int rej_turn     = 0;  // loop-like: |turn angle| > transition_max_turn_rad
    int rej_far      = 0;  // departure point too far from target or candidate too long

    // Fuer die Uebergangs-SUCHE werden die Pylonen leicht aufgeblasen
    // (siehe Aufrufer): der gewaehlte Einscher-Pfad haelt so einen echten
    // Sicherheitsstreifen zu den stehenden Pylonen, statt haarscharf an der
    // harten Kollisionsgrenze entlangzuschrammen. Die abschliessende
    // Pruefung des Gesamtpfads (firstCollisionIndex() beim Aufrufer) nutzt
    // weiterhin die ungepolsterten Hindernisse.
    std::vector<Obstacle> padded = obstacles;
    for (auto& o : padded) o.r += padding;

    // Harte Nutzlosigkeits-Schranke fuer BEIDE Stufen: der Uebergang ist ein
    // lokales Manoever von wenigen Metern -- Kandidaten jenseits des
    // 5-fachen der Soll-Laenge (default 10 m) sind praktisch nie die
    // richtige Antwort und wuerden im Fehlschlag-Fall (gar kein Kandidat
    // kollisionsfrei, keine Bestwert-Schranke aktiv) nur die teure
    // Abtastung + Kollisionspruefung fuer offensichtlich unbrauchbare
    // Riesenboegen bezahlen.
    const double length_cap = 5.0 * cfg.transition_max_length_m;

    // Zielausrichtungs-Kandidaten: nominell + (nur falls der Aufrufer
    // heading_tol_rad > 0 uebergibt) symmetrische Abweichungen bis
    // +-heading_tol_rad.
    std::vector<double> heading_offsets{0.0};
    if (heading_tol_rad > 1e-9) {
        const int steps = std::max(1, cfg.transition_heading_sweep_steps);
        for (int k = 1; k <= steps; ++k) {
            const double off = heading_tol_rad * k / steps;
            heading_offsets.push_back(off);
            heading_offsets.push_back(-off);
        }
    }

    // ── Stufe 1: Biarc-Kandidaten ueber alle Abzweigpunkte ───────────
    // Auswahl unter den fahrbaren (beide Radien >= R_min), kollisionsfreien
    // Kandidaten mit Laenge <= transition_max_length_m (Anforderung: Uebergang
    // < 2 m, harte Praeferenz): NICHT mehr der kuerzeste, sondern der mit dem
    // SPAETESTEN Abzweigpunkt `i` -- das haelt das Fahrzeug so lange wie
    // moeglich auf Spur 1 (der "Default-Map") und laesst den Uebergang nur so
    // kurz wie geometrisch noetig abzweigen, statt unnoetig frueh auf die
    // kuerzestmoegliche Kurve zu wechseln. Bei Gleichstand (gleicher `i`,
    // verschiedene Zielwinkel-Kandidaten) gewinnt die kleinere
    // Zielwinkel-Abweichung. Existiert KEIN <= transition_max_length_m-
    // Kandidat, gewinnt weiterhin der kuerzeste laengere (best-effort statt
    // hartem Scheitern an einer geometrisch erzwungenen Mindestlaenge).
    struct BiarcBest {
        double length = std::numeric_limits<double>::infinity();
        int    i      = 0;
        double off    = 0.0;
        std::vector<Pose> poses;
    };
    std::optional<BiarcBest> best_short, best_long;

    auto evalBiarc = [&](int i, double off) {
        const Pose s{p1[i].x(), p1[i].y(), t1[i]};
        Pose goal = entry_pose;
        goal.th += off;
        // Luftlinie als untere Laengenschranke: nur noch gegen length_cap
        // geprueft, NICHT mehr gegen die bisher beste Laenge -- ein spaeterer
        // (also bevorzugter) Abzweigpunkt darf laenger als der bisherige
        // Gewinner sein, solange er noch <= transition_max_length_m bleibt;
        // ein Vergleich gegen die kuerzeste bisherige Laenge wuerde solche
        // Kandidaten faelschlich verwerfen.
        const double lb = std::hypot(goal.x - s.x, goal.y - s.y);
        if (lb > length_cap) { ++rej_far; return; }
        // HARTE Schranke (siehe transition_max_departure_distance_m,
        // types.hpp): dieser Abzweigpunkt kommt unabhaengig von Kurvenlaenge/
        // Kollisionsfreiheit gar nicht erst infrage, wenn er zu weit
        // (Luftlinie) vom Ziel entfernt liegt -- verhindert einen Uebergang,
        // der weit vom eigentlichen Korridor entfernt abzweigt.
        if (lb > cfg.transition_max_departure_distance_m) { ++rej_far; return; }
        auto bi = makeBiarc(s, goal);
        if (!bi || bi->min_radius < R - 1e-9 || bi->length > length_cap) { ++rej_radius; return; }
        // Schleifen-Sperre: ein kurzradiusiger Vollkreis kann trotzdem KURZ
        // sein (2*pi*R_min < length_cap, siehe dubinsTotalTurn()-Kommentar) --
        // die reine Laengenpruefung oben schliesst ihn also nicht aus. Erst
        // NACH dieser Pruefung darf der Kandidat ueberhaupt in best_short/
        // best_long konkurrieren.
        if (biarcTotalTurn(*bi) > cfg.transition_max_turn_rad) { ++rej_turn; return; }
        const bool is_short = bi->length <= cfg.transition_max_length_m;
        if (!is_short && best_short) return;
        if (is_short) {
            // Guenstiger Vorab-Check vor der teuren bodyClear()-Pruefung:
            // ein Kandidat mit frueherem (oder gleichem, aber staerker
            // abweichendem) Abzweigpunkt kann den aktuellen Gewinner nicht
            // mehr schlagen.
            if (best_short && (i < best_short->i ||
                                (i == best_short->i && std::abs(off) >= std::abs(best_short->off)))) {
                return;
            }
        } else if (best_long && bi->length >= best_long->length) {
            return;
        }
        auto poses = sampleBiarc(s, *bi);
        ++n_tested;
        if (!bodyClear(poses, padded, cfg.vehicle)) { ++rej_collision; return; }
        auto& slot = is_short ? best_short : best_long;
        slot = BiarcBest{bi->length, i, off, std::move(poses)};
    };

    const int stride = std::max(1, N / 40);
    for (int i = 0; i < N; i += stride)
        for (double off : heading_offsets)
            evalBiarc(i, off);

    // Feinschliff: Abzweigpunkte im Fenster um den (spaetesten) Gewinner
    // nachverdichten (die Grobabtastung kann den tatsaechlich spaetesten
    // zulaessigen Abzweigpunkt um bis zu stride-1 Punkte verfehlen -- das Fenster
    // deckt in beide Richtungen ausreichend ab).
    if (best_short || best_long) {
        const int ci = best_short ? best_short->i : best_long->i;
        for (int i = std::max(0, ci - stride); i < std::min(N, ci + stride + 1); ++i)
            for (double off : heading_offsets)
                evalBiarc(i, off);
    }

    auto biarcResult = [&](BiarcBest& win) {
        tr.ok = true;
        tr.departure_index = win.i;
        tr.heading_offset_used_rad = win.off;
        tr.poses = std::move(win.poses);
        tr.method = "biarc transition";
        tr.clearance_used_m = cfg.vehicle.clearance;
        return tr;
    };
    // Ein Biarc unterhalb der Soll-Laenge ist das Wunschergebnis -- fertig.
    if (best_short) return biarcResult(*best_short);

    // ── Stufe 2: Dubins-Rueckfallebene (Radienfaecher + alle Worttypen) ──
    // Erreicht, wenn kein Abzweigpunkt einen fahrbaren, kollisionsfreien
    // Biarc UNTER der Soll-Laenge liefert (Zielpose sehr nah und stark
    // verdreht -- der Biarc-Radius unterschreitet dann R_min oder der Biarc
    // wird zum langen Umweg). Dubins existiert bei R_min konstruktionsbedingt
    // fuer jede Posenkombination und kann in genau diesen Faellen deutlich
    // kuerzer sein; vehicle.R_min ist nur die untere Schranke, der Radius
    // wird geometrisch aufgefaechert (transition_radius_growth/
    // transition_max_radius_factor). Existiert nur ein LANGER Biarc, wird am
    // Ende der kuerzere der beiden Kandidaten gewaehlt.
    std::vector<double> radii;
    for (double Rk = R; Rk <= cfg.transition_max_radius_factor * R;
         Rk *= cfg.transition_radius_growth) {
        radii.push_back(Rk);
    }
    if (radii.empty()) radii.push_back(R);

    struct DubinsBest {
        double score = std::numeric_limits<double>::infinity();
        int    i     = 0;
        double off   = 0.0;
        Pose   start;
        DubinsCandidate cand;
    };
    // Wie bei der Biarc-Stufe: unter den <= transition_max_length_m-
    // Kandidaten gewinnt der spaeteste Abzweigpunkt `i`, nicht der beste
    // Score -- haelt das Fahrzeug so lange wie moeglich auf Spur 1. Der
    // Score bleibt nur INNERHALB einer einzelnen Pose (i, off, Rk) die
    // Auswahlgroesse (dubinsCandidates() liefert score-sortiert, "natuerliche"
    // Anfangsrichtung zuerst) sowie weiterhin das alleinige Kriterium fuer den
    // is_long-Fallback (kein <= transition_max_length_m-Kandidat gefunden).
    std::optional<DubinsBest> dubins_short, dubins_long;
    const int si2 = std::max(1, N / 28);
    for (int i = 0; i < N; i += si2) {
        const Pose s{p1[i].x(), p1[i].y(), t1[i]};
        for (double off : heading_offsets) {
            Pose goal = entry_pose;
            goal.th += off;
            // HARTE Schranke (siehe transition_max_departure_distance_m,
            // types.hpp und Kommentar in evalBiarc() oben): unabhaengig von
            // Radius/Kollisionsfreiheit kommt dieser Abzweigpunkt gar nicht
            // erst infrage, wenn er zu weit (Luftlinie) vom Ziel entfernt liegt.
            if (std::hypot(goal.x - s.x, goal.y - s.y) > cfg.transition_max_departure_distance_m) {
                ++rej_far;
                continue;
            }
            for (double Rk : radii) {
                auto cands = dubinsCandidates(s, goal, Rk);
                for (auto& c : cands) {
                    if (c.length > length_cap) { ++rej_far; continue; }
                    // Schleifen-Sperre wie bei evalBiarc(): RLR/LRL-Kandidaten
                    // koennen bei kleinem Rk trotz fast vollem Kreis kurz genug
                    // sein, um length_cap zu unterschreiten.
                    if (dubinsTotalTurn(c) > cfg.transition_max_turn_rad) { ++rej_turn; continue; }
                    const bool is_short = c.length <= cfg.transition_max_length_m;
                    // Sobald irgendwo (auch bei einem frueheren i) ein kurzer
                    // Dubins gefunden wurde, sind lange Kandidaten fuer die
                    // Endauswahl irrelevant (kurz wird immer bevorzugt, siehe
                    // dwin unten) -- diesen Kandidaten ueberspringen, aber
                    // weiter nach einem evtl. spaeteren kurzen suchen.
                    if (!is_short && dubins_short) continue;
                    if (is_short) {
                        // Guenstiger Vorab-Check vor der teuren bodyClear()-
                        // Pruefung: ein Kandidat mit frueherem (oder
                        // gleichem, staerker abweichendem) Abzweigpunkt kann
                        // den aktuellen Gewinner nicht mehr schlagen.
                        if (dubins_short && (i < dubins_short->i ||
                                              (i == dubins_short->i &&
                                               std::abs(off) >= std::abs(dubins_short->off)))) {
                            continue;
                        }
                    } else {
                        const double sc = dubinsScore(c);
                        if (dubins_long && sc >= dubins_long->score) continue;
                    }
                    auto poses = samplePoses(s, c);
                    ++n_tested;
                    if (!bodyClear(poses, padded, cfg.vehicle)) { ++rej_collision; continue; }
                    if (is_short) {
                        dubins_short = DubinsBest{dubinsScore(c), i, off, s, c};
                    } else {
                        dubins_long = DubinsBest{dubinsScore(c), i, off, s, c};
                    }
                    break;  // bester (score-sortierter) Kandidat fuer diese Pose gefunden
                }
            }
        }
    }
    // Auswahl: kurzer Dubins > langer Biarc/Dubins (der kuerzere von beiden).
    std::optional<DubinsBest>* dwin = dubins_short ? &dubins_short : &dubins_long;
    if (!dubins_short && best_long &&
        (!dubins_long || best_long->length <= dubins_long->cand.length)) {
        return biarcResult(*best_long);
    }
    if (*dwin) {
        tr.ok = true;
        tr.departure_index = (*dwin)->i;
        tr.heading_offset_used_rad = (*dwin)->off;
        tr.poses = samplePoses((*dwin)->start, (*dwin)->cand);
        tr.method = "Dubins fallback";
        tr.clearance_used_m = cfg.vehicle.clearance;
        return tr;
    }
    // No candidate survived -- break down WHY (the counters show whether it was
    // due to collisions, too-tight curves/loops, R_min, or the distance).
    const int max_turn_deg = static_cast<int>(cfg.transition_max_turn_rad * 57.29578);
    tr.fail_reason = "no drivable transition at R_min=" + std::to_string(R) + "m: " +
                      std::to_string(n_tested) + " candidates collision-checked (" +
                      std::to_string(rej_collision) + "x collision), rejected: " +
                      std::to_string(rej_radius) + "x R<R_min, " +
                      std::to_string(rej_turn) + "x loop>" + std::to_string(max_turn_deg) +
                      "deg, " + std::to_string(rej_far) + "x too far/long";
    return tr;
}

}  // anonymous namespace

// Oeffentlicher Einstiegspunkt: erst MIT Sicherheitspolsterung versuchen
// (transition_obstacle_padding, siehe types.hpp -- vermeidet Streifen an
// Pylonen bei grosszuegigen Korridoren), bei Fehlschlag verlustfrei OHNE
// Polsterung wiederholen. Ohne diesen Fallback wuerde ein geometrisch exakt
// an der harten Kollisionsgrenze liegender, aber gueltiger Korridor (z.B.
// ein Tor, dessen Breite nach Abzug von Pylonenradius+clearance+halber
// Fahrzeugbreite null Restspielraum laesst) allein durch die zusaetzliche
// Polsterung als unplanbar gelten -- der Uebergang scheitert dann komplett
// (FAILED), obwohl ein kollisionsfreier Pfad existiert.
TransitionResult planTransition(
    const std::vector<Vec2>&     p1,
    const std::vector<double>&   t1,
    const Pose&                  entry_pose,
    const std::vector<Obstacle>& obstacles,
    const PipelineConfig&        cfg,
    double                       entry_heading_tolerance_rad)
{
    auto tr = planTransitionImpl(p1, t1, entry_pose, obstacles, cfg,
                                  cfg.transition_obstacle_padding,
                                  entry_heading_tolerance_rad);
    if (tr.ok) return tr;

    // Per-stage diagnostics for the failure case: every failed fallback stage
    // appends its reason, so the final fail_reason shows WHICH stages were tried
    // and what each of them failed on -- instead of just "no path found".
    std::string diag = "padded(p=" + std::to_string(cfg.transition_obstacle_padding) +
                        "m): " + tr.fail_reason;

    if (cfg.transition_obstacle_padding > 0.0) {
        auto tr_unpadded = planTransitionImpl(p1, t1, entry_pose, obstacles, cfg, 0.0,
                                               entry_heading_tolerance_rad);
        if (tr_unpadded.ok) {
            tr_unpadded.method += "  [without safety padding -- tightest corridor]";
            return tr_unpadded;
        }
        diag += " | unpadded: " + tr_unpadded.fail_reason;
    }

    // Letzter Ausweg, siehe transition_min_clearance_m in types.hpp: mit
    // abgesenktem vehicle.clearance statt des vollen Standardpuffers
    // wiederholen. Nur eine LOKALE Kopie von cfg -- der Aufrufer bekommt
    // seine eigene PipelineConfig unveraendert zurueck; die abgesenkte
    // clearance wird ausschliesslich ueber TransitionResult::clearance_used_m
    // sichtbar, damit der Aufrufer die abschliessende Kollisionspruefung des
    // Gesamtpfads konsistent damit durchfuehren kann.
    if (cfg.transition_min_clearance_m < cfg.vehicle.clearance) {
        PipelineConfig relaxed_cfg = cfg;
        relaxed_cfg.vehicle.clearance = cfg.transition_min_clearance_m;
        auto tr_relaxed = planTransitionImpl(p1, t1, entry_pose, obstacles, relaxed_cfg, 0.0,
                                              entry_heading_tolerance_rad);
        if (tr_relaxed.ok) {
            tr_relaxed.clearance_used_m = cfg.transition_min_clearance_m;
            tr_relaxed.method += "  [lowered safety margin clearance=" +
                                  std::to_string(cfg.transition_min_clearance_m) +
                                  "m -- tightest corridor]";
            return tr_relaxed;
        }
        diag += " | clearance=" + std::to_string(cfg.transition_min_clearance_m) +
                "m: " + tr_relaxed.fail_reason;
    } else {
        diag += " | clearance stage skipped (transition_min_clearance_m>=vehicle.clearance)";
    }

    // Letzte Rueckfallstufe, siehe transition_tight_radius_factor/
    // transition_tight_max_length_m in types.hpp: kein Kandidat bei R_min hat
    // sowohl die Schleifen-Sperre (transition_max_turn_rad, siehe evalBiarc()/
    // Dubins-Schleife oben) als auch die Kollisionspruefung bestanden -- ohne
    // diese Stufe wuerde planTransition() hier mit FAILED aufgeben, obwohl ein
    // kurzer, eng gefahrener Bereich die Situation loesen koennte. Bewusst
    // NACH der Clearance-Lockerung: ein kleinerer Wenderadius geht (anders als
    // weniger Sicherheitsabstand) an die Lenkgrenze des Fahrzeugs und soll nur
    // versucht werden, wenn wirklich nichts anderes mehr hilft.
    // transition_max_length_m wird fuer DIESEN Versuch auf
    // transition_tight_max_length_m verkleinert -- dieselbe Soft-/Hard-Laengen-
    // Logik wie sonst auch (is_short-Vorzug, length_cap = 5x davon), nur auf
    // einen kleinen Bereich reskaliert, statt eines separaten Laengenchecks.
    if (cfg.transition_tight_radius_factor < 1.0) {
        PipelineConfig tight_cfg = cfg;
        tight_cfg.vehicle.R_min = cfg.vehicle.R_min * cfg.transition_tight_radius_factor;
        tight_cfg.transition_max_length_m = cfg.transition_tight_max_length_m;
        auto tr_tight = planTransitionImpl(p1, t1, entry_pose, obstacles, tight_cfg, 0.0,
                                            entry_heading_tolerance_rad);
        if (tr_tight.ok) {
            tr_tight.method += "  [lowered R_min=" +
                                std::to_string(tight_cfg.vehicle.R_min) +
                                "m -- small arc instead of circle]";
            return tr_tight;
        }
        diag += " | tight R_min=" + std::to_string(tight_cfg.vehicle.R_min) +
                "m: " + tr_tight.fail_reason;
    } else {
        diag += " | tight-R_min stage skipped (transition_tight_radius_factor>=1.0)";
    }

    tr.fail_reason = "all transition fallback stages exhausted -- " + diag;
    return tr;
}

// Verkuerzung ueber den Eintrittspunkt -- siehe cone_corridor.hpp fuer die
// Motivation. Der Eintrittspunkt wandert dabei ausschliesslich AUF der
// bereits berechneten Mittellinie nach vorn; bewertet wird jeder Kandidat am
// fertig komponierten Gesamtpfad, nicht am Uebergang allein.
ShiftedTransition planTransitionShiftedEntry(
    const std::vector<Vec2>&     p1,
    const std::vector<double>&   t1,
    const std::vector<Pose>&     lane,
    const std::vector<Obstacle>& obstacles,
    const PipelineConfig&        cfg)
{
    ShiftedTransition out;
    if (lane.empty()) return out;

    // Sicherheitsbereich: der Fahrtvektor (Heading) an JEDEM als Eintrittspunkt
    // akzeptierten Kandidaten MUSS innerhalb +-30 Grad zur Richtung
    // Kandidat->Mittellinienende (lane.back()) liegen -- unabhaengig von
    // transition_entry_shift_max_m/step_m (die per Default 0 sind, siehe
    // types.hpp, und diese Bedingung sonst nie prueften). Ohne diese Bedingung
    // kann ein lokal tangentenstetiger, aber global falscher Eintrittspunkt
    // (z.B. durch einen Vorzeichenfehler in der Seitenwahl oder eine instabile
    // lokale Tangente am Kettenanfang, siehe computeEntryPose()/classifyRows())
    // einen Uebergang erzeugen, der zwar sauber an die Mittellinie anschliesst,
    // aber effektiv von der falschen Seite in den Korridor einfaedelt. Gilt
    // immer -- lane[0] ist deshalb KEIN automatischer Rueckfall mehr.
    constexpr double kEntryHeadingToleranceRad = kPi / 6.0;  // 30 Grad
    const auto headingTowardLaneEnd = [&](const Pose& p) {
        const double dx = lane.back().x - p.x;
        const double dy = lane.back().y - p.y;
        if (dx * dx + dy * dy < 1e-9) return true;  // p ist praktisch das Ende selbst
        const double dir  = std::atan2(dy, dx);
        const double diff = std::atan2(std::sin(p.th - dir), std::cos(p.th - dir));
        return std::abs(diff) <= kEntryHeadingToleranceRad;
    };

    // Ersten Punkt entlang der GESAMTEN Mittellinie suchen, der die
    // Richtungsbedingung erfuellt -- das ist der neue Basis-Eintrittspunkt
    // (frueher immer lane[0]). Erfuellt keiner der Punkte die Bedingung,
    // scheitert der Uebergang komplett (ok=false): eine Mittellinie, die
    // nirgends grob auf ihr eigenes Ende zeigt, ist kein gueltiges Ziel.
    size_t k0 = lane.size();
    for (size_t i = 0; i < lane.size(); ++i) {
        if (headingTowardLaneEnd(lane[i])) { k0 = i; break; }
    }
    if (k0 == lane.size()) return out;

    double shift_to_k0 = 0.0;
    for (size_t i = 1; i <= k0; ++i) {
        shift_to_k0 += std::hypot(lane[i].x - lane[i - 1].x, lane[i].y - lane[i - 1].y);
    }

    // Ergebnis fuer den (ggf. bereits wegen der Richtungsbedingung
    // verschobenen) Basis-Eintrittspunkt: gleichzeitig der Rueckfall, falls
    // kein weiterer Kandidat einen kollisionsfreien Gesamtpfad liefert.
    out.transition    = planTransition(p1, t1, lane[k0], obstacles, cfg);
    out.entry_index   = k0;
    out.entry_shift_m = shift_to_k0;

    const double max_shift = cfg.transition_entry_shift_max_m;
    const double step      = cfg.transition_entry_shift_step_m;
    if (k0 + 1 >= lane.size() || max_shift <= 0.0 || step <= 0.0) return out;

    // Kandidaten-Indizes AB k0 (nicht mehr ab lane[0]): der jeweils erste
    // Mittellinienpunkt, dessen Bogenlaenge ab k0 die naechste Schrittmarke
    // erreicht UND der die Richtungsbedingung ebenfalls erfuellt. Ueber die
    // BOGENLAENGE statt ueber Indizes, weil die Punktdichte der Mittellinie je
    // nach planendem Zweig (Fahrvektorkette/Voronoi/Spiegel) verschieden ist --
    // "40 cm" muessen in allen dreien dieselben 40 cm sein.
    std::vector<size_t> candidates{k0};
    std::vector<double> shifts{shift_to_k0};
    {
        double s = shift_to_k0;
        double next_mark = shift_to_k0 + step;
        for (size_t i = k0 + 1; i < lane.size(); ++i) {
            s += std::hypot(lane[i].x - lane[i - 1].x, lane[i].y - lane[i - 1].y);
            // Deckel zuerst: ist der naechste Mittellinienpunkt schon weiter als
            // max_shift entfernt, gibt es keinen zulaessigen Kandidaten mehr --
            // auch dann nicht, wenn die Mittellinie grob abgetastet ist und
            // dieser eine Schritt den ganzen Deckel ueberspringt. (Ohne diese
            // Reihenfolge wuerde ein einzelner grosser Schritt als Kandidat
            // durchrutschen und den Eintrittspunkt weiter verschieben, als der
            // Deckel erlaubt.)
            if (s > shift_to_k0 + max_shift + 1e-9) break;
            if (s + 1e-9 < next_mark) continue;   // noch keine neue Schrittmarke
            if (!headingTowardLaneEnd(lane[i])) continue;
            candidates.push_back(i);
            shifts.push_back(s);
            while (next_mark <= s + 1e-9) next_mark += step;
        }
    }

    // Gesamtpfad-Kollisionspruefung EXAKT wie beim Aufrufer (cone_planner.cpp):
    // Uebergang + Rest der Mittellinie, resampled, gegen die ungepolsterten
    // Hindernisse -- und mit derselben ggf. abgesenkten Sicherheitsmarge, die
    // planTransition() fuer genau diesen Uebergang benutzt hat (sonst wuerde
    // die Pruefung einen mit Muehe gefundenen Uebergang an der strengeren
    // Standardmarge wieder verwerfen, siehe TransitionResult::clearance_used_m).
    auto fullPathClear = [&](const TransitionResult& tr, size_t k) {
        std::vector<Vec2> full;
        full.reserve(tr.poses.size() + lane.size() - k);
        for (const auto& pose : tr.poses) full.push_back({pose.x, pose.y});
        for (size_t i = k + 1; i < lane.size(); ++i) full.push_back({lane[i].x, lane[i].y});
        if (full.size() < 2) return false;
        full = resample(full, cfg.resample_ds);
        VehicleParams veh = cfg.vehicle;
        veh.clearance = std::min(cfg.vehicle.clearance, tr.clearance_used_m);
        return firstCollisionIndex(pathToPoses(full), obstacles, veh) < 0;
    };

    auto transitionLength = [](const TransitionResult& tr) {
        double len = 0.0;
        for (size_t i = 1; i < tr.poses.size(); ++i)
            len += std::hypot(tr.poses[i].x - tr.poses[i - 1].x,
                              tr.poses[i].y - tr.poses[i - 1].y);
        return len;
    };

    std::optional<ShiftedTransition> best;
    double best_len = std::numeric_limits<double>::infinity();
    for (size_t c = 0; c < candidates.size(); ++c) {
        const size_t k = candidates[c];
        TransitionResult tr = (k == k0) ? out.transition
                                         : planTransition(p1, t1, lane[k], obstacles, cfg);
        if (!tr.ok) continue;
        const double len = transitionLength(tr);
        if (len >= best_len) continue;          // kann nicht mehr gewinnen
        if (!fullPathClear(tr, k)) continue;
        best_len = len;
        best = ShiftedTransition{std::move(tr), k, shifts[c]};
    }

    if (!best) return out;
    if (best->entry_index > 0) {
        best->transition.method += "  [entry point shifted " +
                                    std::to_string(best->entry_shift_m) +
                                    "m along the centerline]";
    }
    return *best;
}

// ════════════════════════════════════════════════════════════════════
//  4. KORRIDOR-KETTE — Nachlaufplaner (Referenzlinie + Fahrvektoren)
// ════════════════════════════════════════════════════════════════════
//
//  Siehe cone_corridor.hpp Abschnitt 4 fuer die Gliederung. Portierung von
//  cone_corridor_lattice.py (+ der von cone_corridor_vector_testbed.py
//  tatsaechlich benutzten Pipeline) sowie der Ketten-/Reparatur-Hilfsfunktionen
//  aus cone_corridor_phase_planner.py, die beide Skripte wiederverwenden.

namespace {

double median(const std::vector<double>& values) {
    std::vector<double> v = values;
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    if (n == 0) return 0.0;
    if (n % 2 == 1) return v[n / 2];
    return 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

// wrapPi(): eigenstaendig statt mod2pi() (Dubins, Abschnitt 3) wiederzuverwenden
// -- mod2pi() bildet nach [0,2pi) ab, hier wird [-pi,pi) gebraucht.
double wrapPi(double angle) {
    while (angle > kPi) angle -= k2Pi;
    while (angle < -kPi) angle += k2Pi;
    return angle;
}

// ── natuerlicher kubischer Spline (Ersatz fuer scipy.CubicSpline/not-a-knot,
//    fuer den hier genutzten reinen Glaettungszweck aequivalent) ───────────

struct CubicSplineCoeffs {
    std::vector<double> s, a, b, c, d;
};

CubicSplineCoeffs fitNaturalCubicSpline(const std::vector<double>& s, const std::vector<double>& y) {
    const int n = static_cast<int>(s.size());
    CubicSplineCoeffs sp;
    sp.s = s;
    sp.a = y;
    sp.b.assign(n, 0.0);
    sp.c.assign(n, 0.0);
    sp.d.assign(n, 0.0);
    if (n < 3) return sp;

    std::vector<double> h(n - 1);
    for (int i = 0; i < n - 1; ++i) h[i] = s[i + 1] - s[i];

    std::vector<double> alpha(n, 0.0);
    for (int i = 1; i < n - 1; ++i)
        alpha[i] = 3.0 * (y[i + 1] - y[i]) / h[i] - 3.0 * (y[i] - y[i - 1]) / h[i - 1];

    std::vector<double> l(n, 1.0), mu(n, 0.0), z(n, 0.0);
    for (int i = 1; i < n - 1; ++i) {
        l[i] = 2.0 * (s[i + 1] - s[i - 1]) - h[i - 1] * mu[i - 1];
        mu[i] = h[i] / l[i];
        z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
    }
    sp.c[n - 1] = 0.0;
    for (int j = n - 2; j >= 0; --j) {
        sp.c[j] = z[j] - mu[j] * sp.c[j + 1];
        sp.b[j] = (y[j + 1] - y[j]) / h[j] - h[j] * (sp.c[j + 1] + 2.0 * sp.c[j]) / 3.0;
        sp.d[j] = (sp.c[j + 1] - sp.c[j]) / (3.0 * h[j]);
    }
    return sp;
}

double evalCubicSpline(const CubicSplineCoeffs& sp, double x) {
    const int n = static_cast<int>(sp.s.size());
    if (n < 3) return sp.a.empty() ? 0.0 : sp.a[0];
    auto it = std::upper_bound(sp.s.begin(), sp.s.end(), x);
    int i = std::clamp(static_cast<int>(std::distance(sp.s.begin(), it)) - 1, 0, n - 2);
    double dx = x - sp.s[i];
    return sp.a[i] + sp.b[i] * dx + sp.c[i] * dx * dx + sp.d[i] * dx * dx * dx;
}

// ── Nachbarschaftsgraph-Aufbau (portiert aus cone_corridor_phase_planner.py) ─

double adaptiveNeighborDist(const std::vector<Cone>& cones, const VehicleParams& vehicle,
                             double factor = 1.3) {
    const size_t n = cones.size();
    if (n < 4) return vehicle.width + 0.05;
    std::vector<double> nearest(n, std::numeric_limits<double>::infinity());
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j)
            if (i != j) nearest[i] = std::min(nearest[i], (cones[i].pos - cones[j].pos).norm());
    std::sort(nearest.begin(), nearest.end());
    // 25.-Perzentil, linear interpoliert wie np.percentile().
    const double pos = 0.25 * static_cast<double>(nearest.size() - 1);
    const size_t lo = static_cast<size_t>(std::floor(pos));
    const size_t hi = static_cast<size_t>(std::ceil(pos));
    const double frac = pos - static_cast<double>(lo);
    const double p25 = nearest[lo] + frac * (nearest[hi] - nearest[lo]);
    return p25 * factor;
}

std::unordered_map<int, std::vector<int>> neighborGraph(const std::vector<Cone>& cones,
                                                          double neighbor_dist) {
    const size_t n = cones.size();
    std::unordered_map<int, std::vector<int>> adj;
    adj.reserve(n);
    for (size_t i = 0; i < n; ++i) adj[static_cast<int>(i)] = {};
    if (n < 2) return adj;
    for (size_t i = 0; i + 1 < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            if ((cones[i].pos - cones[j].pos).norm() <= neighbor_dist) {
                adj[static_cast<int>(i)].push_back(static_cast<int>(j));
                adj[static_cast<int>(j)].push_back(static_cast<int>(i));
            }
        }
    }
    return adj;
}

std::unordered_map<int, std::vector<int>> pruneToChains(std::unordered_map<int, std::vector<int>> adj,
                                                          const std::vector<Cone>& cones,
                                                          int max_iter = 200) {
    for (int iter = 0; iter < max_iter; ++iter) {
        std::vector<int> high_deg;
        for (auto& kv : adj)
            if (kv.second.size() > 2) high_deg.push_back(kv.first);
        if (high_deg.empty()) break;
        for (int v : high_deg) {
            auto it = adj.find(v);
            if (it == adj.end() || it->second.size() <= 2) continue;
            int longest = it->second[0];
            double longest_d = (cones[v].pos - cones[longest].pos).norm();
            for (int t : it->second) {
                const double d = (cones[v].pos - cones[t].pos).norm();
                if (d > longest_d) { longest_d = d; longest = t; }
            }
            auto& vec = it->second;
            vec.erase(std::remove(vec.begin(), vec.end(), longest), vec.end());
            auto lt = adj.find(longest);
            if (lt != adj.end()) {
                auto& lvec = lt->second;
                lvec.erase(std::remove(lvec.begin(), lvec.end(), v), lvec.end());
            }
        }
    }
    return adj;
}

std::unordered_map<int, std::vector<int>> breakCycles(std::unordered_map<int, std::vector<int>> adj,
                                                        const std::vector<Cone>& cones) {
    std::set<int> visited_global;
    std::vector<int> keys;
    keys.reserve(adj.size());
    for (auto& kv : adj) keys.push_back(kv.first);
    for (int start : keys) {
        if (visited_global.count(start)) continue;
        std::set<int> comp;
        std::vector<int> stack{start};
        while (!stack.empty()) {
            int cur = stack.back();
            stack.pop_back();
            if (comp.count(cur)) continue;
            comp.insert(cur);
            for (int nb : adj[cur])
                if (!comp.count(nb)) stack.push_back(nb);
        }
        for (int v : comp) visited_global.insert(v);

        bool all_deg2 = true;
        for (int v : comp)
            if (adj[v].size() != 2) { all_deg2 = false; break; }
        if (comp.size() < 4 || !all_deg2) continue;

        std::vector<std::pair<int, int>> edges;
        for (int i : comp)
            for (int j : adj[i])
                if (j > i) edges.emplace_back(i, j);
        std::sort(edges.begin(), edges.end(), [&](const auto& e1, const auto& e2) {
            const double d1 = (cones[e1.first].pos - cones[e1.second].pos).norm();
            const double d2 = (cones[e2.first].pos - cones[e2.second].pos).norm();
            return d1 > d2;
        });
        for (size_t k = 0; k < std::min<size_t>(2, edges.size()); ++k) {
            auto [i, j] = edges[k];
            auto& vi = adj[i]; vi.erase(std::remove(vi.begin(), vi.end(), j), vi.end());
            auto& vj = adj[j]; vj.erase(std::remove(vj.begin(), vj.end(), i), vj.end());
        }
    }
    return adj;
}

double bearing(const Vec2& p0, const Vec2& p1) {
    const Vec2 d = p1 - p0;
    return std::atan2(d.y(), d.x());
}

}  // namespace

// ────────────────────────────────────────────────────────────────────
//  4.0 Nachbarschaftsgraph + Ketten-Zerlegung
// ────────────────────────────────────────────────────────────────────

std::unordered_map<int, std::vector<int>> buildChainGraph(const std::vector<Cone>& cones,
                                                            const VehicleParams& vehicle) {
    const double neighbor_dist = adaptiveNeighborDist(cones, vehicle);
    auto adj = neighborGraph(cones, neighbor_dist);
    adj = pruneToChains(adj, cones);
    adj = breakCycles(adj, cones);
    return adj;
}

std::vector<int> walkChain(const std::unordered_map<int, std::vector<int>>& adj,
                            const std::vector<Cone>& cones, int start) {
    std::vector<int> chain{start};
    std::set<int> visited{start};
    while (true) {
        const int cur = chain.back();
        std::vector<int> nxt;
        auto it = adj.find(cur);
        if (it != adj.end())
            for (int nb : it->second)
                if (!visited.count(nb)) nxt.push_back(nb);
        if (nxt.empty()) break;
        std::sort(nxt.begin(), nxt.end(), [&](int a, int b) {
            return (cones[cur].pos - cones[a].pos).norm() < (cones[cur].pos - cones[b].pos).norm();
        });
        chain.push_back(nxt[0]);
        visited.insert(nxt[0]);
        if (chain.size() > adj.size() + 1) break;
    }
    return chain;
}

std::vector<Chain> decomposeChains(const std::vector<Cone>& cones, const VehicleParams& vehicle) {
    const size_t n = cones.size();
    if (n == 0) return {};
    auto adj = buildChainGraph(cones, vehicle);

    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        const size_t da = adj.count(a) ? adj[a].size() : 0;
        const size_t db = adj.count(b) ? adj[b].size() : 0;
        return da < db;
    });

    std::set<int> visited;
    std::vector<Chain> chains;
    for (int start : order) {
        if (visited.count(start)) continue;
        auto chain_idx = walkChain(adj, cones, start);
        const bool all_visited = std::all_of(chain_idx.begin(), chain_idx.end(),
                                              [&](int i) { return visited.count(i) != 0; });
        if (all_visited) continue;
        for (int i : chain_idx) visited.insert(i);
        Chain chain;
        chain.reserve(chain_idx.size());
        for (int i : chain_idx) chain.push_back(cones[i]);
        chains.push_back(std::move(chain));
    }
    return chains;
}

double chainMeanSpacing(const Chain& chain) {
    if (chain.size() < 2) return std::numeric_limits<double>::infinity();
    double total = 0.0;
    for (size_t i = 1; i < chain.size(); ++i) total += (chain[i].pos - chain[i - 1].pos).norm();
    return total / static_cast<double>(chain.size() - 1);
}

std::optional<Chain> pickWallChain(const std::vector<Chain>& chains, int min_len) {
    std::optional<Chain> best;
    double best_spacing = std::numeric_limits<double>::infinity();
    for (const auto& c : chains) {
        if (static_cast<int>(c.size()) < min_len) continue;
        const double sp = chainMeanSpacing(c);
        if (sp < best_spacing) { best_spacing = sp; best = c; }
    }
    return best;
}

std::optional<Chain> pickWallChainContaining(const std::vector<Chain>& chains, int min_len, int cone_id) {
    for (const auto& c : chains) {
        if (static_cast<int>(c.size()) < min_len) continue;
        for (const auto& cone : c) {
            if (cone.id == cone_id) return c;
        }
    }
    return std::nullopt;
}

Chain orientChainTowardCar(const Chain& chain, const Vec2& car_xy) {
    if (chain.size() < 2) return chain;
    const double d_first = (chain.front().pos - car_xy).norm();
    const double d_last = (chain.back().pos - car_xy).norm();
    if (d_last < d_first) return Chain(chain.rbegin(), chain.rend());
    return chain;
}

Chain trimChainOutliers(const Chain& chain_in, double max_kink_deg) {
    Chain chain = chain_in;
    const double max_kink_rad = max_kink_deg * kPi / 180.0;
    while (chain.size() > 3) {
        const Vec2 &p0 = chain[0].pos, &p1 = chain[1].pos, &p2 = chain[2].pos;
        if (std::abs(wrapPi(bearing(p1, p2) - bearing(p0, p1))) > max_kink_rad) {
            chain.erase(chain.begin());
            continue;
        }
        break;
    }
    while (chain.size() > 3) {
        const size_t n = chain.size();
        const Vec2 &p0 = chain[n - 3].pos, &p1 = chain[n - 2].pos, &p2 = chain[n - 1].pos;
        if (std::abs(wrapPi(bearing(p1, p2) - bearing(p0, p1))) > max_kink_rad) {
            chain.pop_back();
            continue;
        }
        break;
    }
    return chain;
}

std::vector<Vec2> mirrorChainThrough(const Chain& chain, const Cone& anchor,
                                      const Vec2& mirror_target) {
    std::vector<Vec2> pts;
    pts.reserve(chain.size());
    for (const auto& c : chain) pts.push_back(c.pos);
    if (pts.empty()) return pts;

    // Ankerindex: der zu `anchor` naechstgelegene Kettenpunkt (Positions-
    // vergleich statt Cone::id -- robust, auch falls `anchor` kein Element
    // von `chain` ist, siehe Funktionskommentar in cone_corridor.hpp).
    size_t anchor_idx = 0;
    double best_d2 = std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < pts.size(); ++i) {
        const double d2 = (pts[i] - anchor.pos).squaredNorm();
        if (d2 < best_d2) { best_d2 = d2; anchor_idx = i; }
    }

    const double theta_anchor = neighborOrientation(pts, static_cast<int>(anchor_idx));
    const Vec2 perp_anchor(-std::sin(theta_anchor), std::cos(theta_anchor));
    // Nur die Normalkomponente von (mirror_target - anchor.pos) ist eine
    // Korridor-BREITE -- ein laengsseitiger Anteil (die eine bekannte
    // Gegenpylone liegt selten exakt quer zu anchor) sagt nichts ueber die
    // Breite aus und wuerde sich beim Rotieren mit der Kettenkruemmung sonst
    // verzerrt auf die gesamte virtuelle Reihe uebertragen.
    const double lateral = perp_anchor.dot(mirror_target - anchor.pos);

    std::vector<Vec2> out(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) {
        const double theta = neighborOrientation(pts, static_cast<int>(i));
        const Vec2 perp(-std::sin(theta), std::cos(theta));
        out[i] = pts[i] + lateral * perp;
    }
    return out;
}

Chain growChainFromCone(const Cone& start, const std::vector<Cone>& pool,
                         double growth_factor, std::optional<double> fixed_threshold) {
    Chain chain{start};
    std::set<int> visited{start.id};
    while (true) {
        const Cone& cur = chain.back();
        const Cone* nearest = nullptr;
        double nearest_d = std::numeric_limits<double>::infinity();
        for (const auto& c : pool) {
            if (visited.count(c.id)) continue;
            const double d = (c.pos - cur.pos).norm();
            if (d < nearest_d) { nearest_d = d; nearest = &c; }
        }
        if (!nearest) break;

        double threshold;
        if (fixed_threshold) {
            threshold = *fixed_threshold;
        } else if (chain.size() >= 2) {
            threshold = chainMeanSpacing(chain) * growth_factor;
        } else {
            // Erster Schritt der "hinteren" Kette: noch kein d_mean bekannt --
            // unbeschraenkt (die naechstgelegene Pylone insgesamt).
            threshold = std::numeric_limits<double>::infinity();
        }
        if (nearest_d > threshold) break;

        chain.push_back(*nearest);
        visited.insert(nearest->id);
    }
    return chain;
}

// ────────────────────────────────────────────────────────────────────
//  4.0b Reihen-Classifier — feste Distanzschwelle statt adaptivem Wachstum
// ────────────────────────────────────────────────────────────────────
//
//  Siehe cone_corridor.hpp fuer die Motivation (Ersatz fuer growChainFromCone()
//  in der laufend gefahrenen Pipeline, CorridorPipeline::runPipeline()).

Line2 fitLineTLS(const std::vector<Vec2>& pts) {
    Line2 line;
    const size_t n = pts.size();
    if (n == 0) return line;

    Vec2 centroid(0.0, 0.0);
    for (const auto& p : pts) centroid += p;
    centroid /= static_cast<double>(n);
    line.point = centroid;

    if (n < 2) {
        line.dir = Vec2(1.0, 0.0);
        return line;
    }

    double sxx = 0.0, sxy = 0.0, syy = 0.0;
    for (const auto& p : pts) {
        const Vec2 d = p - centroid;
        sxx += d.x() * d.x();
        sxy += d.x() * d.y();
        syy += d.y() * d.y();
    }

    // Hauptachse der 2x2-Kovarianzmatrix in geschlossener Form (kein
    // allgemeiner Eigenloeser noetig): Rotationswinkel, der die Kovarianz
    // diagonalisiert.
    const double angle = 0.5 * std::atan2(2.0 * sxy, sxx - syy);
    Vec2 dir(std::cos(angle), std::sin(angle));
    if (dir.squaredNorm() < 1e-18) dir = Vec2(1.0, 0.0);
    line.dir = dir.normalized();
    return line;
}

double projectOntoLine(const Line2& line, const Vec2& p) {
    return (p - line.point).dot(line.dir);
}

std::optional<Vec2> intersectLineWithPolyline(const Line2& line,
                                               const std::vector<Vec2>& polyline,
                                               const Vec2& car_xy) {
    std::optional<Vec2> best;
    double best_d2 = std::numeric_limits<double>::infinity();

    for (size_t i = 0; i + 1 < polyline.size(); ++i) {
        const Vec2 seg = polyline[i + 1] - polyline[i];
        // Loese line.point + t*line.dir == polyline[i] + u*seg nach (t,u).
        Eigen::Matrix2d A;
        A << line.dir.x(), -seg.x(),
             line.dir.y(), -seg.y();
        const double det = A.determinant();
        if (std::abs(det) < 1e-12) continue;  // parallel
        const Vec2 rhs = polyline[i] - line.point;
        const Vec2 tu = A.inverse() * rhs;
        const double u = tu.y();
        if (u < 0.0 || u > 1.0) continue;

        const Vec2 hit = polyline[i] + u * seg;
        const double d2 = (hit - car_xy).squaredNorm();
        if (d2 < best_d2) { best_d2 = d2; best = hit; }
    }
    return best;
}

namespace {

std::vector<std::vector<int>> connectedComponents(
    const std::unordered_map<int, std::vector<int>>& adj, size_t n) {
    std::vector<bool> visited(n, false);
    std::vector<std::vector<int>> components;
    for (size_t start = 0; start < n; ++start) {
        if (visited[start]) continue;
        std::vector<int> comp;
        std::vector<int> stack{static_cast<int>(start)};
        visited[start] = true;
        while (!stack.empty()) {
            const int cur = stack.back();
            stack.pop_back();
            comp.push_back(cur);
            auto it = adj.find(cur);
            if (it == adj.end()) continue;
            for (int nb : it->second) {
                if (!visited[nb]) { visited[nb] = true; stack.push_back(nb); }
            }
        }
        components.push_back(std::move(comp));
    }
    return components;
}

/// Index der Komponente, die die Pylone mit dieser id fuehrt. -1, wenn keine.
int componentContaining(const std::vector<std::vector<int>>& components,
                        const std::vector<Cone>& cones, int cone_id) {
    for (size_t ci = 0; ci < components.size(); ++ci)
        for (int idx : components[ci])
            if (cones[idx].id == cone_id) return static_cast<int>(ci);
    return -1;
}

/// Kleinster paarweiser Pylonenabstand zwischen zwei Komponenten, quadriert.
double componentDistanceSquared(const std::vector<int>& a, const std::vector<int>& b,
                                const std::vector<Cone>& cones) {
    double smallest = std::numeric_limits<double>::infinity();
    for (int ia : a)
        for (int ib : b)
            smallest = std::min(smallest, (cones[ia].pos - cones[ib].pos).squaredNorm());
    return smallest;
}

/// Die `from` naechstgelegene ANDERE Komponente mit mindestens zwei
/// Mitgliedern. -1, wenn es keine solche gibt.
int nearestUsableComponent(const std::vector<std::vector<int>>& components,
                           const std::vector<Cone>& cones, int from) {
    int best = -1;
    double best_d2 = std::numeric_limits<double>::infinity();
    for (size_t ci = 0; ci < components.size(); ++ci) {
        if (static_cast<int>(ci) == from || components[ci].size() < 2) continue;
        const double d2 = componentDistanceSquared(components[from], components[ci], cones);
        if (d2 < best_d2) { best_d2 = d2; best = static_cast<int>(ci); }
    }
    return best;
}

/// Positionen der Pylonen zu einer Indexliste.
std::vector<Vec2> positionsOf(const std::vector<Cone>& cones,
                              const std::vector<int>& indices) {
    std::vector<Vec2> out;
    out.reserve(indices.size());
    for (int idx : indices) out.push_back(cones[idx].pos);
    return out;
}

/// Senkrechter Abstand eines Punkts zur Geraden.
double lateralDistance(const Line2& line, const Vec2& point) {
    const Vec2 d = point - line.point;
    return std::abs(d.x() * line.dir.y() - d.y() * line.dir.x());
}

/// Nimmt alle Komponenten in die hintere Reihe auf, die vollstaendig auf deren
/// Gerade liegen, und meldet, ob dabei etwas dazukam.
///
/// Faellt in einer Reihe eine Detektion aus, zerreisst der Nachbarschaftsgraph
/// sie in zwei Komponenten. Ohne diese Aufnahme wuerde die Fortsetzung
/// derselben Wand faelschlich zur Gegenreihe erklaert, oder die hintere Kette
/// endete mitten im Korridor und das Fahrzeug bliebe dort stehen.
bool absorbCollinearComponents(const std::vector<std::vector<int>>& components,
                               const std::vector<Cone>& cones, const Line2& rear_line,
                               double max_lateral, std::vector<bool>& claimed,
                               std::vector<int>& rear_idx) {
    bool absorbed_any = false;
    for (size_t ci = 0; ci < components.size(); ++ci) {
        if (claimed[ci]) continue;

        bool all_on_line = true;
        for (int idx : components[ci]) {
            if (lateralDistance(rear_line, cones[idx].pos) >= max_lateral) {
                all_on_line = false;
                break;
            }
        }
        if (all_on_line == false) continue;

        claimed[ci] = true;
        absorbed_any = true;
        rear_idx.insert(rear_idx.end(), components[ci].begin(), components[ci].end());
    }
    return absorbed_any;
}

/// Die der hinteren Reihe naechstgelegene freie Komponente, die tatsaechlich
/// seitlich von ihr liegt. -1, wenn keine passt.
///
/// Das Seitlichkeits-Gate verhindert, dass ein nicht aufgenommener kollinearer
/// Rest derselben Wand als Gegenreihe durchgeht.
int findFrontComponent(const std::vector<std::vector<int>>& components,
                       const std::vector<Cone>& cones, const std::vector<bool>& claimed,
                       const std::vector<int>& rear_idx, const Line2& rear_line,
                       double min_lateral) {
    int best = -1;
    double best_d2 = std::numeric_limits<double>::infinity();
    for (size_t ci = 0; ci < components.size(); ++ci) {
        if (claimed[ci]) continue;

        if (min_lateral > 0.0) {
            double lateral_sum = 0.0;
            for (int idx : components[ci])
                lateral_sum += lateralDistance(rear_line, cones[idx].pos);
            const double lateral_mean =
                lateral_sum / static_cast<double>(components[ci].size());
            if (lateral_mean < min_lateral) continue;
        }

        const double d2 = componentDistanceSquared(rear_idx, components[ci], cones);
        if (d2 < best_d2) { best_d2 = d2; best = static_cast<int>(ci); }
    }
    return best;
}

}  // namespace

// Ordnet jede Pylone einer der beiden Reihen zu.
//
// Grundidee: Pylonen derselben Reihe stehen nah beieinander, Pylonen
// verschiedener Reihen deutlich weiter auseinander. Die Zerlegung laeuft
// deshalb ueber eine FESTE Kantenlaenge. Eine mitwachsende Schwelle kippte die
// Kette an engen Stellen auf die Gegenreihe, weil jeder etwas laengere Schritt
// den Mittelwert und damit die Schwelle anhob.
RowClassification classifyRows(const std::vector<Cone>& cones, const Cone& trigger,
                               double same_side_max_dist, double cross_side_min_dist) {
    RowClassification rc;
    if (cones.size() < 2) return rc;

    // ── Schritt 1: Pylonen in Zusammenhangskomponenten zerlegen ─────────
    const auto components =
        connectedComponents(neighborGraph(cones, same_side_max_dist), cones.size());

    const int trigger_component = componentContaining(components, cones, trigger.id);
    if (trigger_component < 0) return rc;

    // ── Schritt 2: die zu verfolgende Wand waehlen ──────────────────────
    // Normalfall ist die Komponente der Trigger-Pylone. Steht die Trigger-
    // Pylone noch allein, waehrend gegenueber schon eine ganze Reihe steht,
    // uebernimmt diese Reihe die Wandrolle. Der Rollentausch wird gemeldet,
    // sonst kann der Aufrufer die beiden Faelle nicht unterscheiden.
    int rear_component = trigger_component;
    if (components[trigger_component].size() < 2) {
        rear_component = nearestUsableComponent(components, cones, trigger_component);
        if (rear_component < 0) return rc;
        rc.trigger_row_isolated = true;
    }

    std::vector<int> rear_idx = components[rear_component];
    if (rear_idx.size() < 2) return rc;
    rc.rear_line = fitLineTLS(positionsOf(cones, rear_idx));

    // ── Schritt 3: zerrissene Teile derselben Wand einsammeln ───────────
    std::vector<bool> claimed(components.size(), false);
    claimed[static_cast<size_t>(rear_component)] = true;

    const double half_row_gap = 0.5 * cross_side_min_dist;
    if (half_row_gap > 0.0) {
        const bool absorbed = absorbCollinearComponents(
            components, cones, rc.rear_line, half_row_gap, claimed, rear_idx);
        if (absorbed) rc.rear_line = fitLineTLS(positionsOf(cones, rear_idx));
    }

    // ── Schritt 4: hintere Kette entlang ihrer Geraden sortieren ────────
    // Danach sind front() und back() die beiden Enden der Reihe.
    std::sort(rear_idx.begin(), rear_idx.end(), [&](int a, int b) {
        return projectOntoLine(rc.rear_line, cones[a].pos) <
               projectOntoLine(rc.rear_line, cones[b].pos);
    });
    rc.rear_chain.reserve(rear_idx.size());
    for (int idx : rear_idx) rc.rear_chain.push_back(cones[idx]);

    // Schritt 3 kann die zuvor isolierte Trigger-Pylone nachtraeglich doch in
    // die hintere Reihe geholt haben. Dann sind die Rollen nicht vertauscht.
    if (rc.trigger_row_isolated) {
        for (const auto& cone : rc.rear_chain) {
            if (cone.id == trigger.id) { rc.trigger_row_isolated = false; break; }
        }
    }

    // ── Schritt 5: die Gegenreihe bestimmen ─────────────────────────────
    const int front_component = findFrontComponent(components, cones, claimed,
                                                   rear_idx, rc.rear_line, half_row_gap);
    if (front_component >= 0) {
        rc.front_chain.reserve(components[front_component].size());
        for (int idx : components[front_component]) rc.front_chain.push_back(cones[idx]);
    }

    rc.ok = true;
    return rc;
}

// ────────────────────────────────────────────────────────────────────
//  4.1 Referenzlinie
// ────────────────────────────────────────────────────────────────────

std::vector<Vec2> resampleChainSpline(const Chain& chain, int n_samples) {
    std::vector<Vec2> pts;
    pts.reserve(chain.size());
    for (const auto& c : chain) pts.push_back(c.pos);

    std::vector<Vec2> kept;
    for (size_t i = 0; i < pts.size(); ++i)
        if (i == 0 || (pts[i] - pts[i - 1]).norm() > 1e-9) kept.push_back(pts[i]);

    if (kept.empty()) return std::vector<Vec2>(n_samples, Vec2(0.0, 0.0));
    if (kept.size() == 1) return std::vector<Vec2>(n_samples, kept[0]);
    if (kept.size() == 2) {
        std::vector<Vec2> out(n_samples);
        for (int k = 0; k < n_samples; ++k) {
            const double t = (n_samples > 1) ? static_cast<double>(k) / (n_samples - 1) : 0.0;
            out[k] = kept[0] + t * (kept[1] - kept[0]);
        }
        return out;
    }

    std::vector<double> s(kept.size(), 0.0);
    for (size_t i = 1; i < kept.size(); ++i) s[i] = s[i - 1] + (kept[i] - kept[i - 1]).norm();
    std::vector<double> xs(kept.size()), ys(kept.size());
    for (size_t i = 0; i < kept.size(); ++i) { xs[i] = kept[i].x(); ys[i] = kept[i].y(); }
    const auto spx = fitNaturalCubicSpline(s, xs);
    const auto spy = fitNaturalCubicSpline(s, ys);

    std::vector<Vec2> out(n_samples);
    for (int k = 0; k < n_samples; ++k) {
        const double su = s.back() * static_cast<double>(k) / static_cast<double>(n_samples - 1);
        out[k] = Vec2(evalCubicSpline(spx, su), evalCubicSpline(spy, su));
    }
    return out;
}

std::vector<double> measureHalfWidthSamples(const std::vector<Vec2>& wall_xy,
                                             const std::vector<Cone>& other_cones) {
    std::vector<double> samples;
    if (wall_xy.size() < 2 || other_cones.empty()) return samples;
    samples.reserve(other_cones.size());
    for (const auto& c : other_cones) {
        double best_d2 = std::numeric_limits<double>::infinity();
        for (const auto& p : wall_xy) best_d2 = std::min(best_d2, (p - c.pos).squaredNorm());
        samples.push_back(std::sqrt(best_d2) / 2.0);
    }
    return samples;
}

double signTowardPoint(const std::vector<Vec2>& side_pts, const Vec2& point) {
    size_t idx = 0;
    double best_d2 = std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < side_pts.size(); ++i) {
        const double d2 = (side_pts[i] - point).squaredNorm();
        if (d2 < best_d2) { best_d2 = d2; idx = i; }
    }
    const auto th = tangents(side_pts);
    const Vec2 perp(-std::sin(th[idx]), std::cos(th[idx]));
    return (perp.dot(point - side_pts[idx]) >= 0.0) ? 1.0 : -1.0;
}

std::vector<Vec2> offsetPolyline(const std::vector<Vec2>& side_pts, double half_width, double sign) {
    const auto th = tangents(side_pts);
    std::vector<Vec2> out(side_pts.size());
    for (size_t i = 0; i < side_pts.size(); ++i) {
        const Vec2 perp(-std::sin(th[i]), std::cos(th[i]));
        out[i] = side_pts[i] + sign * half_width * perp;
    }
    return out;
}

std::vector<Vec2> blendCenterlines(const std::vector<Vec2>& old_cl, const std::vector<Vec2>& new_cl,
                                    double blend_distance) {
    if (old_cl.size() < 2 || new_cl.size() < 2 || blend_distance <= 0.0) return new_cl;

    // old_cl und new_cl koennen aus unterschiedlichen Planungszweigen stammen
    // (z.B. Kette -> Voronoi). Die Bogenlaengen-Interpolation unten setzt aber
    // voraus, dass beide Pfade am Eingang in dieselbe Fahrtrichtung laufen.
    // Ohne diese Pruefung kann der Blend am Eingang eine Haarnadel erzeugen;
    // laneEntryPoses() gibt planTransition() dann einen um pi gedrehten
    // Zielwinkel.
    auto entryDirection = [&](const std::vector<Vec2>& path) -> Vec2 {
        const Vec2 start = path.front();
        double travelled = 0.0;
        const double lookahead = std::min(0.5, blend_distance);
        for (size_t i = 1; i < path.size(); ++i) {
            travelled += (path[i] - path[i - 1]).norm();
            const Vec2 d = path[i] - start;
            if (travelled >= lookahead && d.norm() > 1e-9) return d.normalized();
        }
        const Vec2 d = path.back() - start;
        return d.norm() > 1e-9 ? d.normalized() : Vec2::Zero();
    };

    const Vec2 old_dir = entryDirection(old_cl);
    const Vec2 new_dir = entryDirection(new_cl);
    if (old_dir.squaredNorm() < 1e-12 || new_dir.squaredNorm() < 1e-12 ||
        old_dir.dot(new_dir) <= 0.0) {
        return new_cl;
    }

    std::vector<double> s_new(new_cl.size(), 0.0);
    for (size_t i = 1; i < new_cl.size(); ++i) s_new[i] = s_new[i - 1] + (new_cl[i] - new_cl[i - 1]).norm();
    std::vector<double> s_old(old_cl.size(), 0.0);
    for (size_t i = 1; i < old_cl.size(); ++i) s_old[i] = s_old[i - 1] + (old_cl[i] - old_cl[i - 1]).norm();

    auto interp1d = [&](double s, int axis) -> double {
        if (s <= s_old.front()) return axis == 0 ? old_cl.front().x() : old_cl.front().y();
        if (s >= s_old.back()) return axis == 0 ? old_cl.back().x() : old_cl.back().y();
        auto it = std::upper_bound(s_old.begin(), s_old.end(), s);
        const size_t j = std::distance(s_old.begin(), it);
        const size_t i = j - 1;
        const double t = (s_old[j] - s_old[i] > 1e-12) ? (s - s_old[i]) / (s_old[j] - s_old[i]) : 0.0;
        const double a = axis == 0 ? old_cl[i].x() : old_cl[i].y();
        const double b = axis == 0 ? old_cl[j].x() : old_cl[j].y();
        return a + t * (b - a);
    };

    const double effective_blend = (s_new.back() > 1e-9) ? std::min(blend_distance, s_new.back())
                                                           : blend_distance;

    std::vector<Vec2> blended(new_cl.size());
    for (size_t i = 0; i < new_cl.size(); ++i) {
        const double old_x = interp1d(s_new[i], 0);
        const double old_y = interp1d(s_new[i], 1);
        const double t = std::clamp(s_new[i] / effective_blend, 0.0, 1.0);
        const double w = t * t * (3.0 - 2.0 * t);
        blended[i] = Vec2((1.0 - w) * old_x + w * new_cl[i].x(), (1.0 - w) * old_y + w * new_cl[i].y());
    }

    // Auch gleich orientierte Eingaben koennen bei stark versetzten
    // Eingangspositionen waehrend der Ueberblendung lokal zuruecklaufen. Ein
    // nichtpositives Skalarprodukt aufeinanderfolgender Segmente entspricht
    // einer >=90-Grad-Ecke. Dann ist die frische Mittellinie sicherer.
    for (size_t i = 1; i + 1 < blended.size() && s_new[i] <= effective_blend + 1e-9; ++i) {
        const Vec2 before = blended[i] - blended[i - 1];
        const Vec2 after  = blended[i + 1] - blended[i];
        const double scale = before.norm() * after.norm();
        if (scale < 1e-12 || before.dot(after) <= 0.0) return new_cl;
    }

    // Der gesamte Eingang kann trotz lokal gleichgerichteter Segmente durch
    // den Positionsversatz rueckwaerts zeigen. Auch dann den Blend verwerfen.
    const Vec2 blended_dir = entryDirection(blended);
    if (blended_dir.squaredNorm() < 1e-12 || blended_dir.dot(new_dir) <= 0.0) return new_cl;
    return blended;
}

// ── LatticePlanner ─────────────────────────────────────────────────────────

LatticePlanner::LatticePlanner(VehicleParams vehicle, LatticeConfig config)
    : vehicle_(vehicle), config_(config) {}

void LatticePlanner::reset() {
    reference_.reset();
    last_chosen_d_.reset();
}

std::optional<ReferenceLine> LatticePlanner::update(const std::vector<Cone>& known_cones,
                                                      const Pose& car_pose) {
    const auto& cfg = config_;
    auto chains = known_cones.empty() ? std::vector<Chain>{} : decomposeChains(known_cones, vehicle_);
    auto wall_chain_opt = pickWallChain(chains, cfg.min_wall_chain_len);

    if (!wall_chain_opt) {
        // Keine Kette lang genug: alte Referenz geometrisch unveraendert weiterreichen.
        if (reference_) reference_->extrapolated = true;
        return reference_;
    }
    const Chain& wall_chain = *wall_chain_opt;

    std::vector<int> wall_ids;
    wall_ids.reserve(wall_chain.size());
    for (const auto& c : wall_chain) wall_ids.push_back(c.id);
    std::set<int> wall_id_set(wall_ids.begin(), wall_ids.end());

    std::vector<Vec2> wall_xy = resampleChainSpline(wall_chain, cfg.n_samples);
    std::vector<double> wall_s(wall_xy.size(), 0.0);
    for (size_t i = 1; i < wall_xy.size(); ++i) wall_s[i] = wall_s[i - 1] + (wall_xy[i] - wall_xy[i - 1]).norm();
    std::vector<double> wall_theta = tangents(wall_xy);

    std::vector<Cone> other_cones;
    for (const auto& c : known_cones)
        if (!wall_id_set.count(c.id)) other_cones.push_back(c);

    const auto width_samples = measureHalfWidthSamples(wall_xy, other_cones);
    const double prev_half_width = reference_ ? reference_->half_width_m : cfg.default_half_width_m;
    double half_width;
    if (!width_samples.empty()) {
        const double measured = median(width_samples);
        half_width = (1.0 - cfg.half_width_smoothing) * prev_half_width + cfg.half_width_smoothing * measured;
    } else {
        half_width = prev_half_width;
    }

    if (reference_ && !reference_->extrapolated) {
        wall_xy = blendCenterlines(reference_->xy, wall_xy, cfg.reference_blend_distance_m);
        wall_s.assign(wall_xy.size(), 0.0);
        for (size_t i = 1; i < wall_xy.size(); ++i) wall_s[i] = wall_s[i - 1] + (wall_xy[i] - wall_xy[i - 1]).norm();
        wall_theta = tangents(wall_xy);
    }

    const Vec2 car_xy(car_pose.x, car_pose.y);
    const double sign = signTowardPoint(wall_xy, car_xy);
    const std::vector<Vec2> centerline_xy = offsetPolyline(wall_xy, half_width, sign);

    ReferenceLine ref;
    ref.xy = std::move(wall_xy);
    ref.theta = std::move(wall_theta);
    ref.s = std::move(wall_s);
    ref.half_width_m = half_width;
    ref.centerline_xy = centerline_xy;
    ref.sign = sign;
    ref.wall_chain_ids = std::move(wall_ids);
    ref.extrapolated = false;
    reference_ = std::move(ref);
    return reference_;
}

std::optional<LatticeCandidate> LatticePlanner::plan(const std::vector<Cone>& known_cones,
                                                       const Pose& car_pose) {
    auto ref = update(known_cones, car_pose);
    if (!ref) return std::nullopt;
    auto candidates = generateCandidates(*ref, car_pose, config_);
    if (candidates.empty()) return std::nullopt;
    auto result = selectCandidate(*ref, candidates, known_cones, vehicle_, last_chosen_d_, config_);
    if (!result) return std::nullopt;
    const auto& best_candidate = std::get<0>(*result);
    last_chosen_d_ = best_candidate.d_target;
    return best_candidate;
}

// ────────────────────────────────────────────────────────────────────
//  4.2 Kandidaten (Quintik-Lattice, Alternativpfad)
// ────────────────────────────────────────────────────────────────────

std::array<double, 6> solveQuintic(double d0, double d0p, double d0pp,
                                    double dT, double dTp, double dTpp, double S) {
    if (S < 1e-6) return {dT, 0.0, 0.0, 0.0, 0.0, 0.0};
    Eigen::Matrix<double, 6, 6> A;
    A << 1.0, 0.0,     0.0,       0.0,        0.0,         0.0,
         0.0, 1.0,     0.0,       0.0,        0.0,         0.0,
         0.0, 0.0,     2.0,       0.0,        0.0,         0.0,
         1.0, S,        S * S,     S * S * S,  S * S * S * S,  S * S * S * S * S,
         0.0, 1.0,     2 * S,     3 * S * S,  4 * S * S * S, 5 * S * S * S * S,
         0.0, 0.0,     2.0,       6 * S,      12 * S * S,   20 * S * S * S;
    Eigen::Matrix<double, 6, 1> b;
    b << d0, d0p, d0pp, dT, dTp, dTpp;
    const Eigen::Matrix<double, 6, 1> x = A.colPivHouseholderQr().solve(b);
    std::array<double, 6> coeffs{};
    for (int i = 0; i < 6; ++i) coeffs[i] = x(i);
    return coeffs;
}

void sampleReferenceAt(const ReferenceLine& ref, const std::vector<double>& s_query,
                        std::vector<Vec2>& xy_out, std::vector<double>& theta_out) {
    xy_out.resize(s_query.size());
    theta_out.resize(s_query.size());
    const size_t n = ref.s.size();
    for (size_t k = 0; k < s_query.size(); ++k) {
        const double sc = std::clamp(s_query[k], 0.0, ref.s.back());
        auto it = std::upper_bound(ref.s.begin(), ref.s.end(), sc);
        size_t j = std::min(n - 1, static_cast<size_t>(std::distance(ref.s.begin(), it)));
        if (j == 0) j = 1;
        const size_t i = j - 1;
        const double t = (ref.s[j] - ref.s[i] > 1e-12) ? (sc - ref.s[i]) / (ref.s[j] - ref.s[i]) : 0.0;
        const double x = ref.xy[i].x() + t * (ref.xy[j].x() - ref.xy[i].x());
        const double y = ref.xy[i].y() + t * (ref.xy[j].y() - ref.xy[i].y());
        const double cth = std::cos(ref.theta[i]) + t * (std::cos(ref.theta[j]) - std::cos(ref.theta[i]));
        const double sth = std::sin(ref.theta[i]) + t * (std::sin(ref.theta[j]) - std::sin(ref.theta[i]));
        xy_out[k] = Vec2(x, y);
        theta_out[k] = std::atan2(sth, cth);
    }
}

std::tuple<double, double, double> projectToFrenet(const ReferenceLine& ref, const Pose& car_pose) {
    const Vec2 car_xy(car_pose.x, car_pose.y);
    double best_d2 = std::numeric_limits<double>::infinity();
    double best_s0 = 0.0;
    for (size_t i = 0; i + 1 < ref.xy.size(); ++i) {
        const Vec2 seg = ref.xy[i + 1] - ref.xy[i];
        const double L = seg.norm();
        if (L < 1e-9) continue;
        double t = (car_xy - ref.xy[i]).dot(seg) / (L * L);
        t = std::clamp(t, 0.0, 1.0);
        const Vec2 proj = ref.xy[i] + t * seg;
        const double d2 = (proj - car_xy).squaredNorm();
        if (d2 < best_d2) { best_d2 = d2; best_s0 = ref.s[i] + t * L; }
    }

    std::vector<Vec2> ref_xy0;
    std::vector<double> ref_theta0;
    sampleReferenceAt(ref, {best_s0}, ref_xy0, ref_theta0);
    const double theta0 = ref_theta0[0];
    const Vec2 perp_left(-std::sin(theta0), std::cos(theta0));
    const double d0 = perp_left.dot(car_xy - ref_xy0[0]);
    const double heading_err = wrapPi(car_pose.th - theta0);
    const double d0p = std::tan(std::clamp(heading_err, -1.4, 1.4));
    return {best_s0, d0, d0p};
}

std::vector<double> lateralOffsets(const ReferenceLine& ref, int n, double margin_m) {
    const double lo = margin_m;
    double hi = std::max(margin_m, 2.0 * ref.half_width_m - margin_m);
    if (hi <= lo) hi = lo + 1e-3;
    std::vector<double> out(n);
    for (int i = 0; i < n; ++i) {
        const double t = (n > 1) ? static_cast<double>(i) / (n - 1) : 0.0;
        out[i] = ref.sign * (lo + t * (hi - lo));
    }
    return out;
}

std::vector<LatticeCandidate> generateCandidates(const ReferenceLine& ref, const Pose& car_pose,
                                                  const LatticeConfig& config) {
    const auto [s0, d0, d0p] = projectToFrenet(ref, car_pose);
    const double available = ref.s.back() - s0;
    const auto d_targets = lateralOffsets(ref, config.n_lateral_candidates, config.lateral_margin_m);

    std::vector<LatticeCandidate> candidates;
    for (double S : config.horizons_m) {
        const double S_eff = std::min(S, available);
        if (S_eff < config.candidate_ds * 2.0) continue;
        const int n_pts = std::max(2, static_cast<int>(std::round(S_eff / config.candidate_ds)) + 1);
        std::vector<double> s_local(n_pts);
        for (int i = 0; i < n_pts; ++i)
            s_local[i] = (n_pts > 1) ? S_eff * i / (n_pts - 1) : 0.0;

        for (double d_target : d_targets) {
            const auto coeffs = solveQuintic(d0, d0p, 0.0, d_target, 0.0, 0.0, S_eff);
            std::array<double, 5> dcoef{};
            for (int k = 1; k < 6; ++k) dcoef[k - 1] = coeffs[k] * k;

            std::vector<double> d_vals(n_pts), d_prime_vals(n_pts);
            for (int i = 0; i < n_pts; ++i) {
                const double s = s_local[i];
                double val = 0.0, sp = 1.0;
                for (int k = 0; k < 6; ++k) { val += coeffs[k] * sp; sp *= s; }
                d_vals[i] = val;
                double dval = 0.0; sp = 1.0;
                for (int k = 0; k < 5; ++k) { dval += dcoef[k] * sp; sp *= s; }
                d_prime_vals[i] = dval;
            }

            std::vector<double> s_query(n_pts);
            for (int i = 0; i < n_pts; ++i) s_query[i] = s0 + s_local[i];
            std::vector<Vec2> ref_xy;
            std::vector<double> ref_theta;
            sampleReferenceAt(ref, s_query, ref_xy, ref_theta);

            LatticeCandidate cand;
            cand.d_target = d_target;
            cand.s_horizon = S_eff;
            cand.xy.resize(n_pts);
            cand.theta.resize(n_pts);
            for (int i = 0; i < n_pts; ++i) {
                const Vec2 perp_left(-std::sin(ref_theta[i]), std::cos(ref_theta[i]));
                cand.xy[i] = ref_xy[i] + d_vals[i] * perp_left;
                cand.theta[i] = ref_theta[i] + std::atan(d_prime_vals[i]);
            }
            cand.s.assign(n_pts, 0.0);
            for (int i = 1; i < n_pts; ++i) cand.s[i] = cand.s[i - 1] + (cand.xy[i] - cand.xy[i - 1]).norm();
            cand.d_coeffs = coeffs;
            candidates.push_back(std::move(cand));
        }
    }
    return candidates;
}

// ────────────────────────────────────────────────────────────────────
//  4.3 Kosten, Filter, Auswahl
// ────────────────────────────────────────────────────────────────────

std::vector<double> candidateDValues(const LatticeCandidate& candidate) {
    const size_t n = candidate.xy.size();
    std::vector<double> d_vals(n);
    for (size_t i = 0; i < n; ++i) {
        const double s = (n > 1) ? candidate.s_horizon * static_cast<double>(i) / (n - 1) : 0.0;
        double val = 0.0, sp = 1.0;
        for (int k = 0; k < 6; ++k) { val += candidate.d_coeffs[k] * sp; sp *= s; }
        d_vals[i] = val;
    }
    return d_vals;
}

double minWallClearance(const ReferenceLine& ref, const LatticeCandidate& candidate) {
    const auto d_vals = candidateDValues(candidate);
    double m = std::numeric_limits<double>::infinity();
    for (double d : d_vals) m = std::min(m, ref.sign * d);
    return m;
}

bool isFeasible(const ReferenceLine& ref, const LatticeCandidate& candidate,
                const std::vector<Cone>& known_cones, const VehicleParams& vehicle,
                const LatticeConfig& config) {
    if (minWallClearance(ref, candidate) < config.wall_clearance_min_m) return false;
    if (known_cones.empty()) return true;
    std::vector<Obstacle> obstacles;
    obstacles.reserve(known_cones.size());
    for (const auto& c : known_cones) obstacles.push_back({c.pos.x(), c.pos.y(), c.radius});
    std::vector<Pose> poses(candidate.xy.size());
    for (size_t i = 0; i < candidate.xy.size(); ++i)
        poses[i] = {candidate.xy[i].x(), candidate.xy[i].y(), candidate.theta[i]};
    return bodyClear(poses, obstacles, vehicle);
}

double candidateCost(const ReferenceLine& ref, const LatticeCandidate& candidate,
                      const VehicleParams& vehicle, std::optional<double> last_chosen_d,
                      const LatticeConfig& config, CostTerms& terms_out) {
    const double hw = std::max(ref.half_width_m, 1e-6);

    const double target_center = ref.sign * ref.half_width_m;
    terms_out.wall = config.w_wall_offset * std::pow((candidate.d_target - target_center) / hw, 2.0);

    double kmax = 0.0;
    pathCurvatureOk(candidate.xy, vehicle.R_min, kmax);
    terms_out.curvature = config.w_curvature * std::pow(kmax * vehicle.R_min, 2.0);

    terms_out.hysteresis = last_chosen_d
        ? config.w_hysteresis * std::pow((candidate.d_target - *last_chosen_d) / hw, 2.0)
        : 0.0;

    terms_out.exploration = -config.w_exploration * (candidate.d_target * ref.sign / hw);

    return terms_out.wall + terms_out.curvature + terms_out.hysteresis + terms_out.exploration;
}

std::optional<std::tuple<LatticeCandidate, double, CostTerms>> selectCandidate(
    const ReferenceLine& ref, const std::vector<LatticeCandidate>& candidates,
    const std::vector<Cone>& known_cones, const VehicleParams& vehicle,
    std::optional<double> last_chosen_d, const LatticeConfig& config) {
    std::vector<const LatticeCandidate*> feasible;
    for (const auto& c : candidates)
        if (isFeasible(ref, c, known_cones, vehicle, config)) feasible.push_back(&c);
    if (feasible.empty()) return std::nullopt;

    const LatticeCandidate* best = nullptr;
    double best_cost = std::numeric_limits<double>::infinity();
    CostTerms best_terms;
    for (const auto* c : feasible) {
        CostTerms terms;
        const double cost = candidateCost(ref, *c, vehicle, last_chosen_d, config, terms);
        if (cost < best_cost) { best_cost = cost; best = c; best_terms = terms; }
    }
    return std::make_tuple(*best, best_cost, best_terms);
}

// ────────────────────────────────────────────────────────────────────
//  Geometrie-Hilfsfunktionen (Kruemmung/Glaettung/lokale Reparatur)
// ────────────────────────────────────────────────────────────────────

bool pathCurvatureOk(const std::vector<Vec2>& pts, double R_min, double& kmax_out) {
    kmax_out = 0.0;
    if (pts.size() < 3) return true;
    for (size_t i = 1; i + 1 < pts.size(); ++i) {
        const Vec2 &p0 = pts[i - 1], &p1 = pts[i], &p2 = pts[i + 1];
        const double a = (p2 - p1).norm();
        const double b = (p2 - p0).norm();
        const double c = (p1 - p0).norm();
        const double cross = std::abs((p1.x() - p0.x()) * (p2.y() - p0.y()) -
                                       (p1.y() - p0.y()) * (p2.x() - p0.x()));
        if (cross < 1e-12 || a * b * c < 1e-12) continue;
        const double radius = (a * b * c) / (2.0 * cross);
        kmax_out = std::max(kmax_out, 1.0 / radius);
    }
    return kmax_out <= 1.0 / R_min + 1e-6;
}

std::vector<Vec2> smoothPolyline(const std::vector<Vec2>& pts, int iters, double alpha) {
    if (pts.size() < 3) return pts;
    std::vector<Vec2> P = pts;
    for (int it = 0; it < iters; ++it) {
        std::vector<Vec2> Q = P;
        for (size_t i = 1; i + 1 < P.size(); ++i)
            Q[i] = (1.0 - alpha) * P[i] + alpha * 0.5 * (P[i - 1] + P[i + 1]);
        P = std::move(Q);
    }
    return P;
}

bool centerlineFeasible(const std::vector<Vec2>& centerline, bool curv_ok,
                         const std::vector<Cone>& cones, const VehicleParams& vehicle) {
    if (!curv_ok) return false;
    const auto th = tangents(centerline);
    std::vector<Pose> poses(centerline.size());
    for (size_t i = 0; i < centerline.size(); ++i) poses[i] = {centerline[i].x(), centerline[i].y(), th[i]};
    std::vector<Obstacle> obs;
    obs.reserve(cones.size());
    for (const auto& c : cones) obs.push_back({c.pos.x(), c.pos.y(), c.radius});
    return bodyClear(poses, obs, vehicle);
}

namespace {

std::vector<bool> badIndices(const std::vector<Vec2>& centerline, const std::vector<Cone>& cones,
                              const VehicleParams& vehicle) {
    const size_t n = centerline.size();
    std::vector<bool> bad(n, false);
    for (size_t i = 1; i + 1 < n; ++i) {
        const Vec2 &p0 = centerline[i - 1], &p1 = centerline[i], &p2 = centerline[i + 1];
        const double a = (p2 - p1).norm(), b = (p2 - p0).norm(), c = (p1 - p0).norm();
        const double cross = std::abs((p1.x() - p0.x()) * (p2.y() - p0.y()) -
                                       (p1.y() - p0.y()) * (p2.x() - p0.x()));
        if (cross > 1e-12 && a * b * c > 1e-12) {
            const double radius = (a * b * c) / (2.0 * cross);
            if (radius < vehicle.R_min) bad[i] = true;
        }
    }

    const auto th = tangents(centerline);
    const double rear = vehicle.rear_overhang_frac * vehicle.length;
    const double front = vehicle.length - rear;
    const double hw = vehicle.width / 2.0;
    for (const auto& c : cones) {
        for (size_t i = 0; i < n; ++i) {
            const double dx = c.pos.x() - centerline[i].x(), dy = c.pos.y() - centerline[i].y();
            const double ct = std::cos(th[i]), st = std::sin(th[i]);
            const double lx = dx * ct + dy * st, ly = -dx * st + dy * ct;
            const double qx = std::clamp(lx, -rear, front);
            const double qy = std::clamp(ly, -hw, hw);
            const double d2 = (lx - qx) * (lx - qx) + (ly - qy) * (ly - qy);
            if (d2 <= (c.radius + vehicle.clearance) * (c.radius + vehicle.clearance)) bad[i] = true;
        }
    }
    return bad;
}

std::vector<std::pair<int, int>> contiguousRegions(const std::vector<bool>& bad) {
    std::vector<std::pair<int, int>> regions;
    const int n = static_cast<int>(bad.size());
    int i = 0;
    while (i < n) {
        if (bad[i]) {
            int j = i;
            while (j < n && bad[j]) ++j;
            regions.emplace_back(i, j - 1);
            i = j;
        } else {
            ++i;
        }
    }
    return regions;
}

}  // namespace

std::vector<Vec2> localRepair(const std::vector<Vec2>& centerline, const std::vector<Cone>& cones,
                               const VehicleParams& vehicle, int max_passes) {
    std::vector<Vec2> cl = centerline;

    double total_len = 0.0;
    for (size_t i = 1; i < cl.size(); ++i) total_len += (cl[i] - cl[i - 1]).norm();
    const double avg_spacing = total_len / static_cast<double>(std::max<size_t>(1, cl.size() - 1));
    const int pad_points = std::max(15, static_cast<int>(std::round((1.5 * vehicle.R_min) /
                                                                      std::max(avg_spacing, 1e-6))));

    for (int pass = 0; pass < max_passes; ++pass) {
        const auto bad = badIndices(cl, cones, vehicle);
        if (std::none_of(bad.begin(), bad.end(), [](bool b) { return b; })) break;

        for (const auto& [i0, i1] : contiguousRegions(bad)) {
            const int lo = std::max(0, i0 - pad_points);
            const int hi = std::min(static_cast<int>(cl.size()) - 1, i1 + pad_points);
            if (hi - lo < 2) continue;

            std::vector<Vec2> window(cl.begin() + lo, cl.begin() + hi + 1);
            const size_t n_w = window.size();
            std::vector<double> edge_w(n_w);
            for (size_t k = 0; k < n_w; ++k) {
                const double t = (n_w > 1) ? static_cast<double>(k) / (n_w - 1) : 0.0;
                edge_w[k] = std::sin(kPi * t);
            }

            bool had_collision = false;
            for (const auto& c : cones) {
                double dmin = std::numeric_limits<double>::infinity();
                int idx_closest = 0;
                for (size_t k = 0; k < n_w; ++k) {
                    const double d = (window[k] - c.pos).norm();
                    if (d < dmin) { dmin = d; idx_closest = static_cast<int>(k); }
                }
                const double need = c.radius + vehicle.clearance + vehicle.width / 2.0 + 0.001;
                const double deficit = need - dmin;
                if (deficit <= 0.0) continue;
                had_collision = true;

                Vec2 mean_dir = window.back() - window.front();
                const double mean_norm = mean_dir.norm();
                if (mean_norm < 1e-9) continue;
                mean_dir /= mean_norm;
                const Vec2 perp_fixed(-mean_dir.y(), mean_dir.x());
                const Vec2 rel = window[idx_closest] - c.pos;
                const double side = (perp_fixed.dot(rel) >= 0.0) ? 1.0 : -1.0;
                for (size_t k = 0; k < n_w; ++k) window[k] += (deficit * side * edge_w[k]) * perp_fixed;
            }

            if (!had_collision) window = smoothPolyline(window, 10, 0.6);
            for (size_t k = 0; k < n_w; ++k) cl[lo + k] = window[k];
        }
    }
    return cl;
}

// ────────────────────────────────────────────────────────────────────
//  4.5 Fahrvektoren direkt aus der "hinteren" Kette
// ────────────────────────────────────────────────────────────────────

double neighborOrientation(const std::vector<Vec2>& chain_pts, int i) {
    const int n = static_cast<int>(chain_pts.size());
    const bool has_next = i < n - 1;
    const bool has_prev = i > 0;
    if (has_next && has_prev) {
        const Vec2 k = chain_pts[i + 1] - chain_pts[i];
        const Vec2 l = chain_pts[i] - chain_pts[i - 1];
        const double theta_k = std::atan2(k.y(), k.x());
        const double theta_l = std::atan2(l.y(), l.x());
        const double diff = wrapPi(theta_l - theta_k);
        return wrapPi(theta_k + diff / 2.0);
    }
    if (has_next) {
        const Vec2 k = chain_pts[i + 1] - chain_pts[i];
        return std::atan2(k.y(), k.x());
    }
    if (has_prev) {
        const Vec2 l = chain_pts[i] - chain_pts[i - 1];
        return std::atan2(l.y(), l.x());
    }
    return 0.0;
}

namespace {

/// Nur Pylonen auf der befahrbaren Seite der hinteren Kette kommen als
/// Gegenreihe in Frage.
///
/// Alles andere, etwa eine dem Nachbarschaftsgraphen noch nicht angeschlossene
/// Fortsetzung derselben Wand, verfaelscht die Breitenmessung und war eine
/// Quelle geometrisch unsinniger Kurven.
std::vector<Cone> selectFrontCandidates(const std::vector<Cone>& other_cones,
                                        const std::vector<Vec2>& rear_pts, double sign) {
    std::vector<Cone> candidates;
    candidates.reserve(other_cones.size());
    for (const auto& cone : other_cones) {
        if (signTowardPoint(rear_pts, cone.pos) == sign) candidates.push_back(cone);
    }
    return candidates;
}

/// Sortiert die Gegenreihe entlang ihrer eigenen Geraden und richtet sie am
/// autonahen Ende der hinteren Kette aus.
///
/// Geradenfit und Projektion statt erneutem Kettenwachstum: ein adaptiv
/// mitwachsender Schwellwert wuerde hier denselben Kippfehler reproduzieren,
/// den classifyRows() fuer die hintere Kette gerade vermeidet.
///
/// Ohne die Ausrichtung koennte die Gegenreihe gegenlaeufig zur hinteren
/// sortiert sein. Die Mittelpunktbildung paarte die Splines dann verkehrt herum.
Chain orderFrontChain(std::vector<Cone> candidates, const Chain& rear_chain) {
    if (candidates.size() < 2) return candidates;

    std::vector<Vec2> front_pts;
    front_pts.reserve(candidates.size());
    for (const auto& cone : candidates) front_pts.push_back(cone.pos);
    const Line2 front_line = fitLineTLS(front_pts);

    std::sort(candidates.begin(), candidates.end(), [&](const Cone& a, const Cone& b) {
        return projectOntoLine(front_line, a.pos) < projectOntoLine(front_line, b.pos);
    });

    const Vec2& near_car = rear_chain.front().pos;
    const double dist_from_front = (candidates.front().pos - near_car).norm();
    const double dist_from_back  = (candidates.back().pos  - near_car).norm();
    if (dist_from_back < dist_from_front) std::reverse(candidates.begin(), candidates.end());

    return candidates;
}

/// Radius der tatsaechlichen Pylone, die diesem Punkt am naechsten steht.
/// Die Abtastpunkte der Splines sind selbst keine Pylonen.
double nearestConeRadius(const Chain& chain, const Vec2& point) {
    double best_d2 = std::numeric_limits<double>::infinity();
    double radius = 0.0;
    for (const auto& cone : chain) {
        const double d2 = (cone.pos - point).squaredNorm();
        if (d2 < best_d2) { best_d2 = d2; radius = cone.radius; }
    }
    return radius;
}

/// Mindestabstand der Trajektorie zum Pylonenmittelpunkt einer Seite.
///
/// Das Groessere aus geometrischer Schranke, unter der bodyClear() ohnehin
/// Kollision meldet, und dem gewuenschten Wandabstand aus der Aufgabenstellung.
double sideLimit(double cone_radius, const VehicleParams& vehicle, double wanted_gap) {
    return std::max(cone_radius + vehicle.clearance + vehicle.width * 0.5, wanted_gap);
}

/// Fall B: beide Reihen sind als Reihe bekannt.
///
/// Beide Ketten werden zu glatten Splines abgetastet, die Fahrspur ist ihr
/// elementweiser Mittelpunkt. Das ersetzt die fruehere Messung "halber Abstand
/// zur naechstgelegenen Einzelpylone", die beim Wechsel der massgeblichen
/// vorderen Pylone sprunghaft schwankte und das Schlingern auf gerader Strecke
/// verursachte.
std::vector<Pose> drivingVectorsBetweenRows(const Chain& rear_chain,
                                            const Chain& front_chain, double sign,
                                            const VehicleParams& vehicle, int n_samples,
                                            double wanted_gap) {
    const auto rear_spline  = resampleChainSpline(rear_chain, n_samples);
    const auto front_spline = resampleChainSpline(front_chain, n_samples);
    const auto rear_tangents = tangents(rear_spline);

    // Lokale Korridorbreite als SENKRECHTER Abstand zur vorderen Polylinie.
    // Eine Paarung ueber gleiche Indizes mass bei versetzten Reihen schraeg
    // und zog die Mittellinie systematisch zu einer Wand.
    //
    // Hinter dem Ende der kuerzeren vorderen Reihe misst minDistToPolyline()
    // nur noch den Abstand zu deren Endpunkt, die Breite waechst also
    // scheinbar. Dagegen werden alle Breiten auf das 1,5-fache des Medians
    // gedeckelt.
    std::vector<double> widths(n_samples);
    for (int k = 0; k < n_samples; ++k) widths[k] = minDistToPolyline(rear_spline[k], front_spline);
    const double width_cap = 1.5 * median(widths);

    std::vector<Pose> vectors(n_samples);
    for (int k = 0; k < n_samples; ++k) {
        const Vec2 perpendicular =
            sign * Vec2(-std::sin(rear_tangents[k]), std::cos(rear_tangents[k]));

        const double corridor_width = std::min(widths[k], width_cap);
        const double rear_limit =
            sideLimit(nearestConeRadius(rear_chain, rear_spline[k]), vehicle, wanted_gap);
        const double front_limit =
            sideLimit(nearestConeRadius(front_chain, front_spline[k]), vehicle, wanted_gap);

        // Ziel ist die Mitte. Sie wird nur geklemmt, wenn das zulaessige Band
        // ueberhaupt existiert. Ist der Korridor fuer beide Mindestabstaende
        // zusammen zu schmal, bleibt die Mitte der fairste Kompromiss.
        //
        // Eine frueher einseitige Klemmung auf rear_limit rief std::clamp mit
        // lo > hi auf und presste den Vektor an die hintere Wand.
        double offset = 0.5 * corridor_width;
        const double lower = rear_limit;
        const double upper = corridor_width - front_limit;
        if (lower <= upper) offset = std::clamp(offset, lower, upper);

        const Vec2 position = rear_spline[k] + offset * perpendicular;
        vectors[k] = {position.x(), position.y(), rear_tangents[k]};
    }
    return vectors;
}

/// Fall A: nur die eigene Reihe steht, hoechstens eine einzelne Pylone gegenueber.
///
/// Um jede Pylone der Kette liegt gedanklich ein Kreis mit dem Zielabstand,
/// und der Fahrvektor liegt genau darauf, senkrecht zur lokalen Kettentangente.
/// Die ueberlappenden Kreise aller Kettenpylonen ergeben zusammen die Fahrspur:
/// das Fahrzeug haelt zu jeder einzelnen Pylone denselben Abstand, unabhaengig
/// davon, wie viele bekannt sind.
///
/// Ist die einzelne Gegenpylone bekannt, wird sie nicht ignoriert. Der feste
/// Versatz zeigte sonst geradewegs auf sie, sobald der real gemessene Korridor
/// schmaler war als zweimal Versatz plus Pylonenradien. Die Pipeline verwarf
/// den Pfad dann als kollidierend, und das Auto blieb vor dem Korridor stehen.
std::vector<Pose> drivingVectorsAlongSingleRow(const Chain& rear_chain,
                                               const std::vector<Vec2>& rear_pts,
                                               const std::vector<double>& thetas,
                                               const Chain& front_chain, double sign,
                                               double single_row_offset_m,
                                               const VehicleParams& vehicle,
                                               double wanted_gap) {
    const int n = static_cast<int>(rear_pts.size());
    const bool have_single_front = (front_chain.size() == 1);

    std::vector<Pose> vectors(n);
    for (int i = 0; i < n; ++i) {
        const Vec2 perpendicular = sign * Vec2(-std::sin(thetas[i]), std::cos(thetas[i]));

        // single_row_offset_m ist der gewollte exakte Abstand, nach unten hart
        // auf die geometrische Schranke geklemmt, damit der Vektor nie unter
        // der Kollisionsgrenze liegt, falls die Konfiguration inkonsistent ist.
        const double rear_minimum =
            rear_chain[i].radius + vehicle.clearance + vehicle.width * 0.5;
        double offset = std::max(single_row_offset_m, rear_minimum);

        if (have_single_front) {
            const double dist_to_front = (front_chain[0].pos - rear_pts[i]).norm();
            const double front_limit =
                sideLimit(front_chain[0].radius, vehicle, wanted_gap);
            const double upper = dist_to_front - front_limit;
            offset = (rear_minimum <= upper) ? std::clamp(offset, rear_minimum, upper)
                                             : 0.5 * dist_to_front;
        }

        const Vec2 position = rear_pts[i] + offset * perpendicular;
        vectors[i] = {position.x(), position.y(), thetas[i]};
    }
    return vectors;
}

}  // namespace

DrivingVectorResult computeDrivingVectorsFromChain(
    const Chain& rear_chain, const std::vector<Cone>& other_cones,
    double sign, double single_row_offset_m,
    const VehicleParams& vehicle, int n_samples,
    double rear_wall_clearance_m) {
    DrivingVectorResult result;

    std::vector<Vec2> rear_pts;
    rear_pts.reserve(rear_chain.size());
    for (const auto& cone : rear_chain) rear_pts.push_back(cone.pos);

    std::vector<double> thetas(rear_pts.size());
    for (size_t i = 0; i < rear_pts.size(); ++i)
        thetas[i] = neighborOrientation(rear_pts, static_cast<int>(i));

    result.front_chain =
        orderFrontChain(selectFrontCandidates(other_cones, rear_pts, sign), rear_chain);

    // Zwei Faelle, je nachdem wie viel von der Gegenreihe bekannt ist.
    if (result.front_chain.size() >= 2) {
        result.vectors = drivingVectorsBetweenRows(rear_chain, result.front_chain, sign,
                                                   vehicle, n_samples, rear_wall_clearance_m);
    } else {
        result.vectors = drivingVectorsAlongSingleRow(rear_chain, rear_pts, thetas,
                                                      result.front_chain, sign,
                                                      single_row_offset_m, vehicle,
                                                      rear_wall_clearance_m);
    }
    return result;
}

// Umgekehrter Sichtbarkeitsfall: nur die VORDERE Kette (ohne Trigger-Pylone)
// ist als Reihe sichtbar -- siehe cone_corridor.hpp fuer die Konstruktion.
// Gleiche Bauart wie der single_row_offset_m-Zweig von
// computeDrivingVectorsFromChain() oben (Vektorkette entlang der bekannten
// Reihe, Versatz entlang der lokalen Normale, nach unten geklemmt); der
// Unterschied ist ausschliesslich der VERSATZ: kein fester 45-cm-Wert,
// sondern der halbe Abstand C <-> Trigger-Pylone, also das tatsaechlich
// gemessene Verhaeltnis der beiden Ketten zueinander.
DrivingVectorResult computeDrivingVectorsFromFrontChain(
    const Chain& front_chain,
    const Cone& trigger,
    double sign,
    const VehicleParams& vehicle)
{
    DrivingVectorResult result;
    if (front_chain.empty()) return result;

    std::vector<Vec2> pts;
    pts.reserve(front_chain.size());
    for (const auto& c : front_chain) pts.push_back(c.pos);
    const int n = static_cast<int>(pts.size());

    // Die einzige bekannte Pylone der hinteren Reihe IST die Trigger-Pylone --
    // aus Sicht dieses Zweigs die "vordere" Referenz, gegen die gemessen wird.
    result.front_chain = Chain{trigger};

    // Schritt 1 -- Verhaeltnis der beiden Ketten: C = die Pylone der vorderen
    // Kette, die der Trigger-Pylone am naechsten liegt (nearestConeTo(), ohne
    // Distanzschranke). Ihr Abstand ist die Torbreite an der einzigen Stelle,
    // an der beide Reihen gleichzeitig bekannt sind; die Fahrspur gehoert auf
    // die Haelfte davon. Ueber die NAECHSTGELEGENE Pylone statt z.B. dem
    // Kettenanfang, weil nur dort der gemessene Abstand tatsaechlich quer zum
    // Korridor liegt -- eine weiter entfernte Kettenpylone haette einen grossen
    // Laengsanteil, der die Torbreite systematisch ueberschaetzen wuerde.
    const Cone* cone_c = nearestConeTo(front_chain, trigger.pos);
    result.cone_c = *cone_c;
    const double half_gap = 0.5 * (cone_c->pos - trigger.pos).norm();

    // Schritt 2 -- Parallelverschiebung der Vektorkette um half_gap in Richtung
    // Trigger-Pylone. Nach unten auf das geometrische Minimum geklemmt: ist das
    // Tor schmaler als 2x (Pylonenradius + clearance + halbe Fahrzeugbreite),
    // laege die exakte Mitte innerhalb der bodyClear()-Kollisionsgrenze der
    // vorderen Reihe -- dann ist der groesstmoegliche Abstand zur vorderen
    // Reihe der bessere Kompromiss (die Restenge bleibt Sache der
    // nachgelagerten Kollisionspruefung beim Aufrufer).
    const double min_offset =
        cone_c->radius + vehicle.clearance + vehicle.width * 0.5;
    const double offset = std::max(half_gap, min_offset);

    std::vector<Pose> vectors(n);
    for (int i = 0; i < n; ++i) {
        // Versatz entlang der LOKALEN Normale jedes Kettenpunkts (nicht als
        // starre Verschiebung der ganzen Kette): in einer Kurve dreht der
        // Versatz so mit der Kette mit und die Fahrspur bleibt eine echte
        // Parallelkurve zur Vektorkette -- dieselbe Konstruktion wie in
        // mirrorChainThrough().
        const double theta = neighborOrientation(pts, i);
        const Vec2 perp = sign * Vec2(-std::sin(theta), std::cos(theta));
        const Vec2 pos = pts[i] + offset * perp;
        vectors[i] = {pos.x(), pos.y(), theta};
    }

    result.vectors = std::move(vectors);
    return result;
}

std::vector<Pose> catmullRomSegment(
    const Pose& p1,
    const Pose& p2,
    const Pose& p0,
    const Pose& p3,
    double step)
{
    constexpr double alpha = 0.5;   // centripetal Catmull-Rom

    auto tj = [&](double ti, const Vec2& a, const Vec2& b)
    {
        return ti + std::pow((b-a).norm(), alpha);
    };

    Vec2 P0(p0.x,p0.y);
    Vec2 P1(p1.x,p1.y);
    Vec2 P2(p2.x,p2.y);
    Vec2 P3(p3.x,p3.y);

    double t0 = 0.0;
    double t1 = tj(t0,P0,P1);
    double t2 = tj(t1,P1,P2);
    double t3 = tj(t2,P2,P3);

    double length = (P2-P1).norm();
    int N = std::max(2,(int)std::ceil(length/step));

    std::vector<Pose> out;
    out.reserve(N);

    for(int i=0;i<N;i++)
    {
        double t = t1 + (t2-t1)*(double)i/(N-1);

        Vec2 A1 = (t1-t)/(t1-t0)*P0 + (t-t0)/(t1-t0)*P1;
        Vec2 A2 = (t2-t)/(t2-t1)*P1 + (t-t1)/(t2-t1)*P2;
        Vec2 A3 = (t3-t)/(t3-t2)*P2 + (t-t2)/(t3-t2)*P3;

        Vec2 B1 = (t2-t)/(t2-t0)*A1 + (t-t0)/(t2-t0)*A2;
        Vec2 B2 = (t3-t)/(t3-t1)*A2 + (t-t1)/(t3-t1)*A3;

        Vec2 C  = (t2-t)/(t2-t1)*B1 + (t-t1)/(t2-t1)*B2;

        Vec2 d;

        if(i==0)
            d=P2-P1;
        else
            d=C-Vec2(out.back().x,out.back().y);

        out.push_back({
            C.x(),
            C.y(),
            std::atan2(d.y(),d.x())
        });
    }

    out.front().th = p1.th;
    out.back().th  = p2.th;

    return out;
}

std::optional<std::vector<Pose>>buildDrivingPath(const std::vector<Pose>& vectors,double step)
{
    if(vectors.size()<2)
        return std::nullopt;

    std::vector<Pose> path;

    path.push_back(vectors.front());

    for(size_t i=0;i+1<vectors.size();++i)
    {
        const Pose& p1 = vectors[i];
        const Pose& p2 = vectors[i+1];

        Pose p0 = (i==0)
                    ? Pose{
                        2*p1.x-p2.x,
                        2*p1.y-p2.y,
                        p1.th}
                    : vectors[i-1];

        Pose p3 = (i+2>=vectors.size())
                    ? Pose{
                        2*p2.x-p1.x,
                        2*p2.y-p1.y,
                        p2.th}
                    : vectors[i+2];

        auto seg =
            catmullRomSegment(
                p1,
                p2,
                p0,
                p3,
                step);

        path.insert(path.end(),
                    seg.begin()+1,
                    seg.end());
    }

    return path;
}

RepairedPath buildAndRepairDrivingPath(const std::vector<Pose>& vectors,
                                        const std::vector<Cone>& obstacles,
                                        const VehicleParams& vehicle) {
    RepairedPath result;
    auto path_opt = buildDrivingPath(vectors);
    if (!path_opt) return result;
    const auto& path = *path_opt;

    std::vector<Vec2> xy(path.size());
    for (size_t i = 0; i < path.size(); ++i) xy[i] = Vec2(path[i].x, path[i].y);

    double kmax = 0.0;
    const bool curv_ok = pathCurvatureOk(xy, vehicle.R_min, kmax);
    if (centerlineFeasible(xy, curv_ok, obstacles, vehicle)) {
        result.path = path;
        result.ok = true;
        return result;
    }
    std::vector<Vec2> smoothed = xy;

    for(int iter=0; iter<30; ++iter)
    {
        for(size_t i=1;i+1<smoothed.size();++i)
        {
            Vec2 force(0,0);

            force += 0.15*((smoothed[i-1]+smoothed[i+1])*0.5-smoothed[i]);

            for(const auto& c:obstacles)
            {
                Vec2 d = smoothed[i]-c.pos;

                double dist = d.norm();

                if(dist<0.8)
                {
                    force +=
                        0.08*
                        (0.8-dist)*
                        d.normalized();
                }
            }

            smoothed[i]+=force;
        }
    }
    const auto repaired_xy = localRepair(smoothed,obstacles,vehicle);
    double kmax2 = 0.0;
    const bool curv_ok2 = pathCurvatureOk(repaired_xy, vehicle.R_min, kmax2);
    const bool ok = centerlineFeasible(repaired_xy, curv_ok2, obstacles, vehicle);
    const auto thetas = tangents(repaired_xy);

    std::vector<Pose> repaired(repaired_xy.size());
    for (size_t i = 0; i < repaired_xy.size(); ++i)
        repaired[i] = {repaired_xy[i].x(), repaired_xy[i].y(), thetas[i]};
    result.path = std::move(repaired);
    result.ok = ok;
    return result;
}

// ════════════════════════════════════════════════════════════════════
//  5. VORONOI-DELAUNAY-MITTELLINIE (Bootstrap-Alternative)
// ════════════════════════════════════════════════════════════════════
//
//  Portiert aus feature/global_planner_v1 (dortige cone_corridor.cpp) --
//  siehe Abschnitts-Kommentar in cone_corridor.hpp. Wiederverwendet resample(),
//  pathToPoses(), convexHull(), pointInConvexHull(), minDistToPolyline(),
//  smoothPolyline() und pathCurvatureOk() aus den Abschnitten 1/4 dieser
//  Datei -- nur die Voronoi-/Delaunay-spezifische Geometrie ist neu.

// ────────────────────────────────────────────────────────────────────
//  5.1 Bowyer-Watson Delaunay → dualer Voronoi-Graph
// ────────────────────────────────────────────────────────────────────

namespace {

struct Triangle {
    int v[3];
    Vec2   cc;      // Umkreismittelpunkt
    double cr2;     // Umkreisradius²
};

Vec2 circumcenter(const Vec2& A, const Vec2& B, const Vec2& C) {
    double ax = A.x(), ay = A.y();
    double bx = B.x(), by = B.y();
    double cx = C.x(), cy = C.y();
    double D = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
    if (std::abs(D) < 1e-14) return {1e18, 1e18};
    double ux = ((ax*ax + ay*ay) * (by - cy) +
                 (bx*bx + by*by) * (cy - ay) +
                 (cx*cx + cy*cy) * (ay - by)) / D;
    double uy = ((ax*ax + ay*ay) * (cx - bx) +
                 (bx*bx + by*by) * (ax - cx) +
                 (cx*cx + cy*cy) * (bx - ax)) / D;
    return {ux, uy};
}

struct VoronoiSourceEdge { int a, b; };

bool sameEdge(const VoronoiSourceEdge& e1, const VoronoiSourceEdge& e2) {
    return (e1.a == e2.a && e1.b == e2.b) ||
           (e1.a == e2.b && e1.b == e2.a);
}

std::vector<Triangle> delaunay(const std::vector<Vec2>& sites) {
    const int n = static_cast<int>(sites.size());
    double minx = 1e18, maxx = -1e18, miny = 1e18, maxy = -1e18;
    for (auto& p : sites) {
        minx = std::min(minx, p.x()); maxx = std::max(maxx, p.x());
        miny = std::min(miny, p.y()); maxy = std::max(maxy, p.y());
    }
    double dmax = std::max(maxx - minx + 1.0, maxy - miny + 1.0) * 10.0;
    double midx = (minx + maxx) * 0.5, midy = (miny + maxy) * 0.5;

    std::vector<Vec2> all = sites;
    all.push_back({midx - dmax, midy - dmax});
    all.push_back({midx + dmax, midy - dmax});
    all.push_back({midx,        midy + dmax});

    auto makeTri = [&](int a, int b, int c) -> Triangle {
        Vec2 cc = circumcenter(all[a], all[b], all[c]);
        double cr2 = (all[a] - cc).squaredNorm();
        return {{a, b, c}, cc, cr2};
    };

    std::vector<Triangle> tris;
    tris.push_back(makeTri(n, n + 1, n + 2));

    for (int pi = 0; pi < n; ++pi) {
        const Vec2& P = all[pi];
        std::vector<Triangle> bad, good;
        for (auto& t : tris) {
            if ((P - t.cc).squaredNorm() < t.cr2 + 1e-10)
                bad.push_back(t);
            else
                good.push_back(t);
        }
        std::vector<VoronoiSourceEdge> poly;
        for (auto& t : bad) {
            VoronoiSourceEdge edges[3] = {{t.v[0], t.v[1]}, {t.v[1], t.v[2]}, {t.v[2], t.v[0]}};
            for (auto& e : edges) {
                bool shared = false;
                for (auto& t2 : bad) {
                    if (&t2 == &t) continue;
                    VoronoiSourceEdge e2s[3] = {{t2.v[0], t2.v[1]}, {t2.v[1], t2.v[2]}, {t2.v[2], t2.v[0]}};
                    for (auto& e2 : e2s)
                        if (sameEdge(e, e2)) { shared = true; break; }
                    if (shared) break;
                }
                if (!shared) poly.push_back(e);
            }
        }
        tris = std::move(good);
        for (auto& e : poly)
            tris.push_back(makeTri(e.a, e.b, pi));
    }

    std::vector<Triangle> result;
    for (auto& t : tris)
        if (t.v[0] < n && t.v[1] < n && t.v[2] < n)
            result.push_back(t);
    return result;
}

}  // anonymous namespace

VoronoiGraph buildVoronoiGraph(const std::vector<Vec2>& cones) {
    VoronoiGraph vg;
    if (cones.size() < 4) return vg;

    auto tris = delaunay(cones);
    auto hull = convexHull(cones);

    vg.vertices.resize(tris.size());
    for (size_t i = 0; i < tris.size(); ++i)
        vg.vertices[i].pos = tris[i].cc;

    auto pairKey = [](int a, int b) -> int64_t {
        if (a > b) std::swap(a, b);
        return (static_cast<int64_t>(a) << 32) | static_cast<int64_t>(b);
    };
    std::unordered_map<int64_t, std::vector<int>> edgeToTri;
    for (int ti = 0; ti < static_cast<int>(tris.size()); ++ti) {
        auto& t = tris[ti];
        for (int e = 0; e < 3; ++e) {
            int a = t.v[e], b = t.v[(e + 1) % 3];
            edgeToTri[pairKey(a, b)].push_back(ti);
        }
    }
    for (auto& [key, triList] : edgeToTri) {
        if (triList.size() != 2) continue;
        int t1 = triList[0], t2 = triList[1];
        if (!pointInConvexHull(vg.vertices[t1].pos, hull)) continue;
        if (!pointInConvexHull(vg.vertices[t2].pos, hull)) continue;
        int gen1 = static_cast<int>(key >> 32);
        int gen2 = static_cast<int>(key & 0xFFFFFFFF);
        vg.edges.push_back({t1, t2, gen1, gen2});
    }
    return vg;
}

// ────────────────────────────────────────────────────────────────────
//  5.2 Pruning + Ketten-Traversierung
// ────────────────────────────────────────────────────────────────────

AdjList buildAdj(const VoronoiGraph& vg) {
    AdjList adj;
    for (auto& e : vg.edges) {
        adj[e.v1].push_back(e.v2);
        adj[e.v2].push_back(e.v1);
    }
    for (auto& [v, nb] : adj) {
        std::sort(nb.begin(), nb.end());
        nb.erase(std::unique(nb.begin(), nb.end()), nb.end());
    }
    return adj;
}

void pruneHighDegree(AdjList& adj, const std::vector<VoronoiVertex>& verts,
                     int max_iter) {
    for (int iter = 0; iter < max_iter; ++iter) {
        bool found = false;
        for (auto& [v, nb] : adj) {
            if (nb.size() <= 2) continue;
            found = true;
            int longest = nb[0];
            double bestD = (verts[v].pos - verts[nb[0]].pos).squaredNorm();
            for (size_t k = 1; k < nb.size(); ++k) {
                double d = (verts[v].pos - verts[nb[k]].pos).squaredNorm();
                if (d > bestD) { bestD = d; longest = nb[k]; }
            }
            nb.erase(std::remove(nb.begin(), nb.end(), longest), nb.end());
            if (adj.count(longest)) {
                auto& nb2 = adj[longest];
                nb2.erase(std::remove(nb2.begin(), nb2.end(), v), nb2.end());
            }
        }
        for (auto it = adj.begin(); it != adj.end(); )
            it = it->second.empty() ? adj.erase(it) : ++it;

        if (!found) break;

        // groesste Zusammenhangskomponente behalten
        std::unordered_set<int> visited;
        std::vector<std::vector<int>> comps;
        for (auto& [v, _] : adj) {
            if (visited.count(v)) continue;
            comps.emplace_back();
            std::queue<int> q; q.push(v); visited.insert(v);
            while (!q.empty()) {
                int cur = q.front(); q.pop();
                comps.back().push_back(cur);
                for (int nb : adj[cur])
                    if (!visited.count(nb) && adj.count(nb))
                        { visited.insert(nb); q.push(nb); }
            }
        }
        if (comps.size() > 1) {
            auto& largest = *std::max_element(comps.begin(), comps.end(),
                [](auto& a, auto& b) { return a.size() < b.size(); });
            std::unordered_set<int> keep(largest.begin(), largest.end());
            for (auto it = adj.begin(); it != adj.end(); ) {
                if (!keep.count(it->first)) {
                    for (int nb : it->second)
                        if (adj.count(nb)) {
                            auto& nb2 = adj[nb];
                            nb2.erase(std::remove(nb2.begin(), nb2.end(), it->first), nb2.end());
                        }
                    it = adj.erase(it);
                } else ++it;
            }
        }
    }
}

std::vector<int> degree1Vertices(const AdjList& adj) {
    std::vector<int> d1;
    for (auto& [v, nb] : adj)
        if (nb.size() == 1) d1.push_back(v);
    return d1;
}

std::optional<std::vector<int>> traverseChain(const AdjList& adj, int start, int end) {
    std::vector<int> chain = {start};
    std::unordered_set<int> visited = {start};
    while (chain.back() != end) {
        auto it = adj.find(chain.back());
        if (it == adj.end()) return std::nullopt;
        int next = -1;
        for (int nb : it->second)
            if (!visited.count(nb)) { next = nb; break; }
        if (next < 0) return std::nullopt;
        chain.push_back(next);
        visited.insert(next);
        if (chain.size() > adj.size() + 1) return std::nullopt;
    }
    return chain;
}

std::pair<int, int> longestEndpointPair(
    const AdjList& adj,
    const std::vector<VoronoiVertex>& verts,
    const std::vector<int>& deg1,
    const std::vector<Vec2>& lane1_pts)
{
    int bestS = -1, bestE = -1;
    double bestLen = -1.0;
    for (size_t a = 0; a < deg1.size(); ++a) {
        for (size_t b = a + 1; b < deg1.size(); ++b) {
            auto chain = traverseChain(adj, deg1[a], deg1[b]);
            if (!chain) continue;
            double L = 0.0;
            for (size_t k = 1; k < chain->size(); ++k)
                L += (verts[(*chain)[k]].pos - verts[(*chain)[k - 1]].pos).norm();
            if (L > bestLen) { bestLen = L; bestS = deg1[a]; bestE = deg1[b]; }
        }
    }
    if (bestS < 0) {
        auto sorted = deg1;
        std::sort(sorted.begin(), sorted.end(),
            [&](int a, int b) { return verts[a].pos.x() < verts[b].pos.x(); });
        return {sorted.front(), sorted.back()};
    }
    if (lane1_pts.size() >= 2) {
        double ds = minDistToPolyline(verts[bestS].pos, lane1_pts);
        double de = minDistToPolyline(verts[bestE].pos, lane1_pts);
        if (de < ds) std::swap(bestS, bestE);
    }
    return {bestS, bestE};
}

// ────────────────────────────────────────────────────────────────────
//  5.3 Korridor-Ausgang (Gate am Ende der Kette)
// ────────────────────────────────────────────────────────────────────

namespace {

// Schnittparameter t (>= 0) von Strahl P + t*d mit Segment [A,B], per
// Cramer'scher Regel ueber das half-plane/Kreuzprodukt-Paar (v2,v3).
// std::nullopt falls Strahl parallel zum Segment liegt oder der Schnitt
// ausserhalb von t>=0 bzw. des Segmentbereichs s in [0,1] liegt.
std::optional<double> rayConvexEdgeIntersection(const Vec2& P, const Vec2& d,
                                                const Vec2& A, const Vec2& B) {
    Vec2 v1 = P - A;
    Vec2 v2 = B - A;
    Vec2 v3{-d.y(), d.x()};
    double denom = v2.dot(v3);
    if (std::abs(denom) < 1e-12) return std::nullopt;
    double t = (v2.x() * v1.y() - v2.y() * v1.x()) / denom;
    double s = v1.dot(v3) / denom;
    if (t < 1e-9 || s < -1e-9 || s > 1.0 + 1e-9) return std::nullopt;
    return t;
}

}  // anonymous namespace

std::optional<Vec2> computeExitPoint(const std::vector<Vec2>& raw_chain,
                                      const std::vector<Vec2>& cones,
                                      const std::vector<Vec2>& hull) {
    if (raw_chain.size() < 2 || hull.size() < 3) return std::nullopt;

    // 1. Richtung der Kette an ihrem Ende.
    const Vec2& last = raw_chain.back();
    const Vec2& prev = raw_chain[raw_chain.size() - 2];
    Vec2 dir = last - prev;
    double dnorm = dir.norm();
    if (dnorm < 1e-9) return std::nullopt;
    dir /= dnorm;

    // 2. Strahl bis zum Schnitt mit der Convex Hull der Cones verlaengern.
    std::optional<double> bestT;
    for (size_t i = 0; i < hull.size(); ++i) {
        const Vec2& A = hull[i];
        const Vec2& B = hull[(i + 1) % hull.size()];
        auto t = rayConvexEdgeIntersection(last, dir, A, B);
        if (t && (!bestT || *t < *bestT)) bestT = t;
    }
    if (!bestT) return std::nullopt;
    Vec2 gate_point = last + (*bestT) * dir;

    // 3. Naechste Cone links (cross > 0) und rechts (cross < 0) der
    //    Strahlrichtung, jeweils naechste zum Schnittpunkt.
    int leftIdx = -1, rightIdx = -1;
    double leftBestDist  = std::numeric_limits<double>::max();
    double rightBestDist = std::numeric_limits<double>::max();
    for (size_t i = 0; i < cones.size(); ++i) {
        Vec2 rel = cones[i] - last;
        double cross = dir.x() * rel.y() - dir.y() * rel.x();
        double dist  = (cones[i] - gate_point).norm();
        if (cross > 1e-9) {
            if (dist < leftBestDist) { leftBestDist = dist; leftIdx = static_cast<int>(i); }
        } else if (cross < -1e-9) {
            if (dist < rightBestDist) { rightBestDist = dist; rightIdx = static_cast<int>(i); }
        }
    }
    if (leftIdx < 0 || rightIdx < 0) return std::nullopt;

    // 4. Gate-Mittelpunkt.
    return 0.5 * (cones[leftIdx] + cones[rightIdx]);
}

Pose computeEntryPose(const std::vector<Vec2>& path, size_t lookahead, size_t start) {
    if (path.empty()) return {};
    start = std::min(start, path.size() - 1);
    size_t look = std::min(start + lookahead, path.size() - 1);
    // Am Ende der Mittellinie ist kein Lookahead mehr uebrig (look == start) --
    // dann die Sehne rueckwaerts nehmen: dieselbe Richtung, nur andersherum
    // gemessen, statt eines undefinierten atan2(0,0).
    Vec2 d = (look > start) ? (path[look] - path[start])
                             : (path[start] - path[start > 0 ? start - 1 : start]);
    return {path[start].x(), path[start].y(), std::atan2(d.y(), d.x())};
}

std::vector<Pose> laneEntryPoses(const std::vector<Vec2>& path, size_t lookahead) {
    std::vector<Pose> out(path.size());
    for (size_t i = 0; i < path.size(); ++i) out[i] = computeEntryPose(path, lookahead, i);
    return out;
}

// ────────────────────────────────────────────────────────────────────
//  5.4 Recentering + buildCenterlinePath() — oeffentlicher Einstiegspunkt
// ────────────────────────────────────────────────────────────────────

namespace {

// Zieht jeden Kettenpunkt ein Stueck (Anteil `pull`) Richtung des Mittelpunkts
// der jeweils naechstgelegenen Pylone LINKS und RECHTS der lokalen Tangente
// von `pts` -- haelt die rohe Voronoi-Kette (die exakt auf den Delaunay-
// Umkreismittelpunkten liegt und dadurch an duennen Dreiecken ausschlagen
// kann) nahe der tatsaechlichen Korridor-Mitte. Erhaelt Anfang/Ende der Kette
// exakt (Gate-Ausgang bzw. Kettenstart bleiben fixe Endpunkte fuer die
// nachfolgende Glaettung).
//
// Fruehere Version waehlte stattdessen unter den 6 insgesamt naechsten Cones
// denjenigen mit dem groessten Winkel zum naechstgelegenen Cone ("am
// staerksten entgegengesetzt") als vermeintliche Gegenreihen-Pylone -- ohne
// jede Links/Rechts-Trennung. Bei ungleicher Punktdichte zwischen den beiden
// Reihen (z.B. eine Reihe lokal dichter beieinander) konnten alle 6 naechsten
// Cones aus DERSELBEN Reihe stammen; der "Mittelpunkt" zweier Cones derselben
// Reihe zieht die Linie dann systematisch zu dieser Reihe hin, statt sie zu
// zentrieren -- genau der beobachtete Rand-Bias auf langen Korridoren mit
// vielen, aber ungleich verteilten Pylonen. Die Tangenten-Seitenpruefung hier
// (dieselbe Perpendikular-Vorzeichen-Idee wie signTowardPoint() oben)
// erzwingt stattdessen je einen Kandidaten aus JEDER Seite.
void recenterToCorridor(std::vector<Vec2>& pts,
                        const std::vector<Vec2>& cones, double pull) {
    if (cones.size() < 2) return;
    Vec2 first = pts.front(), last = pts.back();
    const auto th = tangents(pts);
    for (size_t k = 0; k < pts.size(); ++k) {
        const Vec2 p = pts[k];
        const Vec2 n{-std::sin(th[k]), std::cos(th[k])};
        double best_left_d2 = std::numeric_limits<double>::infinity();
        double best_right_d2 = std::numeric_limits<double>::infinity();
        Vec2 c_left{0.0, 0.0}, c_right{0.0, 0.0};
        for (const auto& c : cones) {
            const Vec2 v = c - p;
            const double d2 = v.squaredNorm();
            if (n.dot(v) >= 0.0) {
                if (d2 < best_left_d2) { best_left_d2 = d2; c_left = c; }
            } else {
                if (d2 < best_right_d2) { best_right_d2 = d2; c_right = c; }
            }
        }
        // Keine Pylone auf einer der beiden Seiten lokal bekannt: `p`
        // unveraendert lassen statt aus zwei Pylonen DERSELBEN Reihe einen
        // falschen Mittelpunkt zu raten.
        if (!std::isfinite(best_left_d2) || !std::isfinite(best_right_d2)) continue;
        const Vec2 mid = 0.5 * (c_left + c_right);
        pts[k] = (1.0 - pull) * p + pull * mid;
    }
    pts.front() = first;
    pts.back()  = last;
}

}  // anonymous namespace

CenterlineResult buildCenterlinePath(
    const std::vector<Vec2>& cones,
    const std::vector<Vec2>& lane1_pts,
    const PipelineConfig& cfg)
{
    CenterlineResult result;
    if (cones.size() < 4) return result;

    auto voronoi = buildVoronoiGraph(cones);
    if (voronoi.edges.empty()) return result;

    auto adj = buildAdj(voronoi);
    pruneHighDegree(adj, voronoi.vertices);
    if (adj.empty()) return result;

    auto endpoints = degree1Vertices(adj);
    if (endpoints.size() < 2) return result;

    auto [start_vertex, end_vertex] = longestEndpointPair(adj, voronoi.vertices, endpoints, lane1_pts);
    auto chain = traverseChain(adj, start_vertex, end_vertex);
    if (!chain) return result;

    result.raw_chain.resize(chain->size());
    for (size_t i = 0; i < chain->size(); ++i)
        result.raw_chain[i] = voronoi.vertices[(*chain)[i]].pos;

    // Letzten Kettenknoten durch den geometrisch korrekten Gate-Ausgangspunkt
    // ersetzen, bevor resample()/smoothing laufen, damit die Glaettung den
    // Ausgangspunkt als festen Endpunkt natuerlich einbezieht.
    auto hull = convexHull(cones);
    if (auto exit_pt = computeExitPoint(result.raw_chain, cones, hull)) {
        result.raw_chain.back() = *exit_pt;
    } else {
        result.used_raw_vertex_fallback = true;
    }

    auto path = resample(result.raw_chain, cfg.resample_ds);

    const auto& voronoi_cfg = cfg.voronoi;
    for (int pass_index = 0; pass_index < voronoi_cfg.smooth_outer_iters; ++pass_index) {
        recenterToCorridor(path, cones, voronoi_cfg.recenter_pull);
        path = smoothPolyline(path, voronoi_cfg.smooth_inner_iters, voronoi_cfg.smooth_alpha);
    }

    double kappa_max = 0.0;
    bool curvature_ok = pathCurvatureOk(path, cfg.vehicle.R_min, kappa_max);
    int extra_passes = 0;
    while (!curvature_ok && extra_passes < voronoi_cfg.smooth_curv_extra_max) {
        path = smoothPolyline(path, 6, voronoi_cfg.smooth_alpha);
        curvature_ok = pathCurvatureOk(path, cfg.vehicle.R_min, kappa_max);
        ++extra_passes;
    }

    path = resample(path, cfg.resample_ds);

    // Ueber die konvexe Huelle der Pylonen hinaus verlaengern
    // (corridor_end_extension_m), analog zu planCorridorChain()
    // (cone_planner.cpp): sonst bliebe der Stopppunkt exakt auf Hoehe des
    // Gate-Ausgangs bzw. des rohen Voronoi-Knotens stehen und
    // updateVehiclePosition() koennte die Durchfahrt (corridor_finished /
    // is_end) nie erkennen. Richtung wird rueckwaerts aus dem letzten
    // nicht-entarteten Segment bestimmt, da path (anders als die Pose-Kette im
    // Chain-Zweig) kein Heading speichert.
    if (path.size() >= 2) {
        const Vec2 end = path.back();
        Vec2 dir{0.0, 0.0};
        for (size_t i = path.size() - 1; i-- > 0; ) {
            const Vec2 d = end - path[i];
            const double n = d.norm();
            if (n > 1e-6) { dir = d / n; break; }
        }
        if (dir.squaredNorm() > 0.0) {
            const double step  = std::max(cfg.resample_ds, 1e-3);
            const int extension_steps = static_cast<int>(std::ceil(cfg.corridor_end_extension_m / step));
            for (int k = 1; k <= extension_steps; ++k) {
                path.push_back(end + (k * step) * dir);
            }
        }
    }

    result.ok        = true;
    result.path      = std::move(path);
    result.kappa_max = kappa_max;
    result.curv_ok   = curvature_ok;
    return result;
}

}  // namespace corridor
