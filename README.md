Hi,

Just to give back to the wonderfull ESPHome community, I share my Waveshare 3.7" display driver. Below AI generated desciption to explain a bit more. 

My first ever upload to Github, so if something is not right, please be easy on me :)

Enjoy!

Sagios

# ESPhomeWaveshare37

Custom ESPHome external component for the Waveshare 3.7" e-paper display (280x480, black/white).

This repository is shared as-is. The initial driver was generated with Claude AI, then adjusted until it worked reliably with the included YAML example.

## Status

- Works well for the provided `waveshare37.yaml` setup.
- The thermostat example is a real-world demo, but **not a complete reusable package** yet.
- Treat this as a community/experimental driver: validate behavior on your own hardware before production use.

## What is included

- `components/waveshare37_epaper/`:
	- custom ESPHome display platform (`waveshare37_epaper`)
	- C++ e-paper driver implementation
- `waveshare37.yaml`:
	- example dashboard-style thermostat display for Home Assistant data
	- minute-based display refresh and nightly repeated full refresh

## Quick start

### 1) Add the external component

Use this repository as an ESPHome external component:

```yaml
external_components:
	- source: github://SagiosNL/ESPhomeWaveshare37
		components: [waveshare37_epaper]
```

Or keep a local checkout and point to a local path (as in the example YAML).

### 2) Configure the display

Example (same pinout as `waveshare37.yaml`):

```yaml
display:
	- platform: waveshare37_epaper
		id: epaper
		clk_pin: 18
		mosi_pin: 23
		cs_pin: 5
		dc_pin: 17
		reset_pin: 16
		busy_pin: 4
		rotation: 90°
		update_interval: never
		full_update_every: 0
		lambda: |-
			it.print(10, 10, id(your_font), "Hello e-paper");
```

`full_update_every` controls forced full refresh cycles:
- `0` = only first update is full, following updates are partial
- `N > 0` = do a full update every `N` calls

### 3) Trigger updates explicitly

The example disables automatic display updates and updates on schedule:

```yaml
time:
	- platform: homeassistant
		on_time:
			- seconds: 0
				then:
					- component.update: epaper
```

This pattern is useful for e-paper to reduce unnecessary refreshes.

## Notes about the included thermostat YAML

- Depends on Home Assistant entities that you will need to rename.
- Uses custom fonts from a local `fonts/` folder; provide your own font files.
- It is intended as a reference layout/example, not a plug-and-play package.

## Known limitations / caveats

- Bit-banged SPI is used (CLK + MOSI + CS), so refresh speed is limited.
- Busy wait timeout is fixed in the driver; very slow panels may need tuning.
- Partial update quality/ghosting can vary by panel batch and temperature.
- No guarantee of compatibility with every 3.7" Waveshare hardware revision.

## Development

Local test flow:

1. Clone this repository.
2. Keep your device YAML next to `components/` (or reference the local path).
3. Build/upload with ESPHome as usual.

If you improve stability, panel compatibility, or cleanup the example config, PRs are welcome.
