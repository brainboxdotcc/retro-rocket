\page ENVELOPE ENVELOPE Keyword

[TOC]

```basic
ENVELOPE CREATE n,wave,volume,pulse_width,attack,decay,sustain,release,vibrato_depth,vibrato_rate,glide,pwm_rate,pwm_depth
ENVELOPE DESTROY n
```

Defines or removes a stored **sound envelope**.

An *envelope* describes how a generated tone evolves over time — its loudness, pitch, and timbre changes. This allows Retro Rocket to act like a very simple synthesiser, much like the BBC Micro’s classic sound system, but with added flexibility.

---

### CREATE

Creates or replaces an envelope in slot `n` (0–63).
The envelope can then be used by `SOUND TONE` to shape generated notes.

#### Parameters:

* **n** — Envelope index (0–63).
  A slot number that lets you store multiple different sound shapes and pick between them.

* **wave** — Base waveform type: the raw “tone colour.”

  * `0` = square — harsh and “beepy,” like an old 8-bit computer.
  * `1` = sine — smooth and pure, like a flute.
  * `2` = sawtooth — bright and buzzy, common in synth leads.
  * `3` = triangle — softer, hollow-sounding.
  * `4` = noise — random hiss, used for percussion and effects.

* **volume** — Base loudness (0–255). Higher numbers = louder.

* **pulse\_width** — For square waves only: controls how long the “on” part of the wave lasts in each cycle.

  * 128 = 50% duty cycle = classic square.
  * Lower values = thinner, “nasal” sound.
  * Ignored for other waveforms.

* **attack** — How fast the sound rises from silence to full volume when a note starts.

  * Short attack = instant click or pluck.
  * Long attack = gradual swell.

* **decay** — After the peak, how quickly the sound drops down to the sustain level.

  * Short decay = sharp, percussive.
  * Long decay = smoother, fading into sustain.

* **sustain** — The level (0–255) held while the key/note is pressed.

  * High sustain = sound keeps strong.
  * Low sustain = sound fades to a faint background.

* **release** — How long the note takes to fade back to silence after it ends.

  * Short release = abrupt cut-off.
  * Long release = lingering echo.

* **vibrato\_depth** — How far the pitch wobbles up and down.

  * Measured in cents (100 = one semitone).
  * Small depth = gentle shimmer; large = dramatic warble.

* **vibrato\_rate** — How fast the vibrato wobble happens (Hz = cycles per second).

* **glide** — Portamento: how long it takes to slide from one note’s pitch to the next.

  * Short glide = nearly instant jump.
  * Long glide = smooth slide between notes.

* **pwm\_rate** — Speed of pulse-width modulation (Hz). Only affects square waves. Creates a “chorus-like” movement.

* **pwm\_depth** — How much the pulse width is swept back and forth (0–255).

  * Small depth = subtle shimmer.
  * Large depth = dramatic, hollow sweep.

Together, these controls let you create sounds ranging from short plucks to smooth pads, or classic “SID-style” chiptune effects.

#### Visual example

The blue line shows the amplitude (the technical term for volume, the height of the waveform) of the sound over time, with time being the X axis the Y being the volume (amplitude)

@dot
graph {
  layout=neato; splines=line; overlap=false; outputorder=edgesfirst; bgcolor="white";
  node [shape=point, width=0.08, height=0.08, color="#222222"];
  edge [color="#1f77b4", penwidth=2];

  // Axes (decorative)
  axis0 [shape=plaintext, label="Amplitude (0..255)", pos="0,3.2!"];
  time0 [shape=plaintext, label="Time →", pos="8.6,-0.25!"];
  y0 [shape=point, pos="0,0!", color="#888888"];
  y1 [shape=point, pos="0,3!", color="#888888"];
  y2 [shape=point, pos="0,1.8!", color="#888888"];

  // Tick labels
  l0 [shape=plaintext, label="0", pos="-0.25,0!"];
  l255 [shape=plaintext, label="255", pos="-0.45,3!"];
  lsus [shape=plaintext, label="Sustain", pos="-0.9,1.8!"];

  // Envelope key points (positions are illustrative)
  t0      [pos="0,0!"];
  A_end   [pos="1.0,3!"];     // Attack time (x moved left)
  D_end   [pos="3.0,1.8!"];   // Decay to sustain level
  S_end   [pos="6.0,1.8!"];   // Sustain duration
  R_end   [pos="8.0,0!"];     // Release back to silence

  // Polyline
  t0 -- A_end -- D_end -- S_end -- R_end;

  // Phase braces/labels (plaintext nodes)
  la [shape=plaintext, label="Attack (ms): rise 0 → 255", pos="0.75,3.15!"];
  ld [shape=plaintext, label="Decay (ms): 255 → Sustain", pos="2.25,2.95!"];
  ls [shape=plaintext, label="Sustain (level 0..255)", pos="4.5,2.05!"];
  lr [shape=plaintext, label="Release (ms): Sustain → 0", pos="7.0,0.35!"];

  // Vertical guides (light grey)
  vgA [shape=point, pos="1.0,0!", color="#cccccc"]; // x moved to match A_end
  vgD [shape=point, pos="3.0,0!", color="#cccccc"];
  vgS [shape=point, pos="6.0,0!", color="#cccccc"];

  // Guide lines
  vgA -- A_end [color="#cccccc", penwidth=1];
  vgD -- D_end [color="#cccccc", penwidth=1];
  vgS -- S_end [color="#cccccc", penwidth=1];

  // Horizontal sustain guide
  y2 -- D_end [color="#cccccc", penwidth=1];
  y2 -- S_end [color="#cccccc", penwidth=1];

  // Callouts tying to parameters
  ca [shape=plaintext, label="attack_ms", pos="1.0,-0.25!"]; // x moved
  cd [shape=plaintext, label="decay_ms",  pos="3.0,-0.25!"];
  cs [shape=plaintext, label="(note holds here)", pos="4.5,1.5!"];
  cr [shape=plaintext, label="release_ms", pos="8.0,-0.25!"];
}
@enddot

---

### DESTROY

Removes an envelope from slot `n` (0–63), freeing it for reuse.

---

### Example

```basic
' Envelope 0: simple organ tone
ENVELOPE CREATE 0,0,255,128,5,50,200,200,0,0,0,0,0

' Envelope 1: slow sine pad
ENVELOPE CREATE 1,1,200,0,500,300,180,800,0,0,0,0,0

' Play notes with the defined envelopes
STREAM CREATE s
SOUND TONE s,440,100,0   ' A4 with organ
SOUND TONE s,220,200,1   ' A3 with sine pad
```

---

### Notes

* Envelopes are stored in slots, numbered 0–63. Each BASIC program has its own 63 slots.
* Each \ref SOUND "SOUND" TONE` may optionally use an envelope, or omit te envelope number to play a raw, simple square wave.
* Unlike sampled sounds (\ref SOUND "SOUND" LOAD, SOUND PLAY), tones are generated on the fly and do not consume memory.
* You can create expressive, evolving sounds by experimenting with different waveforms and envelope timings.

---

### See also

\ref SOUND "SOUND" · \ref STREAM "STREAM"
