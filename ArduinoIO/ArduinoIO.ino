// pin 2-8: Output
// pin 9-12: Input
// pin 13: LED
const int g_iFirstOutput = 2;
const int g_iFirstInput = 9;
const int g_iMaxOutput = g_iFirstInput - g_iFirstOutput;
const int g_iMaxInput = 4;
const int g_iMaxAnalogIn = 6;
const int g_iInputCheckInterval = 100;
const int g_iHeartbeatInterval = 5000;

unsigned long g_ulLastStateCheckMillis = 0;
unsigned long g_ulFirstSerialByteMillis = 0;
unsigned long g_ulLastLedBlinkMillis = 0;
unsigned long g_ulLastHeartbeatMillis = 0;
uint8_t g_tui8LastState[g_iMaxInput];
char g_tcSerialcmd[4];
char g_uiSerialBytesRead = 0;
uint16_t g_tui16LowThresh[g_iMaxAnalogIn];
uint16_t g_tui16HighThresh[g_iMaxAnalogIn];
uint8_t g_ui8AnalogState[g_iMaxAnalogIn];

int g_iNbInput = g_iMaxInput;
int g_iNbOutput = g_iMaxOutput;
int g_iNbAnalogIn = g_iMaxAnalogIn;
uint8_t g_tui8ActiveLow[g_iMaxOutput];
int g_iBlinkInterval = 1000;

uint8_t g_ui8Initialized = 0;

void reset()
{
  for (int pin = g_iFirstOutput; pin < g_iFirstOutput + g_iMaxOutput; pin++)
    pinMode(pin, INPUT);
  for (int pin = g_iFirstInput; pin < g_iFirstInput + g_iMaxInput; pin++)
    pinMode(pin, INPUT);
  for (int i = 0; i < g_iMaxOutput; i++)
    g_tui8ActiveLow[i] = 0;
  g_iNbInput = g_iMaxInput;
  g_iNbOutput = g_iMaxOutput;
  g_iNbAnalogIn = g_iMaxAnalogIn;
  g_iBlinkInterval = 1000;
  g_ui8Initialized = 0;
  Serial.println("Reset");
}

void printConf()
{
  Serial.print("NbInputs=");
  Serial.println(g_iNbInput);
  Serial.print("NbOutputs=");
  Serial.println(g_iNbOutput);
  Serial.print("NbAnalog=");
  Serial.println(g_iNbAnalogIn);
  for (int i = 0; i < g_iNbOutput; i++)
  {
    Serial.print("ActiveLow ");
    Serial.print(i);
    Serial.print("=");
    Serial.println(g_tui8ActiveLow[i]);
  }
}

void initPorts()
{
  for (int pin = g_iFirstOutput; pin < g_iFirstOutput + g_iNbOutput; pin++)
  {
    digitalWrite(pin, 0 ^ g_tui8ActiveLow[pin - g_iFirstOutput]);
    pinMode(pin, OUTPUT);
  }
  for (int pin = g_iFirstInput; pin < g_iFirstInput + g_iNbInput; pin++)
  {
    pinMode(pin, INPUT_PULLUP);
    g_tui8LastState[pin -g_iFirstInput] = digitalRead(pin);
  }
  for (int i = 0; i < g_iNbAnalogIn; i++)
  {
    g_tui16LowThresh[i] = 200;
    g_tui16HighThresh[i] = 700;
    int val = analogRead(A0 + i);
    if (val >= g_tui16HighThresh[i])
      g_ui8AnalogState[i] = 1;
    else
      g_ui8AnalogState[i] = 0;
  }
  g_iBlinkInterval = 250;
  g_ui8Initialized = 1;
  printConf();
  g_ulLastHeartbeatMillis = millis();
}

void setup()
{
  delay(500);
  // initialize serial communications at 9600 bps:
  Serial.begin(19200);
  Serial.setTimeout(1);
  pinMode(13, OUTPUT);
  g_ulLastStateCheckMillis = millis();
  g_ulLastLedBlinkMillis = millis();
  g_ulLastHeartbeatMillis = millis();
  reset();
}

void handleGet(const char *cmd)
{
  int pin;
  if (cmd[1] == 'A')
  {
    switch (cmd[2])
    {
    case '0': pin = A0; break;
    case '1': pin = A1; break;
    case '2': pin = A2; break;
    case '3': pin = A3; break;
    case '4': pin = A4; break;
    case '5': pin = A5; break;
    default:  pin = A0; break;
    }
    int val = analogRead(pin);
    Serial.print('A');
    Serial.print(pin - A0);
    Serial.print('=');
    Serial.println(val);
    // delay 1ms to let the ADC recover:
    delay(1);
  }
  else
  {
    pin = cmd[2] - '0' + g_iFirstInput;
    if (pin < g_iFirstInput || pin >= g_iFirstInput + g_iNbInput)
      pin = g_iFirstInput;
    int val = digitalRead(pin);
    Serial.print('I');
    Serial.print(pin - g_iFirstInput);
    Serial.print('=');
    Serial.println(val);
  }
}

void handlePut(char *cmd)
{
  int pin = cmd[1] - '0' + g_iFirstOutput;
  if (pin < g_iFirstOutput || pin >= g_iFirstOutput + g_iNbOutput)
  {
    return;
  }
  uint8_t val;
  if (cmd[2] == '1')
    val = 1;
  else
    val = 0;
  digitalWrite(pin, val ^ g_tui8ActiveLow[pin - g_iFirstOutput]);
  Serial.print('O');
  Serial.print(pin - g_iFirstOutput);
  Serial.print('=');
  Serial.println(val);
}

void handleStatus()
{
  for (uint8_t i = 0; i < g_iNbOutput; i++)
  {
    uint8_t val = digitalRead(i + g_iFirstOutput);
    Serial.print('O');
    Serial.print(i);
    Serial.print('=');
    Serial.println(val ^ g_tui8ActiveLow[i]);
  }
  for (uint8_t i = 0; i < g_iNbInput; i++)
  {
    uint8_t val = digitalRead(i + g_iFirstInput);
    Serial.print('I');
    Serial.print(i);
    Serial.print('=');
    Serial.println(val);
  }
  for (int i = 0; i < g_iNbAnalogIn; i++)
  {
    int pin = A0 + i;
    int val = analogRead(pin);
    Serial.print('A');
    Serial.print(pin - A0);
    Serial.print('=');
    Serial.println(val);
    // delay 1ms to let the ADC recover:
    delay(1);
  }
}

void handleVersion()
{
  Serial.println("ArduinoIO-v1.0");
}

void checkStateChanged()
{
  // Digital inputs
  for (uint8_t i = 0; i < g_iNbInput; i++)
  {
    uint8_t val = digitalRead(i + g_iFirstInput);
    uint8_t ui8OldVal = g_tui8LastState[i];
    if (val != ui8OldVal)
    {
      Serial.print('I');
      Serial.print(i);
      Serial.print('=');
      Serial.println(val);
      g_tui8LastState[i] = val;
    }
  }

  // Analog inputs
  for (int i = 0; i < g_iNbAnalogIn; i++)
  {
    int val = analogRead(A0 + i);
    bool bChanged = 0;
    if (g_ui8AnalogState[i] == 0 && val >= g_tui16HighThresh[i])
    {
      g_ui8AnalogState[i] = 1;
      bChanged = 1;
    }
    else if (g_ui8AnalogState[i] == 1 && val <= g_tui16LowThresh[i])
    {
      g_ui8AnalogState[i] = 0;
      bChanged = 1;
    }
    if (bChanged != 0)
    {
      Serial.print('A');
      Serial.print(i);
      Serial.print("D=");
      Serial.println(g_ui8AnalogState[i]);
    }
    // delay 1ms to let the ADC recover:
    delay(1);
  }
}

void handleConf(char *cmd)
{
  if(cmd[1] == 'L')
  {
    // CLNV: Set output pin N as active low, val=V
    int pin = cmd[2] - '0';
    if (pin < 0 || pin >= g_iNbOutput)
    {
      return;
    }
    g_tui8ActiveLow[pin] = cmd[3] == '0' ? 0 : 1;
  }
  else if (cmd[1] == 'N')
  {
    // CN[IOA]V: Set number of Inputs/Ouptus/Analog to V
    int iNb = cmd[3] - '0';
    switch (cmd[2])
    {
    case 'I':
      if (iNb >= 0 && iNb < g_iMaxInput)
        g_iNbInput = iNb;
      break;
    case 'O':
      if (iNb >= 0 && iNb < g_iMaxOutput)
        g_iNbOutput = iNb;
      break;
    case 'A':
      if (iNb >= 0 && iNb < g_iMaxAnalogIn)
        g_iNbAnalogIn = iNb;
      break;
    }
  }
}

uint8_t g_ui8LastLedState = 0;

void loop()
{
  int bytes;
  unsigned long ulCurrentTime = millis();

  if (ulCurrentTime - g_ulLastLedBlinkMillis > g_iBlinkInterval)
  {
    digitalWrite(13, g_ui8LastLedState);
    g_ui8LastLedState = !g_ui8LastLedState;
    g_ulLastLedBlinkMillis += g_iBlinkInterval;
    if (g_ui8Initialized == 0)
      Serial.println("Reset");
  }

  if (g_ui8Initialized != 0 && ulCurrentTime - g_ulLastHeartbeatMillis > g_iHeartbeatInterval)
  {
    g_ulLastHeartbeatMillis = ulCurrentTime;
    Serial.println("Heartbeat");
  }

  if (g_ui8Initialized != 0 && ulCurrentTime - g_ulLastStateCheckMillis > g_iInputCheckInterval)
  {
    checkStateChanged();
    g_ulLastStateCheckMillis += g_iInputCheckInterval;
    // handleGet("GA0X");
  }
  if (Serial.available() > 0)
  {
    if (g_uiSerialBytesRead == 0)
      g_ulFirstSerialByteMillis = ulCurrentTime;
    g_tcSerialcmd[g_uiSerialBytesRead] = Serial.read();
    g_uiSerialBytesRead++;
    if (g_uiSerialBytesRead == 4)
    {
      if (g_ui8Initialized != 0)
      {
        if (g_tcSerialcmd[0] == 'G')
          handleGet(g_tcSerialcmd);
        else if (g_tcSerialcmd[0] == 'P')
          handlePut(g_tcSerialcmd);
        else if (strncmp(g_tcSerialcmd, "STXX", 4) == 0)
          handleStatus();
      }
      if (strncmp(g_tcSerialcmd, "VERS", 4) == 0)
        handleVersion();
      else if (strncmp(g_tcSerialcmd, "RSTX", 4) == 0)
        reset();
      else if (strncmp(g_tcSerialcmd, "STAR", 4) == 0)
      {
        initPorts();
        handleStatus();
      }
      else if (g_tcSerialcmd[0] == 'C')
        handleConf(g_tcSerialcmd);
      g_uiSerialBytesRead = 0;
    }
  }
  else if (g_uiSerialBytesRead > 0 && ulCurrentTime - g_ulFirstSerialByteMillis > 50)
  {
    g_uiSerialBytesRead = 0;
  }
}
