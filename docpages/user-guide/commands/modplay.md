\page modplay modplay command

`modplay` is a simple music player for Retro Rocket.

It plays music files from the `/system/media` directory and displays a colourful real-time music visualiser while the track is playing.

Supported file types include:

* WAV
* MP3
* FLAC
* OGG
* MOD
* XM

Music will loop automatically.

\image html modplay.png

---

### Usage

```text
modplay filename
```

Example:

```text
modplay song.mp3
```

This plays:

```text
/system/media/song.mp3
```

---

### Features

* Plays music files from `/system/media`
* Real-time animated spectrum visualiser
* Automatic loading of audio support
* Supports both modern audio formats and classic tracker music

---

### Notes

* A sound card must be available.
* Playback continues until a key is pressed.
* If the file cannot be recognised or loaded, an error will occur.

---

### Examples

Play an MP3:

```text
modplay demo.mp3
```

Play a tracker module:

```text
modplay gate.xm
```

Play a FLAC file:

```text
modplay ambient.flac
```
