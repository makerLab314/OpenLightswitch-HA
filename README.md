# OpenLightswitch-HA

OpenLightswitch-HA is an open-source hardware project that uses an Arduino Uno WiFi Rev2, SG90 servo motor, capacitive touch sensor, and 3D-printed parts to physically toggle European-style light switches. It supports MQTT for integration with systems like Home Assistant and allows local control via touch. The device is non-invasive and requires no changes to your home's wiring.

---

## Features

* MQTT-controlled light switch actuation
* Local control via capacitive touch sensor
* Customizable servo movement and timing
* Non-destructive mounting with 3D-printed holder
* Works with most European-style rocker switches

---

## Hardware Required

* Arduino Uno WiFi Rev2
* SG90 Micro Servo
* Capacitive Touch Sensor (e.g., TTP223)
* USB Power Supply (or 5V adapter)
* 3D-printed bracket and servo arm (STL files in `/hardware/`)
* Jumper wires and optional mounting adhesive or tape

---

## Firmware Setup

1. Open the Arduino IDE and install the required libraries:

   * WiFiNINA
   * ArduinoMqttClient
   * Servo

2. Paste the firmware code [Firmware][https://github.com/makerLab314/OpenLightswitch-HA/blob/main/Lichtschalter_FINAL.ino] folder or from the code section below into a new sketch.

3. Modify these configuration values:

   ```cpp
   const char* ssid = "<YOUR_WIFI_SSID>";
   const char* password = "<YOUR_WIFI_PASSWORD>";
   const char* mqtt_server = "<YOUR_MQTT_BROKER_IP>";
   const char* mqtt_user = "<MQTT_USERNAME_IF_REQUIRED>";
   const char* mqtt_pass = "<MQTT_PASSWORD_IF_REQUIRED>";
   const char* command_topic = "<MQTT_TOPIC_COMMAND>";
   const char* state_topic = "<MQTT_TOPIC_STATE>";
   ```

4. Upload the sketch to your Arduino Uno WiFi Rev2.

---

## Home Assistant Configuration

Add the following YAML to your `configuration.yaml`:

```yaml
mqtt:
  switch:
    - name: "Office Light Switch (AMC)"
      unique_id: "office_lightswitch_amc"
      command_topic: "home/lightswitch/office/set"
      state_topic: "home/lightswitch/office/state"
      payload_on: "ON"
      payload_off: "OFF"
      state_on: "ON"
      state_off: "OFF"
      optimistic: false
      qos: 1
      retain: false
```

> Ensure MQTT is properly configured in Home Assistant and that the Arduino has network access.

---

## Assembly Instructions

1. Print the provided STL files located in the `/hardware/` folder.
2. Mount the SG90 servo into the bracket.
3. Attach the arm to the servo horn and align it with your light switch paddle.
4. Use double-sided tape or another non-permanent adhesive to mount the unit over the switch.
5. Connect components as follows:

   * Servo signal to pin 9
   * Touch sensor output to pin 2
   * Power all components from 5V and GND
6. Power the device via USB or stable 5V supply.

---

## Operation

* Tapping the sensor toggles the switch manually (alternates ON/OFF).
* Sending `"ON"` or `"OFF"` to the configured MQTT command topic will trigger the servo.
* The Arduino reports its logical state back to the state topic after each action.

---

## Firmware

Full source code is in the `/firmware/` folder. Paste it directly into the Arduino IDE or upload the `.ino` file.

---

## TODOs / Future Improvements

* Add support for OTA firmware updates
* Allow calibration of servo angle via MQTT
* Support alternative switch layouts (e.g. double or toggle switches)

---

## License

This project is licensed under the MIT License. See `LICENSE` file for details.

---

## Credits

* Hardware design and 3D parts: -\[Insert names or GitHub handles]-
* Firmware development: -\[Insert names]-
* Documentation: -\[Insert names]-
