# Version 2.3.0

## 🚀 New Features

- **Stepper Motor Diagnostics**
  - Added TMC2209 error detection (over-temperature, short circuits, open loads)
  - Automatic motor disable on error with console and web UI notifications

## 🛠️ Improvements & Enhancements

- Enhanced PWM channel fault detection logging with detailed voltage and current readings
- Added optional raw data output to ADCHelper::printDebug() for debugging ADC readings
- Added configuration settings for motor and water temperature sensors (NONE, EXTERNAL, and DS18B20)
- Added TDS calibration offset configuration options for product and brine salinity sensors. (fixes #5)

## 🐛 Bug Fixes

- Failed homing now sets position to zero (fixes #16)
- Added input validation for watermaker settings (fixes #20)
- Pressure / Flow hardware config extra fields now hiding/showing appropriately.

# Version 2.2.0

## 🚀 New Features

- **Hardware Configuration**
  - Hardware settings can now be edited in MANUAL mode
  - Added UI config for has_water_temperature
  - Added brineomatic hardware capabilities hook
  - Added inverted configuration support for relay channels (diverter valve, boost pump, HP pump, flush valve, cooling fan)
  - Added configuration support for diverter valve relays

- **Display & UI Improvements**
  - Show membrane/filter pressure at idle too
  - Title changes for cleaner settings navigation
  - Better date/elapsed formatting for "pickled since" display
  - Now tracking pickled on timestamp (survives reboot) to show how long unit has been pickled

- **Hardware Support**
  - Added FrothFET Rev F target and configuration
  - Added Brineomatic Rev C board target and configuration
  - Added SendIt Rev C configuration and target
  - Added output enable pin support (FrothFet)
  - Added basic INA226 support for FrothFET Rev F
  - Added basic LM75 init to PWM channel
  - Implemented overcurrent ISR
  - Added overheat check

- **Release & Development Tools**
  - Added images for firmware uploader
  - Prepared new release system with automated scripts
  - Pulled in release and coredump scripts

## 🛠️ Improvements & Enhancements

- **YarrboardFramework Updates**
  - Version bump to YarrboardFramework 2.2.0
  - Updated ADC channels to new settings style
  - Refactored code into controllers (ADC, PWM, bus voltage, fans, etc)
  - Controller hooks working with new framework
  - Channel controllers now using new handleConfigCommand call
  - Split CSS between framework and project
  - New gulped file name format (no gz)
  - Now building with new framework gulp system

- **Per-Channel ADC Configuration**
  - Variable ADC gain per-channel
  - Channels can now have different gain settings (e.g., channels 0-1 use ±4.096V, channels 2-3 use ±2.048V)
  - Updated Brineomatic Rev B and Rev C configurations to use new array-based ADC configuration

- **Temperature Sensor Improvements**
  - Switched to OneWireNG library and fixed temperature sensor issues
  - Only show DS18B20 errors, not success messages
  - Made volt/current/temp async
  - Sensors working on Rev C

- **Code Refactoring & Architecture**
  - Lots of refactoring Brineomatic code into controllers
  - Converted multiple components to controller pattern (fans, bus voltage, PWM, ADC)
  - Refactored commands from YarrboardFramework
  - Channel re-arrangement
  - Cleanup of gulp config file
  - Added support for pio symlink to gulp
  - Cleaned up unused files that accumulated
  - Removed redundant code and duplicate ChannelRegistry

- **Hardware Control**
  - Turned down stepper motor current slightly
  - Added global brightness hook to PWM controller
  - Added PWM stats
  - Defaulted to manual HP valve control
  - Set smart defaults (manual, no sensors, etc.)

- **Build & Configuration**
  - Updated PlatformIO with new config/library setup
  - Added protocol context param for FrothFet
  - Updated protocol callbacks for Brineomatic
  - Added set_brightness command example
  - Added support for hardware_url, project_name and url
  - Added firmware manifest URL
  - Added framework version and URLs
  - Multiple logos now supported again

- **UI & Display**
  - Calibration table working again
  - ADC channel UI and editing works again
  - Loop and message stats improvements
  - Added interval timer to stats page
  - Config page now starts/stops update poller on open/close

- **MQTT**
  - Added MQTT hook to Brineomatic controller (fixes #12)
  - addMessageHandler → onMessage refactoring

## 🐛 Bug Fixes

- **Hardware Issues**
  - Fixed missing Advanced Mode content
  - Fixed potential Windows path bug
  - Fixed rename to home error
  - Fixed an issue where submitting hardware edit form too quickly could lose data due to restart
  - Cooling fan relay ID default value fixed (fixes #7)
  - Diverter valve settings now showing correctly (fixes #6)
  - Duplicate relays in dropdown fixed (fixes #8)

- **URL & Configuration**
  - Updated OTA firmware URL and public key
  - Compilation tweaks

## ⚙️ Project Structure

- **Logo Management**
  - Removed OSHW logo

- **Hardware Targets**
  - All targets compiling again
  - Multiple hardware revisions now supported (Brineomatic Rev B/C, FrothFET Rev E/F, SendIt Rev A/C)

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