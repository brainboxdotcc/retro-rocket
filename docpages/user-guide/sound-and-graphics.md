\page sound-and-graphics Sound and Graphics

Retro Rocket includes built-in support for both sound and graphics.
You don’t need to install drivers or change any settings — everything is automatic.

---

### Graphics
- The system uses your PC’s built-in graphics to display text, colours, and pixel art.
- Demos and programs (such as `cube`, `graphicstest`, or `demo`) can draw directly to the screen.
- Resolution and colour depth are detected at boot and just work.
- There is no need to configure graphics drivers.

### Sound
- If your PC has a supported sound chip (AC’97 or HDA), music and sound effects play automatically.
- Programs can use `SOUND`, `ENVELOPE`, and other BASIC keywords to generate tones or play tracker modules.
- Demo programs like `bach`, `musicdemo`, and `modplay` show off the audio system.
- If no sound device is present, programs still run - you just won’t hear audio output.

### Notes
- Retro Rocket is focused on simplicity: sound and graphics are built-in parts of the system, not add-ons.
- Demos and examples are the easiest way to see (and hear) what the system can do.
- Advanced users can also explore writing their own audio and graphics programs in BASIC.
