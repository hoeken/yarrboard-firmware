# Version 2.5.0

## Brineomatic

### 🚀 New Features

- **Battery Level Monitoring**
  - Added backend support for battery level sensing
  - Configurable sensor type for battery level (NONE, EXTERNAL, MQTT)
  - Battery Low safeguard check added
  - API now accepts battery_level and tank_level values > 1.0

- **Flush Valve Servo Control**
  - Backend and frontend support for controlling the flush valve via servo
  - Flush valve servo configurable in hardware settings

- **Customizable Gauge Dashboard**
  - Added UI for re-ordering, removing, and adding gauges in MANUAL mode
  - Backend support for gauge order and visibility customization

- **MQTT Expansions**
  - Added MQTT support for motor_temperature and water_temperature
  - Added MQTT support for tank_level
  - Added MQTT support for battery_level

- **Safeguards Improvements**
  - Tank low level check now applied during flush process (fixes #29)
  - Safeguard sensor check checkboxes automatically unchecked when disabled due to hardware

### 🛠️ Improvements & Enhancements

- Moved autoflush settings to flush valve hardware config section
- Moved tank capacity to hardware configuration
- Added configurable sensor type for tank level and battery level
- Inverted diverter valve indicator so it lights up in "active" (to tanks) mode
- Added headers to each hardware section in settings
- Added helper text to all safeguard sensor checks and timeouts
- Improved safeguard sensor checks layout with viewport conditional breakpoints
- MFD mode: disabled sortable UI and prevented spurious updates when switching modes
- Cleaned up text for mobile display

### 🐛 Bug Fixes

- Fixed bug where pickle error was not showing

---

## FrothFET

### 🛠️ Improvements & Enhancements

- Updated Rev E averaging code to match Rev F behavior to reduce false overcurrent trips

### 🐛 Bug Fixes

- Fixed averaging bug affecting disabled channels

---

## SendIt

No changes in this release.

---

## Infrastructure

- Updated to YarrboardFramework v2.6.2
- Moved changelog out of `ota_manifest.json` (now a separate file)
- Updated firmware release script to use smaller `ota_manifest.json`
- Changed URLs in preparation for repository split