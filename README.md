[![Shabbos Mode Banner](images/Shabbos%20Mode%20Banner.png)](https://github.com/chabad-source/esphome-shabbos-mode)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](http://makeapullrequest.com)
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/donate/?hosted_button_id=Q9A7HG8NQEJRU)

# Shabbos Mode ESPHome External Component

An ESPHome external component that exposes a binary sensor which is `ON` during Shabbos or Yom Tov.

Use the binary sensor's `on_press` automation for the start of Shabbos/Yom Tov, and `on_release` for the end.

## Quick Start

```yaml
external_components:
  - source: github://chabad-source/esphome-shabbos-mode@stable
    components: [shabbos_mode]

time:
  - platform: homeassistant
    id: ha_time

binary_sensor:
  - platform: shabbos_mode
    id: shabbos_active
    name: "Shabbos / Yom Tov Active"
    icon: mdi:candelabra
    time_id: ha_time
    latitude: 40.66896
    longitude: -73.94284
    elevation: 34
    in_israel: false
    start_degree: 0.0
    start_offset_minutes: -18
    end_degree: 8.5
    end_offset_minutes: 0

    shabbos:
      id: shabbos_only_active
      name: "Shabbos Only Active"
      icon: mdi:candles
      on_press:
        - logger.log: "Shabbos started"
      on_release:
        - logger.log: "Shabbos ended"

    yom_tov:
      id: yom_tov_only_active
      name: "Yom Tov Only Active"
      icon: mdi:calendar-star
      on_press:
        - logger.log: "Yom Tov started"
      on_release:
        - logger.log: "Yom Tov ended"

    on_press:
      - logger.log: "Shabbos or Yom Tov started"
    on_release:
      - logger.log: "Shabbos and Yom Tov are both over"
```

For local development, replace the GitHub source with:

```yaml
external_components:
  - source:
      type: local
      path: /config/esphome/esphome-shabbos-mode
    components: [shabbos_mode]
```

`stable` is a moving tag that points at the latest recommended release. For exact reproducibility, pin an immutable version tag such as `v0.3.0`. For development builds, use `github://chabad-source/esphome-shabbos-mode@main`.

## Main Options

- `latitude` and `longitude`: required location coordinates.
- `elevation`: optional elevation in meters, default `0`.
- `in_israel`: controls one-day vs two-day Yom Tov handling, default `false`.
- `start_degree`: degrees below the geometric horizon for the start boundary, default `0`.
- `start_offset_minutes`: minutes added to the start time, default `-18`.
- `end_degree`: degrees below the geometric horizon for the end boundary, default `8.5`.
- `end_offset_minutes`: minutes added to the end time, default `0`.
- `shabbos`: optional child binary sensor that is `ON` only for Shabbos.
- `yom_tov`: optional child binary sensor that is `ON` only for melacha-prohibited Yom Tov days.

`start_degree: 0` means sunset. A common setup is candle lighting at 18 minutes before sunset, with Shabbos/Yom Tov ending at 8.5 degrees.

The original platform sensor remains the combined Shabbos-or-Yom-Tov state. Although `shabbos` and `yom_tov` are nested in its YAML configuration, ESPHome registers them as separate binary-sensor entities. Each supports its own ID, filters, `on_press`, `on_release`, and other standard binary-sensor options.

When Shabbos and Yom Tov overlap, both optional child sensors are `ON`. When one flows directly into the other, each child changes at the configured boundary while the combined sensor remains continuously `ON`.

## Early Take-In

Early take-in is optional and plag-based. When enabled, the component will never start earlier than the selected plag hamincha opinion.

```yaml
binary_sensor:
  - platform: shabbos_mode
    # ...
    early_take_in:
      time: "18:30"
      offset_minutes: 0
      plag_opinion: baal_hatanya
      applies_to_yom_tov: false
      from: "05-01"
      to: "09-15"
```

Early take-in options:

- `time`: optional local time in `HH:MM`. If omitted, the requested early time is plag.
- `offset_minutes`: optional minutes added to the requested early time, default `0`.
- `plag_opinion`: `baal_hatanya`, `gra`, or `mga`, default `baal_hatanya`.
- `applies_to_yom_tov`: when `true`, also applies on Erev Yom Tov, default `false`.
- `from` and `to`: optional Gregorian range in `MM-DD`. If omitted, the rule is eligible all year.

Effective early start:

```text
final_early_start = max(plag, requested_time_or_plag + offset_minutes)
```

The component then compares that early start against the normal start and uses whichever is earlier. Early take-in applies only to first-night starts. It does not apply to second-night Yom Tov starts, which begin at nightfall.

## Web Server Runtime Controls

ESPHome YAML is the startup default, but this component also supports runtime-editable companion entities for the ESPHome `web_server`. Runtime edits apply immediately and are persisted across reboot. Writes are debounced before saving to flash.

Persisted runtime values take precedence over YAML after reboot. Add the reset button below to restore every runtime value to the defaults compiled from the current YAML configuration.

The compact setup below keeps the web UI smaller by combining related fields into text entries:

- `location`: `latitude,longitude`, for example `40.66896,-73.94284`
- `early_take_in_time`: `HH:MM`, or blank to use plag
- `early_take_in_range`: `5/1-9/15`, or blank to allow all year

```yaml
web_server:

binary_sensor:
  - platform: shabbos_mode
    id: shabbos_active
    name: "Shabbos / Yom Tov Active"
    icon: mdi:candelabra
    time_id: ha_time
    latitude: 40.66896
    longitude: -73.94284
    elevation: 34
    in_israel: false
    start_degree: 0.0
    start_offset_minutes: -18
    end_degree: 8.5
    end_offset_minutes: 0

text:
  - platform: shabbos_mode
    name: "Shabbos Location"
    icon: mdi:map-marker
    shabbos_mode_id: shabbos_active
    type: location
    mode: text
  - platform: shabbos_mode
    name: "Early Take-In Time"
    icon: mdi:clock-start
    shabbos_mode_id: shabbos_active
    type: early_take_in_time
    mode: text
  - platform: shabbos_mode
    name: "Early Take-In Range"
    icon: mdi:calendar-range
    shabbos_mode_id: shabbos_active
    type: early_take_in_range
    mode: text

number:
  - platform: shabbos_mode
    name: "Shabbos Start Offset"
    icon: mdi:timer-minus
    shabbos_mode_id: shabbos_active
    type: start_offset_minutes
    mode: box
  - platform: shabbos_mode
    name: "Shabbos End Degree"
    icon: mdi:weather-sunset-down
    shabbos_mode_id: shabbos_active
    type: end_degree
    mode: box
  - platform: shabbos_mode
    name: "Shabbos End Offset"
    icon: mdi:timer-plus
    shabbos_mode_id: shabbos_active
    type: end_offset_minutes
    mode: box
  - platform: shabbos_mode
    name: "Early Take-In Offset"
    icon: mdi:timer-cog
    shabbos_mode_id: shabbos_active
    type: early_take_in_offset_minutes
    mode: box

switch:
  - platform: shabbos_mode
    name: "In Israel"
    icon: mdi:map
    shabbos_mode_id: shabbos_active
    type: in_israel
  - platform: shabbos_mode
    name: "Early Take-In Enabled"
    icon: mdi:clock-check
    shabbos_mode_id: shabbos_active
    type: early_take_in_enabled
  - platform: shabbos_mode
    name: "Early Take-In Applies To Yom Tov"
    icon: mdi:calendar-star
    shabbos_mode_id: shabbos_active
    type: early_take_in_applies_to_yom_tov

select:
  - platform: shabbos_mode
    name: "Early Take-In Plag Opinion"
    icon: mdi:book-open-variant
    shabbos_mode_id: shabbos_active

button:
  - platform: shabbos_mode
    name: "Restore Shabbos YAML Defaults"
    icon: mdi:restore
    shabbos_mode_id: shabbos_active

text_sensor:
  - platform: shabbos_mode
    name: "Next Shabbos Mode Turn On"
    icon: mdi:calendar-clock
    shabbos_mode_id: shabbos_active
    type: next_turn_on
  - platform: shabbos_mode
    name: "Next Shabbos Mode Turn Off"
    icon: mdi:calendar-clock
    shabbos_mode_id: shabbos_active
    type: next_turn_off
  - platform: shabbos_mode
    name: "Current Hebrew Date"
    icon: mdi:calendar-today
    shabbos_mode_id: shabbos_active
    type: current_hebrew_date
```

If a text value is invalid, the component keeps the previous value and logs a warning to the ESPHome logs.

The `early_take_in_range` text field accepts `5/1-9/15`. The older `05-01..09-15` format is still accepted for backward compatibility. YAML `early_take_in.from` and `early_take_in.to` should use `MM-DD`, such as `05-01`.

## Companion Entity Types

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

Available `text` types:

- `location`
- `early_take_in_time`
- `early_take_in_range`

Available `switch` types:

- `in_israel`
- `early_take_in_enabled`
- `early_take_in_applies_to_yom_tov`

The `select` companion entity controls plag opinion with the web-friendly labels `Baal HaTanya`, `Gra`, and `MGA`.

Available `text_sensor` types:

- `next_turn_on`: next actual state transition to `ON`
- `next_turn_off`: next actual state transition to `OFF`
- `current_hebrew_date`

The transition text sensors use a friendly local format like `Fri, Apr 23, 6:32 PM`. Future events use the timezone offset that applies on the event date, including daylight-saving changes. The Hebrew date sensor is formatted like `23 Nissan 5786` and advances at local geometric sunset.

The optional `button` companion restores all persisted runtime settings to the values compiled from YAML and immediately refreshes the binary and text sensors.

## Common Configs

Standard Brooklyn-style defaults:

```yaml
start_degree: 0.0
start_offset_minutes: -18
end_degree: 8.5
end_offset_minutes: 0
```

Israel:

```yaml
in_israel: true
```

Summer early Shabbos only:

```yaml
early_take_in:
  plag_opinion: baal_hatanya
  applies_to_yom_tov: false
  from: "05-01"
  to: "09-15"
```

Summer early Shabbos and Erev Yom Tov, with a requested time that will be clamped to plag if too early:

```yaml
early_take_in:
  time: "18:30"
  offset_minutes: 0
  plag_opinion: baal_hatanya
  applies_to_yom_tov: true
  from: "05-01"
  to: "09-15"
```

## Advanced: Set Values From Lambda

You can change the main component directly from an ESPHome `lambda`, for example in `on_boot`, a button press, or another automation.

After changing values, call `update()` so the binary sensor recalculates immediately. If lambda-based changes should persist across reboot, call `save_runtime_settings()`.

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
          id(shabbos_active).set_early_take_in_plag_opinion("Baal HaTanya");
          id(shabbos_active).set_early_take_in_time(18, 30);
          id(shabbos_active).set_early_take_in_offset_minutes(0);
          id(shabbos_active).set_early_take_in_for_yom_tov(false);
          id(shabbos_active).set_early_take_in_from(5, 1);
          id(shabbos_active).set_early_take_in_to(9, 15);

          id(shabbos_active).save_runtime_settings();
          id(shabbos_active).update();
```

To discard persisted runtime changes and restore the current YAML defaults from a lambda, call:

```cpp
id(shabbos_active).reset_runtime_settings();
```

Direct setters clamp unsafe values to the same ranges used by the web controls.

## Notes And Assumptions

- This component is a scheduling helper, not a halachic ruling. Confirm zmanim, offsets, degree choices, early Shabbos practice, and Yom Tov handling with your rav or local minhag.
- `start_degree: 0` is geometric sunset. The default end degree of `8.5` uses elevation adjustment.
- The binary sensor turns on for Shabbos and Yom Tov only. Chanukah candle-lighting dates do not activate it.
- For second-day Yom Tov transitions, the component uses the end settings at nightfall so the active period stays continuous.
- Early take-in is plag-based. A requested time or negative offset will never move the start earlier than the selected plag opinion.
- Calendar and astronomical calculations are vendored from `yparitcher/libzmanim`.
- The default `update_interval` is `30s`, so binary-sensor automations can run up to approximately 30 seconds after a calculated boundary. Set a shorter `update_interval` on the binary sensor if needed.

## Development And Testing

The repository includes calendar regression tests and an ESPHome host-build fixture. CI runs both for pushes and pull requests.

```bash
cc -std=c11 -Wall -Wextra -Werror \
  tests/calendar_test.c components/shabbos_mode/hebrewcalendar.c \
  -lm -o /tmp/calendar_test
/tmp/calendar_test

esphome config tests/fixtures/basic.yaml
esphome compile tests/fixtures/basic.yaml
```

## Repository Layout

```text
components/
  shabbos_mode/
    __init__.py
    binary_sensor.py
    button.py
    number.py
    select.py
    shabbos_mode_binary_sensor.cpp
    shabbos_mode_binary_sensor.h
    shabbos_mode_controls.cpp
    shabbos_mode_controls.h
    switch.py
    text.py
    text_sensor.py
    hebrewcalendar.c
    hebrewcalendar.h
    NOAAcalculator.c
    NOAAcalculator.h
```

## License

This ESPHome component wrapper is licensed under PolyForm Noncommercial 1.0.0. See `LICENSE`.

Vendored `libzmanim` files remain under their original LGPL license. See `LICENSE.libzmanim` and `THIRD_PARTY_NOTICES.md`.

## Contribute

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/donate/?hosted_button_id=Q9A7HG8NQEJRU) - or - [!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/rebbepod)
