# v2.4

* more detailed testing of timing on interrupt alert w/ oscilloscope
    * pwm signal
    * alert signal
    * 24v output
    * mosfet gate
* test:
    * slow blow w/ switch
    * fast blow
    * interrupt

* test with interrupt to see how fast we can make it or if there is any room for improvement.
* motor load
* resistive load
* led load

## BRINEOMATIC

* add stepper power configuration: normal and homing

# LONG TERM:

* update yarrboard client if any changes needed - probably for state
* update signalk plugin - same

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