#include <kernel.h>

#define IS_LEAP(y) ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))

static datetime_t current_datetime = { 0 };
static unsigned char* tzdata = NULL;
static time_t system_boot = 0;

const char* weekday_map[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* month_map[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void rtc_read_datetime();
int get_weekday_from_date(datetime_t* dt);

void get_datetime(datetime_t* dt) {
	rtc_read_datetime();
	memcpy(dt, &current_datetime, sizeof(datetime_t));
}

bool is_updating_rtc() {
	outb(CMOS_ADDR, 0x0A);
	return (inb(CMOS_DATA) & 0x80);
}

uint8_t get_rtc_register(int reg_num) {
	outb(CMOS_ADDR, reg_num);
	return inb(CMOS_DATA);
}

void set_rtc_register(uint16_t reg_num, uint8_t val) {
	outb(CMOS_ADDR, reg_num);
	outb(CMOS_DATA, val);
}

uint64_t day_of_year(uint64_t year, uint8_t month, uint8_t day) {
	return (275 * month / 9) - (((month + 9) / 12) * (1 + (year - 4 * (year / 4) + 2) / 3)) + day - 30;
}

void datetime_from_time_t(time_t timestamp, datetime_t* dt) {
	if (!dt) {
		return;
	}

	// Break into days and seconds
	int64_t days = timestamp / 86400;
	int64_t secs_of_day = timestamp % 86400;

	// Extract time
	dt->hour = secs_of_day / 3600;
	dt->minute = (secs_of_day % 3600) / 60;
	dt->second = secs_of_day % 60;

	// Start from epoch year
	int64_t year = 1970;

	// Leap year helpers
	const int days_in_month[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

	while (1) {
		int64_t days_this_year = IS_LEAP(year) ? 366 : 365;
		if (days < days_this_year)
			break;
		days -= days_this_year;
		year++;
	}

	// Now determine month and day
	int64_t month = 1;
	while (1) {
		int dim = days_in_month[month - 1];
		if (month == 2 && IS_LEAP(year))
			dim++;

		if (days < dim)
			break;

		days -= dim;
		month++;
	}

	// Final assignments
	dt->year = year % 100;
	dt->century = year / 100;
	dt->month = month;
	dt->day = days + 1;
}


time_t time(time_t* t) {
	rtc_read_datetime();
	get_weekday_from_date(&current_datetime); // to update century

	uint64_t real_year = (((current_datetime.century - 1) * 100) + current_datetime.year);
	uint64_t tm_year = real_year - 1900;

	time_t  epoch = current_datetime.second + current_datetime.minute * 60 + current_datetime.hour * 3600 +
		(day_of_year(real_year, current_datetime.month, current_datetime.day) - 1) * 86400 +
		(tm_year - 70) * 31536000 + ((tm_year - 69) / 4) * 86400 -
		((tm_year - 1) / 100) * 86400 + ((tm_year + 299) / 400) * 86400;

	if (t) {
		/* If pointer is valid, fill it */
		*t = epoch;
	}
	/* Always return the value regardless */
	add_random_entropy((uint64_t)epoch);
	return epoch;
}

void rtc_read_datetime() {
	while(is_updating_rtc());

	current_datetime.second = get_rtc_register(0x00);
	current_datetime.minute = get_rtc_register(0x02);
	current_datetime.hour = get_rtc_register(0x04);
	current_datetime.day = get_rtc_register(0x07);
	current_datetime.month = get_rtc_register(0x08);
	current_datetime.year = get_rtc_register(0x09);

	if (!(get_rtc_register(0x0B) & 0x04)) {
		current_datetime.second = (current_datetime.second & 0x0F) + ((current_datetime.second / 16) * 10);
		current_datetime.minute = (current_datetime.minute & 0x0F) + ((current_datetime.minute / 16) * 10);
		current_datetime.hour = ((current_datetime.hour & 0x0F) + (((current_datetime.hour & 0x70) / 16) * 10)) | (current_datetime.hour & 0x80);
		current_datetime.day = (current_datetime.day & 0x0F) + ((current_datetime.day / 16) * 10);
		current_datetime.month = (current_datetime.month & 0x0F) + ((current_datetime.month / 16) * 10);
		current_datetime.year = (current_datetime.year & 0x0F) + ((current_datetime.year / 16) * 10);
	}
}

int is_leap_year(int year, int month) {
	if (year % 4 == 0 && (month == 1 || month == 2)) {
		return 1;
	};
	return 0;
}

uint64_t uptime_secs() {
	return time(NULL) - system_boot;
}

/*
 * Given a date, calculate it's weekday, using the algorithm described here:
 * http://blog.artofmemory.com/how-to-calculate-the-day-of-the-week-4203.html
 */
int get_weekday_from_date(datetime_t* dt) {
	char month_code_array[] = {0x0, 0x3, 0x3, 0x6, 0x1, 0x4, 0x6, 0x2, 0x5, 0x0, 0x3, 0x5};
	char century_code_array[] = {0x4, 0x2, 0x0, 0x6, 0x4, 0x2, 0x0};	// Starting from 18 century
	dt->century = 21;
	int year_code = (dt->year + (dt->year / 4)) % 7;
	int month_code = month_code_array[dt->month - 1];
	int century_code = century_code_array[dt->century - 1 - 17];
	int leap_year_code = is_leap_year(dt->year, dt->month);
	return (year_code + month_code + century_code + dt->day - leap_year_code) % 7;
}

const char* datetime_to_str(datetime_t* dt) {
	const char* weekday = weekday_map[get_weekday_from_date(dt)];
	const char* monthname = month_map[dt->month - 1];
	static char buffer[256];
	snprintf(
		buffer,
		256,
		"%s, %02d %s %02d%02d %02d:%02d:%02d",
		weekday,
		dt->day,
		monthname,
		dt->century - 1,
		dt->year,
		dt->hour,
		dt->minute,
		dt->second
	);
	return buffer;
}

const char* get_datetime_str() {
	rtc_read_datetime();
	return datetime_to_str(&current_datetime);
}

void init_realtime_clock() {
	rtc_read_datetime();
	system_boot = time(NULL);
	kprintf("System boot time (UTC): %s\n", get_datetime_str());
}

bool load_timezone(const char* timezone) {
	char path[1024];
	snprintf(path, sizeof(path), "/system/timezones/%s", timezone);
	void* new_tzdata = NULL;

	fs_directory_entry_t* fsi = fs_get_file_info(path);
	if (!fsi || fsi->flags & FS_DIRECTORY) {
		return false;
	}

	new_tzdata = kmalloc(fsi->size + 1);
	if (!new_tzdata) {
		return false;
	}

	bool ok = fs_read_file(fsi, 0, fsi->size, new_tzdata);
	if (!ok) {
		kfree_null(&new_tzdata);
		return false;
	}

	kfree_null(&tzdata);
	tzdata = new_tzdata;
	return true;
}

time_t local_time(time_t timestamp) {
	if (tzdata == NULL) {
		return timestamp;
	}
	return timestamp + get_local_offset_from_buffer(tzdata, timestamp);
}

static inline int32_t ntohl_signed(const uint8_t *p) {
	return (int32_t)(
		((uint32_t)p[0] << 24) |
		((uint32_t)p[1] << 16) |
		((uint32_t)p[2] << 8) |
		((uint32_t)p[3]));
}

int32_t get_local_offset_from_buffer(const uint8_t *tzdata, time_t timestamp) {
	if (!tzdata) {
		return 0;
	}
	const struct tz_header *hdr = (const struct tz_header *) tzdata;
	if (memcmp(hdr->magic, "TZif", 4) != 0) {
		return 0;
	}

	uint32_t timecnt = ntohl(hdr->timecnt);
	uint32_t typecnt = ntohl(hdr->typecnt);

	// Offsets (after the 44-byte header)
	const uint8_t *trans_times = tzdata + 44;
	const uint8_t *trans_types = trans_times + timecnt * 4;
	const uint8_t *ttinfos = trans_types + timecnt;

	uint8_t type_index = 0;
	const int32_t *trans_times_32 = (const int32_t *)trans_times;

	// Find latest applicable transition
	for (uint32_t i = 0; i < timecnt; i++) {
		int32_t t = ntohl_signed((uint8_t*)&trans_times_32[i]);
		if (timestamp >= t) {
			type_index = trans_types[i];
		} else {
			break;
		}
	}

	if (type_index >= typecnt) {
		return 0;
	}

	int32_t offset = ntohl_signed(&ttinfos[type_index * 6]);
	return offset;
}
