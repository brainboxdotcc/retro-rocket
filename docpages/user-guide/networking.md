\page user-networking Networking

Retro Rocket includes basic networking support so you can connect to the internet and use network-enabled programs.

---

### Network Drivers
- The network driver is loaded in the **boot script** `/programs/init`.
- By default, Retro Rocket loads the **e1000** driver, which also supports the Intel 82541PI chipset.
- On an installed system, you can freely edit `/programs/init` (using the `edit` command) to change which driver is loaded.

For most users the default works automatically.

---

### Network Configuration
Once the network driver has been initialised, the system reads its configuration from:

```
/system/config/network.conf
```

This file controls how your machine identifies itself and how it connects.

#### Example: Default configuration

```
# Retro Rocket Network Configuration
# ----------------------------------
#
# This file is read by the network system once a network interface card has
# been initialised.
#
# The loopback interface (127.0.0.1) is always available and does not need
# to be configured here.
#
# At present only a single physical network card is supported. The first
# kernel module that successfully initialises a NIC takes precedence.

# Hostname of the machine
hostname retrorocket

# IPv4 address of the machine in dotted notation, or the literal value 'dhcp'
ip dhcp

# One or more IPv4 DNS server addresses, comma-separated, or 'dhcp'
dns dhcp

# Default gateway IPv4 address in dotted notation, or 'dhcp'
gateway dhcp

# IPv4 network mask in dotted notation, or 'dhcp'
netmask dhcp
```

---

### Notes
- By default, Retro Rocket uses **DHCP** for IP, DNS, gateway, and netmask.
- You can replace `dhcp` with fixed values if you want a static configuration.
- Only **one network card** is supported. The first successfully initialised driver is used.
- The loopback interface `127.0.0.1` is always present for local networking and testing.
