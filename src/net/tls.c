#include <kernel.h>
#include <mbedtls/entropy.h>
#include <mbedtls/platform.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl.h>
#include <tls.h>

#define ENTROPY_POOL_SIZE 256

static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context drbg;
static volatile unsigned char entropy_pool[ENTROPY_POOL_SIZE];
static volatile size_t entropy_head = 0;

struct tls_peer {
	mbedtls_ssl_context  ssl;
	mbedtls_ssl_config   conf;
	struct tls_io        io;
	int                  fd;
};

size_t tls_peer_size() {
	return sizeof(struct tls_peer);
}

static struct tls_peer **tls_by_fd   = NULL;
static size_t            tls_fd_cap  = 0;

bool tls_fd_table_init(size_t fd_cap) {
	tls_by_fd  = kcalloc(fd_cap, sizeof(*tls_by_fd));
	if (!tls_by_fd) {
		return false;
	}
	tls_fd_cap = fd_cap;
	return true;
}

bool tls_fd_in_range(int fd) {
	return (fd >= 0) && ((size_t)fd < tls_fd_cap);
}

struct tls_peer *tls_get(int fd) {
	return (tls_fd_in_range(fd)) ? tls_by_fd[fd] : NULL;
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

bool tls_read_fd(int fd, void *buf, size_t len, int *want, int *out_n) {
	struct tls_peer *p = tls_get(fd);
	if (!p) {
		return false;
	}
	int n = tls_read_nb(&p->ssl, buf, len, want);
	if (n >= 0) {
		*out_n = n;
		return true;
	}
	return false;
}

bool tls_write_fd(int fd, const void *buf, size_t len, int *want, int *out_n) {
	struct tls_peer *p;
	int n;

	p = tls_get(fd);
	if (p == NULL) {
		return false;
	}

	int rc = mbedtls_ssl_handshake(&p->ssl);
	if (rc != 0) {
		*out_n = 0;

		if (rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
			*want = 2;
			return false;
		}

		if (rc == MBEDTLS_ERR_SSL_WANT_READ) {
			*want = 1;
			return false;
		}

		*want = -1;
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
	volatile unsigned char *p = (volatile unsigned char *)buf;
	while (len > 0) {
		*p++ = 0;
		len--;
	}
}

void entropy_irq_event(void) {
	unsigned long long tsc;
	__asm__ volatile("rdtsc" : "=A"(tsc));
	unsigned char sample = (unsigned char)(tsc & 0xFF);
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

void nonconst_kfree(void* p) {
	kfree(p);
}

mbedtls_time_t mbedtls_platform_time(mbedtls_time_t *tloc) {
	time_t now = time(NULL);
	if (tloc) *tloc = now;
	return now;
}

int tls_global_init(void) {
	mbedtls_platform_set_calloc_free(kcalloc, nonconst_kfree);

	mbedtls_entropy_init(&entropy);
	mbedtls_ctr_drbg_init(&drbg);
	mbedtls_entropy_add_source(&entropy, entropy_poll, NULL, 32, MBEDTLS_ENTROPY_SOURCE_STRONG);

	if (mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)"retro-rocket", 12) != 0) {
		return -1;
	}
	return 0;
}

void tls_global_free(void) {
	mbedtls_ctr_drbg_free(&drbg);
	mbedtls_entropy_free(&entropy);
}

static int send_shim(void *p, const unsigned char *buf, size_t len) {
	const struct tls_io *io = (const struct tls_io*)p;
	int n = io->send_fn(io->ctx, buf, len);
	if (n > 0) return n;
	return MBEDTLS_ERR_SSL_WANT_WRITE;
}

static int recv_shim(void *p, unsigned char *buf, size_t len) {
	const struct tls_io *io = (const struct tls_io*)p;
	int n = io->recv_fn(io->ctx, buf, len);
	if (n > 0) return n;
	return MBEDTLS_ERR_SSL_WANT_READ;
}

int tls_client_init(mbedtls_ssl_context *ssl, mbedtls_ssl_config *conf,
		    const struct tls_io *io,
		    const char *sni,           /* nullable */
		    const char *alpn_csv,      /* e.g. "h2,http/1.1" or NULL */
		    const uint8_t *ca_pem, size_t ca_len)
{
	static mbedtls_x509_crt ca;  /* you can share one global CA chain */
	static int ca_loaded;

	mbedtls_ssl_init(ssl);
	mbedtls_ssl_config_init(conf);

	mbedtls_ssl_config_defaults(conf, MBEDTLS_SSL_IS_CLIENT,
				    MBEDTLS_SSL_TRANSPORT_STREAM,
				    MBEDTLS_SSL_PRESET_DEFAULT);

	mbedtls_ssl_conf_rng(conf, mbedtls_ctr_drbg_random, &drbg);

	if (!ca_loaded && ca_pem) {
		mbedtls_x509_crt_init(&ca);
		if (mbedtls_x509_crt_parse(&ca, ca_pem, ca_len) != 0) return -1;
		ca_loaded = 1;
	}
	if (ca_loaded) mbedtls_ssl_conf_ca_chain(conf, &ca, NULL);

	if (alpn_csv) {
		/* build a NULL-terminated array once from csv */
		static const char *alpn_list[] = {"h2","http/1.1",NULL};
		mbedtls_ssl_conf_alpn_protocols(conf, alpn_list);
	}

	if (mbedtls_ssl_setup(ssl, conf) != 0) return -1;
	if (sni) mbedtls_ssl_set_hostname(ssl, sni);

	mbedtls_ssl_set_bio(ssl, io, send_shim, recv_shim, NULL);
	return 0;
}

int tls_server_init(mbedtls_ssl_context *ssl, mbedtls_ssl_config *conf,
		    const struct tls_io *io,
		    const uint8_t *cert_pem, size_t cert_len,
		    const uint8_t *key_pem,  size_t key_len,
		    const char *alpn_csv)
{
	static mbedtls_x509_crt cert;
	static mbedtls_pk_context key;
	static int cred_loaded;

	mbedtls_ssl_init(ssl);
	mbedtls_ssl_config_init(conf);

	mbedtls_ssl_config_defaults(conf, MBEDTLS_SSL_IS_SERVER,
				    MBEDTLS_SSL_TRANSPORT_STREAM,
				    MBEDTLS_SSL_PRESET_DEFAULT);

	mbedtls_ssl_conf_rng(conf, mbedtls_ctr_drbg_random, &drbg);

	if (!cred_loaded) {
		mbedtls_x509_crt_init(&cert);
		mbedtls_pk_init(&key);
		if (mbedtls_x509_crt_parse(&cert, cert_pem, cert_len) != 0) return -1;
		if (mbedtls_pk_parse_key(&key, key_pem, key_len, NULL, 0) != 0) return -1;
		cred_loaded = 1;
	}
	mbedtls_ssl_conf_own_cert(conf, &cert, &key);

	if (alpn_csv) {
		static const char *alpn_list[] = {"h2","http/1.1",NULL};
		mbedtls_ssl_conf_alpn_protocols(conf, alpn_list);
	}

	if (mbedtls_ssl_setup(ssl, conf) != 0) return -1;
	mbedtls_ssl_set_bio(ssl, io, send_shim, recv_shim, NULL);
	return 0;
}

/* non-blocking handshake driver */
int tls_handshake_step_nb(mbedtls_ssl_context *ssl) {
	int rc = mbedtls_ssl_handshake_step(ssl);
	if (rc == 0) {
		/* handshake is finished when state reaches HANDSHAKE_OVER */
		return (ssl->state == MBEDTLS_SSL_HANDSHAKE_OVER) ? 1 : 0;
	}
	if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
		return 0; /* still in progress */
	}
	return -1; /* fatal */
}

/* reads/writes return -1 with WANT_READ/WANT_WRITE via rc for your scheduler */
int tls_read_nb(mbedtls_ssl_context *ssl, void *buf, size_t len, int *want) {
	int n = mbedtls_ssl_read(ssl, buf, len);
	if (n >= 0) return n;
	*want = (n == MBEDTLS_ERR_SSL_WANT_READ) ? 1 : (n == MBEDTLS_ERR_SSL_WANT_WRITE) ? 2 : -1;
	return -1;
}
int tls_write_nb(mbedtls_ssl_context *ssl, const void *buf, size_t len, int *want) {
	int n = mbedtls_ssl_write(ssl, buf, len);
	if (n >= 0) return n;
	*want = (n == MBEDTLS_ERR_SSL_WANT_WRITE) ? 2 : (n == MBEDTLS_ERR_SSL_WANT_READ) ? 1 : -1;
	return -1;
}

static int tcp_send_nb(void *ctx, const uint8_t *buf, size_t len) {
	int fd = *(int *)ctx;
	int n  = send(fd, buf, (uint32_t)len);
	if (n < 0) return -1;   /* hard fail */
	return (int)len;        /* always enqueue all */
}

static int tcp_recv_nb(void *ctx, uint8_t *buf, size_t len) {
	int fd = *(int *)ctx;
	int n  = recv(fd, buf, (uint32_t)len, false, 0);
	if (n > 0) return n;
	if (n == 0) return 0;   /* would-read */
	return -1;              /* hard fail */
}

int ssl_connect(uint32_t target_addr, uint16_t target_port, uint16_t source_port, bool blocking, const char *sni, const char *alpn_csv, const uint8_t *ca_pem, size_t ca_len) {
	int fd = connect(target_addr, target_port, source_port, blocking);
	if (fd < 0) {
		return fd;
	}

	if (!tls_fd_in_range(fd)) {
		return TCP_ERROR_OUT_OF_DESCRIPTORS;
	}

	struct tls_peer *p = kcalloc(1, tls_peer_size());
	if (!p) {
		return TCP_ERROR_OUT_OF_DESCRIPTORS;
	}

	p->fd        = fd;
	p->io.send_fn = tcp_send_nb;
	p->io.recv_fn = tcp_recv_nb;
	p->io.ctx     = &p->fd;

	if (!tls_attach(fd, p)) {
		kfree(p);
		return TCP_ERROR_OUT_OF_DESCRIPTORS;
	}

	if (!tls_client_init(&p->ssl, &p->conf, &p->io, sni, alpn_csv, ca_pem, ca_len)) {
		tls_detach(fd);
		kfree(p);
		return TCP_ERROR_CONNECTION_FAILED;
	}

	if (!blocking) {
		return fd;
	}

	time_t start = get_ticks();
	for (;;) {
		int step = tls_handshake_step_nb(&p->ssl);
		if (step == 1) {
			return fd;
		}
		if (step < 0)  {
			return TCP_ERROR_CONNECTION_FAILED;
		}
		if (get_ticks() - start > 3000) {
			return TCP_CONNECTION_TIMED_OUT;
		}
		__asm__ volatile("pause");
	}
}

int ssl_accept(int listen_fd, const uint8_t *cert_pem, size_t cert_len, const uint8_t *key_pem,  size_t key_len, const char *alpn_csv, bool blocking) {
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

	p->fd          = fd;
	p->io.send_fn  = tcp_send_nb;   /* always-enqueue TX */
	p->io.recv_fn  = tcp_recv_nb;   /* 0 = WANT_READ */
	p->io.ctx      = &p->fd;

	if (!tls_attach(fd, p)) {
		kfree(p);
		return TCP_ERROR_OUT_OF_DESCRIPTORS;
	}

	if (!tls_server_init(&p->ssl, &p->conf, &p->io,
			     cert_pem, cert_len,
			     key_pem,  key_len,
			     alpn_csv)) {
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
			return TCP_ERROR_CONNECTION_FAILED;
		}
		if ((get_ticks() - start) > 3000) {
			tls_close_fd(fd);
			return TCP_CONNECTION_TIMED_OUT;
		}
		__asm__ volatile("pause");
	}
}

bool tls_ready_fd(int fd)
{
	struct tls_peer *p;
	int rc;

	p = tls_get(fd);
	if (p == NULL) {
		return false;
	}

	/* Progress the handshake; 0 means finished */
	rc = mbedtls_ssl_handshake(&p->ssl);
	if (rc != 0) {
		return false;
	}

	if (mbedtls_ssl_get_bytes_avail(&p->ssl) > 0) {
		return true;
	}

	return false;
}
