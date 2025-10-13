\page SSLCONNECT SSLCONNECT Keyword

```basic
SSLCONNECT integer-variable, string-expression, integer-expression [, sni$]
```

Opens a **TLS-encrypted TCP connection** to a remote host.

* **First parameter**: an **integer variable** (created if it does not yet exist).
  On success it will be set to a **non-negative handle** that you then pass to
  \ref SOCKREAD "SOCKREAD",
  \ref SOCKWRITE "SOCKWRITE",
  and \ref SOCKCLOSE "SOCKCLOSE".
* **Second parameter**: a **string** containing the IP address.
  To resolve a hostname into an address string, use \ref DNS "DNS$".
* **Third parameter**: the **port number** (integer expression).
* **Optional fourth parameter**: `sni$` - the Server Name Indication string, used when connecting to servers hosting multiple domains on the same IP.

On **failure**, an error is raised.

---

##### Example: secure web request

```basic
HOST$ = "example.com"
IP$ = DNS$(HOST$)
SSLCONNECT H, IP$, 443, HOST$
SOCKWRITE H, "HEAD / HTTP/1.0" + CHR(13) + CHR(10) + CHR(13) + CHR(10)
SOCKREAD H, REPLY$
PRINT REPLY$
SOCKCLOSE H
```

---

##### Notes

* The first argument **must be an integer variable**, not a literal or expression.
* Validates the remote server’s certificate against the system CA bundle in `/system/ssl/cacert.pem`.
* Supports modern TLS (SSLv3 and older are not supported).
* Always close the handle with `SOCKCLOSE` when finished.
* `SOCKREAD` is a **blocking** operation; see its page for cancellation with `CTRL+ESC`.

---

## SSL Certificate Bundle

Retro Rocket uses a CA certificate bundle to validate remote servers when making secure connections with \ref SSLCONNECT "SSLCONNECT".
The expected location is:

```
/system/ssl/cacert.pem
```

This file should contain one or more trusted **root certificate authorities** in PEM format.

---

### How it is used

* On the first call to \ref SSLCONNECT "SSLCONNECT", the bundle is loaded into memory.
* Every subsequent secure connection reuses the cached bundle.
* If the bundle is missing or invalid, connections will fail with a certificate error.

---

### Managing the bundle

* Retro Rocket does not generate this file automatically.
* You may copy a pre-built `cacert.pem` (such as the one provided by the cURL project) into `/system/ssl/`.
* Alternatively, you may regenerate your own bundle from Mozilla’s trusted store, using the `mk-ca-bundle` script.
* The bundle must be kept up to date to maintain security.
* If a certificate authority is compromised or revoked, you should refresh `cacert.pem`.
* The file must be world-readable so all processes can validate servers.

---

**See also:**
\ref DNS "DNS$" · \ref SOCKREAD "SOCKREAD" · \ref SOCKWRITE "SOCKWRITE" · \ref SOCKCLOSE "SOCKCLOSE" · \ref SOCKSTATUS "SOCKSTATUS" · \ref SSLSOCKACCEPT "SSLSOCKACCEPT"
