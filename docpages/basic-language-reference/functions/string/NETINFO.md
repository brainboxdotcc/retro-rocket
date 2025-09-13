\page NETINFO NETINFO$ Function

```basic
NETINFO$(string-expression)
```

Returns **network configuration settings** as a string.
The parameter specifies which setting to query:

* `"ip"` → current IP address
* `"gw"` → gateway address
* `"mask"` → network mask
* `"dns"` → DNS server address

\image html ip.png

---

### Examples

```basic
PRINT "IP Address: "; NETINFO$("ip")
PRINT "Gateway:    "; NETINFO$("gw")
PRINT "Mask:       "; NETINFO$("mask")
PRINT "DNS:        "; NETINFO$("dns")
```

Might produce:

```
IP Address: 192.168.1.42
Gateway:    192.168.1.1
Mask:       255.255.255.0
DNS:        8.8.8.8
```

---

### Notes

* Returned values depend on the active network configuration of the operating system.
* If no network is configured, an error is raised.
* All results are IPv4 addresses in dotted decimal notation.

---

**See also:**
\ref DNS "DNS$" · \ref CONNECT "CONNECT" · \ref SOCKSTATUS "SOCKSTATUS"
