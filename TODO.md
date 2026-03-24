# v2.4 TODO

## FROTHFET

* measure soft fuse response times.
* setup ina226 interrupt on update (override handleConfigCommand in PWMController)
* add printDebug for averages
* add a lastStateChange check to generateUpdate() -> use latest or average depending.

* make sure fast updates are working
* go through and re-adjust the various page sizes for our different viewports
* update yarrboard client if any changes needed - probably for state
* update signalk plugin - same

# LONG TERM:

## BRINEOMATIC

* add stepper power configuration: normal and homing
* add simple HA support to brineomatic?
    * on/off switch to start/stop the watermaker
    * sensors for each output type
    * YAML dashboard

* protocol documentation

* signalk plugins
    * should we move this to the firmware instead?
    * frothfet plugin - use key for paths instead of id
    * signalk sendit plugin - use key for paths instead of id
* remove all onclick calls from the html.
* global cleanup of strcpy, sprintf, etc.
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