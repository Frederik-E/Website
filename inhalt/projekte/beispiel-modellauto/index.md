---
# Beispielprojekt. Ersetze es durch dein eigenes oder loesche den ganzen Ordner
# inhalt/projekte/beispiel-modellauto/. npm run check meldet Beispielprojekte als Fehler.
titel: Line-following model car
kurz: A 1:10 model car that follows a taped line at 2 m/s. I wrote the perception and the steering controller.
datum: 2025-06
rolle: Perception and control, team of four
tags: [ROS 2, OpenCV, Python]
titelbild: ./titelbild.png
titelbild_alt: Placeholder image in dusty rose with the word Platzhalter Titelbild
video: demo.webm
code: https://github.com/example/line-follower
startseite: true
reihenfolge: 1
---

## Problem

The car had to follow a taped line on the lab floor without leaving it, fast
enough to finish the 40 m course in under a minute. The first version lost the
line in every second curve because the controller reacted to a single pixel row.

## Approach

I warped the camera image with a fixed homography into a top-down strip, then
thresholded it in HSV and fitted a second-order polynomial to the remaining
points. A pure pursuit controller turned that curve into a steering angle. The
three nodes ran on a Raspberry Pi 5 at 30 Hz:

```python
lookahead = clamp(0.6 * speed, 0.3, 1.2)
angle = atan2(2 * wheelbase * sin(alpha), lookahead)
```

The lookahead distance grows with speed, which is what stopped the oscillation
in the tight curves.

## Result

The car finished the course at 2.0 m/s with a mean lateral error of 4 cm and no
manual rescue in ten runs. <mark>The polynomial fit cut the failure rate from
every second curve to zero.</mark>

## What I learned

Fitting a curve instead of tracking a point made the difference, not a faster
controller. I also learned to log every frame that the thresholding rejected,
which is how I found that the lab lights changed the line colour after 6 p.m.
