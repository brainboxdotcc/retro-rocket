\page init init program

## init

`init` is the first program that runs when Retro Rocket starts.
It takes care of the initial setup: mounting filesystems, loading drivers, creating a RAM disk, and finally launching the shell (`rocketsh`).

### What it does
- Sets the default library path for BASIC programs.
- Loads drivers (for example, network support).
- Mounts system filesystems such as `/devices` and `/boot`.
- Sets the keyboard layout.
- Creates a 1 GB RAM disk at `/ramdisk` so you have scratch space to save and run programs in LiveCD mode.
- Displays the boot logo and prepares the screen.
- Finally, launches the shell (`rocketsh`). If the shell ever exits, `init` simply restarts it.

### Usage
You never need to run `init` yourself - it is started automatically at boot.

### Installed systems
When Retro Rocket is installed to a hard disk, the `init` program is just another BASIC program stored under `/programs`.
This means you can freely edit it, customise it, and even replace parts of the startup process to suit your own needs.

### Notes
- On the LiveCD, `init` is fixed.
- On an installed system, you are in full control - it’s your startup script.
- Be careful: if you remove or break the call to `rocketsh`, you may end up without a shell!
