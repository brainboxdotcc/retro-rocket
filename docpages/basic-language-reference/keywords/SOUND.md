\page SOUND SOUND Keyword

\[TOC]

```basic
SOUND VOLUME stream, gain
SOUND PLAY stream, sound[, pitch]
SOUND PLAY stream
SOUND STOP stream
SOUND PAUSE stream
SOUND LOAD variable, "filename"
SOUND UNLOAD sound
SOUND TONE stream, frequency, duration[, envelope]
```

Provides control over **sound playback**.
Sounds may come from loaded audio files (e.g. WAV, MP3, FLAC, Ogg/Vorbis) or be generated on the fly as **tones** using simple synthesiser-style envelopes.

A stream acts like a playback channel; multiple streams can be active at once, each with their own volume and queue of sounds.

---

### SOUND VOLUME

```basic
SOUND VOLUME stream, gain
```

Sets the **volume** for a stream.
The `gain` value ranges from 0–255:

* `0` is silence.
* `255` is full volume (internally treated as 256 for exact unity).
* Intermediate values reduce the volume proportionally.

**Notes**

* For convenience, the function `DECIBELS()` can be used to convert from dB into the correct gain value.
* Volume changes affect all future playback on the stream immediately.

**Errors**

* `Invalid STREAM handle` if the stream does not exist.

**Examples**

```basic
SOUND VOLUME music, 64
SOUND VOLUME music, DECIBELS(-12)
```

---

### SOUND PLAY

```basic
SOUND PLAY stream, sound
SOUND PLAY stream, sound, pitch
SOUND PLAY stream
```

Plays or resumes audio on a stream.

* With a `sound` handle, queues the sound into the stream for playback.
* Without a `sound`, resumes a previously paused stream.
* Passing the third parameter applies a pitch adjustment to the sound, in Hz.

**Notes**

* Streams are asynchronous. Program execution continues while playback occurs.
* More than one sound can be queued on the same stream; they will play in order.
* If you call `SOUND PLAY` with no sound argument while nothing is paused, nothing happens.
* Adjusting the pitch of a sound with the third parameter also slows it down (for negative values) or speeds it up (for positive values).

**Errors**

* `Invalid STREAM handle` if the stream does not exist.
* `SOUND PLAY: Invalid sound handle` if the sound is invalid or has been unloaded.

**Examples**

```basic
SOUND PLAY music, song
SOUND PAUSE music
SOUND PLAY music   ' resumes paused playback
```

---

### SOUND STOP

```basic
SOUND STOP stream
```

Stops playback on the given stream immediately and clears any queued sounds.

**Errors**

* `Invalid STREAM handle` if the stream does not exist.

**Examples**

```basic
SOUND STOP music
```

---

### SOUND PAUSE

```basic
SOUND PAUSE stream
```

Pauses playback on the given stream.
Playback can be resumed later with `SOUND PLAY stream` (without a sound argument).

**Errors**

* `Invalid STREAM handle` if the stream does not exist.

**Examples**

```basic
SOUND PAUSE music
SOUND PLAY music   ' resume
```

---

### SOUND LOAD

```basic
SOUND LOAD variable, "filename"
```

Loads an audio file into memory and assigns a **sound handle** to `variable`.

**Notes**

* WAV, MP3, FLAC, and Ogg/Vorbis are supported, depending on which codecs are loaded.
* Sound data is stored in RAM until freed. Large audio files may consume significant memory.
* Sound handles are distinct from streams:

  * A **sound handle** is the decoded audio data.
  * A **stream** is where playback occurs.
* Always free loaded sounds when no longer needed using `SOUND UNLOAD`.

**Errors**

* `Unable to load file 'filename'` if the file could not be opened or decoded.
* `Out of memory loading 'filename'` if allocation fails.

**Examples**

```basic
SOUND LOAD song, "/system/webserver/media/retro-revival.mp3"
PRINT song
```

---

### SOUND UNLOAD

```basic
SOUND UNLOAD sound
```

Unloads a sound and frees its memory.
After unloading, the handle becomes invalid and cannot be used with `SOUND PLAY`.

**Errors**

* `SOUND UNLOAD: Invalid sound handle` if the handle is not valid.

**Examples**

```basic
SOUND UNLOAD song
```

---

### SOUND TONE

```basic
SOUND TONE stream, frequency, duration[, envelope]
```

Generates and plays a tone on the fly.
This does not use a sound handle; the waveform is synthesised directly into the stream.

Parameters:

* **stream** — Stream to play into.
* **frequency** — Pitch in Hz (e.g. `440` for concert A).
* **duration** — Length of the note in centiseconds (1/100th of a second).
* **envelope** (optional) — Envelope slot (0–63) previously defined with \ref ENVELOPE "ENVELOPE CREATE". If omitted, the tone plays raw with no shaping.

**Notes**

* Tones are ephemeral and consume no long-term memory.
* Envelopes let you design the “shape” of the note: attack, decay, sustain, release, vibrato, PWM, etc.
* Multiple tones can overlap across different streams for chords, effects, or music.

**Errors**

* `Invalid STREAM handle` if the stream does not exist.
* `ENVELOPE number out of range` if the envelope index is invalid.
* `Out of memory for SOUND TONE` if synthesis buffer allocation fails.

**Examples**

```basic
' Simple beep
SOUND TONE music, 440, 50

' With a shaped envelope
ENVELOPE CREATE 0,0,255,128,5,50,200,200,0,0,0,0,0
SOUND TONE music, 220, 100, 0
```

---

### Usage example

```basic
STREAM CREATE music
SOUND LOAD song, "/system/webserver/media/retro-revival.wav"
SOUND VOLUME music, DECIBELS(-6)
SOUND PLAY music, song

' Play a synthesised tone while song plays
SOUND TONE music, 880, 50

SOUND PAUSE music
SOUND PLAY music   ' resume playback
SOUND STOP music
SOUND UNLOAD song
STREAM DESTROY music
```

---

**See also**
\ref STREAM "STREAM" · \ref DECIBELS "DECIBELS" · \ref ENVELOPE "ENVELOPE"
