\page SOUND SOUND Keyword

[TOC]

```basic
SOUND VOLUME stream, gain
SOUND PLAY stream, sound
SOUND PLAY stream
SOUND STOP stream
SOUND PAUSE stream
SOUND LOAD variable, "filename"
SOUND UNLOAD sound
```

Provides control over **sound playback**. Sounds are loaded from WAV files and played on **streams**.
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
SOUND PLAY stream
```

Plays or resumes audio on a stream.

* With a `sound` handle, queues the sound into the stream for playback.
* Without a `sound`, resumes a previously paused stream.

**Notes**

* Streams are asynchronous. Program execution continues while playback occurs.
* More than one sound can be queued on the same stream; they will play in order.
* If you call `SOUND PLAY` with no sound argument while nothing is paused, nothing happens.

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

Loads a WAV file into memory and assigns a **sound handle** to `variable`.

**Notes**

* Only PCM and IEEE float WAV files are supported.
* Sound data is stored in RAM until freed. Large audio files may consume significant memory.
* Sound handles are distinct from streams:

  * A **sound handle** is the decoded audio data.
  * A **stream** is where playback occurs.
* Always free loaded sounds when no longer needed using `SOUND UNLOAD`.

**Errors**

* `Unable to load WAV file 'filename'` if the file could not be opened.
* `Out of memory loading WAV file 'filename'` if allocation fails.

**Examples**

```basic
SOUND LOAD song, "/system/webserver/media/retro-revival.wav"
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

### Usage example

```basic
STREAM CREATE music
SOUND LOAD song, "/system/webserver/media/retro-revival.wav"
SOUND VOLUME music, DECIBELS(-6)
SOUND PLAY music, song

' ... do other work here ...

SOUND PAUSE music
SOUND PLAY music   ' resume playback
SOUND STOP music
SOUND UNLOAD song
STREAM DESTROY music
```

---

**See also**
\ref STREAM "STREAM" · \ref DECIBELS "DECIBELS"
