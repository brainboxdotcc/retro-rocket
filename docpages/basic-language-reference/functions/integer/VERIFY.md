\page VERIFY VERIFY Function

```basic
check = VERIFY(file$, sig$, cert$)
```

Verifies a signed file against a detached signature and package signing certificate.

The package certificate is validated against the built-in Retro Rocket package root certificate before the file signature is checked.

Returns non-zero if the signature and certificate chain are valid, otherwise returns zero.

---

### Examples

```basic
REM Verify a downloaded package

IF VERIFY("/ramdisk/editor.gz", "/ramdisk/editor.gz.sig", "/ramdisk/packages.pem") = TRUE THEN
	PRINT "Package verified"
ELSE
	PRINT "Package verification failed"
ENDIF
```

```basic
REM Refuse to install invalid packages

IF VERIFY(PKG$, SIG$, CERT$) THEN
	PRINT "Package is not trusted"
	END
ENDIF
```

---

### Notes

* `file$` is the file to verify.
* `sig$` is the detached package signature file.
* `cert$` is the PEM package signing certificate.
* Detached signatures are generated using \ref SIGNS "SIGN$".
* The trusted package root certificate is loaded automatically from `/system/ssl/package_root_ca.pem`.
* Detached signatures are expected to contain escaped binary signature data.
* Verification checks:

  * certificate chain validity
  * certificate validity dates
  * detached Ed25519 signature validity

---

### Errors

* File not found
* Error reading file
* Error reading signature
* Error reading certificate
* Error reading root certificate
* Out of memory reading file
* Out of memory reading signature
* Out of memory reading certificate
* Out of memory reading root certificate

---

**See also:**
\ref SIGNS "SIGN$" · \ref BINWRITE "BINWRITE"

