\page SETTIMEZONE SETTIMEZONE Keyword
```basic
SETTIMEZONE string-expression
```

Sets the **system time zone** to the value provided by `string-expression`.  
The string must be a valid **IANA time zone** (tzdb) name, for example:

- "Europe/London"
- "America/New_York"
- "Asia/Tokyo"

After setting the time zone, functions that are **DST-aware** will use it when you pass `TRUE` to them, such as:

- `TIME$(TRUE)`
- `DATE$(TRUE)`
- `HOUR(TRUE)`, `MINUTE(TRUE)`, `SECOND(TRUE)`
- `DAY(TRUE)`, `MONTH(TRUE)`, `YEAR(TRUE)`

When called **with `TRUE`**, these functions apply the selected time zone and any **daylight saving time** rules in effect for the timestamp they produce.


> The identifier must be an **IANA tzdb name** (e.g. "Europe/London").  
> Do **not** use Windows-style names like "GMT Standard Time" or abbreviations like "BST" or "PST".


> Calling the date and time functions **without** the `TRUE` argument returns values **without** applying the configured time zone and DST adjustment.


> If you are unsure of the exact spelling, consult the IANA tz database list and copy the region/city form verbatim, e.g. "America/Los_Angeles".

---

### Examples

**Set to the UK and print local date and time**
```basic
SETTIMEZONE "Europe/London"

PRINT "Local date: "; DATE$(TRUE)
PRINT "Local time: "; TIME$(TRUE)
PRINT "Hour: "; HOUR(TRUE); "  Minute: "; MINUTE(TRUE); "  Second: "; SECOND(TRUE)
```

**Compare adjusted versus unadjusted outputs**
```basic
SETTIMEZONE "America/New_York"

PRINT "Unadjusted time: "; TIME$()
PRINT "Adjusted time:   "; TIME$(TRUE)
```

**Switch to Tokyo and read parts**
```basic
SETTIMEZONE "Asia/Tokyo"

PRINT "Y/M/D: "; YEAR(TRUE); "/"; MONTH(TRUE); "/"; DAY(TRUE)
```

**Handle an invalid time zone**
```basic
ON ERROR PROCbadtz
SETTIMEZONE "Not/AZone"
PRINT "This line is not reached if the name is invalid"
END

DEF PROCbadtz
    PRINT "Failed to set time zone: "; ERR$
ENDPROC
```

---

### Behaviour

- The setting affects the **DST-aware** forms of the listed functions when you pass `TRUE`.
- The argument to `SETTIMEZONE` is a **string expression** and must resolve to a valid tzdb identifier.
- An **invalid** identifier raises a runtime error (catch with `ON ERROR PROC...` if needed).

**See also:**  
[`TIME$`](https://github.com/brainboxdotcc/retro-rocket/wiki/TIME) ·
[`DATE$`](https://github.com/brainboxdotcc/retro-rocket/wiki/DATE) ·
[`HOUR`](https://github.com/brainboxdotcc/retro-rocket/wiki/HOUR) ·
[`MINUTE`](https://github.com/brainboxdotcc/retro-rocket/wiki/MINUTE) ·
[`SECOND`](https://github.com/brainboxdotcc/retro-rocket/wiki/SECOND) ·
[`DAY`](https://github.com/brainboxdotcc/retro-rocket/wiki/DAY) ·
[`MONTH`](https://github.com/brainboxdotcc/retro-rocket/wiki/MONTH) ·
[`YEAR`](https://github.com/brainboxdotcc/retro-rocket/wiki/YEAR)
