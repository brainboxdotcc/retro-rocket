/**
 * @file basic/datetime.h
 * @brief Header for BASIC date/time functions
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief Get the current hour of the day.
 *
 * @param ctx The BASIC context.
 * @return The current hour.
 */
int64_t basic_gethour(struct basic_ctx* ctx);

/**
 * @brief Get the current minute of the hour.
 *
 * @param ctx The BASIC context.
 * @return The current minute.
 */
int64_t basic_getminute(struct basic_ctx* ctx);

/**
 * @brief Get the current second of the minute.
 *
 * @param ctx The BASIC context.
 * @return The current second.
 */
int64_t basic_getsecond(struct basic_ctx* ctx);

/**
 * @brief Get the current epoch time (seconds since January 1, 1970).
 *
 * @param ctx The BASIC context.
 * @return The current epoch time.
 */
int64_t basic_getepoch(struct basic_ctx* ctx);

/**
 * @brief Get the current month of the year.
 *
 * @param ctx The BASIC context.
 * @return The current month.
 */
int64_t basic_getmonth(struct basic_ctx* ctx);

/**
 * @brief Get the current day of the month.
 *
 * @param ctx The BASIC context.
 * @return The current day of the month.
 */
int64_t basic_getday(struct basic_ctx* ctx);

/**
 * @brief Get the current weekday (1 = Sunday, 2 = Monday, ..., 7 = Saturday).
 *
 * @param ctx The BASIC context.
 * @return The current weekday.
 */
int64_t basic_getweekday(struct basic_ctx* ctx);

/**
 * @brief Get the current year.
 *
 * @param ctx The BASIC context.
 * @return The current year.
 */
int64_t basic_getyear(struct basic_ctx* ctx);

/**
 * @brief Get the current day of the year (1 = January 1st, 365 = December 31st).
 *
 * @param ctx The BASIC context.
 * @return The current day of the year.
 */
int64_t basic_get_day_of_year(struct basic_ctx* ctx);

/**
 * @brief Get the current date in the format "YYYY-MM-DD".
 *
 * @param ctx The BASIC context.
 * @return The current date as a string.
 */
char* basic_date(struct basic_ctx* ctx);

/**
 * @brief Get the current time in the format "HH:MM:SS".
 *
 * @param ctx The BASIC context.
 * @return The current time as a string.
 */
char* basic_time(struct basic_ctx* ctx);

/**
 * @brief Set the system's timezone.
 *
 * @param ctx The BASIC context.
 */
void settimezone_statement(struct basic_ctx* ctx);

char* basic_get_upstr(struct basic_ctx* ctx);

int64_t basic_getupsecs(struct basic_ctx* ctx);