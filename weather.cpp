/**
 * @file weather.cpp
 * @brief Open-Meteo Weather + BigDataCloud Reverse Geocode CLI Tool
 *
 * A C++ implementation of weather.sh that fetches weather data from Open-Meteo API
 * and location information from BigDataCloud reverse geocoding service.
 *
 * @section Features
 * - Current weather conditions (temperature, wind, weather code)
 * - Hourly forecast for the next 6 hours
 * - 10-day daily forecast with min/max temperatures
 * - Location details from reverse geocoding
 * - Support for city/country lookup or direct coordinates
 *
 * @section Dependencies
 * - libcurl: For HTTP requests
 * - nlohmann/json: For JSON parsing
 *
 * @section Usage
 * @code
 *   ./weather --latitude LAT --longitude LON
 *   ./weather --city CITY --country COUNTRY
 * @endcode
 *
 * @section Options
 *   --hourly         Show hourly forecast (next 6h)
 *   --daily          Show 10-day daily forecast
 *   --location       Show location details
 *   --sources        Show data source attribution
 *   --help           Show help message and exit
 *
 * @author Generated from weather.sh
 * @version 1.0
 */

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cwchar>
#include <clocale>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

// Windows compatibility for POSIX time functions
#ifdef _WIN32
#include <time.h>

// Thread-safe localtime for Windows
inline struct tm *localtime_r(const time_t *timer, struct tm *buf)
{
    localtime_s(buf, timer);
    return buf;
}

// Thread-safe gmtime for Windows
inline struct tm *gmtime_r(const time_t *timer, struct tm *buf)
{
    gmtime_s(buf, timer);
    return buf;
}

// Simple strptime implementation for Windows (supports %Y-%m-%d format)
inline char *strptime(const char *s, const char *format, struct tm *tm)
{
    std::istringstream ss(s);
    ss >> std::get_time(tm, format);
    if (ss.fail())
        return nullptr;
    return const_cast<char *>(s + ss.tellg());
}
#endif

using json = nlohmann::json;
using namespace std;

// Version information - use compile-time define if available
#ifndef VERSION_STRING
#define VERSION_STRING "1.0.0"
#endif
const string VERSION = VERSION_STRING;
const string PROGRAM_NAME = "weather";

/**
 * @brief CURL write callback function for handling HTTP response data
 *
 * This callback is invoked by libcurl when data is received from an HTTP request.
 * It appends the received data to a string buffer.
 *
 * @param contents Pointer to the received data buffer
 * @param size Size of each data element (always 1 for character data)
 * @param nmemb Number of data elements received
 * @param output Pointer to the output string where data will be appended
 * @return Total number of bytes processed (size * nmemb on success)
 *
 * @note This function is registered with CURLOPT_WRITEFUNCTION
 */
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, string *output)
{
    size_t totalSize = size * nmemb;
    output->append((char *)contents, totalSize);
    return totalSize;
}

/**
 * @brief Performs an HTTP GET request to the specified URL
 *
 * Initializes a CURL session, configures it for a GET request, and returns
 * the response body as a string. Handles redirects automatically and has
 * a 30-second timeout.
 *
 * @param url The complete URL to fetch (must be properly encoded)
 * @return Response body as string, or empty string on error
 *
 * @note Errors are logged to stderr
 * @note Follows redirects (CURLOPT_FOLLOWLOCATION enabled)
 * @note 30-second timeout (CURLOPT_TIMEOUT)
 *
 * @example
 * @code
 * string response = httpGet("https://api.example.com/data");
 * if (!response.empty()) {
 *     // Process response
 * }
 * @endcode
 */
string httpGet(const string &url)
{
    CURL *curl = curl_easy_init();
    string response;

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            cerr << "CURL error: " << curl_easy_strerror(res) << endl;
            response.clear();
        }
        curl_easy_cleanup(curl);
    }
    return response;
}

/**
 * @brief URL-encodes a string for safe use in HTTP requests
 *
 * Converts special characters to percent-encoded format (e.g., spaces to %20).
 * Uses libcurl's curl_easy_escape for RFC 3986 compliant encoding.
 *
 * @param str The string to encode (can contain any characters)
 * @return URL-encoded string, or empty string on error
 *
 * @example
 * @code
 * string city = "New York";
 * string encoded = urlEncode(city);  // Returns "New%20York"
 * @endcode
 */
string urlEncode(const string &str)
{
    CURL *curl = curl_easy_init();
    string encoded;
    if (curl)
    {
        char *output = curl_easy_escape(curl, str.c_str(), str.length());
        if (output)
        {
            encoded = output;
            curl_free(output);
        }
        curl_easy_cleanup(curl);
    }
    return encoded;
}

/**
 * @brief Converts a string to lowercase
 *
 * Creates a new string with all alphabetic characters converted to lowercase.
 * Non-alphabetic characters remain unchanged.
 *
 * @param str The input string to convert
 * @return New string with all characters in lowercase
 *
 * @note Uses std::tolower with unsigned char cast for safe handling
 */
string toLower(const string &str)
{
    string result = str;
    transform(result.begin(), result.end(), result.begin(),
              [](unsigned char c)
              { return tolower(c); });
    return result;
}

/**
 * @brief Returns the terminal display width of a Unicode codepoint.
 */
int codepointWidth(uint32_t cp)
{
    // Zero-width: variation selectors, ZWJ, combining marks
    if ((cp >= 0x0300 && cp <= 0x036F) ||
        (cp >= 0x1AB0 && cp <= 0x1AFF) ||
        (cp >= 0x1DC0 && cp <= 0x1DFF) ||
        (cp >= 0x20D0 && cp <= 0x20FF) ||
        (cp >= 0xFE00 && cp <= 0xFE0F) ||
        (cp >= 0xE0100 && cp <= 0xE01EF) ||
        cp == 0x200D || cp == 0x200B || cp == 0x200C || cp == 0x200E || cp == 0x200F ||
        cp == 0xFEFF)
        return 0;
    int w = wcwidth(static_cast<wchar_t>(cp));
    return (w < 0) ? 0 : w;
}

/**
 * @brief Decodes one UTF-8 codepoint from a string.
 */
int decodeUtf8(const char *s, uint32_t &cp)
{
    unsigned char c = (unsigned char)s[0];
    if (c < 0x80)
    {
        cp = c;
        return 1;
    }
    if (c < 0xE0)
    {
        cp = (c & 0x1F) << 6 | (s[1] & 0x3F);
        return 2;
    }
    if (c < 0xF0)
    {
        cp = (c & 0x0F) << 12 | (s[1] & 0x3F) << 6 | (s[2] & 0x3F);
        return 3;
    }
    cp = (c & 0x07) << 18 | (s[1] & 0x3F) << 12 | (s[2] & 0x3F) << 6 | (s[3] & 0x3F);
    return 4;
}

/**
 * @brief Returns the display width of a UTF-8 string in terminal columns.
 */
int displayWidth(const string &str)
{
    int w = 0;
    const char *s = str.c_str();
    while (*s)
    {
        uint32_t cp;
        int bytes = decodeUtf8(s, cp);
        w += codepointWidth(cp);
        s += bytes;
    }
    return w;
}

/**
 * @brief Pads a UTF-8 string with spaces to reach a target display width.
 */
string padRight(const string &str, int targetWidth)
{
    int w = displayWidth(str);
    int padding = targetWidth - w;
    if (padding <= 0)
        return str;
    return str + string(padding, ' ');
}

/**
 * @brief Converts WMO weather code to corresponding weather emoji
 *
 * Maps World Meteorological Organization (WMO) weather codes to Unicode
 * weather emojis for visual representation in terminal output.
 *
 * @param code WMO weather code (0-99)
 * @return Weather emoji string with trailing space for formatting
 *
 * @see https://open-meteo.com/en/docs for WMO code reference
 *
 * WMO Code Reference:
 * - 0: Clear sky (☀️)
 * - 1: Mainly clear (🌤️)
 * - 2: Partly cloudy (⛅️)
 * - 3: Overcast (☁️)
 * - 45, 48: Fog (🌫️)
 * - 51, 53, 55: Drizzle (🌦️)
 * - 56, 57: Freezing drizzle (🌧️)
 * - 61, 63, 65: Rain (🌧️)
 * - 66, 67: Freezing rain (🌧️)
 * - 71, 73, 75: Snowfall (🌨️)
 * - 77: Snow grains (🌨️)
 * - 80, 81, 82: Rain showers (🌧️)
 * - 85, 86: Snow showers (🌨️)
 * - 95: Thunderstorm (⛈️)
 * - 96, 99: Thunderstorm with hail (⛈️)
 */
string wmoEmoji(int code)
{
    switch (code)
    {
    case 0:
        return "☀️ ";
    case 1:
        return "🌤️ ";
    case 2:
        return "⛅️ ";
    case 3:
        return "☁️ ";
    case 45:
    case 48:
        return "🌫️ ";
    case 51:
    case 53:
    case 55:
        return "🌦️ ";
    case 56:
    case 57:
        return "🌧️ ";
    case 61:
    case 63:
    case 65:
        return "🌧️ ";
    case 66:
    case 67:
        return "🌧️ ";
    case 71:
    case 73:
    case 75:
        return "🌨️ ";
    case 77:
        return "🌨️ ";
    case 80:
    case 81:
    case 82:
        return "🌧️ ";
    case 85:
    case 86:
        return "🌨️ ";
    case 95:
        return "⛈️ ";
    case 96:
    case 99:
        return "⛈️ ";
    default:
        return "❓ ";
    }
}

/**
 * @brief Converts WMO weather code to human-readable description
 *
 * Maps World Meteorological Organization (WMO) weather codes to short
 * descriptive text strings suitable for display.
 *
 * @param code WMO weather code (0-99)
 * @return Human-readable weather condition string
 *
 * @see wmoEmoji() for corresponding emoji representation
 */
string wmoText(int code)
{
    switch (code)
    {
    case 0:
        return "Clear sky";
    case 1:
        return "Mainly clear";
    case 2:
        return "Partly cloudy";
    case 3:
        return "Overcast";
    case 45:
    case 48:
        return "Fog";
    case 51:
    case 53:
    case 55:
        return "Drizzle";
    case 56:
    case 57:
        return "Frz. drizzle";
    case 61:
    case 63:
    case 65:
        return "Rain";
    case 66:
    case 67:
        return "Frz. rain";
    case 71:
    case 73:
    case 75:
        return "Snowfall";
    case 77:
        return "Snow grains";
    case 80:
    case 81:
    case 82:
        return "Rain showers";
    case 85:
    case 86:
        return "Snow showers";
    case 95:
        return "Thunderstorm";
    case 96:
    case 99:
        return "T-storm/hail";
    default:
        return "Unknown";
    }
}

/**
 * @brief Gets the current local time as a formatted string
 *
 * Returns the current system time formatted according to strftime format codes.
 *
 * @param format strftime format string (default: "%Y-%m-%d %H:%M")
 * @return Formatted time string in local timezone
 *
 * @example
 * @code
 * string time = getLocalTime();           // "2026-03-02 14:30"
 * string date = getLocalTime("%Y-%m-%d"); // "2026-03-02"
 * @endcode
 */
string getLocalTime(const string &format = "%Y-%m-%d %H:%M")
{
    time_t now = time(nullptr);
    struct tm *tm_info = localtime(&now);
    char buffer[64];
    strftime(buffer, sizeof(buffer), format.c_str(), tm_info);
    return string(buffer);
}

/**
 * @brief Gets the local timezone abbreviation
 *
 * Returns the system's current timezone abbreviation (e.g., "CET", "EST", "PST").
 *
 * @return Timezone abbreviation string
 *
 * @example Returns "CET" for Central European Time
 */
string getLocalTimezone()
{
    time_t now = time(nullptr);
    struct tm *tm_info = localtime(&now);
    char buffer[16];
    strftime(buffer, sizeof(buffer), "%Z", tm_info);
    return string(buffer);
}

/**
 * @brief Extracts and formats time string from ISO 8601 format
 *
 * Parses an ISO 8601 datetime string and returns the date/time portion.
 * The timezone parameter is reserved for future timezone conversion support.
 *
 * @param timeStr ISO 8601 formatted time string (e.g., "2026-03-02T14:30")
 * @param timezone Target timezone (currently unused, reserved for future use)
 * @return Formatted time string (first 16 characters of input)
 *
 * @note Currently returns time as-is from API (already in target timezone)
 */
string convertToTimezone(const string &timeStr, [[maybe_unused]] const string &timezone)
{
    // For simplicity, we'll use the time from the API which is already in the remote timezone
    return timeStr.substr(0, 16); // Extract YYYY-MM-DDTHH:MM and remove T
}

/**
 * @brief Converts a date string to abbreviated day name with date
 *
 * Parses an ISO date string and returns the abbreviated weekday name
 * followed by month-day (e.g., "Mon 03-02").
 *
 * @param dateStr Date string in "YYYY-MM-DD" format
 * @return Formatted string as "Day MM-DD" (e.g., "Mon 03-02")
 *
 * @example
 * @code
 * string dayName = getDayName("2026-03-02"); // Returns "Mon 03-02"
 * @endcode
 */
string getDayName(const string &dateStr)
{
    struct tm tm = {};
    strptime(dateStr.c_str(), "%Y-%m-%d", &tm);
    char buffer[16];
    strftime(buffer, sizeof(buffer), "%a %m-%d", &tm);
    return string(buffer);
}

/**
 * @brief Prints data source attribution
 *
 * Displays the APIs used for weather and location data.
 *
 * @param fancy If true, uses emojis, ANSI codes, and decorative formatting.
 *              If false, uses simple text format suitable for help output.
 */
void printDataSources(bool fancy = false)
{
    if (fancy)
    {
        cout << " Data Sources" << endl;
        cout << " ──────────────────────────────────────────────────────────────────" << endl;
        printf("  🌤\033[7G%-16s  %s\n", "Weather data:", "Open-Meteo API (https://open-meteo.com)");
        printf("  🔍\033[7G%-16s  %s\n", "Geocoding:", "Open-Meteo Geocoding API (https://open-meteo.com)");
        printf("  📍\033[7G%-16s  %s\n", "Reverse geocode:", "BigDataCloud (https://www.bigdatacloud.com)");
        cout << endl;
        cout << "  All data is provided free of charge for non-commercial use." << endl;
        cout << endl;
    }
    else
    {
        cout << "Data Sources:" << endl;
        cout << "  Weather data:      Open-Meteo API (https://open-meteo.com)" << endl;
        cout << "  Geocoding:         Open-Meteo Geocoding API" << endl;
        cout << "  Reverse geocode:   BigDataCloud (https://www.bigdatacloud.com)" << endl;
    }
}

/**
 * @brief Prints the command-line help message
 *
 * Displays usage information, available options, and examples to stdout.
 *
 * @param progName The program name (typically argv[0]) for usage display
 */
void printHelp(const char *progName)
{
    cout << PROGRAM_NAME << " v" << VERSION << " - Command-line weather tool" << endl;
    cout << endl;
    cout << "Usage: " << progName << " --latitude LAT --longitude LON [OPTIONS]" << endl;
    cout << "       " << progName << " --city CITY --country COUNTRY [OPTIONS]" << endl;
    cout << endl;
    cout << "Options:" << endl;
    cout << "  --latitude LAT     Latitude coordinate" << endl;
    cout << "  --longitude LON    Longitude coordinate" << endl;
    cout << "  --city CITY        City name (used with --country)" << endl;
    cout << "  --country COUNTRY  Country name (used with --city)" << endl;
    cout << "  --hourly           Show hourly forecast (next 6h)" << endl;
    cout << "  --daily            Show 10-day daily forecast" << endl;
    cout << "  --location         Show location details" << endl;
    cout << "  --sources          Show data source attribution" << endl;
    cout << "  --version          Show version information" << endl;
    cout << "  --help             Show this help message and exit" << endl;
    cout << endl;
    printDataSources(false);
}

/**
 * @brief Main entry point for the weather CLI application
 *
 * Parses command-line arguments, fetches weather data from Open-Meteo API,
 * performs reverse geocoding via BigDataCloud, and displays formatted output.
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, 1 on error (invalid arguments, network error, API error)
 *
 * @section Workflow
 * 1. Parse command-line arguments (coordinates or city/country)
 * 2. If city/country provided, geocode to coordinates via Open-Meteo Geocoding API
 * 3. Reverse geocode coordinates to get location details via BigDataCloud
 * 4. Fetch weather data from Open-Meteo Weather API
 * 5. Display current conditions and optional forecasts
 *
 * @section APIs Used
 * - Open-Meteo Geocoding: https://geocoding-api.open-meteo.com/v1/search
 * - Open-Meteo Weather: https://api.open-meteo.com/v1/forecast
 * - BigDataCloud Reverse Geocode: https://api.bigdatacloud.net/data/reverse-geocode-client
 */
int main(int argc, char *argv[])
{
    setlocale(LC_CTYPE, "");

    // Command-line argument storage
    string lat, lon, cityArg, countryArg;

    // Display flags for optional output sections
    bool showHourly = false, showDaily = false, showLocation = false, showSources = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i)
    {
        string arg = argv[i];

        if (arg == "--help")
        {
            printHelp(argv[0]);
            return 0;
        }
        else if (arg == "--version")
        {
            cout << PROGRAM_NAME << " v" << VERSION << endl;
            return 0;
        }
        else if (arg == "--latitude" && i + 1 < argc)
        {
            lat = argv[++i];
        }
        else if (arg == "--longitude" && i + 1 < argc)
        {
            lon = argv[++i];
        }
        else if (arg == "--city" && i + 1 < argc)
        {
            cityArg = argv[++i];
        }
        else if (arg == "--country" && i + 1 < argc)
        {
            countryArg = argv[++i];
        }
        else if (arg == "--hourly")
        {
            showHourly = true;
        }
        else if (arg == "--daily")
        {
            showDaily = true;
        }
        else if (arg == "--location")
        {
            showLocation = true;
        }
        else if (arg == "--sources")
        {
            showSources = true;
        }
        else
        {
            cerr << "Unknown option: " << arg << endl;
            printHelp(argv[0]);
            return 1;
        }
    }

    // Initialize CURL globally
    curl_global_init(CURL_GLOBAL_ALL);

    // Resolve coordinates from city/country if needed
    if (!cityArg.empty() && !countryArg.empty())
    {
        string encodedCity = urlEncode(cityArg);
        string geoUrl = "https://geocoding-api.open-meteo.com/v1/search?name=" + encodedCity + "&count=10&language=en&format=json";
        string geoResponse = httpGet(geoUrl);

        if (geoResponse.empty())
        {
            cerr << "❌ Could not geocode '" << cityArg << ", " << countryArg << "'" << endl;
            curl_global_cleanup();
            return 1;
        }

        try
        {
            json geoJson = json::parse(geoResponse);

            if (!geoJson.contains("results") || geoJson["results"].empty())
            {
                cerr << "❌ Could not geocode '" << cityArg << ", " << countryArg << "'" << endl;
                curl_global_cleanup();
                return 1;
            }

            // Find matching result by country name
            json match;
            string countryLower = toLower(countryArg);

            for (const auto &result : geoJson["results"])
            {
                if (result.contains("country"))
                {
                    string resultCountry = toLower(result["country"].get<string>());
                    if (resultCountry == countryLower)
                    {
                        match = result;
                        break;
                    }
                }
            }

            // Fallback to first result
            if (match.is_null())
            {
                match = geoJson["results"][0];
            }

            lat = to_string(match["latitude"].get<double>());
            lon = to_string(match["longitude"].get<double>());
        }
        catch (const json::exception &e)
        {
            cerr << "❌ JSON parsing error: " << e.what() << endl;
            curl_global_cleanup();
            return 1;
        }
    }
    else if (lat.empty() || lon.empty())
    {
        printHelp(argv[0]);
        curl_global_cleanup();
        return 1;
    }

    // =========================================================================
    // SECTION 1: Reverse Geocoding
    // Converts coordinates to human-readable location information
    // API: BigDataCloud (https://api.bigdatacloud.net)
    // =========================================================================
    string locationUrl = "https://api.bigdatacloud.net/data/reverse-geocode-client?latitude=" + lat + "&longitude=" + lon;
    string locationResponse = httpGet(locationUrl);

    // Location data with defaults for missing fields
    string city = "Unknown city", country = "Unknown country";
    string locality, subdivision, postcode, countryCode, continent, plusCode;

    if (!locationResponse.empty())
    {
        try
        {
            json locJson = json::parse(locationResponse);

            if (locJson.contains("city") && !locJson["city"].is_null() && !locJson["city"].get<string>().empty())
            {
                city = locJson["city"].get<string>();
            }
            else if (locJson.contains("principalSubdivision") && !locJson["principalSubdivision"].is_null())
            {
                city = locJson["principalSubdivision"].get<string>();
            }

            if (locJson.contains("countryName") && !locJson["countryName"].is_null())
            {
                country = locJson["countryName"].get<string>();
            }

            if (locJson.contains("locality") && !locJson["locality"].is_null())
            {
                locality = locJson["locality"].get<string>();
            }
            if (locJson.contains("principalSubdivision") && !locJson["principalSubdivision"].is_null())
            {
                subdivision = locJson["principalSubdivision"].get<string>();
            }
            if (locJson.contains("postcode") && !locJson["postcode"].is_null())
            {
                postcode = locJson["postcode"].get<string>();
            }
            if (locJson.contains("countryCode") && !locJson["countryCode"].is_null())
            {
                countryCode = locJson["countryCode"].get<string>();
            }
            if (locJson.contains("continent") && !locJson["continent"].is_null())
            {
                continent = locJson["continent"].get<string>();
            }
            if (locJson.contains("plusCode") && !locJson["plusCode"].is_null())
            {
                plusCode = locJson["plusCode"].get<string>();
            }
        }
        catch (const json::exception &e)
        {
            // Continue with defaults
        }
    }

    // =========================================================================
    // SECTION 2: Weather Data Fetch
    // Retrieves current conditions, hourly, and daily forecasts
    // API: Open-Meteo (https://api.open-meteo.com)
    // Parameters:
    //   - current: temperature_2m, wind_speed_10m, weather_code
    //   - hourly: temperature_2m, relative_humidity_2m, wind_speed_10m
    //   - daily: temp min/max, weather_code, wind_speed_10m_max, precipitation_sum
    //   - forecast_days: 10 (10-day forecast)
    //   - timezone: auto (API determines from coordinates)
    // =========================================================================
    string weatherUrl = "https://api.open-meteo.com/v1/forecast?latitude=" + lat + "&longitude=" + lon +
                        "&current=temperature_2m,wind_speed_10m,weather_code" +
                        "&hourly=temperature_2m,relative_humidity_2m,wind_speed_10m" +
                        "&daily=temperature_2m_max,temperature_2m_min,weather_code,wind_speed_10m_max,precipitation_sum" +
                        "&forecast_days=10&timezone=auto";

    string weatherResponse = httpGet(weatherUrl);

    if (weatherResponse.empty())
    {
        cerr << "❌ Network error or no weather response" << endl;
        curl_global_cleanup();
        return 1;
    }

    json weatherJson;
    try
    {
        weatherJson = json::parse(weatherResponse);

        if (weatherJson.contains("error"))
        {
            cerr << "❌ Weather API error in response" << endl;
            curl_global_cleanup();
            return 1;
        }
    }
    catch (const json::exception &e)
    {
        cerr << "❌ JSON parsing error: " << e.what() << endl;
        curl_global_cleanup();
        return 1;
    }

    // =========================================================================
    // SECTION 3: Current Weather Display
    // Shows current temperature, wind speed, and weather condition
    // Time display shows both remote location time and user's local time
    // =========================================================================
    double currentTemp = weatherJson["current"]["temperature_2m"].get<double>();
    double currentWind = weatherJson["current"]["wind_speed_10m"].get<double>();
    string currentTime = weatherJson["current"]["time"].get<string>();
    int weatherCode = weatherJson["current"]["weather_code"].get<int>();
    string remoteTz = weatherJson["timezone"].get<string>();
    string remoteTzAbbr = weatherJson["timezone_abbreviation"].get<string>();
    string localTz = getLocalTimezone();

    // Capture current time once for consistency (both times derived from same instant)
    time_t now = time(nullptr);

    // Format as local time
    struct tm tm_local;
    localtime_r(&now, &tm_local);
    char localTimeBuf[32];
    strftime(localTimeBuf, sizeof(localTimeBuf), "%Y-%m-%d %H:%M", &tm_local);
    string localTime = localTimeBuf;

    // Format as remote time using API's UTC offset
    int remoteUtcOffset = weatherJson.value("utc_offset_seconds", 0);
    time_t remoteAdjusted = now + remoteUtcOffset;
    struct tm tm_remote;
    gmtime_r(&remoteAdjusted, &tm_remote);
    char remoteTimeBuf[32];
    strftime(remoteTimeBuf, sizeof(remoteTimeBuf), "%Y-%m-%d %H:%M", &tm_remote);
    string remoteTime = remoteTimeBuf;

    cout << endl;
    cout << " Weather for: " << city << ", " << country << endl;
    cout << " ──────────────────────────────────────────────────────────────────" << endl;
    cout << wmoEmoji(weatherCode) << "\033[7G" << wmoText(weatherCode) << endl;
    cout << "🌡️\033[7GTemperature:  " << fixed << setprecision(1) << currentTemp << " °C" << endl;
    cout << "💨\033[7GWind Speed:   " << fixed << setprecision(1) << currentWind << " km/h" << endl;
    cout << "🕐\033[7GLocal time:   " << remoteTime.c_str() << " (" + remoteTzAbbr + ")" << endl;
    cout << "🏠\033[7GYour time:    " << localTime.c_str() << " (" + localTz + ")" << endl;
    cout << endl;

    // =========================================================================
    // SECTION 4: Hourly Forecast (--hourly flag)
    // Displays temperature, humidity, and wind for next 6 hours
    // Starts from the next full hour after current time
    // =========================================================================
    if (showHourly)
    {
        // Find the current hour in the hourly array
        string currentHourPrefix = currentTime.substr(0, 13);
        int startIndex = 0;

        const auto &hourlyTimes = weatherJson["hourly"]["time"];
        for (size_t i = 0; i < hourlyTimes.size(); ++i)
        {
            string t = hourlyTimes[i].get<string>();
            if (t.substr(0, 13) == currentHourPrefix)
            {
                startIndex = i;
                break;
            }
        }
        startIndex++; // Skip current hour

        // Get today's date in remote timezone (approximate)
        string todayRemote = currentTime.substr(0, 10);

        cout << " Hourly Forecast (next 6h)" << endl;
        cout << " ──────────────────────────────────────────────────" << endl;
        cout << " " << left << setw(10) << "Day" << "  " << setw(5) << remoteTzAbbr
             << " │ " << setw(7) << "Temp °C" << " │ " << setw(7) << "Hum. %"
             << " │ " << setw(10) << "Wind km/h" << endl;
        cout << " ──────────────────┼─────────┼─────────┼───────────" << endl;

        for (int offset = 0; offset < 6; ++offset)
        {
            int i = startIndex + offset;
            if (i >= (int)hourlyTimes.size())
                break;

            string hourTime = hourlyTimes[i].get<string>();
            double temp = weatherJson["hourly"]["temperature_2m"][i].get<double>();
            int humid = weatherJson["hourly"]["relative_humidity_2m"][i].get<int>();
            double wind = weatherJson["hourly"]["wind_speed_10m"][i].get<double>();

            string hourPart = hourTime.substr(11, 5);
            string datePart = hourTime.substr(0, 10);

            string dayLabel = (datePart == todayRemote) ? "Today" : "Tomorrow";

            cout << " " << left << setw(10) << dayLabel << "  " << setw(5) << hourPart
                 << " │ " << right << setw(5) << fixed << setprecision(1) << temp << "  "
                 << " │ " << setw(5) << humid << "  "
                 << " │ " << setw(6) << fixed << setprecision(1) << wind << endl;
        }
        cout << endl;
    }

    // =========================================================================
    // SECTION 5: Daily Forecast (--daily flag)
    // Displays 10-day forecast with min/max temps, conditions, wind, rain
    // Uses ANSI escape codes (\033[18G) for column alignment with emojis
    // =========================================================================
    if (showDaily)
    {
        const auto &dailyTimes = weatherJson["daily"]["time"];
        int numDays = dailyTimes.size();

        cout << " 10-Day Forecast" << endl;
        cout << " ──────────────────────────────────────────────────────────────────" << endl;
        cout << " " << left << setw(10) << "Date" << " │    " << setw(13) << "Condition"
             << " │ " << setw(7) << "Min°C" << " │ " << setw(7) << "Max°C"
             << " │ " << setw(6) << "km/h" << " │ " << setw(7) << "Rain mm" << endl;
        cout << " ───────────┼──────────────────┼────────┼────────┼────────┼────────" << endl;

        for (int d = 0; d < numDays; ++d)
        {
            string dDate = dailyTimes[d].get<string>();
            int dCode = weatherJson["daily"]["weather_code"][d].get<int>();
            double dMin = weatherJson["daily"]["temperature_2m_min"][d].get<double>();
            double dMax = weatherJson["daily"]["temperature_2m_max"][d].get<double>();
            double dWind = weatherJson["daily"]["wind_speed_10m_max"][d].get<double>();
            double dRain = weatherJson["daily"]["precipitation_sum"][d].get<double>();

            string dEmoji = wmoEmoji(dCode);
            string dText = wmoText(dCode);
            string dayName = getDayName(dDate);

            printf(" %-10s │ %s\033[18G%-13s │ %6.1f │ %6.1f │ %6.1f │ %7.1f\n",
                   dayName.c_str(), dEmoji.c_str(), dText.c_str(), dMin, dMax, dWind, dRain);
        }
        cout << endl;
    }

    // =========================================================================
    // SECTION 6: Location Details (--location flag)
    // Displays detailed location info from reverse geocoding
    // Includes coordinates, locality, region, postal code, country, etc.
    // Uses ANSI escape codes (\033[7G) for column alignment with emojis
    // =========================================================================
    if (showLocation)
    {
        cout << " Location Details" << endl;
        cout << " ──────────────────────────────────────────────────────────────────" << endl;
        printf("  📍\033[7G%-16s  %s, %s\n", "Coordinates:", lat.c_str(), lon.c_str());
        if (!locality.empty())
            printf("  🏘\033[7G%-16s  %s\n", "Locality:", locality.c_str());
        if (!subdivision.empty())
            printf("  🗺\033[7G%-16s  %s\n", "Region:", subdivision.c_str());
        if (!postcode.empty())
            printf("  📮\033[7G%-16s  %s\n", "Postal code:", postcode.c_str());
        if (!countryCode.empty())
            printf("  🏳\033[7G%-16s  %s (%s)\n", "Country:", country.c_str(), countryCode.c_str());
        if (!continent.empty())
            printf("  🌍\033[7G%-16s  %s\n", "Continent:", continent.c_str());
        if (!plusCode.empty())
            printf("  📌\033[7G%-16s  %s\n", "Plus code:", plusCode.c_str());
        printf("  🕐\033[7G%-16s  %s (%s)\n", "Timezone:", remoteTz.c_str(), remoteTzAbbr.c_str());
        printf("  🗺️\033[7G%-16s  https://www.google.com/maps?q=%s,%s\n", "Google Maps:", lat.c_str(), lon.c_str());
        cout << endl;
    }

    // =========================================================================
    // SECTION 7: Data Sources Attribution (--sources flag)
    // Credits the APIs used for weather and location data
    // All services are free for non-commercial use
    // =========================================================================
    if (showSources)
    {
        printDataSources(true);
    }

    curl_global_cleanup();
    return 0;
}
