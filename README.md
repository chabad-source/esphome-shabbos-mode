# Shabbos Mode ESPHome External Component

This repository contains an ESPHome external component that exposes a binary sensor which is `ON` during Shabbos or Yom Tov.

Use the binary sensor's built-in `on_press` action for the start of Shabbos/Yom Tov, and `on_release` for the end.

## What it supports

- `start_degree`: degrees below the horizon for the start boundary
- `start_offset_minutes`: minutes added after the start degree calculation, or subtracted when negative
- `end_degree`: degrees below the horizon for the end boundary
- `end_offset_minutes`: minutes added after the end degree calculation, or subtracted when negative
- `in_israel`: controls one-day vs two-day Yom Tov handling
- `early_take_in`: optional plag-based early take-in settings

`start_degree: 0` means sunset. A common setup is `start_degree: 0` with `start_offset_minutes: -18`, and `end_degree: 8.5` with `end_offset_minutes: 0`.

For early take-in, the component uses `plag hamincha` as the earliest valid start.

- `early_take_in.time`: optional local time in `HH:MM`
- `early_take_in.offset_minutes`: optional minutes added to the requested early time
- `early_take_in.plag_opinion`: `baal_hatanya`, `gra`, or `mga`
- `early_take_in.applies_to_yom_tov`: `true` to also allow early starts on Erev Yom Tov
- `early_take_in.from`: optional Gregorian start date in `MM-DD`
- `early_take_in.to`: optional Gregorian end date in `MM-DD`

Defaults:

- if `time` is omitted, the requested early time defaults to `plag`
- if `offset_minutes` is omitted, it defaults to `0`
- if `plag_opinion` is omitted, it defaults to `baal_hatanya`
- if `applies_to_yom_tov` is omitted, it defaults to `false`
- if `from` and `to` are omitted, early take-in is eligible all year

How the early take-in time is chosen:

- start with the requested time, or `plag` if no time was provided
- apply `offset_minutes`
- never allow the result to be earlier than `plag`
- only use the early-take-in result if it is earlier than the normal calculated start

This means the effective early start is:

```text
final_early_start = max(plag, requested_time_or_plag + offset)
```

Then the component compares that against the normal start and uses whichever is earlier.

This feature does not apply to second-night Yom Tov starts that begin at nightfall.

## Example

```yaml
external_components:
  - source:
      type: local
      path: /config/esphome/shabbos_mode
    components: [shabbos_mode]

time:
  - platform: homeassistant
    id: ha_time

binary_sensor:
  - platform: shabbos_mode
    name: "Shabbos / Yom Tov Active"
    time_id: ha_time
    latitude: 40.66896
    longitude: -73.94284
    elevation: 34
    in_israel: false
    start_degree: 0.0
    start_offset_minutes: -18
    end_degree: 8.5
    end_offset_minutes: 0
    early_take_in:
      time: "18:30"
      offset_minutes: 0
      plag_opinion: baal_hatanya
      applies_to_yom_tov: false
      from: "05-01"
      to: "09-15"
    on_press:
      - logger.log: "Shabbos or Yom Tov started"
    on_release:
      - logger.log: "Shabbos or Yom Tov ended"
```

## Web Server Runtime Controls

ESPHome YAML is still compile-time configuration, but this component also supports runtime-editable companion entities that the ESPHome `web_server` can expose.

Add `number`, `switch`, and `select` entities with `platform: shabbos_mode` and point them at the main binary sensor with `shabbos_mode_id`.

```yaml
web_server:

binary_sensor:
  - platform: shabbos_mode
    id: shabbos_active
    name: "Shabbos / Yom Tov Active"
    time_id: ha_time
    latitude: 40.66896
    longitude: -73.94284
    elevation: 34
    in_israel: false
    start_degree: 0.0
    start_offset_minutes: -18
    end_degree: 8.5
    end_offset_minutes: 0
    early_take_in:
      plag_opinion: baal_hatanya

number:
  - platform: shabbos_mode
    name: "Shabbos Latitude"
    shabbos_mode_id: shabbos_active
    type: latitude
    mode: box
  - platform: shabbos_mode
    name: "Shabbos Longitude"
    shabbos_mode_id: shabbos_active
    type: longitude
    mode: box
  - platform: shabbos_mode
    name: "Shabbos Start Offset"
    shabbos_mode_id: shabbos_active
    type: start_offset_minutes
    mode: box
  - platform: shabbos_mode
    name: "Shabbos End Degree"
    shabbos_mode_id: shabbos_active
    type: end_degree
    mode: box
  - platform: shabbos_mode
    name: "Early Take-In Hour"
    shabbos_mode_id: shabbos_active
    type: early_take_in_hour
    mode: box
  - platform: shabbos_mode
    name: "Early Take-In Minute"
    shabbos_mode_id: shabbos_active
    type: early_take_in_minute
    mode: box
  - platform: shabbos_mode
    name: "Early Take-In From Month"
    shabbos_mode_id: shabbos_active
    type: early_take_in_from_month
    mode: box
  - platform: shabbos_mode
    name: "Early Take-In From Day"
    shabbos_mode_id: shabbos_active
    type: early_take_in_from_day
    mode: box
  - platform: shabbos_mode
    name: "Early Take-In To Month"
    shabbos_mode_id: shabbos_active
    type: early_take_in_to_month
    mode: box
  - platform: shabbos_mode
    name: "Early Take-In To Day"
    shabbos_mode_id: shabbos_active
    type: early_take_in_to_day
    mode: box

switch:
  - platform: shabbos_mode
    name: "In Israel"
    shabbos_mode_id: shabbos_active
    type: in_israel
  - platform: shabbos_mode
    name: "Early Take-In Enabled"
    shabbos_mode_id: shabbos_active
    type: early_take_in_enabled
  - platform: shabbos_mode
    name: "Early Take-In Applies To Yom Tov"
    shabbos_mode_id: shabbos_active
    type: early_take_in_applies_to_yom_tov

select:
  - platform: shabbos_mode
    name: "Early Take-In Plag Opinion"
    shabbos_mode_id: shabbos_active

text_sensor:
  - platform: shabbos_mode
    name: "Next Shabbos Mode Turn On"
    shabbos_mode_id: shabbos_active
    type: next_turn_on
  - platform: shabbos_mode
    name: "Next Shabbos Mode Turn Off"
    shabbos_mode_id: shabbos_active
    type: next_turn_off
  - platform: shabbos_mode
    name: "Current Hebrew Date"
    shabbos_mode_id: shabbos_active
    type: current_hebrew_date
```

Available `number` types:

- `latitude`
- `longitude`
- `elevation`
- `start_degree`
- `start_offset_minutes`
- `end_degree`
- `end_offset_minutes`
- `early_take_in_hour`
- `early_take_in_minute`
- `early_take_in_offset_minutes`
- `early_take_in_from_month`
- `early_take_in_from_day`
- `early_take_in_to_month`
- `early_take_in_to_day`

Available `switch` types:

- `in_israel`
- `early_take_in_enabled`
- `early_take_in_applies_to_yom_tov`

The `select` companion entity controls `plag_opinion` with:

- `Baal HaTanya`
- `Gra`
- `MGA`

Available `text_sensor` types:

- `next_turn_on`
- `next_turn_off`
- `current_hebrew_date`

Text sensor formats:

- `next_turn_on` and `next_turn_off` default to a friendly local format like `Fri, Apr 23, 6:32 PM`
- `current_hebrew_date` is formatted like `23 Nissan 5786`

These web-editable companion entities change the running device state immediately and now persist across reboot. Once you edit a runtime control from the web UI, the restored runtime value takes precedence over the YAML startup default on future boots.

## Advanced: Set Values From Lambda

You can also change the main component directly from an ESPHome `lambda`, for example in `on_boot`, a button press, or another automation.

After changing values, call `update()` so the binary sensor recalculates immediately instead of waiting for the next poll interval.

If you want lambda-based changes to also persist across reboot, call `save_runtime_settings()` after setting the new values.

```yaml
esphome:
  on_boot:
    priority: -10
    then:
      - lambda: |-
          id(shabbos_active).set_latitude(40.66896);
          id(shabbos_active).set_longitude(-73.94284);
          id(shabbos_active).set_elevation(34);
          id(shabbos_active).set_in_israel(false);
          id(shabbos_active).set_start_degree(0.0);
          id(shabbos_active).set_start_offset_minutes(-18);
          id(shabbos_active).set_end_degree(8.5);
          id(shabbos_active).set_end_offset_minutes(0);

          id(shabbos_active).set_early_take_in_enabled(true);
          id(shabbos_active).set_early_take_in_plag_opinion("baal_hatanya");
          id(shabbos_active).set_early_take_in_time(18, 30);
          id(shabbos_active).set_early_take_in_offset_minutes(0);
          id(shabbos_active).set_early_take_in_for_yom_tov(false);
          id(shabbos_active).set_early_take_in_from(5, 1);
          id(shabbos_active).set_early_take_in_to(9, 15);

          id(shabbos_active).save_runtime_settings();
          id(shabbos_active).update();
```

Available direct setters:

- `set_latitude(double latitude)`
- `set_longitude(double longitude)`
- `set_elevation(double elevation)`
- `set_in_israel(bool in_israel)`
- `set_start_degree(double start_degree)`
- `set_start_offset_minutes(int minutes)`
- `set_end_degree(double end_degree)`
- `set_end_offset_minutes(int minutes)`
- `set_early_take_in_enabled(bool enabled)`
- `set_early_take_in_plag_opinion(const std::string &opinion)` with `baal_hatanya`, `gra`, `mga`, `Baal HaTanya`, `Gra`, or `MGA`
- `set_early_take_in_time(int hour, int minute)`
- `set_early_take_in_offset_minutes(int minutes)`
- `set_early_take_in_for_yom_tov(bool enabled)`
- `set_early_take_in_from(int month, int day)`
- `set_early_take_in_to(int month, int day)`

If you are using the web-server companion entities, lambda-based changes and web edits both operate on the same in-memory runtime state.

## Repository layout

To use this as a git-based external component, keep the repository layout like this:

```text
components/
  shabbos_mode/
    __init__.py
    binary_sensor.py
    number.py
    select.py
    shabbos_mode_binary_sensor.cpp
    shabbos_mode_binary_sensor.h
    shabbos_mode_controls.cpp
    shabbos_mode_controls.h
    switch.py
    text_sensor.py
    hebrewcalendar.c
    hebrewcalendar.h
    NOAAcalculator.c
    NOAAcalculator.h
```

## Notes

- The binary sensor turns on for Shabbos and Yom Tov only. Chanukah candle-lighting dates do not activate it.
- For second-day Yom Tov transitions, the component uses the end settings at nightfall so the active period stays continuous.
- Early take-in is plag-based. A requested time or negative offset will never move the start earlier than the selected plag opinion.
- The calendar and astronomical calculations are vendored from `yparitcher/libzmanim`.
