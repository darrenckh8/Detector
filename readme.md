# Detector Project

This project is an Arduino-based system designed to detect objects and classify them as metal or non-metal. It uses a combination of sensors, an LCD display, audio playback, and Wi-Fi connectivity to log detection data to an InfluxDB database.

### Key Files

- **`APP/app.ino`**: Main Arduino sketch containing the project logic.
- **`APP/creds.h`**: Contains Wi-Fi and InfluxDB credentials (excluded from version control).
- **`APP/partitions.csv`**: Defines the partition scheme for the ESP32.
- **`APP/data/`**: Directory containing audio files for playback.
- **`config.txt`**: Configuration file for project settings.

## Setup Instructions

1. **Install Dependencies**:
   - Install the required Arduino libraries:
     - `Arduino.h`
     - `FS.h`
     - `LittleFS.h`
     - `Audio.h`
     - `Wire.h`
     - `LiquidCrystal_I2C.h`
     - `WiFiMulti.h`
     - `InfluxDbClient.h`
     - `InfluxDbCloud.h`

2. **Configure Wi-Fi and InfluxDB**:
   - Update the Wi-Fi and InfluxDB credentials in `APP/creds.h`.

3. **Prepare LittleFS**:
   - Upload audio files to the `APP/data/` directory.
   - Use the Arduino IDE's LittleFS plugin to upload the `APP/data/` directory to the ESP32's filesystem.

4. **Flash the ESP32**:
   - Use the Arduino IDE to compile and upload `APP/app.ino` to your ESP32.

5. **Partition Scheme**:
   - Ensure the partition scheme matches the `APP/partitions.csv` file.

6. **Run the Project**:
   - Power on the ESP32 and monitor the serial output for status updates.

## Usage

- **Object Detection**:
  - Place an object in front of the sensor to classify it as metal or non-metal.
  - The result will be displayed on the LCD and logged to InfluxDB (if online).

- **Audio Playback**:
  - Audio feedback is played based on the detection result.

- **Wi-Fi Toggle**:
  - Long-press the button connected to `PIN_INPUT_3` to toggle Wi-Fi on or off.

- **File Listing**:
  - Press the button connected to `PIN_INPUT_3` to list all files in the LittleFS filesystem.

- **Volume Control**:
  - Send a serial command in the format `volume <value>` to adjust the playback volume (0-100).

## Template for `APP/creds.h`

The `APP/creds.h` file should define the Wi-Fi and InfluxDB credentials required for the project. Below is the template format:

```cpp
#ifndef CREDS_H
#define CREDS_H

// Wi-Fi credentials
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"

// InfluxDB credentials
#define INFLUXDB_URL "http://your-influxdb-url.com"
#define INFLUXDB_ORG "YourOrganization"
#define INFLUXDB_BUCKET "YourBucket"
#define INFLUXDB_TOKEN "YourAuthToken"

#endif // CREDS_H
```

Ensure you replace the placeholder values with your actual credentials before uploading the project to the ESP32.

## Visualization with Grafana

- The data logged to InfluxDB can be visualized using Grafana.
- Set up a Grafana dashboard and connect it to your InfluxDB instance.
- Create panels to display detection results, such as the number of metal and non-metal objects detected over time.
- Use Grafana's query builder to customize the visualization to your needs.

### Example Dashboard

Below is an example of a Grafana dashboard displaying detection results:

![Example Grafana Dashboard](img/dashboard.png)

## Notes

- The `APP/creds.h` file and audio files in `APP/data/` are excluded from version control for security and storage reasons.
- Ensure the `config.txt` settings match your ESP32's hardware configuration.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.