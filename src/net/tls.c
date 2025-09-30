#include <kernel.h>
#include <mbedtls/entropy.h>
#include <mbedtls/platform.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl.h>
#include <tls.h>
#include <mbedtls/error.h>
#include <mbedtls/debug.h>

#define ENTROPY_POOL_SIZE 256

static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context drbg;
static volatile unsigned char entropy_pool[ENTROPY_POOL_SIZE];
static volatile size_t entropy_head = 0;

struct tls_peer {
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	struct tls_io io;
	int fd;
	int error;
};

size_t tls_peer_size() {
	return sizeof(struct tls_peer);
}

static struct tls_peer **tls_by_fd = NULL;
static size_t tls_fd_cap = 0;

bool tls_fd_table_init(size_t fd_cap) {
	tls_by_fd = kcalloc(fd_cap, sizeof(*tls_by_fd));
	if (!tls_by_fd) {
		return false;
	}
	tls_fd_cap = fd_cap;
	return true;
}

bool tls_fd_in_range(int fd) {
	return (fd >= 0) && ((size_t) fd < tls_fd_cap);
}

struct tls_peer *tls_get(int fd) {
	return (tls_fd_in_range(fd)) ? tls_by_fd[fd] : NULL;
}

const char* tls_error_get(int err, char* buffer, size_t size) {
	mbedtls_strerror(err, buffer, size);
	return buffer;
}

bool tls_attach(int fd, struct tls_peer *p) {
	if (!tls_fd_in_range(fd) || tls_by_fd[fd] != NULL) {
		return false;
	}
	tls_by_fd[fd] = p;
	return true;
}

void tls_detach(int fd) {
	if (tls_fd_in_range(fd)) {
		tls_by_fd[fd] = NULL;
	}
}

bool tls_read_fd(int fd, void *buf, size_t len, int *want, int *out_n, int* err) {
	struct tls_peer *p = tls_get(fd);
	if (!p) {
		return false;
	}
	int n = tls_read_nb(&p->ssl, buf, len, want);
	if (n >= 0) {
		*out_n = n;
		return true;
	}
	*err = n;
	return false;
}

bool tls_write_fd(int fd, const void *buf, size_t len, int *want, int *out_n) {
	struct tls_peer *p;
	int n;

	p = tls_get(fd);
	if (p == NULL) {
		return false;
	}

	n = mbedtls_ssl_write(&p->ssl, (const unsigned char *) buf, len);
	if (n >= 0) {
		*out_n = n;
		return true;
	}

	if (n == MBEDTLS_ERR_SSL_WANT_WRITE) {
		*want = 2;
		*out_n = 0;
		return false;
	}
	if (n == MBEDTLS_ERR_SSL_WANT_READ) {
		*want = 1;
		*out_n = 0;
		return false;
	}
	char e[128];
	mbedtls_strerror(n, e, sizeof(e));
	dprintf("write error: %s (%d)\n", e, n);

	*want = -1;
	*out_n = 0;
	return false;
}

bool tls_close_fd(int fd) {
	struct tls_peer *p = tls_get(fd);
	if (!p) {
		return false;
	}
	mbedtls_ssl_close_notify(&p->ssl);
	mbedtls_ssl_free(&p->ssl);
	mbedtls_ssl_config_free(&p->conf);
	kfree(p);
	tls_detach(fd);
	return true;
}

void mbedtls_platform_zeroize(void *buf, size_t len) {
	volatile unsigned char *p = (volatile unsigned char *) buf;
	while (len > 0) {
		*p++ = 0;
		len--;
	}
}

void entropy_irq_event(void) {
	unsigned long long tsc;
	__asm__ volatile("rdtsc" : "=A"(tsc));
	unsigned char sample = (unsigned char) (tsc & 0xFF);
	size_t pos = entropy_head % ENTROPY_POOL_SIZE;
	entropy_pool[pos] = sample;
	entropy_head++;
}

int entropy_poll(void *data, unsigned char *output, size_t len, size_t *olen) {
	size_t count = 0;

	while (count < len) {
		/* spin until we have a new sample */
		size_t head = entropy_head;
		if (head == 0) {
			continue;
		}
		size_t pos = (head - 1) % ENTROPY_POOL_SIZE;
		output[count++] = entropy_pool[pos];
	}

	*olen = count;
	return 0;
}

void nonconst_kfree(void *p) {
	kfree(p);
}

mbedtls_time_t mbedtls_platform_time(mbedtls_time_t *tloc) {
	time_t now = time(NULL);
	if (tloc) *tloc = now;
	return now;
}

static int cert_verify_cb(void *ctx, mbedtls_x509_crt *crt, int depth, uint32_t *flags) {
	if (*flags != 0) {
		char buf[1024];
		mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "    ", crt);
		dprintf("cert verify: depth=%d flags=0x%08x\n%s\n", depth, (unsigned) *flags, buf);

		if (*flags & MBEDTLS_X509_BADCERT_EXPIRED)
			dprintf("  -> cert expired\n");
		if (*flags & MBEDTLS_X509_BADCERT_FUTURE)
			dprintf("  -> cert not yet valid\n");
		if (*flags & MBEDTLS_X509_BADCERT_CN_MISMATCH)
			dprintf("  -> hostname mismatch\n");
		if (*flags & MBEDTLS_X509_BADCERT_NOT_TRUSTED)
			dprintf("  -> not signed by a trusted CA\n");
		if (*flags & MBEDTLS_X509_BADCERT_OTHER)
			dprintf("  -> other X509 problem\n");
	}

	return 0;
}


int tls_global_init(void) {
	mbedtls_platform_set_calloc_free(kcalloc, nonconst_kfree);

	mbedtls_entropy_init(&entropy);
	mbedtls_platform_set_time(mbedtls_platform_time);
	mbedtls_ctr_drbg_init(&drbg);
	mbedtls_entropy_add_source(&entropy, entropy_poll, NULL, 32, MBEDTLS_ENTROPY_SOURCE_STRONG);

	if (mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) "retro-rocket", 12) != 0) {
		return -1;
	}
	return 0;
}

void tls_global_free(void) {
	mbedtls_ctr_drbg_free(&drbg);
	mbedtls_entropy_free(&entropy);
}

static int send_shim(void *p, const unsigned char *buf, size_t len) {
	const struct tls_io *io = (const struct tls_io *) p;
	int n = io->send_fn(io->ctx, buf, len);
	if (n > 0) return n;
	if (n < 0) return MBEDTLS_ERR_SSL_CONN_EOF;
	return MBEDTLS_ERR_SSL_WANT_WRITE;
}

static int recv_shim(void *p, unsigned char *buf, size_t len) {
	const struct tls_io *io = (const struct tls_io *) p;
	int n = io->recv_fn(io->ctx, buf, len);
	if (n > 0) return n;
	//dprintf("recv_shim n=%d\n", n);
	if (n < 0 && n != MBEDTLS_ERR_SSL_WANT_READ) {
		return MBEDTLS_ERR_SSL_CONN_EOF;
	}
	return MBEDTLS_ERR_SSL_WANT_READ;
}

static void tls_debug(void *ctx, int level, const char *file, int line, const char *str) {
	dprintf("mbedtls:%d: %s", level, str);
}

int tls_client_init(mbedtls_ssl_context *ssl, mbedtls_ssl_config *conf, struct tls_io *io, const char *sni, const char *alpn_csv, const uint8_t *ca_pem, size_t ca_len) {
	static mbedtls_x509_crt ca;
	static int ca_loaded;

	mbedtls_ssl_init(ssl);
	mbedtls_ssl_config_init(conf);
	mbedtls_ssl_conf_dbg(conf, tls_debug, NULL);
	mbedtls_ssl_conf_legacy_renegotiation(conf, MBEDTLS_SSL_LEGACY_NO_RENEGOTIATION);
	static const int tls_ciphersuites[] = {
		MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
		MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
		MBEDTLS_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256,
		MBEDTLS_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
		0 /* terminator */
	};
	mbedtls_ssl_conf_ciphersuites(conf, tls_ciphersuites);

	//mbedtls_debug_set_threshold(4);
	mbedtls_ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_verify(conf, cert_verify_cb, NULL);
	mbedtls_ssl_conf_cert_profile(conf, &mbedtls_x509_crt_profile_default);
	mbedtls_ssl_conf_cert_req_ca_list(conf, 0);

	if (mbedtls_ssl_config_defaults(conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
		dprintf("mbledtls_ssl_config_defaults failed\n");
		return TCP_SSL_CANT_SET_CLIENT_CONFIG;
	}

	mbedtls_ssl_conf_rng(conf, mbedtls_ctr_drbg_random, &drbg);

	if (!ca_loaded && ca_pem) {
		mbedtls_x509_crt_init(&ca);
		if (mbedtls_x509_crt_parse(&ca, ca_pem, ca_len) != 0) {
			dprintf("mbedtls_x509_crt_parse failed\n");
			return TCP_SSL_CANT_LOAD_CA_CERT;
		}
		ca_loaded = 1;
	}
	if (ca_loaded) {
		mbedtls_ssl_conf_ca_chain(conf, &ca, NULL);
	}

	if (alpn_csv) {
		/* build a NULL-terminated array once from csv */
		static const char *alpn_list[] = {"h2", "http/1.1", NULL};
		if (mbedtls_ssl_conf_alpn_protocols(conf, alpn_list) != 0) {
			return TCP_SSL_INVALID_ALPN;
		}
	}

	if (mbedtls_ssl_setup(ssl, conf) != 0) {
		return TCP_SSL_CLIENT_SETUP_FAILED;
	}
	if (sni) {
		dprintf("set sni %s\n", sni);
		if (mbedtls_ssl_set_hostname(ssl, sni) != 0) {
			return TCP_SSL_INVALID_SNI;
		}
	}
	mbedtls_ssl_set_bio(ssl, io, send_shim, recv_shim, NULL);
	return 0;
}

int tls_server_init(mbedtls_ssl_context *ssl, mbedtls_ssl_config *conf, const struct tls_io *io, const uint8_t *cert_pem, size_t cert_len, const uint8_t *key_pem, size_t key_len, const char *alpn_csv) {
	static mbedtls_x509_crt cert;
	static mbedtls_pk_context key;
	static int cred_loaded;

	mbedtls_ssl_init(ssl);
	mbedtls_ssl_config_init(conf);

	mbedtls_ssl_config_defaults(conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
	mbedtls_ssl_conf_rng(conf, mbedtls_ctr_drbg_random, &drbg);
	mbedtls_ssl_conf_dbg(conf, tls_debug, NULL);
	mbedtls_ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_verify(conf, cert_verify_cb, NULL);
	mbedtls_ssl_conf_legacy_renegotiation(conf, MBEDTLS_SSL_LEGACY_NO_RENEGOTIATION);
	static const int tls_ciphersuites[] = {
		MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
		MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
		MBEDTLS_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256,
		MBEDTLS_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
		0 /* terminator */
	};
	mbedtls_ssl_conf_ciphersuites(conf, tls_ciphersuites);

	if (!cred_loaded) {
		mbedtls_x509_crt_init(&cert);
		mbedtls_pk_init(&key);
		if (mbedtls_x509_crt_parse(&cert, cert_pem, cert_len) != 0) {
			return -1;
		}
		if (mbedtls_pk_parse_key(&key, key_pem, key_len, NULL, 0) != 0) {
			return -1;
		}
		cred_loaded = 1;
	}
	mbedtls_ssl_conf_own_cert(conf, &cert, &key);

	if (alpn_csv) {
		static const char *alpn_list[] = {"h2", "http/1.1", NULL};
		mbedtls_ssl_conf_alpn_protocols(conf, alpn_list);
	}

	if (mbedtls_ssl_setup(ssl, conf) != 0) return -1;
	mbedtls_ssl_set_bio(ssl, io, send_shim, recv_shim, NULL);
	return 0;
}

[[maybe_unused]] static const char *hs_state_name(int s) {
	switch (s) {
		case MBEDTLS_SSL_HELLO_REQUEST:
			return "HELLO_REQUEST";
		case MBEDTLS_SSL_CLIENT_HELLO:
			return "CLIENT_HELLO";
		case MBEDTLS_SSL_SERVER_HELLO:
			return "SERVER_HELLO";
		case MBEDTLS_SSL_SERVER_CERTIFICATE:
			return "SERVER_CERT";
		case MBEDTLS_SSL_SERVER_KEY_EXCHANGE:
			return "SERVER_KEY_EXCHANGE";
		case MBEDTLS_SSL_CERTIFICATE_REQUEST:
			return "CERT_REQUEST";
		case MBEDTLS_SSL_SERVER_HELLO_DONE:
			return "SERVER_HELLO_DONE";
		case MBEDTLS_SSL_CLIENT_CERTIFICATE:
			return "CLIENT_CERT";
		case MBEDTLS_SSL_CLIENT_KEY_EXCHANGE:
			return "CLIENT_KEY_EXCHANGE";
		case MBEDTLS_SSL_CERTIFICATE_VERIFY:
			return "CERT_VERIFY";
		case MBEDTLS_SSL_CLIENT_CHANGE_CIPHER_SPEC:
			return "CLIENT_CCS";
		case MBEDTLS_SSL_CLIENT_FINISHED:
			return "CLIENT_FINISHED";
		case MBEDTLS_SSL_SERVER_CHANGE_CIPHER_SPEC:
			return "SERVER_CCS";
		case MBEDTLS_SSL_SERVER_FINISHED:
			return "SERVER_FINISHED";
		case MBEDTLS_SSL_FLUSH_BUFFERS:
			return "FLUSH_BUFFERS";
		case MBEDTLS_SSL_HANDSHAKE_WRAPUP:
			return "WRAPUP";
		case MBEDTLS_SSL_HANDSHAKE_OVER:
			return "OVER";
		default:
			return "UNKNOWN";
	}
}

/* non-blocking handshake driver */
int tls_handshake_step_nb(mbedtls_ssl_context *ssl) {
	dprintf("handshake step\n");
	int rc = mbedtls_ssl_handshake_step(ssl);
	if (rc == 0) {
		/* handshake is finished when state reaches HANDSHAKE_OVER */
		return (ssl->state == MBEDTLS_SSL_HANDSHAKE_OVER) ? 1 : 0;
	}
	if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE || rc == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS || rc == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS) {
		return 0; /* still in progress */
	}
	return rc; /* fatal */
}

/* reads/writes return negative error */
int tls_read_nb(mbedtls_ssl_context *ssl, void *buf, size_t len, int *want) {
	int n = mbedtls_ssl_read(ssl, buf, len);
	if (n >= 0) return n;
	*want = (n == MBEDTLS_ERR_SSL_WANT_READ) ? 1 : (n == MBEDTLS_ERR_SSL_WANT_WRITE) ? 2 : -1;
	if (n != MBEDTLS_ERR_SSL_WANT_READ && n != MBEDTLS_ERR_SSL_WANT_WRITE) {
		char e[128];
		mbedtls_strerror(n, e, sizeof(e));
		dprintf("read error: %s (%d)\n", e, n);
	}
	return n;
}

static int tcp_send_nb(void *ctx, const unsigned char *buf, size_t len) {
	int fd = (int) (uintptr_t) ctx;
	int n = send(fd, buf, (uint32_t) len);

	if (n < 0) {
		return -1;  /* hard fail */
	}
	return n;
}

static int tcp_recv_nb(void *ctx, unsigned char *buf, size_t len) {
	int fd = (int) (uintptr_t) ctx;
	int n = recv(fd, buf, (uint32_t) len, false, 0); // non-blocking

	tcp_idle(); // kick buffer drain
	//dprintf("BIO recv ask=%lu -> rc=%d\n", len, n);

	if (n > 0) {
		return n;
	}
	if (n == 0) {
		return MBEDTLS_ERR_SSL_WANT_READ;  /* no data yet */
	}
	return -1;  /* hard fail */
}

int ssl_connect(uint32_t target_addr, uint16_t target_port, uint16_t source_port, const char *sni, const char *alpn_csv, const uint8_t *ca_pem, size_t ca_len) {
	int fd = connect(target_addr, target_port, source_port, true);
	if (fd < 0) {
		return fd;
	}

	if (!tls_fd_in_range(fd)) {
		return TCP_ERROR_OUT_OF_DESCRIPTORS;
	}

	struct tls_peer *p = kcalloc(1, tls_peer_size());
	if (!p) {
		return TCP_ERROR_OUT_OF_MEMORY;
	}

	p->fd = fd;
	p->io.send_fn = tcp_send_nb;
	p->io.recv_fn = tcp_recv_nb;
	p->io.ctx = (uintptr_t) p->fd;

	if (!tls_attach(fd, p)) {
		dprintf("TLS attach failed\n");
		kfree(p);
		return TCP_ERROR_OUT_OF_DESCRIPTORS;
	}

	int status = tls_client_init(&p->ssl, &p->conf, &p->io, sni, alpn_csv, ca_pem, ca_len);
	if (status != 0) {
		tls_detach(fd);
		kfree(p);
		dprintf("tls_client_init failed\n");
		return status;
	}

	time_t start = get_ticks();
	for (;;) {
		int rv;
		while ((rv = mbedtls_ssl_handshake(&p->ssl)) != 0) {
			if (get_ticks() - start > 6000) {
				tls_detach(fd);
				kfree(p);
				closesocket(fd);
				return TCP_CONNECTION_TIMED_OUT;
			} else if (rv == MBEDTLS_ERR_SSL_WANT_READ) {
				while (!sock_ready_to_read(p->fd)) {
					__asm__ volatile("pause");
				}
				continue;
			} else if (rv != MBEDTLS_ERR_SSL_WANT_WRITE && rv != MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS && rv != MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS) {
				/* Error */
				tls_detach(fd);
				kfree(p);
				closesocket(fd);
				return TCP_ERROR_SSL_FIRST + rv;
			}
			__asm__ volatile("pause");
		}
		// TODO: Copy these into the tls_peer so BASIC can use them
		const char *ver = mbedtls_ssl_get_version(&p->ssl);
		const char *cipher = mbedtls_ssl_get_ciphersuite(&p->ssl);
		dprintf("tls on socket %u: negotiated %s: %s\n", fd, ver ? ver : "unknown", cipher ? cipher : "unknown");
		return fd;
	}
}

int ssl_accept(int listen_fd, const uint8_t *cert_pem, size_t cert_len, const uint8_t *key_pem, size_t key_len, const char *alpn_csv, bool blocking) {
	int fd = tcp_accept(listen_fd);
	if (fd < 0) {
		/* TCP_ERROR_WOULD_BLOCK etc. propagate unchanged */
		return fd;
	}

	if (!tls_fd_in_range(fd)) {
		return TCP_ERROR_OUT_OF_DESCRIPTORS;
	}

	struct tls_peer *p = kcalloc(1, sizeof(*p));
	if (p == NULL) {
		return TCP_ERROR_OUT_OF_DESCRIPTORS;
	}

	p->fd = fd;
	p->io.send_fn = tcp_send_nb;   /* always-enqueue TX */
	p->io.recv_fn = tcp_recv_nb;   /* 0 = WANT_READ */
	p->io.ctx = p->fd;

	if (!tls_attach(fd, p)) {
		kfree(p);
		return TCP_ERROR_OUT_OF_DESCRIPTORS;
	}

	if (tls_server_init(&p->ssl, &p->conf, &p->io, cert_pem, cert_len, key_pem, key_len, alpn_csv) == -1) {
		tls_detach(fd);
		kfree(p);
		return TCP_ERROR_CONNECTION_FAILED;
	}

	if (!blocking) {
		return fd; /* handshake progresses on RX events */
	}

	time_t start = get_ticks();
	for (;;) {
		int step = tls_handshake_step_nb(&p->ssl);
		if (step == 1) {
			return fd; /* TLS established */
		}
		if (step < 0) {
			tls_close_fd(fd); /* tidy up TLS state */
			return TCP_ERROR_SSL_FIRST + step;
		}
		if ((get_ticks() - start) > 6000) {
			tls_close_fd(fd);
			return TCP_CONNECTION_TIMED_OUT;
		}
		__asm__ volatile("pause");
	}
}

bool tls_ready_fd(int fd) {
	struct tls_peer *p;

	p = tls_get(fd);
	if (p == NULL) {
		return false;
	}

	if (mbedtls_ssl_get_bytes_avail(&p->ssl) > 0) {
		dprintf("tls_ready_fd is ready");
		return true;
	}

	return false;
}
