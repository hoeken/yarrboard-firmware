# Version 2.2.0

## 🛠️ Improvements & Enhancements

- **Per-Channel ADC Configuration**
  - Added alternative constructor to `ADS1115Helper` to support per-channel voltage references and gains
  - Channels can now have different gain settings (e.g., channels 0-1 use ±4.096V, channels 2-3 use ±2.048V)
  - Updated Brineomatic Rev B and Rev C configurations to use new array-based ADC configuration

- Now tracking pickled on timestamp so you can see how long the unit has been pickled for. (Survives reboot)

# Version 2.1.0

## 🚀 New Features

- **General Features**
  - Native USB serial support, no need for serial converter.
  - Nicer debug system, with full debug via web console (click the copyright year)
  - Better piezo support with selectable premade melodies
  - Updated Brineomatic icon + support for adding bookmark to home screen on Apple / Android
  - MQTT for brineomatic now has all data
  - Added flush volume tracking
  - Autoflush now has 3 modes: automatic (salinity), duration, and volume

- **Modbus Support**
  - First pass at GD20 VFD Modbus integration.
  - Added modbus configuration options: `slaveId` and `frequency`.

- **Mid-Level Hardware Control**
  - Added mid-level commands to control hardware.
  - Mid-level UI control implemented.

- **Config System Enhancements**
  - Added `autoflushUseHighPressureMotor` config option.
  - Added `loadConfig()` for JSON-based configuration.
  - Autoflush now respects config values.
  - Hardware initialization now respects configuration.
  - Enforced unique relay IDs on the client side.
  - UI now shows/disables control fields dynamically based on available sensors.
  - Safeguards UI fields enabled/disabled based on hardware.
  - Config form UI is responsive.

## 🛠️ Improvements & Refactoring
- Major refactor of sensors into the main Brineomatic object:
  - Flowmeters
  - TDS
  - Pressure sensors
  - Temperature readings
- Refactored ADS1115 into object structure.
- Adopted ADCHelper interrupt-style ADC reading with averages.
- Removed/cleaned up old PID code.
- Tidied/removed old graph code.
- Updated hardware checks to use control settings (`has*`).
- Channel ID validity now verified in config.
- Added helper text to hardware config form.
- Moved firmware section to top of system UI.
- UI is now more dynamic based on hardware availability.
- Web UI now collects, validates, and sends config data.
- Error checking is now timeout based

## 🐛 Bug Fixes
- Firmware updater works again; fixed firmware path issues.
- Fixed stepper motor issues, including:
  - Degree/angle configuration now respected.
  - Several stepper bugs addressed.
- Fixed ADC alert line pull-up issue.
- Pressures now read zero at idle.
- Fixed B&G MFD display error.
- Fixed copy-paste issues and several small float/int bugs.
- Fixed total time tracking.
- Now properly shows errors when sensors are missing.
- Can't flush without a valve.
- Ensured `saveHardwareConfig` only allowed in **IDLE** mode.
- Server-side and client-side validation improved and now working.
- Ensured channel object calls wrapped in control-mode checks.
- Fixed various UI bugs in new config interface.

## ⚙️ Behavioral Changes
- Modbus initialization only happens when appropriate.
- Autostore now works via NTP.
- Error-enable booleans are now respected.
- Hardware enable booleans now used for sensor logic.
- saveHardwareConfig locked to IDLE state to avoid unsafe actions.

# Version 2.0.0

Massive under the hood rewrite of just about everything.
More stability, better UI, setting the stage for good things to come.

## Frontend

* Split the app into multiple files and classes, with a new BaseChannel system
* Much cleaner UI for editing configurations and settings
* added app hooks for registering message handlers for a more modular system
* Added Improv Wifi provisioning support

## Backend

* Updated channels to have a BaseChannel class with lots of deduplication of code
* Lots of stability fixes
* Added support for FrothFET, Brineomatic, and SendIt
* Added MQTT + Home Assistant support.
* Laid the groundwork for making yarrboard-firmware into a library

# Version 1.1.1

* Minor ui improvements for debugging

# Version 1.1.0

* New crash handler and coredump recording.

# Version 1.0.1

* Better release automation.

# Version 1.0.0

* First 'official' release into the wild.