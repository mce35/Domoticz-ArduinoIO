# ArduinoIO Python plugin for Domoticz
#
#
"""
<plugin key="arduino-io" name="ArduinoIO" author="mce35" version="0.1.0">
    <description>
        <h1>Plugin for general purpose I/O based on Arduino UNO</h1><br/>
        <h2>Parameters</h2>
    </description>
    <params>
        <param field="SerialPort" label="Serial Port" width="300px" required="true" default="/dev/ttyUSB0" >
            <description><br/>Set the serial port where the Arduino UNO board is connected (/dev/ttyUSB0 for example)</description>
        </param>
        <param field="Mode6" label="Debug" width="75px">
          <options>
            <option label="True" value="Debug"/>
            <option label="False (default)" value="Normal" default="true" />
          </options>
        </param>
    </params>
</plugin>
"""
import re
import time
import Domoticz

class BasePlugin:
    nextConnect = 3
    oustandingPings = 0
    
    UNIT_OFFSET_OUTPUT = 1
    NB_OUTPUTS = 5
    UNIT_OFFSET_INPUT = 10
    NB_INPUTS = 2
    UNIT_OFFSET_ANALOG_INPUT = 20
    UNIT_OFFSET_RAW_ANALOG_INPUT = 30
    NB_ANALOG_INPUTS = 1

    def __init__(self):
        self.num_outputs = 7
        self.serial_port = None
        self.last_heartbeat = time.time()
        return

    def onStart(self):
        
        # Domoticz.Debug("onStart called")
        if Parameters["Mode6"] == "Debug":
            Domoticz.Debugging(1)
        
        self.serial_port = Domoticz.Connection(Name="ArduinoIO", Transport="Serial", Protocol="Line", Address=Parameters["SerialPort"], Baud=19200)
        self.serial_port.Connect()

        # types / subtypes reference: https://github.com/domoticz/domoticz/blob/master/hardware/hardwaretypes.h
        for x in range(0, self.NB_OUTPUTS):
            unit = x + self.UNIT_OFFSET_OUTPUT
            if (unit not in Devices):
                Domoticz.Log("Creating device Output_"+str(x))
                Domoticz.Device(Name="Output_"+str(x), Unit=unit, TypeName="Switch", Used=1).Create()
            else:
                Domoticz.Debug("Device Output_"+str(x)+" already exists")
        for x in range(0, self.NB_INPUTS):
            unit = x + self.UNIT_OFFSET_INPUT
            if (unit not in Devices):
                Domoticz.Log("Creating device Input_"+str(x))
                Domoticz.Device(Name="Input_"+str(x), Unit=unit, TypeName="Switch", Used=1).Create()
            else:
                Domoticz.Debug("Device Input_"+str(x)+" already exists")
        for x in range(0, self.NB_ANALOG_INPUTS):
            unit = x + self.UNIT_OFFSET_ANALOG_INPUT
            if (unit not in Devices):
                Domoticz.Log("Creating device Analog_input_"+str(x))
                Domoticz.Device(Name="Analog_input_"+str(x), Unit=unit, TypeName="Switch", Used=1).Create()
            else:
                Domoticz.Debug("Device Analog_input_"+str(x)+" already exists")
            unit = x + self.UNIT_OFFSET_RAW_ANALOG_INPUT
            if (unit not in Devices):
                Domoticz.Log("Creating device Raw_analog_input_"+str(x))
                Domoticz.Device(Name="Raw_analog_input_"+str(x), Unit=unit, TypeName="General", Subtype=8, Used=1).Create()
            else:
                Domoticz.Debug("Device Analog_input_"+str(x)+" already exists")

        Domoticz.Heartbeat(10)

    def onStop(self):
        Domoticz.Debug("onStop called")
        self.serial_port.Disconnect()
        self.serial_port = None

    def initPorts(self, doReset):
        cmd = ""
        if doReset:
            cmd += "RSTX"
        cmd += "VERS" + "CNI" + str(self.NB_INPUTS) + "CNO" + str(self.NB_OUTPUTS) + "CNA" + str(self.NB_ANALOG_INPUTS)
        for i in range(0, self.NB_OUTPUTS):
            cmd += "CL" + str(i) + "1"
        cmd += "STAR"
        self.serial_port.Send(cmd)

    def onConnect(self, Connection, Status, Description):
        if (Status == 0):
            Domoticz.Log("Connected successfully to: " + Parameters["SerialPort"])
            self.serial_port = Connection
            self.initPorts(True)
        else:
            Domoticz.Log("Failed to connect (" + str(Status) + ") to: " + Parameters["SerialPort"] + " with error: " + Description)
        return True

    def onMessage(self, Connection, Data):
        msg = str(Data, 'utf-8')
        Domoticz.Debug("Message: '" + msg + "'")
        if msg == "Reset":
            self.initPorts(False)
        elif msg == "Heartbeat":
            self.last_heartbeat = time.time()
        else:
            m = re.search(r'^O([0-9]+)=([0-9])$', msg)
            if m:
                out_num = int(m.group(1)) + self.UNIT_OFFSET_OUTPUT
                if (out_num in range(self.UNIT_OFFSET_OUTPUT, self.UNIT_OFFSET_OUTPUT + self.NB_OUTPUTS + 1)):
                    val = m.group(2)
                    Devices[out_num].Update(int(val),val)
                else:
                    Domoticz.Error("Output number invalid: '" + msg + "'")
            else:
                m = re.search(r'^I([0-9]+)=([0-9])$', msg)
                if m:
                    in_num = int(m.group(1)) + self.UNIT_OFFSET_INPUT
                    if (in_num in range(self.UNIT_OFFSET_INPUT, self.UNIT_OFFSET_INPUT + self.NB_INPUTS + 1)):
                        val = m.group(2)
                        Devices[in_num].Update(int(val),val)
                    else:
                        Domoticz.Error("Input number invalid: '" + msg + "'")
                else:
                    m = re.search(r'^A([0-9])D=([0-9])$', msg)
                    if m:
                        in_num = int(m.group(1)) + self.UNIT_OFFSET_ANALOG_INPUT
                        if (in_num in range(self.UNIT_OFFSET_ANALOG_INPUT, self.UNIT_OFFSET_ANALOG_INPUT + self.NB_ANALOG_INPUTS + 1)):
                            val = m.group(2)
                            Devices[in_num].Update(int(val),val)
                        else:
                            Domoticz.Error("Analog input number invalid: '" + msg + "'")
                    else:
                        m = re.search(r'^A([0-9])=([0-9]+)$', msg)
                        if m:
                            in_num = int(m.group(1)) + self.UNIT_OFFSET_RAW_ANALOG_INPUT
                            if (in_num in range(self.UNIT_OFFSET_RAW_ANALOG_INPUT, self.UNIT_OFFSET_RAW_ANALOG_INPUT + self.NB_ANALOG_INPUTS + 1)):
                                val = float(m.group(2))
                                val = val * 5 / 1024
                                Devices[in_num].Update(int(val), str(val))
                            else:
                                Domoticz.Error("Analog input number invalid: '" + msg + "'")
                        else:
                            Domoticz.Error("Message not handled: '" + msg + "'")
        return

    # Called when a command is received from Domoticz. The Unit parameters matches the Unit specified in the device definition and should be used to map commands
    # to Domoticz devices. Level is normally an integer but may be a floating point number if the Unit is linked to a thermostat device. This callback should be
    # used to send Domoticz commands to the external hardware. 
    def onCommand(self, Unit, Command, Level, Hue):
        Domoticz.Debug("onCommand called. Unit: " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level) + ", Hue:" + str(Hue))
        if Unit in range(self.UNIT_OFFSET_OUTPUT, self.UNIT_OFFSET_OUTPUT + self.NB_OUTPUTS + 1):
            val = '0'
            if str(Command) == "On":
                val = '1'
            cmd = 'P' + str(Unit - 1) + str(val) + 'X'
            Domoticz.Debug("Send command '" + cmd + "'")
            self.serial_port.Send(cmd)
        elif Unit in range(self.UNIT_OFFSET_INPUT, self.UNIT_OFFSET_INPUT + self.NB_INPUTS + 1):
            Domoticz.Error("Cannot change input Input_" + str(Unit-self.UNIT_OFFSET_INPUT) + ' state')

    def onNotification(self, Name, Subject, Text, Status, Priority, Sound, ImageFile):
        Domoticz.Debug("Notification: " + Name + "," + Subject + "," + Text + "," + Status + "," + str(Priority) + "," + Sound + "," + ImageFile)

    def onDisconnect(self, Connection):
        for Device in Devices:
            Devices[Device].Update(nValue=Devices[Device].nValue, sValue=Devices[Device].sValue, TimedOut=1)
        Domoticz.Log("Connection '" + Connection.Name + "' disconnected.")

    def onHeartbeat(self):
        Domoticz.Debug("onHeartbeat called")
        if self.serial_port is not None:
            if self.serial_port.Connected():
                cmd = ""
                for i in range(0, self.NB_ANALOG_INPUTS):
                    cmd += "GA" + str(i) + "X"
                self.serial_port.Send(cmd)
                last_heart = time.time() - self.last_heartbeat
                if last_heart > 30:
                    Domoticz.Error('Last heartbeat was ' + str(last_heart) + ' seconds ago, reconnecting')
                    self.serial_port.Disconnect()
            else:
                Domoticz.Error('Transport is not connected, reconnecting')
                self.serial_port.Connect()


global _plugin
_plugin = BasePlugin()

def onStart():
    global _plugin
    _plugin.onStart()

def onStop():
    global _plugin
    _plugin.onStop()

def onConnect(Connection, Status, Description):
    global _plugin
    _plugin.onConnect(Connection, Status, Description)

def onMessage(Connection, Data):
    global _plugin
    _plugin.onMessage(Connection, Data)

def onCommand(Unit, Command, Level, Hue):
    global _plugin
    _plugin.onCommand(Unit, Command, Level, Hue)

def onNotification(Name, Subject, Text, Status, Priority, Sound, ImageFile):
    global _plugin
    _plugin.onNotification(Name, Subject, Text, Status, Priority, Sound, ImageFile)

def onDisconnect(Connection):
    global _plugin
    _plugin.onDisconnect(Connection)

def onHeartbeat():
    global _plugin
    _plugin.onHeartbeat()
