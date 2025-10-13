\page http HTTP Library

```BASIC
LIBRARY LIB$ + "/http"
```

The HTTP library provides simple URL parsing helpers and minimal client-side HTTP request routines (HTTP/1.0 with `Connection: close`).
Responses are stored in an internal buffer for later inspection.

The following publicly documented procedures and functions are available via this library.

---

## URL helpers

### FNhttp_url_scheme$(url$)

Return the scheme (e.g. `http`, `https`, `ws`, `wss`, `ftp`) or an empty string if none.

### FNhttp_url_anchor$(url$)

Return the fragment (text after `#`) or an empty string if none.

### FNhttp_url_query$(url$)

Return the query string (text after `?`, excluding `#…`) or an empty string if none.

### FNhttp_url_path$(url$)

Return the path component beginning with `/`. Returns `/` for bare hosts and an empty string if not applicable.

### FNhttp_url_host$(url$)

Return the host portion of the authority. Supports IPv4/hostnames and bracketed IPv6. Returns an empty string if not present.

### FNhttp_url_port(url$)

Return the explicit port if present; otherwise the default for the scheme (`http`/`ws`=80, `https`/`wss`=443, `ftp`=21).
Returns `0` if no scheme is recognised and no explicit port is present.

### FNhttp_url_param$(url$, key$)

Look up a single query parameter value by name. Returns the value if present, an empty string if the key appears with no value, or an empty string if the key is absent.

---

## Requests

### PROChttp_get(url$)

Perform a `GET` request to `url$`. The full response (headers + body) is captured internally.

### PROChttp_head(url$)

Perform a `HEAD` request. Only headers are expected; body may be empty.

### PROChttp_post(url$, postdata$)

Perform a `POST` with `postdata$` as the raw request body. `Content-Length` is set automatically.

### PROChttp_put(url$, putdata$)

Perform a `PUT` with `putdata$` as the raw request body.

### PROChttp_patch(url$, patchdata$)

Perform a `PATCH` with `patchdata$` as the raw request body.

### PROChttp_delete(url$)

Perform a `DELETE` request.

### PROChttp_request(method$, url$, body$)

Low-level entry point used by the above helpers. Builds the request line and minimal headers (`Host`, `Connection: close`, optional `Content-Length`) and sends
the request over HTTP, or HTTPS when the scheme/port implies HTTPS. The full response is stored for later retrieval.

---

## Response access

### FNhttp_result$(part$)

Return a specific portion of the most recent response. `part$` must be one of:

* `"body"` — response body only
* `"headers"` — raw header block (status line + header lines, CRLF-delimited)
* `"status"` — status line (e.g. `HTTP/1.1 200 OK`)
* `"code"` — status code as a string (e.g. `200`)
* `"reason"` — reason phrase (e.g. `OK`)
* `"header:<name>"` — value of a single header (case-insensitive match on `<name>`). Returns the first occurrence, trimmed of surrounding whitespace.

If no response is available, or the requested part does not exist, an empty string is returned.

---

## Example

```BASIC
LIBRARY LIB$ + "/http"

PROChttp_get("http://neuron.brainbox.cc/test.txt")

PRINT "HTTP status code: "; FNhttp_result$("code")
PRINT "Response body:"
PRINT FNhttp_result$("body")
```

This performs a simple `GET`, prints the numeric status code, then prints the body.
