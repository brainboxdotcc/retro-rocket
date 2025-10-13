\page file-system File System

Retro Rocket uses a simple, structured file system.
Everything is visible under the root (`/`) directory.
This section explains what each directory is for and what you can expect to find inside.

### Directory overview

- **/boot**
  The boot partition.
  - On LiveCD it points to the EFI boot partition of the install media.
  - On an installed system it contains the boot files used at startup.

- **/images**
  Operating system images and graphics used by Retro Rocket and its demos.

- **/programs**
  BASIC programs and software.
  - `/programs/libraries` - shared BASIC libraries.
  - `/programs/drivers` - drivers written in BASIC.

- **/system**
  Core system files and configuration.
  - `/system/config` - configuration files such as `network.conf` (used to configure the network interface).
  - `/system/keymaps` - keyboard layouts.
  - `/system/modules` - Kernel modules
  - `/system/ssl` - SSL root certificates.
  - `/system/timezones` - IANA time zone database.
  - `/system/webserver` - root directory for the built-in webserver.
    - `/system/webserver/media` - media files available to the webserver.

- **/ramdisk**
  A temporary 1 GB RAM disk created in LiveCD mode.
  - Use this as scratch space for creating or editing BASIC programs.
  - Files here are lost when the system is shut down or restarted.

- **/harddisk**
  Exists only when running from the LiveCD.
  - Used by the installer to mount the installation target disk.
  - Not normally used by end-users.

---

Retro Rocket does not attempt to hide or complicate its file system. Everything is in plain view, and you can explore with simple commands like `dir`.
