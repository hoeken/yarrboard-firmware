# v2.3 TODO

## BRINEOMATIC

* user selectable units
    * add conversion to PPL settings
* check if config panel exists before adding to avoid dupes
* remove old chart code
* add simple HA support to brineomatic?
    * on/off switch to start/stop the watermaker
    * sensors for each output type
    * YAML dashboard

v2.4 TODO

## FROTHFET

* Rev F: add support for INA226 to each channel
    * add support for overcurrent alert interrupt to each channel
* Rev F: add support for LM75 to each channel
    * add to client UI (orange)
    * add overheat trip count to stats page
* sporadic crash when fading and saving to prefs at the same time.
* slow vs fast blow
    * fast checks getLatestReading()
    * slow checks getAverageReading()
* update yarrboard client if any changes needed - probably for state
* update signalk plugin - same
* go through and re-adjust the various page sizes for our different viewports
* protocol documentation
* add buzzer support - enable alarms for tripped or bypassed on a per-channel basis
* how to handle global set_brightness command
* make sure fast updates are working

# LONG TERM:

* UI
    * combine config and settings page into a single mega page
    * dio channels
    * fan channels?
* signalk plugins
    * should we move this to the firmware instead?
    * frothfet plugin - use key for paths instead of id
    * signalk sendit plugin - use key for paths instead of id
* remove all onclick calls from the html.
* global cleanup of strcpy, sprintf, etc.
* espwebtools?  https://esphome.github.io/esp-web-tools/
* yarrboard c++ framework
    * more yarrboard.js functions:
        * app.addPage("page", "content", openCallback);
        * app.onOpenPage(page, callback);
        * many other app.* callbacks to register various things.
    * refactor yarrboard-firmware into:
        * frothfet-firmware
        * brineomatic-firmware
        * sendit-firmware
* other MFD integrations:
    * garmin?
    * raymarine?

# B&G MFD Dev Info
B&G : User Agent: Mozilla/5.0 (X11; Linux aarch64) AppleWebKit/537.36 (KHTML, like Gecko) QtWebEngine/5.12.9 Chrome/69.0.3497.128 Safari/537.36
https://ungoogled-software.github.io/ungoogled-chromium-binaries/releases/linux_portable/64bit/