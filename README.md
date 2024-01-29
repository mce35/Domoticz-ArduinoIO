# Domoticz-ArduinoIO-Plugin

A simple Domoticz Python plugin to control GPIO using an Arduino UNO R3 board.

The plugin allows to configure 7 digital outputs, 4 digital inputs and 6 analog inputs.

The analog inputs also creates digital inputs using a software Schmitt trigger.

## Plugin Parameters

| Field | Information|
| ----- | ---------- |
| Port  | The serial port used by the Arduino UNO board |
| Debug | When true the logging level will be much higher to help with troubleshooting |

## How to use

- Connect the Arduino UNO board using an USB cable.
- Compile/upload the firmware found in the `Arduino.ino` file using the Arduino IDE.
- Add a new `ArduinoIO` hardware plugin in domoticz
- New Domoticz widgets are created by the plugin.

> [!TIP]
> The number of inputs/outputs/analog inputs actually used can be configured in the plugin using the variables `NB_INPUTS`, `NB_OUTPUTS` and `NB_ANALOG_INPUTS` in the `plugin.py` file.

> [!TIP]
> Output pins can be configured as active low (setting it to 0 actually makes it HIGH). This is hardcoded in the `plugin.py` file in the `initPorts()` method.

> [!WARNING]
> This is a very basic usage of the Arduino UNO board. The protocol is very simple and not reliable. It may not work at all times.

The pins used are:

- D2-D8: Digital outputs
- D9-D12: Digital inputs
- A0-A6: Analog inputs

## Protocol

The protocol used to control the Arduino UNO is a text protocol on the serial port of the board at 19200 bauds. Each command is composed of 4 characters, and each response consists of a single line.

Commands and responses
1. `VERS`: Returns the firmware version
    - `ArduinoIO-v1.0`
2. `RSTX`: Reset the ports (all pins are set to input)
    - `Reset`: Indicates that the board is not configured. This message is emitted every second.
3. ``CN[IOA]V``: Set number of Inputs/Outputs/Analog inputs to V
    - No response sent
4. `CLNV`: Set output pin N as active low, val=V (example: `CL01` sets output 0 as active low - setting it to 0 makes it HIGH)
    - No response sent
5. `STAR`: Start the output ports (all outputs set to 0 (low or high depending on the active low configuration))
    - `NbInputs=V`: Display the number of inputs configured
    - `NbOutputs=V`: Display the number of outputs configured
    - `NbAnalog=V`: Display the number of analog inputs configured
    - `ActiveLow N=V`: Display the active low status of output N
    - Followed by the status of all ports (see `STXX`)
6. `GANX`: Gets the value of analog intput N (example: `GA0X`)
    - `AN=V`: example `A0=123` (0<=V<=1023)
7. `GDNX`: Gets the value of digital input N (example: `GD0X`)
    - `IN=V`: example `I0=1` (V=0 or 1)
8. `PNVX`: Set the value of digital output N to V (example: `P01X` sets output 0 to 1)
    - `ON=V`: example `O0=1` (V=0 or 1)
9. `STXX`: Display the status of all configured GPIO
    - `ON=V`: display the status of all outputs
    - `IN=V`: display the status of all inputs
    - `AN=V`: display the value of all analog inputs

Commands 6 to 9 cannot be sent before configuring the board with `STAR`.

When an input changes (digital input or analog input with Schmitt trigger), the following response is sent by the board:
    - `IN=V`: example `I0=1` (V=0 or 1), for digital inputs
    - `AND=V`: example `A0D=1` (V=0 or 1), for analog inputs

## TODO

- Allow to configure the number of Inputs/Outputs/Analog inputs by other mean than editing the plugin
- Allow to configure active low pins by other mean than editing the plugin
- Allow to configure the thresholds of the Schmitt trigger
- Make protocol more reliable
