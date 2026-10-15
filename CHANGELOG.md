# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.7.0] - 2026-10-14

### Added
- **SX1262 LoRa Support**: Added support for Semtech SX1262 LoRa modules (e.g. Ai-Thinker Ra-01SH) alongside existing SX1276/RFM95 support. Configurable via `LoRaModule::Options::module_type` or automatically detected when `gpio_busy >= 0`.
- **Hardware Configuration**: Added `gpio_busy` and `tcxo_voltage` to `LoRaModule::Options`.
- **Radio Abstraction Interface**: Introduced internal polymorphic `RadioInterface` abstraction to cleanly support multiple RadioLib module types.

### Changed
- **og3 v0.7.0 Compatibility**: Updated `chl33/og3` dependency to `~0.7.0`.
- **Lifecycle & Safety**: Explicitly deleted copy constructors and assignment operators for `LoRaModule`, and adopted `std::unique_ptr` for `m_mod` and `m_radio` lifecycle management.

## [0.6.3] - 2026-03-30

### Changed
- **Update variable strings to camelCase**

## [0.6.1] - 2026-03-28

### Changed
- **og3 v0.6.1 Compatibility**: Updated dependency to `og3` v0.6.1.
- **Cross-Platform Support**: Improved build configuration for consistent behavior on `native` platforms.

## [0.6.0] - 2026-03-24

### Added
- **RadioLib Migration**: Switched the backend from `sandeepmistry/LoRa` to `jgromes/RadioLib` for improved hardware support and reliability.
- **Enhanced API**: Added non-blocking `poll_packet()` method and support for signal metrics (`last_rssi()`, `last_snr()`).
- **Power Management**: Added `sleep()` and `standby()` methods for battery-operated devices.

### Changed
- **og3 v0.6.0 Compatibility**: Refactored to use the new `require()` declarative dependency pattern.
- **Hardware Configuration**: Updated `LoRaModule::Options` to include `gpio_dio1`.

## [0.5.0] - 2026-03-07

### Added
- Initial support for RTM95 LoRa module.
