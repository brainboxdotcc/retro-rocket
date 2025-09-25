#include <stddef.h>
#include <kernel.h>
#include <mbedtls/entropy.h>
#include <mbedtls/platform.h>
#include <mbedtls/ctr_drbg.h>
#include "mbedtls/ssl.h"

/* globals */
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context drbg;

#define ENTROPY_POOL_SIZE 256

static volatile unsigned char entropy_pool[ENTROPY_POOL_SIZE];
static volatile size_t entropy_head = 0;

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
	(void)data;
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
	return 0; /* success */
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

struct tls_io {
	int  (*send_fn)(void *ctx, const uint8_t *buf, size_t len);  // >0 bytes or ERR_WOULD_BLOCK
	int  (*recv_fn)(void *ctx, uint8_t *buf, size_t len);        // >0 bytes or ERR_WOULD_BLOCK
	void *ctx;
};

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

