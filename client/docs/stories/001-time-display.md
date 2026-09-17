# Den Clock — Time Display Story

Status: Planned; no client implementation exists in this repository.
Human acceptance is required before marking this story complete.

This preserves the original client requirements as planned behavior, not an
implemented contract. The server owns the [current API contract](../../../server/docs/contract.md);
the API example below is illustrative. See [client status](../../STATUS.md)
for prerequisites and unresolved choices.

## 1. Purpose

initial time-display story establishes the basic Den Clock client architecture and provides a functional, attractive time display.

The purpose of this revision is to prove:

- communication with the Den Clock Time Server;
- synchronization to server-provided time;
- independent advancement of time between server synchronizations;
- correct handling of the server's civil-time offset;
- basic loss-of-server behavior;
- the browser-based display architecture;
- the visual foundation for the eventual clock and weather display.

Weather information is explicitly outside the scope of initial time-display story.

---

## 2. Target Platform

The production client will run in a Chromium/Edge-class browser on the Den Clock NUC.

The target display is:

- ROADOM 10.1-inch IPS touchscreen;
- 1024 × 600 pixels;
- full-screen/kiosk presentation.

During development, the client may run on another computer and communicate with the Den Clock Time Server over the LAN. Client development therefore does not require deployment to the NUC for each development cycle.

---

## 3. Client Technology

initial time-display story will use plain browser technologies:

- HTML;
- CSS;
- JavaScript.

No application framework or JavaScript build system is required.

The initial source organization will be approximately:

```text
client/
    index.html
    clock.css
    clock.js
    config.js
```

The server address shall be configurable so that the same client can operate:

- against the NUC over the LAN during development; or
- against localhost when deployed on the NUC.

---

## 4. Time Server API

The Den Clock Time Server provides:

```text
GET /api/time
```

The server currently listens on TCP port 5000.

A representative response is:

```json
{
  "utcTime": "2026-09-17T12:50:03.3784745+00:00",
  "unixTimeMilliseconds": 1789649403378,
  "timeZoneId": "Eastern Standard Time",
  "utcOffsetSeconds": -14400
}
```

### Required fields

#### `unixTimeMilliseconds`

Integer UTC Unix epoch time in milliseconds.

This is the authoritative time value used by the initial time-display story client.

#### `utcOffsetSeconds`

Integer offset in seconds between UTC and the civil time that the clock is to display.

For example:

```text
-14400 = UTC - 4 hours
```

The client shall use this value rather than deriving the UTC offset from the browser or host operating system.

### Informational fields

#### `utcTime`

ISO-8601 representation of the server's UTC time.

This is informational and may be useful for diagnostics.

#### `timeZoneId`

The timezone identifier configured on the server.

For example:

```text
Eastern Standard Time
```

initial time-display story does not need to interpret the timezone identifier.

---

## 5. Time Authority

The Den Clock Time Server is authoritative.

The displayed civil time shall be derived solely from:

1. server-provided UTC time;
2. server-provided UTC offset;
3. elapsed monotonic time since synchronization.

The browser or host computer's wall clock shall **not** determine the displayed time.

The browser or host computer's configured timezone shall **not** determine the displayed timezone or UTC offset.

Conceptually:

```text
displayed time =
    server UTC time
    + server UTC offset
    + monotonic elapsed time since synchronization
```

This requirement allows the client to behave identically whether it is running on the NUC, a development computer, or another future display platform.

---

## 6. Timezone and DST Responsibility

The server owns timezone interpretation and daylight-saving-time behavior.

The client does not need a timezone database or DST rules.

When a timezone transition changes the applicable UTC offset, a subsequent synchronization will provide a new `utcOffsetSeconds` value.

The client shall then use the new offset.

---

## 7. Initial Synchronization

On startup, the client shall request `/api/time`.

Until a valid response has been received, the client shall **not display a plausible but unauthoritative time obtained from the host computer**.

An unsynchronized indication such as:

```text
--:--
```

may be displayed instead.

After the first successful synchronization, normal clock operation begins.

---

## 8. Time Advancement

The client shall not request server time for every displayed second.

After synchronization, the client shall advance time locally using a monotonic elapsed-time source.

The displayed time is therefore based upon:

```text
server time at synchronization
        +
monotonic elapsed time
```

rather than repeated reads of the host operating system's wall clock.

This protects the display from host clock corrections or other changes to the computer's local civil-time configuration.

---

## 9. Server Resynchronization

The client shall periodically resynchronize with the server.

For initial time-display story, the initial resynchronization interval will be approximately:

```text
60 seconds
```

This value may be changed later based on observed behavior.

Server requests are therefore synchronization events rather than the mechanism used to advance every displayed second.

---

## 10. Network Transit Compensation

The client should make a simple estimate of network transit time for each synchronization.

Conceptually:

```text
t0 = monotonic time immediately before request

        request sent
             |
             v

        server timestamp

             |
             v
        response received

t1 = monotonic time immediately after response
```

Estimated server time at receipt is:

```text
server timestamp + (t1 - t0) / 2
```

This assumes approximately symmetric request and response latency.

On the local LAN the correction will normally be very small, but implementing it avoids introducing a systematic synchronization error unnecessarily.

---

## 11. Loss of Server Communication

Failure of a synchronization request shall not immediately stop the clock.

After at least one successful synchronization, the client shall continue advancing time using the most recently established server time and the monotonic elapsed-time source.

The client shall retain enough state to determine how long it has been since the last successful server synchronization.

Loss of server communication shall eventually be indicated unobtrusively so that a clock whose synchronization state is questionable is not silently presented as fully authoritative.

The exact visual treatment and timeout threshold for this indication are not specified in initial time-display story and may be determined during development.

---

## 12. Time Display

The normal initial time-display story display contains only:

- hours and minutes;
- seconds;
- full date.

Conceptually:

```text
                 8:58  32

          Thursday, September 17
```

### Time format

The clock shall use:

- 12-hour time;
- no leading zero on the hour;
- no AM/PM indication;
- prominently displayed hours and minutes;
- visually subordinate seconds.

### Date format

The date shall include:

- full weekday;
- full month name;
- day of month.

For example:

```text
Thursday, September 17
```

---

## 13. Visual Design

initial time-display story should look like a purpose-built clock rather than a webpage or desktop application.

The initial visual direction is:

- dark background;
- high-contrast primary time display;
- restrained secondary text;
- large, highly legible typography;
- generous negative space;
- no visible browser chrome in production.

Exact fonts, colors, sizes, spacing, and positions are intentionally **not part of the initial time-display story functional contract**.

These should be tuned while viewing the client on the actual 1024 × 600 display.

---

## 14. User Interface Exclusions

The normal initial time-display story display shall not contain:

- weather information;
- menus;
- settings controls;
- buttons;
- status bars;
- logos;
- development information;
- debugging information.

Synchronization problems may be represented by a small visual indication as described above, but the display should remain visually dominated by the clock.

---

## 15. Client Architecture

Time synchronization and presentation shall be logically separated.

Conceptually:

```text
                 /api/time
                     |
                     v
              +--------------+
              |  TimeClient  |
              +--------------+
                     |
              authoritative
              display time
                     |
                     v
              +--------------+
              | ClockDisplay |
              +--------------+
                     |
                     v
                    DOM
```

### TimeClient responsibility

`TimeClient` is responsible for:

- server communication;
- initial synchronization;
- periodic resynchronization;
- round-trip latency estimation;
- monotonic time advancement;
- server UTC offset;
- synchronization state;
- detection of communication failure.

It shall not be concerned with DOM layout or visual formatting.

### ClockDisplay responsibility

`ClockDisplay` is responsible for:

- formatting hours and minutes;
- formatting seconds;
- formatting the date;
- updating DOM elements;
- presentation of synchronization state when required.

It shall not communicate directly with the server or determine authoritative time.

---

## 16. Acceptance Criteria

initial time-display story is successful when:

1. The client can run on a development computer and obtain time from the NUC Time Server over the LAN.
2. The displayed time is derived from `unixTimeMilliseconds` and `utcOffsetSeconds`.
3. The host computer's wall clock and timezone do not determine displayed time.
4. The clock advances smoothly between server requests.
5. The client periodically resynchronizes without disrupting normal display.
6. Temporary loss of the server does not stop an already synchronized clock.
7. Startup without server synchronization does not display an unauthoritative host-derived time.
8. The client presents a clean full-screen time-and-date display suitable for the 1024 × 600 Den Clock display.
9. The implementation provides a clean foundation for adding weather information in a later revision.

---

## 17. Explicitly Deferred

The following are deferred to later stories:

- weather data and presentation;
- indoor/outdoor sensor data;
- LoRa sensor integration;
- configuration UI;
- touch interaction;
- user-selectable display modes;
- additional status information;
- final production visual design;
- production kiosk startup and recovery behavior.

initial time-display story is deliberately limited to establishing a reliable, server-authoritative clock client.
