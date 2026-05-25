# Shabbos Mode ESPHome External Component

This repository contains an ESPHome external component that exposes a binary sensor which is `ON` during Shabbos or Yom Tov.

Use the binary sensor's built-in `on_press` action for the start of Shabbos/Yom Tov, and `on_release` for the end.

## What it supports

- `start_degree`: degrees below the horizon for the start boundary
- `start_offset_minutes`: minutes added after the start degree calculation, or subtracted when negative
- `end_degree`: degrees below the horizon for the end boundary
- `end_offset_minutes`: minutes added after the end degree calculation, or subtracted when negative
- `in_israel`: controls one-day vs two-day Yom Tov handling
- `early_take_in`: optional wall-clock time for taking in Shabbos or Yom Tov early

`start_degree: 0` means sunset. A common setup is `start_degree: 0` with `start_offset_minutes: -18`, and `end_degree: 8.5` with `end_offset_minutes: 0`.

For early take-in, add:

- `early_take_in.time`: local time in `HH:MM`
- `early_take_in.applies_to: shabbos` for Fridays only
- `early_take_in.applies_to: shabbos_and_yom_tov` for both Fridays and Erev Yom Tov

The early time only overrides the regular start when it is earlier than the normal calculated start. It does not apply to second-night Yom Tov starts that begin at nightfall.

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
      applies_to: shabbos
    on_press:
      - logger.log: "Shabbos or Yom Tov started"
    on_release:
      - logger.log: "Shabbos or Yom Tov ended"
```

## Repository layout

To use this as a git-based external component, keep the repository layout like this:

```text
components/
  shabbos_mode/
    __init__.py
    binary_sensor.py
    shabbos_mode_binary_sensor.cpp
    shabbos_mode_binary_sensor.h
    hebrewcalendar.c
    hebrewcalendar.h
    NOAAcalculator.c
    NOAAcalculator.h
```

## Notes

- The binary sensor turns on for Shabbos and Yom Tov only. Chanukah candle-lighting dates do not activate it.
- For second-day Yom Tov transitions, the component uses the end settings at nightfall so the active period stays continuous.
- The calendar and astronomical calculations are vendored from `yparitcher/libzmanim`.
