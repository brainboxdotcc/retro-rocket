\page STREAM STREAM Keyword

[TOC]

```basic
STREAM CREATE variable
STREAM DESTROY stream
```

Creates or destroys an **audio stream**.
A stream is a playback channel that can accept sounds loaded with `SOUND LOAD`. Each stream has its own independent queue, volume, and state (playing, paused, or stopped).

---

### STREAM CREATE

```basic
STREAM CREATE variable
```

Allocates a new audio stream and assigns its numeric handle to `variable`.

**Notes**

* A stream handle is required before any sound can be played.
* Each stream is independent: multiple streams can be active at once, each with different sounds or volume levels.
* There is a fixed maximum number of streams (implementation dependent). Once all are in use, no new streams can be created until one is destroyed.
* If no audio driver is loaded, the command fails.

**Errors**

* `Out of STREAMs` if the maximum number of streams is reached.

**Examples**

```basic
STREAM CREATE music
PRINT music
```

---

### STREAM DESTROY

```basic
STREAM DESTROY stream
```

Frees a previously created stream, releasing its resources.

**Notes**

* Destroying a stream automatically stops any playback and clears its queue.
* Once destroyed, the stream handle is invalid and cannot be reused.

**Errors**

* `Invalid STREAM handle` if the given handle is not valid.

**Examples**

```basic
STREAM DESTROY music
```

---

### Relationship to SOUND

* A **stream** is like a playback channel. It does not store sound data itself.
* A **sound handle** (from `SOUND LOAD`) represents decoded audio data in memory.
* To play a sound, you must queue it on a stream with `SOUND PLAY`.

---

### Typical usage

```basic
STREAM CREATE s
SOUND LOAD song, "/system/webserver/media/retro-revival.wav"
SOUND VOLUME s, DECIBELS(-6)
SOUND PLAY s, song
```

Later, when finished:

```basic
SOUND UNLOAD song
STREAM DESTROY s
```

---

**See also**
\ref SOUND "SOUND" · \ref DECIBELS "DECIBELS" · \ref ENVELOPE "ENVELOPE"
