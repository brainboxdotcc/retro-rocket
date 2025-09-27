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

typedef struct mbedtls_ssl_context mbedtls_ssl_context;
typedef struct mbedtls_ssl_config mbedtls_ssl_config;

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
int tls_client_init(mbedtls_ssl_context *ssl, mbedtls_ssl_config *conf, struct tls_io *io, const char *sni, const char *alpn_csv, const uint8_t *ca_pem, size_t ca_len);

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

/**
 * @brief Initialise TLS file descriptor table.
 *
 * Call once during tls_global_init(), after the maximum FD count is known.
 *
 * @param fd_cap Maximum number of descriptors to support.
 * @return true on success, false on allocation failure.
 */
bool tls_fd_table_init(size_t fd_cap);

/**
 * @brief Remove TLS association from a file descriptor.
 *
 * Clears the TLS pointer slot but does not free underlying TCP resources.
 *
 * @param fd Descriptor to detach.
 */
void tls_detach(int fd);

/**
 * @brief Perform a non-blocking TLS read on a socket.
 *
 * @param fd     File descriptor to read from.
 * @param buf    Destination buffer.
 * @param len    Maximum number of bytes to read.
 * @param want   Output: 1 = WANT_READ, 2 = WANT_WRITE, -1 = fatal.
 * @param out_n  Output: number of bytes read on success.
 * @return true if data was read, false if WANT_* or fatal.
 */
bool tls_read_fd(int fd, void *buf, size_t len, int *want, int *out_n, int *err);

/**
 * @brief Perform a non-blocking TLS write on a socket.
 *
 * @param fd     File descriptor to write to.
 * @param buf    Source buffer.
 * @param len    Number of bytes to write.
 * @param want   Output: 1 = WANT_READ, 2 = WANT_WRITE, -1 = fatal.
 * @param out_n  Output: number of bytes written on success.
 * @return true if data was written, false if WANT_* or fatal.
 */
bool tls_write_fd(int fd, const void *buf, size_t len, int *want, int *out_n);

/**
 * @brief Close a TLS session and free associated state.
 *
 * Sends close_notify, frees mbedTLS contexts, and detaches from the fd.
 *
 * @param fd Descriptor to close.
 * @return true if TLS state existed and was freed, false if not attached.
 */
bool tls_close_fd(int fd);

/**
 * @brief Check if a descriptor is within TLS table bounds.
 *
 * @param fd Descriptor to check.
 * @return true if fd is within configured table range, false otherwise.
 */
bool tls_fd_in_range(int fd);

/**
 * @brief Connect to a remote host and negotiate a TLS session.
 *
 * Wraps tcp_connect() and performs TLS client initialisation.
 *
 * @param target_addr IPv4 address of remote host (network order).
 * @param target_port TCP port to connect to.
 * @param source_port Local source port, or 0 for ephemeral.
 * @param blocking    If true, blocks until handshake completes or timeout.
 * @param sni         Optional SNI hostname (nullable).
 * @param alpn_csv    Optional ALPN CSV list (nullable).
 * @param ca_pem      Trusted CA certificates (PEM).
 * @param ca_len      Length of ca_pem buffer.
 * @return fd of connected TLS socket, or negative TCP_ERROR_* code.
 */
int ssl_connect(uint32_t target_addr, uint16_t target_port, uint16_t source_port, bool blocking, const char *sni, const char *alpn_csv, const uint8_t *ca_pem, size_t ca_len);

/**
 * @brief Accept an inbound TCP connection and upgrade to TLS.
 *
 * Wraps tcp_accept() and performs TLS server initialisation.
 *
 * @param listen_fd Listening socket fd.
 * @param cert_pem  Server certificate in PEM format.
 * @param cert_len  Length of cert_pem buffer.
 * @param key_pem   Server private key in PEM format.
 * @param key_len   Length of key_pem buffer.
 * @param alpn_csv  Optional ALPN CSV list (nullable).
 * @param blocking  If true, blocks until handshake completes or timeout.
 * @return fd of accepted TLS socket, or negative TCP_ERROR_* code.
 */
int ssl_accept(int listen_fd, const uint8_t *cert_pem, size_t cert_len, const uint8_t *key_pem,  size_t key_len, const char *alpn_csv, bool blocking);

/**
 * @brief Get the TLS structure associated with the fd, or null if not TLS
 * @param fd Socket descriptor
 * @return opaque pointer, or NULL if not TLS
 */
struct tls_peer *tls_get(int fd);

bool tls_ready_fd(int fd);

const char* tls_error_get(int err, char* buffer, size_t size);