# v2.6

## Brineomatic

* cooling fan -> manual should show on/off temps in UI
    * make sure it also respects it in code
* flush valve should have a servo option as well with open/close angles
* custom gauges for each state?  idle, running, stopping, pickling, etc?

# LONG TERM:

* fork repo into brineomatic-firmware, frothfet-firmware, and sendit-firmware
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