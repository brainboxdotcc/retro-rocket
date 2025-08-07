#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/* Sockets functionality */
void sockwrite_statement(struct basic_ctx* ctx);
char* basic_dns(struct basic_ctx* ctx);
char* basic_netinfo(struct basic_ctx* ctx);
int64_t basic_sockstatus(struct basic_ctx* ctx);
char* basic_insocket(struct basic_ctx* ctx);
void sockclose_statement(struct basic_ctx* ctx);
void connect_statement(struct basic_ctx* ctx);
void sockread_statement(struct basic_ctx* ctx);
