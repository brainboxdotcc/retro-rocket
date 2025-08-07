/**
 * @file basic/datetime.c
 * @brief BASIC time and date functions
 */
#include <kernel.h>

int64_t basic_gethour(struct basic_ctx* ctx)
{
	datetime_t date;
	get_datetime(&date);
	return date.hour;
}

int64_t basic_getminute(struct basic_ctx* ctx)
{
	datetime_t date;
	get_datetime(&date);
	return date.minute;
}

int64_t basic_getsecond(struct basic_ctx* ctx)
{
	datetime_t date;
	get_datetime(&date);
	return date.second;
}

int64_t basic_getepoch(struct basic_ctx* ctx)
{
	return time(NULL);
}

int64_t basic_getmonth(struct basic_ctx* ctx)
{
	datetime_t date;
	get_datetime(&date);
	return date.month;
}

int64_t basic_getday(struct basic_ctx* ctx)
{
	datetime_t date;
	get_datetime(&date);
	return date.day;
}

int64_t basic_getweekday(struct basic_ctx* ctx)
{
	datetime_t date;
	get_datetime(&date);
	return get_weekday_from_date(&date);
}

int64_t basic_getyear(struct basic_ctx* ctx)
{
	datetime_t date;
	get_datetime(&date);
	return (((date.century - 1) * 100) + date.year);
}

int64_t basic_get_day_of_year(struct basic_ctx* ctx)
{
	datetime_t date;
	get_datetime(&date);
	return day_of_year(date.year, date.month, date.day);
}

char* basic_date(struct basic_ctx* ctx)
{
	char time[32];
	datetime_t date;
	get_datetime(&date);
	snprintf(time, 32, "%04d-%02d-%02d", ((date.century - 1) * 100) + date.year, date.month, date.day);
	return gc_strdup(ctx, time);
}

char* basic_time(struct basic_ctx* ctx)
{
	char time[32];
	datetime_t date;
	get_datetime(&date);
	snprintf(time, 32, "%02d:%02d:%02d", date.hour, date.minute, date.second);
	return gc_strdup(ctx, time);
}
