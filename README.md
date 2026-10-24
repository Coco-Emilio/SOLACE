# SOLACE ESP32 firmware

This is a configurable Arduino-ESP32 starting point, not a final wiring plan. It
implements:

- six directly connected calibrated LDR channels using ESP32 ADC1;
- two-axis servo tracking with deadband and movement limits;
- selectable HC-SR04 or A02YYUW ultrasonic occupancy detection;
- a digital heat-threshold input such as a KY-028 `DO` pin;
- occupancy-and-heat-gated fan/cooling output;
- automatic objective switching: solar-power tracking while unoccupied and a
  calibrated shade pose while occupied;
- Wi-Fi status, remote stow commands, incoming weather data, and optional JSON
  telemetry suitable for a heat-map backend;
- separate wind, rain, fault, low-light, shade, tracking, and aligned behavior.

## Power mode versus shade mode

The operating priority is:

```text
FAULT / WIND / RAIN
        ↓
LOW LIGHT
        ↓
PERSON PRESENT → SHADE pose
        ↓
NO PERSON → LDR solar-power tracking
```

`SHADE_AZ_DEG` and `SHADE_EL_DEG` in `Config.h` define the initial shade pose.
Tune them against the fixed miniature passenger area. The occupancy sensor only
reports that something entered its beam; it does not report the person's 2D
position, so one sensor cannot dynamically aim a shadow at arbitrary locations.
For that later capability, use two or more ranging zones/sensors or a small
thermal/ToF array and combine passenger position with the measured sun vector.

## Dependencies

Install the Espressif ESP32 Arduino board package. No servo library is needed;
the firmware drives both servos directly with the ESP32 LEDC PWM peripheral.
Open `SOLACE.ino` in Arduino IDE and keep all files in this folder.

## SG servo PWM calibration

Both outputs use 50 Hz PWM. The initial 1000-2000 microsecond range is
deliberately conservative because Arduino-kit SG90-compatible servos vary.

1. Power the servo from a separate regulated 5 V supply and connect that
   supply's ground to ESP32 ground. Do not power both servos from the ESP32.
2. Disconnect the horn or linkage before the first powered test.
3. Leave the pulse limits at 1000-2000 us and boot at the configured 90-degree
   start position.
4. Fit the horn at the mechanism's physical midpoint.
5. Move toward each end gradually by changing the start angle. Stop before the
   servo buzzes, stalls, or pushes the linkage into a hard stop.
6. Set `AZ_MIN_DEG`, `AZ_MAX_DEG`, `EL_MIN_DEG`, and `EL_MAX_DEG` to the safe
   mechanical angles with margin. Only widen the microsecond pulse range in
   small steps if the mechanism genuinely needs more travel.
7. If an axis moves backward, change its `SERVO_*_REVERSED` flag instead of
   rewiring power.

Pulse endpoints and mechanical angle limits solve different problems: pulse
endpoints characterize the servo; angle limits protect the assembled linkage.

## Before connecting hardware

1. Select an ESP32-WROOM development-board target in Arduino IDE and verify every
   GPIO in `Config.h` against that board's printed pin labels.
2. Confirm each servo's supply voltage/current and power servos from a separate
   regulated rail with common ground. Never power them from the ESP32 3.3 V pin.
3. Connect the cooling fan only through an appropriate MOSFET/driver with flyback
   protection if required.
4. Choose the ultrasonic model with `USE_A02YYUW` in `Config.h`. The default is
   HC-SR04. Its 5 V `ECHO` must pass through a resistor divider or level shifter
   before reaching the 3.3 V ESP32 input. For an A02YYUW, power it at 3.3 V when
   its UART output connects directly to the ESP32 and verify its pin order.
5. Mechanically disconnect the servo horns for the first power-on. Establish
   safe minimum, maximum, and stow angles before allowing loaded movement.
6. Fill in Wi-Fi credentials locally. Avoid publishing them in source control.

## Six direct LDRs

No analog multiplexer is required. Build six identical LDR/10 kOhm voltage
dividers and connect their midpoint nodes as follows:

| Sensor | ESP32 pin |
| --- | --- |
| T1 | GPIO32 |
| T2 | GPIO33 |
| B1 | GPIO34 |
| B2 | GPIO35 |
| L1 | GPIO36 / VP |
| R1 | GPIO39 / VN |

The firmware averages T1/T2 and B1/B2 for elevation tracking. L1 and R1 provide
the azimuth error. GPIO34, GPIO35, GPIO36, and GPIO39 are input-only, which is
appropriate for these analog divider outputs. The HC-SR04 moves to GPIO16/17,
and the temperature threshold input moves to GPIO13.

## Local Wi-Fi interface

After reading the ESP32 IP address from your router, browser, or serial debug:

```text
GET http://DEVICE_IP/status
GET http://DEVICE_IP/command?mode=wind
GET http://DEVICE_IP/command?mode=rain
GET http://DEVICE_IP/command?mode=auto
GET http://DEVICE_IP/weather?wind=12.4&rain=0
```

`/weather` is intentionally provider-neutral. A laptop, phone, dashboard, or
later cloud function can fetch a weather service and forward only wind/rain
values to the device. Local tracking and cooling continue without Wi-Fi.

## Heat-map telemetry

Start the dashboard with `cd webapp && python3 server.py`, then set
`TELEMETRY_URL` to `http://<laptop-LAN-IP>:8080/api/telemetry`. Set `NODE_ID`,
`NODE_MAP_X`, and `NODE_MAP_Y` in `Config.h` for each ESP. Every ten
seconds the sketch can POST a JSON record containing location, temperature
status, occupancy, light readings, tracking errors, and system state.

The current 37-in-1 digital threshold module cannot supply a numerical
temperature, so `temperatureC` is sent as `null`. For an actual heat map, add a
calibrated, shaded, ventilated I2C temperature/humidity sensor such as an SHT31
or SHT40. I2C devices can share the existing SDA/SCL pair with a future INA219.

## Multimeter-only solar demo

A multimeter is enough for snapshots:

- Measure panel voltage **across** a fixed load.
- Measure current by moving the meter lead to its current jack and inserting the
  meter **in series** with that load.
- Compute `power_W = voltage_V * current_A`.
- Record one sample every fixed interval and estimate energy with
  `energy_Wh += power_W * interval_seconds / 3600`.

An easier and safer one-meter method is to use a known resistor as the load and
measure only its voltage. Then calculate `current_A = voltage_V / resistance_ohm`
and `power_W = voltage_V * voltage_V / resistance_ohm`. Choose a resistor with a
power rating comfortably above the calculated power. If the panel label gives
`Vmp` and `Imp`, a useful first load is approximately `R = Vmp / Imp`.

Do not place a meter in current mode directly across the solar panel; that is a
short circuit. Do not use open-circuit voltage alone as an energy measurement.
For a defensible comparison, use the same load, lamp geometry, sampling interval,
and trial duration for fixed and tracking runs.
