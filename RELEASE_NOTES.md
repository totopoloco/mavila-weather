# Release Notes

## v1.0.6
- Refactored data sources into reusable `printDataSources()` function
- Data sources now displayed in `--help` output
- Updated README with downloads section and data sources

## v1.0.5
- Added Windows compatibility for POSIX time functions (`strptime`, `localtime_r`, `gmtime_r`)
- Cross-compilation for Windows now fully functional

## v1.0.4
- Enhanced release notes with detailed features and installation instructions
- Updated CI workflow for improved Windows build process

## v1.0.3
- Added Windows cross-compilation using MinGW-w64
- Windows release package includes `weather.exe` and required DLLs
- Automated Windows binary upload to GitHub releases

## v1.0.2
- Added automatic GitHub release creation on push to master/main
- Created RELEASE_NOTES.md for release documentation
- Release includes compiled Linux binary as asset

## v1.0.1
- Fixed CI workflow permissions for pushing tags
- Added `contents: write` permission for GitHub Actions

## v1.0.0
- Initial release
- Current weather conditions (temperature, wind, weather code)
- Hourly forecast for the next 6 hours
- 10-day daily forecast with min/max temperatures
- Location details from reverse geocoding
- Support for city/country lookup or direct coordinates
- Dynamic versioning from git tags
- CI/CD with Ubuntu and Fedora builds

---

## Features

- **Current Weather**: Temperature, wind speed, and weather conditions with emoji indicators
- **Hourly Forecast**: Next 6 hours with temperature, humidity, and wind data
- **Daily Forecast**: 10-day forecast with min/max temperatures, conditions, and precipitation
- **Location Details**: Reverse geocoding with coordinates, region, postal code, and timezone
- **Flexible Input**: Query by city/country name or latitude/longitude coordinates
- **Cross-Platform**: Available for Linux and Windows

## Data Sources

- Weather data: [Open-Meteo API](https://open-meteo.com)
- Geocoding: [Open-Meteo Geocoding API](https://open-meteo.com)
- Reverse geocoding: [BigDataCloud](https://www.bigdatacloud.com)

## Installation

Download the appropriate binary for your platform:
- **Linux**: `weather` (make executable with `chmod +x weather`)
- **Windows**: `weather-windows.zip` (extract and run `weather.exe`)
