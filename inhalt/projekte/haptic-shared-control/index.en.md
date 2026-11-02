---
# Englische Fassung von index.md. Hier stehen nur die übersetzten Texte;
# Datum, Links, Reihenfolge und Entwurf kommen aus index.md.
titel: Scientific Paper - haptic feedback shared control
kurz: User study on haptic forces on a joystick that help with steering a drone
rolle: Software development, data integrity, organisation
tags: [ROS 2, Python, Haptics, User study, Statistics, Software architecture]
titelbild: ./joystick.en.png
titelbild_alt: "Drawing of a participant from behind at a table. The participant holds a joystick with force feedback in the right hand. On the screen in front of them, a drone flies between circular obstacles towards a goal. A blue arrow at the joystick and one at the drone show the force that pushes away from the nearest obstacle."
---

## What it was about

The research seminar "Cooperative and Haptic Human-Machine Interactions" was
about a joystick that generates forces of its own. When the user steers a drone,
for example, it pushes their hand away from an obstacle. Human and assistance
system thus steer together; this is known as Haptic Shared Control. The forces are meant to
bring the pilot to the goal more safely without patronising them.

## My work

Within the team, I helped develop the force model and conducted and evaluated the
user study. Together, we wrote the paper "Phased Haptic Shared Control for Safe
and Autonomy-Preserving UAV Teleoperation" on this work. It has not been published
yet.

I set up the experiment in ROS 2 so that the experimenter can control the whole
procedure via a dashboard. This includes recovery methods: if part of the
system crashes, no data is lost, and the participant continues from the same
point.
