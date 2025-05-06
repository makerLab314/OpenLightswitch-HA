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
* 3D-printed bracket and servo arm STL files in [STLs](https://github.com/makerLab314/OpenLightswitch-HA/tree/main/STLs)
* Jumper wires and optional mounting adhesive or tape

---

## Firmware Setup

1. Open the Arduino IDE and install the required libraries:

   * WiFiNINA
   * ArduinoMqttClient
   * Servo

2. Paste the firmware code [Firmware](https://github.com/makerLab314/OpenLightswitch-HA/blob/main/Lichtschalter_FINAL.ino) folder or from the code section below into a new sketch.

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

To integrate the OpenLightswitch-HA into Home Assistant, you need to make sure your Home Assistant instance has MQTT properly set up. This means:

1. An MQTT broker (e.g. Mosquitto) is installed and running.
2. The broker is reachable from both Home Assistant and your Arduino device.
3. The Arduino is configured with the same credentials and IP address as used by the broker.
4. Home Assistant has MQTT integration enabled (either via the UI or `configuration.yaml`).

Once your MQTT broker is ready, add the following configuration to your `configuration.yaml` file:

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

### Explanation of Parameters:

* **name**: The user-visible name in the Home Assistant dashboard.
* **unique\_id**: A unique identifier to ensure the device does not conflict with other entities.
* **command\_topic**: Topic that Home Assistant sends commands to.
* **state\_topic**: Topic from which Home Assistant reads the actual device state.
* **payload\_on / payload\_off**: Commands sent to the Arduino.
* **state\_on / state\_off**: Expected payloads reported by the Arduino.
* **optimistic**: Set to false because the Arduino reports back its state.
* **qos**: Use 1 for reliable delivery.
* **retain**: Set to false to avoid retaining stale commands.

### Optional: UI Integration

After restarting Home Assistant, the switch should automatically appear in the UI. If it doesn’t, go to *Settings > Devices & Services > MQTT* and check if the device is listed under discovered entities. You can also manually add it to a dashboard via *Settings > Dashboards*.

> ✅ Tip: You can test the setup by toggling the switch in the Home Assistant UI and verifying the Arduino responds with a tap movement and updates the state accordingly.

> ⚠️ Make sure your Home Assistant and Arduino are on the same network segment and your MQTT topics match exactly.

---
## Hardware Assembly

The OpenLightswitch-HA system was designed to be non-invasive and easy to install on standard European light switches.

### Components Required:

* Arduino Uno WiFi Rev2
* SG90 Micro Servo
* Capacitive touch sensor (e.g. TTP223)
* 3D-printed mounting bracket (custom-designed for EU switch frames)
* USB power supply (or equivalent stable 5V source)
* Jumper wires
* A M2x25 screw

### Assembly Instructions:

1. **Print the 3D Bracket**

   * Use the provided STL (3mf) files.
   * The bracket is designed to fit under the plastic frame of standard European light switches without damaging the wall or switch.
   * Make sure the print has enough infill to stay rigid under servo force.

2. **Install the Bracket**

   * Carefully remove the frame of your light switch (no tools required for most EU models).
   * Insert the 3D bracket under the frame and reattach the frame to hold the mount in place.
   * The servo arm should align with the light switch rocker so that it can tap either side.

3. **Attach Components**

   * Mount the SG90 servo onto the bracket using a M2x25 screw.
   * Connect the servo to pin 9 of the Arduino.
   * Connect the touch sensor to pin 2 of the Arduino.
   * Power the Arduino via USB or a stable 5V source.

> 🛠️ Make sure the servo has room to move the rocker switch in both directions. You may need to adjust the angle or bracket position slightly depending on your specific switch model.

> 💡 The design is reversible and requires no screws or adhesives on the wall, making it renter-friendly and easy to relocate.

---

## Operation

* Tapping the sensor toggles the switch manually (alternates ON/OFF).
* Sending `"ON"` or `"OFF"` to the configured MQTT command topic will trigger the servo.
* The Arduino reports its logical state back to the state topic after each action.

---

## License

This project is licensed under the GNU GPL v3 License. See `LICENSE` file for details.

