\page TLSVERSION TLSVERSION$ Function

```basic
V$ = TLSVERSION$(ssl-socket-handle)
```

If the provided `ssl-socket-handle` is associated with a connected TLS/SSL connection, the TLS version will be returned from this function.

For any other socket type, including invalid sockets, the string `"unknown"` is returned.

***Example***

```BASIC
SSLCONNECT ssl, "secure.server.example.com", 443
PRINT TLSVERSION$(ssl)
CLOSE ssl
```

**See also:**
\ref TLSCIPHER "TLSCIPHER$" · \ref SSLCONNECT "SSLCONNECT" · \ref SSLACCEPT "SSLACCEPT"