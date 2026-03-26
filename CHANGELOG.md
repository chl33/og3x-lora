# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
