# mavila-weather

A command-line weather tool written in C++ that fetches weather data from the [Open-Meteo API](https://open-meteo.com/) and location information from [BigDataCloud](https://www.bigdatacloud.com/) reverse geocoding service.

## Features

- Current weather conditions (temperature, wind, weather code)
- Hourly forecast for the next 6 hours
- 10-day daily forecast with min/max temperatures
- Location details from reverse geocoding
- Support for city/country lookup or direct coordinates

## Dependencies

- **libcurl** - For HTTP requests
- **nlohmann/json** - For JSON parsing

### Installing Dependencies

**Debian/Ubuntu:**
```bash
make deps-debian
```

**Fedora/RHEL:**
```bash
make deps-fedora
```

**Arch Linux:**
```bash
make deps-arch
```

## Compiling

```bash
make
```

This will produce a `weather` executable in the current directory.

To clean build artifacts:
```bash
make clean
```

## Installation (Optional)

To install the binary system-wide:
```bash
sudo make install
```

To install bash completion:
```bash
sudo make install-completion
```

## Usage

```bash
# Using coordinates
./weather --latitude LAT --longitude LON

# Using city and country
./weather --city CITY --country COUNTRY
```

### Options

| Option       | Description                        |
|--------------|------------------------------------|
| `--hourly`   | Show hourly forecast (next 6h)     |
| `--daily`    | Show 10-day daily forecast         |
| `--location` | Show location details              |
| `--sources`  | Show data source attribution       |
| `--version`  | Show version information           |
| `--help`     | Show help message and exit         |

### Examples

```bash
# Get current weather for Paris, France
./weather --city Paris --country France

# Get weather with all details for specific coordinates
./weather --latitude 48.8566 --longitude 2.3522 --hourly --daily --location
```

## License

See [LICENSE](LICENSE) for details.
