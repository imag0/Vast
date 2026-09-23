# Vast 3.2.13 — controlled inertial navigation

Vast 3.2.13 adds fling behavior to infinite-canvas navigation without moving document work into the animation loop.

## Behavior

- One- and two-finger camera pans record recent centroid translation with event timestamps.
- Release velocity uses the most recent 90 ms of camera translation. Pinch scale changes are excluded, so zoom alone cannot create false pan momentum.
- Releases below 200 px/s stop immediately. Faster releases are capped at 3200 px/s.
- Momentum decelerates at a constant 6400 px/s² using elapsed time. Even an extreme release therefore coasts no more than 800 px for 0.5 seconds. The final partial stopping interval uses the analytic stopping distance, avoiding refresh-rate-dependent overshoot.
- A new finger, second pointer, stylus contact, input cancellation, activity pause, or input-queue teardown cancels the fling before the new input is handled.

## Rendering and persistence

Fling changes `offX`/`offY`, marks the camera/minimap view as changed, and requests animation frames. It does not alter strokes, text, photos, shapes, frames, OCR geometry, or undo history. Retained world-space GPU content is reused. Saves, OCR dispatch, performance dumps, and expensive idle work wait until momentum is finished and the existing idle window has elapsed.

## Radial-menu pan grace period

The finger-hold progress ring stays hidden for the first 320 ms. Only after that grace period does it fill over the configured radial hold duration. The timer and visible progress now use the same delay, eliminating the earlier mismatch that displayed the ring immediately even though menu activation itself had a delay.

## Automated validation

The host input regression suite covers:

- fast-release direction and maximum-velocity clamping;
- slow-release suppression;
- immediate interruption by finger and stylus contact;
- camera-only motion during fling;
- equal stopping distance at simulated 60, 90, and 120 Hz;
- hidden radial progress during the grace period and smooth progress afterward;
- all prior pointer ownership, cancellation, transform rollback, eraser, and UI commit behavior.

APK inspection verifies package identity, version 3.2.13/code 48, SDK/ABI declarations, packaged native libraries and OCR assets, lack of network permissions, and APK Signature Scheme v2 integrity.

The initial tablet candidate used a 5200 px/s cap with 5200 px/s² deceleration. A very fast device swipe showed that this could coast almost another screen width, so the shipped constants were tightened to the controlled 3200/6400 values above before final packaging.

## Manual tablet acceptance

On the target tablet, verify slow drags stop directly, fast one- and two-finger swipes coast briefly without a direction jump, very fast swipes remain controlled, and touching or pinching again takes control immediately. Confirm stylus contact stops motion before drawing/erasing and that the radial progress ring does not appear during an ordinary finger pan.
