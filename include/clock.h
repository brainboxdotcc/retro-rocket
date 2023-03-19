#pragma once

#include <kernel.h>
#include <stdint.h>

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
void clock_init();
