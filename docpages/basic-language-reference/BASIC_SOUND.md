\page audio-basics Audio Introduction

[TOC]

## Audio System Structure

Retro Rocket’s audio system has three main parts:

### Drivers

The driver is the low-level hardware backend (e.g. `MODLOAD "ac97"`).
It is specific to your sound card in your PC. Without a driver, **no audio commands will work**.


### Streams

A stream is a **playback channel** created with `STREAM CREATE`.

* Each stream has its own queue of sounds, volume, and state.
* Multiple streams can run at once (for mixing music, effects, voices, etc.).
* A stream handle is always a positive integer ID.


### Sounds

A sound is **decoded audio data** loaded into memory with `SOUND LOAD`.

* Sounds can be loaded from any .WAV file which contains PCM or FLOAT format audio.
* Sounds are stored in RAM as 44.1 kHz stereo, 16-bit PCM.
* Sound handles are always non-zero integer IDs.
* Sounds must be freed with `SOUND UNLOAD` when no longer needed.


## Flow of audio

```
File on disk (WAV) → SOUND LOAD → Sound handle
Sound handle + Stream → SOUND PLAY → Playback
```

## Example

```basic
MODLOAD "ac97"             ' load audio driver
STREAM CREATE music        ' create playback channel
SOUND LOAD song, "track.wav"  ' load audio into RAM
SOUND PLAY music, song     ' play on stream
```

\image html sound.png

## Audio Keywords/Functions

- \ref STREAM "STREAM"
- \ref STREAM "SOUND"
- \ref DECIBELS "DECIBELS"