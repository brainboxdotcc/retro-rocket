/**
 * @file tls.h
 * @brief TLS wrapper for Retro Rocket using mbedTLS.
 *
 * Provides initialisation, configuration, and non-blocking I/O
 * functions for client and server TLS connections.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

struct mbedtls_ssl_context;
struct mbedtls_ssl_config;

/**
 * @brief Non-blocking I/O transport functions for TLS sockets.
 */
struct tls_io {
	/**
	 * @brief Send function.
	 *
	 * @param ctx User-supplied context (e.g. socket).
	 * @param buf Buffer to send.
	 * @param len Number of bytes to send.
	 * @return >0 on success (bytes written),
	 *         <0 on would-block or error.
	 */
	int (*send_fn)(void *ctx, const uint8_t *buf, size_t len);

	/**
	 * @brief Receive function.
	 *
	 * @param ctx User-supplied context (e.g. socket).
	 * @param buf Buffer to fill.
	 * @param len Max bytes to read.
	 * @return >0 on success (bytes read),
	 *         <0 on would-block or error.
	 */
	int (*recv_fn)(void *ctx, uint8_t *buf, size_t len);

	/** @brief User context passed to send_fn/recv_fn. */
	void *ctx;
};

/**
 * @brief Initialise global TLS state.
 *
 * Must be called once before any TLS usage.
 *
 * @return 0 on success, -1 on error.
 */
int tls_global_init(void);

/**
 * @brief Free global TLS state.
 */
void tls_global_free(void);

/**
 * @brief Initialise a TLS client connection.
 *
 * @param ssl   SSL context (must be zeroed or mbedtls_ssl_init called).
 * @param conf  SSL config (must be zeroed or mbedtls_ssl_config_init called).
 * @param io    Transport I/O functions.
 * @param sni   Server Name Indication (nullable).
 * @param alpn_csv CSV string of ALPN protocols, or NULL.
 * @param ca_pem PEM-encoded CA certs.
 * @param ca_len Length of CA PEM buffer.
 * @return 0 on success, -1 on error.
 */
int tls_client_init(mbedtls_ssl_context *ssl, mbedtls_ssl_config *conf, const struct tls_io *io, const char *sni, const char *alpn_csv, const uint8_t *ca_pem, size_t ca_len);

/**
 * @brief Initialise a TLS server connection.
 *
 * @param ssl   SSL context (must be zeroed or mbedtls_ssl_init called).
 * @param conf  SSL config (must be zeroed or mbedtls_ssl_config_init called).
 * @param io    Transport I/O functions.
 * @param cert_pem PEM-encoded server certificate.
 * @param cert_len Length of certificate PEM buffer.
 * @param key_pem PEM-encoded private key.
 * @param key_len Length of key PEM buffer.
 * @param alpn_csv CSV string of ALPN protocols, or NULL.
 * @return 0 on success, -1 on error.
 */
int tls_server_init(mbedtls_ssl_context *ssl, mbedtls_ssl_config *conf, const struct tls_io *io, const uint8_t *cert_pem, size_t cert_len, const uint8_t *key_pem, size_t key_len, const char *alpn_csv);

/**
 * @brief Perform one step of the handshake in non-blocking mode.
 *
 * Call whenever the underlying transport is ready for RX/TX.
 *
 * @param ssl SSL context.
 * @return 1 if handshake complete,
 *         0 if still in progress,
 *        -1 on fatal error.
 */
int tls_handshake_step_nb(mbedtls_ssl_context *ssl);

/**
 * @brief Read from a TLS connection (non-blocking).
 *
 * @param ssl SSL context.
 * @param buf Buffer to fill.
 * @param len Max bytes to read.
 * @param want Set to 1 (want read), 2 (want write), or -1 (fatal).
 * @return Number of bytes read on success,
 *         -1 on would-block or fatal.
 */
int tls_read_nb(mbedtls_ssl_context *ssl, void *buf, size_t len, int *want);

/**
 * @brief Write to a TLS connection (non-blocking).
 *
 * @param ssl SSL context.
 * @param buf Data buffer to send.
 * @param len Number of bytes to send.
 * @param want Set to 2 (want write), 1 (want read), or -1 (fatal).
 * @return Number of bytes written on success,
 *         -1 on would-block or fatal.
 */
int tls_write_nb(mbedtls_ssl_context *ssl, const void *buf, size_t len, int *want);

