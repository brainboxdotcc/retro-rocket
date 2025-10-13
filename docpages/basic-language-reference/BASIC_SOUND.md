\page audio-basics Audio Introduction

[TOC]

## Audio System Structure

Retro Rocket’s audio system has five main parts:

### Drivers

The driver is the low-level hardware backend (e.g. `MODLOAD "ac97"`).  
It is specific to your sound card in your PC. Without a driver, **no audio commands will work**.

Currently supported sound cards:

| Device | Description                                        | Command          |
|--------|----------------------------------------------------|------------------|
| AC'97  | Audio Codec ’97; legacy PCI audio standard         | `MODLOAD "ac97"` |
| HDA    | High Definition Audio; Azalia (successor to AC’97) | `MODLOAD "hda"`  |

### Codecs

A codec is a small specialised program that knows how to **decode audio files** into the raw data format used by Retro Rocket.
Codecs are provided as kernel modules, just like drivers, and must be loaded before you can use them.

| Format | Description                                         | Command                |
|--------|-----------------------------------------------------|------------------------|
| WAV    | Uncompressed PCM; built-in and always available     | *(no module required)* |
| MP3    | MPEG-1 Layer III compressed audio                   | `MODLOAD "mp3"`        |
| FLAC   | Free Lossless Audio Codec                           | `MODLOAD "flac"`       |
| OGG    | Ogg Vorbis compressed audio                         | `MODLOAD "ogg"`        |
| MOD    | Amiga-style tracker modules (ProTracker, etc.)      | `MODLOAD "mod"`        |

If you try to load any file type except WAV without first loading the corresponding codec module, you will receive an error as the file type will be unrecognised.

### Streams

A stream is a **playback channel** created with `STREAM CREATE`.

* Each stream has its own queue of sounds, volume, and state.
* Multiple streams can run at once (for mixing music, effects, voices, etc.).
* A stream handle is always a positive integer ID.

### Sounds

A sound is **decoded audio data** loaded into memory with `SOUND LOAD`.

* Sounds can be loaded from any supported codec (e.g. WAV, MP3, FLAC, OGG).
* Sounds are stored in RAM as 44.1 kHz stereo, 16-bit PCM.
* Sound handles are always non-zero integer IDs.
* Sounds must be freed with `SOUND UNLOAD` when no longer needed.
* You may generate basic square wave sounds without loading a sound file via `SOUND TONE`

### Envelopes

An envelope is a **shape applied to the loudness or tone of a sound over time**.
If you ever played games on a C64 or BBC Micro, you’ll remember how the same “beep” could sometimes be a pluck, a swelling chord, or a fading echo. That was done with **envelopes**: they control how a note evolves after it starts.

Retro Rocket uses the classic **ADSR model** (Attack, Decay, Sustain, Release) found on many vintage synthesisers:

* **Attack** - how quickly the sound rises from silence to full strength when a note begins.
* **Decay** - once the peak is reached, how fast the sound falls back down to a steady level.
* **Sustain** - the level held as long as the note continues.
* **Release** - how long the note takes to fade back to silence after it ends.

## Flow of audio

```
File on disk (WAV/MP3/other) → Codec module decodes → SOUND LOAD → Sound handle
Sound handle + Stream → SOUND PLAY → Playback
```

## Examples

### Play a WAV file

```basic
MODLOAD "ac97"                     ' load audio driver
STREAM CREATE music
SOUND LOAD song, "track.wav"
SOUND PLAY music, song
```

\image html sound.png

### Play an MP3 file

```basic
MODLOAD "ac97"                     ' load audio driver
MODLOAD "mp3"                      ' load MP3 codec module
STREAM CREATE music
SOUND LOAD song, "track.mp3"
SOUND PLAY music, song
```

\image html mp3.png

### Play a FLAC file

```basic
MODLOAD "ac97"                     ' load audio driver
MODLOAD "flac"                     ' load FLAC codec module
STREAM CREATE music
SOUND LOAD song, "track.flac"
SOUND PLAY music, song
```

### Playing tones/envelopes

```basic
MODLOAD "ac97"
STREAM CREATE s
' Simple square wave, no envelope applied
SOUND TONE s, 750, 100
' Simple square wave with organ-like sustain
ENVELOPE CREATE 0, 0, 255, 128, 5, 50, 200, 200, 0, 0, 0, 0, 0
' Soft sine wave pad, slow attack and release
ENVELOPE CREATE 1, 1, 200, 0, 500, 300, 180, 800, 0, 0, 0, 0, 0
' Plucked sawtooth, short attack/decay, no sustain
ENVELOPE CREATE 2, 2, 255, 0, 5, 100, 0, 200, 0, 0, 0, 0, 0
' Vibrato lead (square wave with pitch wobble)
ENVELOPE CREATE 3, 0, 220, 128, 10, 100, 180, 300, 30, 6, 0, 0, 0
' PWM effect (pulse-width modulation sweep)
ENVELOPE CREATE 4, 0, 240, 64, 20, 100, 200, 400, 0, 0, 0, 5, 120
SOUND TONE s, 750, 100, 0
SOUND TONE s, 750, 100, 1
SOUND TONE s, 750, 100, 2
SOUND TONE s, 750, 100, 3
SOUND TONE s, 750, 100, 4
```

## Audio Keywords/Functions

* \ref STREAM "STREAM"
* \ref STREAM "SOUND"
* \ref DECIBELS "DECIBELS"
* \ref ENVELOPE "ENVELOPE"
