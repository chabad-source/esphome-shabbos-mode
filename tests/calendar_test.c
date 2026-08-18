#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../components/shabbos_mode/hebrewcalendar.h"

static hdate from_gregorian(int year, int month, int day, int hour) {
  struct tm value;
  memset(&value, 0, sizeof(value));
  value.tm_year = year - 1900;
  value.tm_mon = month - 1;
  value.tm_mday = day;
  value.tm_hour = hour;
  value.tm_isdst = -1;
  return convertDate(value);
}

int main(void) {
  hdate pesach = from_gregorian(2026, 4, 2, 12);
  assert(pesach.year == 5786);
  assert(pesach.month == 1);
  assert(pesach.day == 15);
  assert(getyomtov(pesach) == PESACH_DAY1);
  assert(isassurbemelachah(pesach));

  hdate second_day = from_gregorian(2026, 4, 3, 12);
  setEY(&second_day, 0);
  assert(getyomtov(second_day) == PESACH_DAY2);
  assert(isassurbemelachah(second_day));
  setEY(&second_day, 1);
  assert(getyomtov(second_day) == CHOL_HAMOED_PESACH_DAY1);
  assert(!isassurbemelachah(second_day));

  hdate friday = from_gregorian(2026, 8, 14, 12);
  assert(friday.wday == 6);
  assert(iscandlelighting(friday) == 1);

  hdate saturday = from_gregorian(2026, 8, 15, 12);
  assert(saturday.wday == 0);
  assert(isassurbemelachah(saturday));

  /* In the diaspora, Pesach 2026 flows from Yom Tov on Friday into Shabbos. */
  hdate pesach_friday = from_gregorian(2026, 4, 3, 12);
  setEY(&pesach_friday, 0);
  assert(pesach_friday.wday == 6);
  assert(getyomtov(pesach_friday) == PESACH_DAY2);
  assert(iscandlelighting(pesach_friday) == 1);
  hdate pesach_shabbos = from_gregorian(2026, 4, 4, 12);
  setEY(&pesach_shabbos, 0);
  assert(pesach_shabbos.wday == 0);
  assert(getyomtov(pesach_shabbos) == CHOL_HAMOED_PESACH_DAY1);
  assert(isassurbemelachah(pesach_shabbos));

  hdate leap_adar = from_gregorian(2024, 3, 24, 12);
  assert(leap_adar.year == 5784);
  assert(leap_adar.leap);
  assert(leap_adar.month == 13);
  assert(leap_adar.day == 14);
  assert(getyomtov(leap_adar) == PURIM);

  puts("calendar regression tests passed");
  return 0;
}
