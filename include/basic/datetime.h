#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/* Date/time functions */
int64_t basic_gethour(struct basic_ctx* ctx);
int64_t basic_getminute(struct basic_ctx* ctx);
int64_t basic_getsecond(struct basic_ctx* ctx);
int64_t basic_getepoch(struct basic_ctx* ctx);
int64_t basic_getmonth(struct basic_ctx* ctx);
int64_t basic_getday(struct basic_ctx* ctx);
int64_t basic_getweekday(struct basic_ctx* ctx);
int64_t basic_getyear(struct basic_ctx* ctx);
int64_t basic_get_day_of_year(struct basic_ctx* ctx);
char* basic_date(struct basic_ctx* ctx);
char* basic_time(struct basic_ctx* ctx);
void settimezone_statement(struct basic_ctx* ctx);
