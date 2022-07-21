## file.h

DateTimeRec:

```c
struct DateTimeRec {
  short year;
  short month;
  short day;
  short hour;
  short minute;
  short second;
  short dayOfWeek;
};
typedef struct DateTimeRec DateTimeRec;
```

From: https://opensource.apple.com/source/CarbonHeaders/CarbonHeaders-9A581/DateTimeUtils.h.auto.html