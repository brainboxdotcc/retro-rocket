#include <kernel.h>

static datetime_t current_datetime;

const char* weekday_map[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* month_map[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void rtc_read_datetime();

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
	const char* monthname = month_map[dt->month];
	static char buffer[256];
	sprintf(
		buffer,
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

void clock_init() {
	rtc_read_datetime();
}
