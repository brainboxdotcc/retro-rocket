/**
 * @file basic/datetime.c
 * @brief BASIC time and date functions
 */
#include <kernel.h>

void local_or_not(bool local, datetime_t* date)
{
	if (local) {
		datetime_from_time_t(local_time(time(NULL)), date);
	} else {
		get_datetime(date);
	}
}

int64_t basic_gethour(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	bool local = intval;
	PARAMS_END("HOUR",0);
	datetime_t date;
	local_or_not(local, &date);
	return date.hour;
}

int64_t basic_getminute(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	bool local = intval;
	PARAMS_END("MINUTE",0);
	datetime_t date;
	local_or_not(local, &date);
	return date.minute;
}

int64_t basic_getsecond(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	bool local = intval;
	PARAMS_END("SECOND",0);
	datetime_t date;
	local_or_not(local, &date);
	return date.second;
}

int64_t basic_getepoch(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	bool local = intval;
	PARAMS_END("EPOCH",0);
	return local ? local_time(time(NULL)) : time(NULL);
}

int64_t basic_getmonth(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	bool local = intval;
	PARAMS_END("MONTH",0);
	datetime_t date;
	local_or_not(local, &date);
	return date.month;
}

int64_t basic_getday(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	bool local = intval;
	PARAMS_END("DAY",0);
	datetime_t date;
	local_or_not(local, &date);
	return date.day;
}

int64_t basic_getweekday(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	bool local = intval;
	PARAMS_END("WEEKDAY",0);
	datetime_t date;
	local_or_not(local, &date);
	return get_weekday_from_date(&date);
}

int64_t basic_getyear(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	bool local = intval;
	PARAMS_END("YEAR",0);
	datetime_t date;
	local_or_not(local, &date);
	return (((date.century - 1) * 100) + date.year);
}

int64_t basic_get_day_of_year(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	bool local = intval;
	PARAMS_END("YDAY",0);
	datetime_t date;
	local_or_not(local, &date);
	return day_of_year(((date.century - 1) * 100) + date.year, date.month, date.day);
}

char* basic_date(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	bool local = intval;
	PARAMS_END("DATE$","");
	char tm[32];
	datetime_t date;
	local_or_not(local, &date);
	snprintf(tm, 32, "%04d-%02d-%02d", ((date.century - 1) * 100) + date.year, date.month, date.day);
	return gc_strdup(ctx, tm);
}

char* basic_time(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	bool local = intval;
	PARAMS_END("TIME$","");
	char tm[32];
	datetime_t date;
	local_or_not(local, &date);
	snprintf(tm, 32, "%02d:%02d:%02d", date.hour, date.minute, date.second);
	return gc_strdup(ctx, tm);
}

void settimezone_statement(struct basic_ctx* ctx)
{
	accept_or_return(SETTIMEZONE, ctx);
	const char* tz = str_expr(ctx);
	accept_or_return(NEWLINE, ctx);

	if (!load_timezone(tz)) {
		tokenizer_error_printf(ctx, "Failed to load timezone '%s'", tz);
	}
}
