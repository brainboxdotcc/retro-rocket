/**
 * @file clock.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012–2025
 * @brief Interfaces for accessing and manipulating real-time clock (RTC) values
 */
#pragma once

#include <kernel.h>
#include <stdint.h>

/**
 * @brief POSIX-style time representation, counting seconds since epoch
 */
typedef int64_t time_t;

typedef struct {
	int32_t utc_offset;   // In seconds
	bool is_dst;          // True if DST in effect
	const char* abbrev;   // Timezone abbreviation string (from string table)
} tz_offset_t;

#define TZ_MAGIC "TZif"

// tzfile header struct is 44 bytes for v1
struct tz_header {
	char magic[4];    // 'T','Z','i','f'
	char version[1];  // '2' or '3' or '\0' for v1
	char reserved[15];
	uint32_t ttisgmtcnt;
	uint32_t ttisstdcnt;
	uint32_t leapcnt;
	uint32_t timecnt;
	uint32_t typecnt;
	uint32_t charcnt;
} __attribute__((packed));

/**
 * @brief CMOS I/O port addresses used for RTC access
 */
enum cmos_port_t {
	CMOS_ADDR = 0x70,  ///< CMOS register select port
	CMOS_DATA = 0x71,  ///< CMOS data read/write port
};

/**
 * @brief Structure representing date and time components
 *
 * Note: All values are in binary (not BCD). The `century` field
 * is supplied by CMOS register 0x32 if supported by the BIOS.
 */
typedef struct datetime {
	uint8_t century;  ///< Century (e.g., 20 for year 20xx)
	uint8_t year;     ///< Year within the century (0–99)
	uint8_t month;    ///< Month (1–12)
	uint8_t day;      ///< Day of the month (1–31)
	uint8_t hour;     ///< Hour (0–23)
	uint8_t minute;   ///< Minute (0–59)
	uint8_t second;   ///< Second (0–59)
} datetime_t;

void datetime_from_time_t(time_t timestamp, datetime_t* dt);

int32_t get_local_offset_from_buffer(const uint8_t *tzdata, time_t timestamp);

time_t local_time(time_t timestamp);

/**
 * @brief Retrieve the current date and time as a human-readable string
 *
 * Returns a static buffer containing the current date and time,
 * formatted in the style:
 * `"Weekday, DD Month YYYY HH:MM:SS"`
 *
 * Example: `"Mon, 28 Jul 2025 01:45:12"`
 *
 * @return const char* Pointer to a statically allocated string buffer
 *         (overwritten on subsequent calls).
 */
const char* get_datetime_str();


/**
 * @brief Retrieve the current date and time into a datetime_t structure
 *
 * Accesses the hardware RTC and converts its registers to a structured format.
 *
 * @param dt Pointer to a datetime_t structure to fill with current values
 */
void get_datetime(datetime_t* dt);

/**
 * @brief Initialise the Real-Time Clock (RTC) subsystem
 *
 * Must be called during early boot to configure RTC access and disable periodic interrupts.
 */
void init_realtime_clock();

/**
 * @brief Retrieve the current time in POSIX epoch format
 *
 * @param t If non-null, the time value is also stored at this pointer
 * @return time_t Number of seconds since 00:00:00 UTC on 1 January 1970
 */
time_t time(time_t* t);

/**
 * @brief Calculate the ordinal day of the year for a given date
 *
 * Ported from JavaScript (see: https://stackoverflow.com/a/75032366/3022125)
 * Useful for computing time offsets or determining seasonal positions.
 *
 * @param year  Full year (e.g., 2025)
 * @param month Month (1–12)
 * @param day   Day of the month (1–31)
 * @return uint64_t Day of the year (1–366)
 */
uint64_t day_of_year(uint64_t year, uint8_t month, uint8_t day);

/**
 * @brief Calculate the weekday index from a given date
 *
 * Uses a modified version of Zeller's Congruence with precomputed tables for month codes
 * and century codes (starting from the 18th century). The function accounts for leap years.
 *
 * Weekdays are returned as an integer where:
 * - 0 = Sunday
 * - 1 = Monday
 * - 2 = Tuesday
 * - 3 = Wednesday
 * - 4 = Thursday
 * - 5 = Friday
 * - 6 = Saturday
 *
 * @param dt Pointer to a `datetime_t` structure containing the full date
 * @return int Weekday index (0 = Sunday ... 6 = Saturday)
 */
int get_weekday_from_date(datetime_t* dt);
