#pragma once

#include <kernel.h>
#include <stdint.h>

typedef uint64_t time_t;

/**
 * @brief CMOS IO port addresses
 */
enum cmos_port_t {
	CMOS_ADDR = 0x70,
	CMOS_DATA = 0x71,
};

/**
 * @brief A representation of date and time
 */
typedef struct datetime {
	uint8_t century;
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} datetime_t;

/**
 * @brief Get the current date and time as a string
 * @return const char* date and time
 */
const char* get_datetime_str();

/**
 * @brief Get the current date and time as a datetime_t
 * @param dt date and time to fill
 */
void get_datetime(datetime_t* dt);

/**
 * @brief Initialise the realtime clock
 */
void init_realtime_clock();

/**
 * @brief POSIX time function returning time_t
 * 
 * @param t Pointer to copy time value into if non-null
 * @return time_t Current time
 */
time_t time(time_t* t);

/**
 * @brief Calculate the day of year given any particular year, month an day
 * Ported from js: https://stackoverflow.com/a/75032366/3022125
 * 
 * @param year year number
 * @param month month number, 1-12
 * @param day day number, 1-31
 * @return uint64_t days
 */
uint64_t day_of_year(uint64_t year, uint8_t month, uint8_t day);