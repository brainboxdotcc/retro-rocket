\page http HTTP Library

```BASIC
LIBRARY LIB$ + "/http"
```

The HTTP library provides simple URL parsing helpers and minimal client-side HTTP request routines (HTTP/1.0 with `Connection: close`).

Responses are stored internally as **chunked data** to avoid string size limits. The response is split into headers and body after receipt. The body is exposed as a sequence of chunks rather than a single concatenated string.

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

Perform a `GET` request to `url$`.

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
the request over HTTP, or HTTPS when the scheme/port implies HTTPS.

The full response is received in chunks and internally split into headers and body.

---

## Response access

### FNhttp_result(part$)

Return a structured portion of the most recent response.

* `"headers"` - returns the header map
* `"body"` - returns the body map

Both are MAP-backed collections:

* numeric keys `"0"` … `"n-1"` contain chunks
* `"size"` contains the number of chunks

These maps should be treated as read-only.

---

### FNhttp_result$(part$)

Return string-based response data. `part$` must be one of:

* `"status"` - status line (e.g. `HTTP/1.1 200 OK`)
* `"code"` - status code as a string (e.g. `200`)
* `"reason"` - reason phrase (e.g. `OK`)
* `"header:<name>"` - value of a single header (case-insensitive match on `<name>`)

Returns an empty string if no response is available or the requested value does not exist.

---

### PROCfree_http_result

Frees MAPs used by the result. You should call this after you are finished with any result.

---

### FNhttp_has_response

Returns TRUE if the last HTTP request returned a valid response

---

### FNhttp_has_body

Returns TRUE if the last HTTP request had a body part

---

## Body access pattern

The response body is not returned as a single string. Instead, iterate over chunks:

```BASIC
body = FNhttp_result("body")

FOR i = 0 TO MAPGET(body, "size") - 1
    PRINT MAPGET$(body, STR$(i));
NEXT

PROCfree_http_result
```

This allows handling arbitrarily large responses without exceeding string limits.

---

## Example

```BASIC
LIBRARY LIB$ + "/http"

PROChttp_get("http://neuron.brainbox.cc/test.txt")

PRINT "HTTP status code: "; FNhttp_result$("code")
PRINT "Response body:"

body = FNhttp_result("body")

FOR i = 0 TO MAPGET(body, "size") - 1
    PRINT MAPGET$(body, STR$(i));
NEXT
```

This performs a simple `GET`, prints the numeric status code, then streams the body safely.

---

## Notes

* Responses are processed in a streaming-friendly manner; large bodies are never concatenated automatically.
* Header parsing is performed once after the response is fully received.
* HTTP/1.0 with `Connection: close` is used to simplify response handling (no chunked transfer decoding).
* The returned MAP structures should not be modified by user code.
