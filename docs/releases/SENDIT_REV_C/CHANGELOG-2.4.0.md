# Version 2.4.0

## FrothFET

### 🚀 New Features

- **Interrupt-Based Soft Fuse (Overcurrent Protection)**
  - Soft fuse type is now configurable: SLOW, MEDIUM, FAST
  - Added INA226 interrupt-driven soft fuse for near-instant overcurrent tripping
  - Channels can now survive a short circuit event

- **Buzzer Alarms**
  - Added buzzer alerts for: blown fuse, soft fuse trip, bypass active, and overheat

- **Overheat Monitoring**
  - Added overheat status display to the UI
  - Added overheat event counter and broke out aggregate stats on the stats page

- **Home Assistant**
  - Added wattage as a Home Assistant entity per channel

### 🛠️ Improvements & Enhancements

- Transitioned from `getVoltage`/`getAmperage` to `getAverage`/`getLatest` for clearer semantics
- `generateUpdate` now uses latest or averaged readings based on when the load was last switched on
- Disabled channels are now ignored during processing
- Global brightness command now performs a one-time overwrite for dimmable lights rather than a persistent override
- Default startup melody set to `ACTIVE_STARTUP`
- Removed redundant `fadeOver` flag and associated checks

---

## Brineomatic

### 🚀 New Features

- **Home Assistant Integration**
  - Watermaker on/off switch now fully working in Home Assistant
  - Comprehensive sensor coverage: all Brineomatic sensors added to HA discovery

- **Configurable Stepper Motor Power**
  - Stepper motor current is now independently configurable for normal running and homing modes

---

## SendIt

### 🚀 New Features

- **Configurable Home Assistant Device Class**
  - Device class is now a per-channel configurable option

- **Improved Home Assistant Discovery**
  - Digital inputs now included in HA discovery
  - Discovery respects per-channel calibrated data settings

- **Configurable Averaging Window**
  - Backend support and UI controls for configurable ADC averaging window length per channel

---

## Infrastructure

- Updated to YarrboardFramework 2.5.0
