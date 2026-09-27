# fnweather for the Amiga

FujiNet weather for **Workbench 1.3 and later** on any Amiga (68000 upward),
talking to FujiNet through **FujiNet NIO** (`fujinet-nio.device`). It uses
the same services as the Apple2 and MS-DOS ports: Open-Meteo for weather and
geocoding, and ip-api for the starting location.

```text
 FujiNet Weather  Philadelphia, US   Sun 27 Sep 2026  16:15 EDT

     .--.       Overcast
  .-(    ).     Temperature 16.6°C   Feels like 15.0°C
 (___.__)__)    Humidity 90%   Dew point 15.1°C   Clouds 100%
                Pressure 997.6 hPa   Wind 22.2 km/h N
                Sunrise 06:53   Sunset 18:49

 Day        Weather                  High    Low   Precip   UV  Wind
 Today      Heavy rain               17.0   13.9  22.80mm 0.75  32.2 km/h N
 Mon 28     Heavy drizzle            16.9   15.0   1.50mm 1.90  18.7 km/h NW
 ...
 R)efresh  U)nits: metric  L)ocation  Q)uit
```

## Running

**From Workbench:** double-click the `fnweather` icon. Set its Tool Types
(Workbench → Info) to choose a place or units:

| Tool Type | Effect |
| --- | --- |
| `PLACE=London` | Look up this place instead of locating by IP |
| `UNITS=IMPERIAL` | °F, mph and inches (default `METRIC`) |

**From the Shell:**

```text
fnweather                    window; locate by IP
fnweather New York           window; named place
fnweather -t -i Chicago      print one report in the Shell and exit
```

In the window: **R** refresh, **U** toggle units, **L** enter a new place,
**Q** or **Esc** quit. It also refreshes itself after 15 idle minutes.

`fujinet-nio.device` must be resident. If it is not, fnweather loads it with
`fujinet-load-resident`, first from `C:`/`DEVS:` (as installed by
`Install-FujiNet-WB13`), then from the `NIO:` volume.

## Building

Needs bebbo's `m68k-amigaos-gcc` and a
[fujinet-nio-workspace](https://github.com/markjfisher/fujinet-nio-workspace)
checkout:

```sh
make NIO_WORKSPACE=/path/to/fujinet-nio-workspace   # build/fnweather + fnweather.info
make test                                           # host tests (any C compiler)
FNW_LIVE=1 make test                                # ...against the live services
```

It links with libnix's `nix13` runtime, so the same binary runs on
Kickstart 1.3 through 3.x.

## How it differs from the other ports

* **JSON is parsed on the Amiga.** Other ports have FujiNet parse the JSON
  (`network_json_query`); NIO offers no such service on the Amiga, so
  `src/json.c` looks up the same `/key/index` paths itself. The responses
  are 0.3–1.5 KB each.
* **Five requests, not one.** The Amiga build of fujinet-nio-lib caps URLs at
  256 bytes, so the Open-Meteo query is split into requests of at most 237
  bytes (checked by `make test`).
* **http by default.** FujiNet does the TLS, so https would cost the Amiga
  nothing, but the macOS POSIX NIO build currently fails every https fetch.
  Build with `CFLAGS+='-DOM_SCHEME="https://"'` for a FujiNet that handles
  it. ip-api's free tier is http only.
* **No floating point.** Values stay as the strings Open-Meteo sent, so no
  math library is needed on a stock 1.3 system.
* **`snprintf`, never `sprintf`.** With `-lamiga` on the link line,
  `sprintf` resolves to amiga.lib's `RawDoFmt()` wrapper, which has no `%u`
  and reads `%d` as a 16-bit word.

## Files

| Path | What |
| --- | --- |
| `src/main.c` | Arguments, Tool Types, driver loading, the window loop |
| `src/net.c` | HTTP GET through fujinet-nio-lib (`fn_open`/`fn_read`) |
| `src/meteo.c` | Open-Meteo / ip-api requests, zone names, WMO code text |
| `src/json.c` | Minimal JSON path lookup |
| `src/display.c` | RAW: window with ANSI colour, or plain Shell output |
| `src/timeutil.c` | Unix time to local date, integer-only |
| `support/mkicon.py` | Draws `fnweather.info` (sun and cloud, WB1.3 palette) |
| `tests/` | Host tests and saved API responses |
