/*
********************************************************************************
  Advanced Tuner software for NXP TEF668x ESP32
   (c) 2023 Sjef Verhoeven pe5pvb@het-bar.net & Noobish noobish@noobish.eu
    Based on Catena / NXP semiconductors API
********************************************************************************
  Analog signalmeter:
                       to meter
                          |
                R         V
  IO27 -------=====-----=====---|
               1 k     5 k POT

********************************************************************************
  Setup:
  During boot:
  Hold BW button to change rotary encoder direction
  Hold Mode button the flip screen
  Hold Standby button to calibrate analogue s-meter
  Hold rotary button to switch between normal and optical rotary encoder

  Manual:
  STANDBY SHORT PRESS: Switch band
  STANDBY LONG PRESS: Standby
  ROTARY SHORT PRESS: Set stepsize or navigate
  ROTARY LONG PRESS: Toggle iMS & EQ
  MODE SHORT PRESS: Switch between auto/manual
                    or exit menu
  MODE LONG PRESS: Open menu
  BW SHORT PRESS: Switch bandwidth setting
  BW LONG PRESS: Switch mono, or auto stereo
* ***********************************************
  ATTENTION: If you want to compile this code, use Arduino Core for ESP32 v1.0.6 - newer versions won't work!
* ***********************************************
  Use these settings in TFT_eSPI User_Setup.h
  #define ILI9341_DRIVER
  #define TFT_CS          5
  #define TFT_DC          17
  #define TFT_RST         16
  #define LOAD_GLCD
  #define LOAD_FONT2
  #define LOAD_FONT4
  #define LOAD_FONT6
  #define LOAD_FONT7
  #define LOAD_FONT8
  #define LOAD_GFXFF
  #define SMOOTH_FONT
  #define SPI_FREQUENCY   10000000 (if you hear interference, consider lowering the frequency)

  ALL OTHER SETTINGS SHOULD STAY !!!!

*/

#include "src/TEF6686.h"
#include "src/constants.h"
#include <EEPROM.h>
#include <Wire.h>
#include "SPIFFS.h"
#include <analogWrite.h>        // https://github.com/ERROPiX/ESP32_AnalogWrite
#include <TFT_eSPI.h>           // https://github.com/Bodmer/TFT_eSPI
#include <RotaryEncoder.h>      // https://github.com/enjoyneering/RotaryEncoder
#include <Hash.h>               // https://github.com/bbx10/Hash_tng
#include <WiFiConnect.h>        // https://registry.platformio.org/libraries/mrfaptastic/WiFiConnect%20Lite
#include <ESPAsyncWebServer.h>  // https://github.com/me-no-dev/ESPAsyncWebServer
#include <AsyncTCP.h>           // https://github.com/me-no-dev/AsyncTCP

/* Button -> PIN connections */
#define ROTARY_PIN_A 34
#define ROTARY_PIN_B 36
#define ROTARY_BUTTON 39
#define PIN_POT 35
#define PWRBUTTON 4
#define BWBUTTON 25
#define MODEBUTTON 26
#define CONTRASTPIN 2
#define STANDBYLED 19
#define SMETERPIN 27
#define BATTERYPIN 13

#define BWINTERVAL 200  // Refresh interval for certain UI elements

//#define ARS       // uncomment for BGR type display (ARS version)
#ifdef ARS
#define VERSION "v1.22ARS"
#include "TFT_Colors.h"
TFT_eSPI tft = TFT_eSPI(320, 240);
#else
#define VERSION "v1.22"
TFT_eSPI tft = TFT_eSPI(240, 320);
#endif
RotaryEncoder encoder(ROTARY_PIN_A, ROTARY_PIN_B, ROTARY_BUTTON);
int16_t position = 0;

void ICACHE_RAM_ATTR encoderISR() {
  encoder.readAB();
}
void ICACHE_RAM_ATTR encoderButtonISR() {
  encoder.readPushButton();
}

bool tuned;
bool setupmode;
bool direction;
bool seek;
bool screenmute = false;
bool power = true;
bool change2;
bool BWreset;
bool menu;
bool menu2;
bool setupmenu;
bool menuopen = false;
bool menu2open = false;
bool LowLevelInit = false;
bool RDSstatusold;
bool SQ;
bool Stereostatusold;
bool StereoToggle = true;
bool store;
bool tunemode = false;
bool USBstatus = false;
bool WiFistatus = false;
bool wificonnect;
bool XDRMute;
bool wifisetupopen = false;
extern bool ProgramTP;
extern bool TPStatus;
byte band;
byte BWset;
byte ContrastSet;
byte displayflip;
byte freqoldcount;
byte rotarymode;
byte SStatusoldcount;
byte stepsize;
byte SignalUnits;
byte SNRold;
byte SNR;
byte LowLevelSet;
byte iMSset;
byte EQset;
byte iMSEQ;
byte TEF;
byte optenc;
byte RDSClear;
byte XDRGTKShutdown;
byte WiFiSwitch = EEPROM.readByte(63);
char buff[16];
char programServicePrevious[9];
char programTypePrevious[17];
char radioIdPrevious[4];
char radioTextPrevious[65];
int AGC;
int BWOld;
int ConverterSet;
int DeEmphasis;
int ForceMono;
int freqold;
int HighCutLevel;
int HighCutOffset;
int HighEdgeSet;
int LevelOffset;
int LowEdgeSet;
int menuoption = 30;
int menu2option = 30;
int MStatusold;
int OStatusold;
int peakholdold;
int peakholdtimer;
int lowsignaltimer;
int scanner_filter;
int Sqstatusold;
int Squelch;
int Squelchold;
int SStatusold;
int status = WL_IDLE_STATUS;
int StereoLevel;
int Stereostatus;
int VolSet;
int volume;
int XDRBWset;
int XDRBWsetold;

// Theme Engine Colors
int PrimaryColor;
int SecondaryColor;
int FrameColor;
int FrequencyColor;
int GreyoutColor;
int BackgroundColor;
int ActiveColor;
int OptimizerColor;
int RDSColor;
int StereoColor;
int CurrentTheme;

// Scrolling RT settings
int xPos = 6;
int yPos = 2;
int charWidth = tft.textWidth("AA");

uint16_t BW;
uint16_t MStatus;
int16_t OStatus;
int16_t SStatus;
uint16_t USN;
uint16_t WAM;
String ContrastString;
String ConverterString;
String CurrentThemeString;
String SignalUnitsString;
String cryptedpassword;
String HighCutLevelString;
String HighCutOffsetString;
String HighEdgeString;
String hostname = "ESP32 TEF6686 FM Tuner";
String LevelOffsetString;
String LowEdgeString;
String LowLevelString;
String password = "12345";
String salt;
String saltkey = "                ";
String PIold;
String PSold;
String PTYold;
String RDSClearString;
String RTold;
String StereoLevelString;
String VolString;
String XDRGTKShutdownString;
String WiFiSwitchString;

// Extra settings strings
const char* SignalUnitsStrings[] = { "dBuV", "dBf" };
const char* RDSClearStrings[] = { "OFF", "ON" };
const char* XDRGTKShutdownStrings[] = { "OFF", "ON" };
const char* WiFiSwitchStrings[] = { "OFF", "ON" };

// Wi-Fi setup
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";
String ssid;
String pass;
String ip;
String gateway;
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";
IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 255, 0);
unsigned long previousMillis = 0;
const long interval = 2500;  // interval to wait for Wi-Fi connection (milliseconds)

uint16_t rdsB;
uint16_t rdsC;
uint16_t rdsD;
uint16_t rdsErr;
uint8_t buff_pos = 0;
uint8_t RDSstatus;
int16_t SAvg;
unsigned int change;
unsigned int freq_scan;
unsigned int frequency;
unsigned int frequency_AM;
unsigned int frequencyold;
unsigned int scanner_end;
unsigned int scanner_start;
unsigned int scanner_step;
unsigned long peakholdmillis;
unsigned long BWCLOCK = 0;
unsigned long RTCLOCK = 0;
unsigned long TPCLOCK = 0;
unsigned long VCLOCK = 0;

// Battery calculation
const int MAX_ANALOG_VAL = 4095;
const float MIN_BATTERY_VOLTAGE = 3.15;
const float MAX_BATTERY_VOLTAGE = 4.05;
int rawValue = analogRead(BATTERYPIN);
float voltageLevel = (rawValue / 4095.0) * 2 * 1.1 * 3.3 - 0.2;  // calculate voltage level
float batteryFraction = (voltageLevel - MIN_BATTERY_VOLTAGE) / (MIN_BATTERY_VOLTAGE / MAX_BATTERY_VOLTAGE);
float batteryPercentage = batteryFraction * 100;

// Wi-Fi Setup Webpage HTML Code
const char index_html[] PROGMEM = "<!DOCTYPE html><html><head> <title>TEF6686 Wi-Fi Setup</title> <meta name='viewport' content='width=device-width, initial-scale=1'> <link rel='stylesheet' type='text/css' href='style.css'> <style> html { font-family: Arial, Helvetica, sans-serif;  display: inline-block;  text-align: center;}h1 { font-size: 1.8rem;  color: #252525;}.topnav {  overflow: hidden;  background-color: #00ea8f; }body {  margin: 0; background: #252525;}.content {  padding: 5%;}.card {  max-width: 800px;  margin: 0 auto;  background-color: white;  padding: 5px 5% 5px 5%;}input[type=submit] { color: #252525; background-color: #00ea8f; border: 3px solid #252525; cursor: pointer; font-weight: bold; padding: 15px 15px; text-align: center; text-decoration: none; font-size: 16px; width: 160px; margin-top: 20px; margin-bottom: 10px; border-radius: 8px; transition: .3s ease-in-out; }input[type=submit]:hover { background-color: #252525; border: 3px solid #00ea8f; color: #00ea8f;}input[type=text], input[type=number], select { width: 100%; padding: 12px 20px; border: 1px solid #ccc; box-sizing: border-box;}a { text-decoration: none; color: #00ea8f;}p { font-size: 14px; text-transform: uppercase;  text-align: left; margin: 0; margin-top: 15px;}.card p{ font-weight: bold;}.gateway-info{ text-transform: initial; text-align: center; color: white;} </style></head><body> <div class='topnav'> <h1>TEF6686 Wi-Fi SETUP</h1> </div> <div class='content'> <div class='card'> <form action='/' method='POST'> <p>SSID:</p> <input type='text' id ='ssid' name='ssid' placeholder='The exact name of your Wi-Fi network'><br> <p>Password:</p> <input type='text' id ='pass' name='pass' placeholder='Password to your Wi-Fi network'><br> <p>IP Address:</p> <input type='text' id ='ip' name='ip' placeholder='Example: 192.168.1.200'><br> <p>Gateway:</p> <input type='text' id ='gateway' name='gateway' placeholder='Example: 192.168.1.1'><br> <input type ='submit' value ='SUBMIT'> </form> </div> <p class='gateway-info'>If you're unsure, you can <a href='https://www.noip.com/support/knowledgebase/finding-your-default-gateway/' target='_blank'>look up how to get your gateway</a>.</p> </div></body></html>";

TEF6686 radio;
RdsInfo rdsInfo;
AsyncWebServer setupserver(80);
WiFiServer server(7373);
WiFiClient RemoteClient;
TFT_eSprite sprite = TFT_eSprite(&tft);

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
}

// Read File from SPIFFS
String readFile(fs::FS& fs, const char* path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available()) {
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS& fs, const char* path, const char* message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}

bool initWiFi() {
  if (ssid == "" || ip == "") {
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  if (ip != "") {
    localIP.fromString(ip.c_str());
  }

  if (gateway != "") {
    localGateway.fromString(gateway.c_str());
  }

  if (!WiFi.config(localIP, localGateway, subnet)) {
    Serial.println("STA Failed to configure");
    return false;
  }

  Serial.println(WiFi.localIP());
  return true;
}

void setup() {
  setupmode = true;
  EEPROM.begin(64);
  if (EEPROM.readByte(41) != 15) {
    EEPROM.writeByte(2, 0);
    EEPROM.writeByte(3, 0);
    EEPROM.writeUInt(0, 10000);
    EEPROM.writeInt(4, 0);
    EEPROM.writeInt(8, 0);
    EEPROM.writeInt(12, 87);
    EEPROM.writeInt(16, 108);
    EEPROM.writeInt(20, 50);
    EEPROM.writeInt(24, 0);
    EEPROM.writeInt(28, 0);
    EEPROM.writeInt(32, 70);
    EEPROM.writeInt(36, 0);
    EEPROM.writeByte(41, 15);
    EEPROM.writeByte(42, 0);
    EEPROM.writeByte(43, 20);
    EEPROM.writeByte(44, 1);
    EEPROM.writeByte(45, 1);
    EEPROM.writeByte(46, 0);
    EEPROM.writeUInt(47, 828);
    EEPROM.writeByte(52, 0);
    EEPROM.writeByte(53, 0);
    EEPROM.writeByte(54, 0);
    EEPROM.writeByte(55, 0);
    EEPROM.writeByte(56, 0);
    EEPROM.writeByte(57, 0);
    EEPROM.writeByte(58, 0);
    EEPROM.writeInt(59, 0);
    EEPROM.writeByte(63, 1);
    EEPROM.commit();
  }
  frequency = EEPROM.readUInt(0);
  VolSet = EEPROM.readInt(4);
  ConverterSet = EEPROM.readInt(8);
  LowEdgeSet = EEPROM.readInt(12);
  HighEdgeSet = EEPROM.readInt(16);
  ContrastSet = EEPROM.readInt(20);
  LevelOffset = EEPROM.readInt(24);
  StereoLevel = EEPROM.readInt(28);
  HighCutLevel = EEPROM.readInt(32);
  HighCutOffset = EEPROM.readInt(36);
  stepsize = EEPROM.readByte(42);
  LowLevelSet = EEPROM.readByte(43);
  iMSset = EEPROM.readByte(44);
  EQset = EEPROM.readByte(45);
  band = EEPROM.readByte(46);
  frequency_AM = EEPROM.readUInt(47);
  rotarymode = EEPROM.readByte(52);
  displayflip = EEPROM.readByte(53);
  TEF = EEPROM.readByte(54);
  optenc = EEPROM.readByte(55);
  SignalUnits = EEPROM.readByte(56);
  RDSClear = EEPROM.readByte(57);
  XDRGTKShutdown = EEPROM.readByte(58);
  CurrentTheme = EEPROM.readInt(59);
  WiFiSwitch = EEPROM.readByte(63);

  EEPROM.commit();
  encoder.begin();
  btStop();
  Serial.begin(115200);
  initSPIFFS();

  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile(SPIFFS, gatewayPath);
  WiFi.setHostname(hostname.c_str());

  rawValue = analogRead(BATTERYPIN);
  if (voltageLevel < 1.5) {
    voltageLevel = voltageLevel * 3.6;
  }
  delay(50);

  if (initWiFi()) {
    if (WiFiSwitch == 0) {
      WiFi.mode(WIFI_OFF);
    } else {
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid.c_str(), pass.c_str());
      server.begin();
    }
  }

  SignalUnitsString = SignalUnitsStrings[SignalUnits];
  RDSClearString = RDSClearStrings[RDSClear];
  XDRGTKShutdownString = XDRGTKShutdownStrings[XDRGTKShutdown];
  WiFiSwitchString = WiFiSwitchStrings[WiFiSwitch];

  if (iMSset == 1 && EQset == 1) {
    iMSEQ = 2;
  }
  if (iMSset == 0 && EQset == 1) {
    iMSEQ = 3;
  }
  if (iMSset == 1 && EQset == 0) {
    iMSEQ = 4;
  }
  if (iMSset == 0 && EQset == 0) {
    iMSEQ = 1;
  }
  tft.init();

  if (displayflip == 0) {
#ifdef ARS
    tft.setRotation(0);
#else
    tft.setRotation(3);
#endif
  } else {
#ifdef ARS
    tft.setRotation(2);
#else
    tft.setRotation(1);
#endif
  }

  TEF = EEPROM.readByte(54);

  if (TEF != 101 && TEF != 102 && TEF != 205) {
    SetTunerPatch();
  }

  radio.init(TEF);
  uint16_t device;
  uint16_t hw;
  uint16_t sw;
  radio.getIdentification(device, hw, sw);
  if (TEF != (highByte(hw) * 100 + highByte(sw))) {
    SetTunerPatch();
  }

  doTheme();
  sprite.createSprite(313, 18);

  analogWrite(CONTRASTPIN, ContrastSet * 2 + 27);
  analogWrite(SMETERPIN, 0);

  uint8_t version;
  radio.getIdentification(device, hw, sw);
  version = highByte(hw) * 100 + highByte(sw);

  pinMode(MODEBUTTON, INPUT);
  pinMode(BWBUTTON, INPUT);
  pinMode(CONTRASTPIN, OUTPUT);
  pinMode(SMETERPIN, OUTPUT);
  pinMode(BATTERYPIN, INPUT);

  if (digitalRead(BWBUTTON) == LOW) {
    if (rotarymode == 0) {
      rotarymode = 1;
    } else {
      rotarymode = 0;
    }
    EEPROM.writeByte(52, rotarymode);
    EEPROM.commit();
    tft.fillScreen(BackgroundColor);
    tft.setTextColor(SecondaryColor);
    tft.drawCentreString("Rotary direction changed", 150, 70, 4);
    tft.drawCentreString("Please release button", 150, 100, 4);
    while (digitalRead(BWBUTTON) == LOW) {
      delay(5);
    }
  }

  if (digitalRead(MODEBUTTON) == LOW) {
    if (displayflip == 0) {
      displayflip = 1;
      tft.setRotation(1);
    } else {
      displayflip = 0;
      tft.setRotation(3);
    }
    EEPROM.writeByte(53, displayflip);
    EEPROM.commit();
    tft.fillScreen(BackgroundColor);
    tft.setTextColor(SecondaryColor);
    tft.drawCentreString("Screen flipped", 150, 70, 4);
    tft.drawCentreString("Please release button", 150, 100, 4);
    while (digitalRead(MODEBUTTON) == LOW) {
      delay(50);
    }
  }

  if (digitalRead(PWRBUTTON) == LOW) {
    analogWrite(SMETERPIN, 511);
    tft.fillScreen(BackgroundColor);
    tft.setTextColor(SecondaryColor);
    tft.drawCentreString("Calibrate analog meter", 150, 70, 4);
    tft.drawCentreString("Release button when ready", 150, 100, 4);
    while (digitalRead(PWRBUTTON) == LOW) {
      delay(50);
    }
    analogWrite(SMETERPIN, 0);
  }

  if (digitalRead(ROTARY_BUTTON) == LOW) {
    tft.fillScreen(BackgroundColor);
    tft.setTextColor(SecondaryColor);
    if (optenc == 0) {
      optenc = 1;
      tft.drawCentreString("encoder set to optical", 150, 70, 4);
    } else {
      optenc = 0;
      tft.drawCentreString("encoder set to standard", 150, 70, 4);
    }
    EEPROM.writeByte(55, optenc);
    EEPROM.commit();
    tft.drawCentreString("Please release button", 150, 100, 4);
    while (digitalRead(ROTARY_BUTTON) == LOW) {
      delay(5);
    }
  }

  /* Splash screen */
  tft.setSwapBytes(true);
  tft.fillScreen(BackgroundColor);
  tft.fillCircle(160, 70, 60, PrimaryColor);
  tft.fillCircle(160, 70, 52, BackgroundColor);
  tft.drawBitmap(130, 60, TEFLogo, 59, 23, ActiveColor);
  tft.setTextColor(ActiveColor);
  tft.drawCentreString("NXP", 160, 42, 2);
  tft.drawCentreString("TEF6686", 160, 85, 2);
  tft.drawCentreString("AM/FM Receiver", 160, 145, 4);
  tft.setTextColor(PrimaryColor);
  if (CurrentTheme == 0) {
    tft.setTextColor(FrameColor);
  } else {
    tft.setTextColor(GreyoutColor);
  }
  tft.drawString(String(VERSION), 10, 220, 2);
  tft.drawRightString("Modded by Noobish.eu", 310, 205, 2);
  tft.drawRightString("Original software by PE5PVB.nl", 310, 220, 2);
  tft.fillRect(120, 180, 16, 6, GreyoutColor);
  tft.fillRect(152, 180, 16, 6, GreyoutColor);
  tft.fillRect(184, 180, 16, 6, GreyoutColor);



  if (lowByte(device) != 14 && lowByte(device) != 1 && lowByte(device) != 9 && lowByte(device) != 3) {
    tft.setTextColor(TFT_RED);
    tft.drawString("Tuner: !None! (TEF ERROR)", 160, 20, 2);
    while (true)
      ;
    for (;;)
      ;
  }

  tft.drawString("Patch: v" + String(TEF), 10, 205, 2);
  delay(200);
  tft.fillRect(120, 180, 16, 6, PrimaryColor);
  delay(375);
  tft.fillRect(152, 180, 16, 6, PrimaryColor);
  delay(375);
  tft.fillRect(184, 180, 16, 6, PrimaryColor);
  delay(375);

  radio.setVolume(VolSet);
  radio.setOffset(LevelOffset);
  radio.setStereoLevel(StereoLevel);
  radio.setHighCutLevel(HighCutLevel);
  radio.setHighCutOffset(HighCutOffset);
  radio.clearRDS();
  radio.setMute();
  LowLevelInit = true;

  if (ConverterSet >= 200) {
    Wire.beginTransmission(0x12);
    Wire.write(ConverterSet >> 8);
    Wire.write(ConverterSet & (0xFF));
    Wire.endTransmission();
  }

  SelectBand();
  ShowSignalLevel();
  ShowBW();

  setupmode = false;

  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_B), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_BUTTON), encoderButtonISR, FALLING);

  radio.setUnMute();
  if (VolSet > 10) {
    VolSet = 10;
  }
}

void loop() {
  if (digitalRead(PWRBUTTON) == LOW && USBstatus == false) {
    PWRButtonPress();
  }

  if (power == true) {

    if (seek == true) {
      Seek(direction);
    }

    if (SStatus / 10 > LowLevelSet && LowLevelInit == false && menu == false && menu2 == false && band == 0) {
      if (RDSClear == 1) {
        radio.clearRDS();
        sprite.fillSprite(BackgroundColor);
        sprite.pushSprite(6, 220);
      }
      if (screenmute == false) {
        sprite.fillSprite(BackgroundColor);
        sprite.pushSprite(6, 220);
        tft.setTextColor(SecondaryColor);
        tft.drawString("20", 20, 153, 1);
        tft.drawString("40", 50, 153, 1);
        tft.drawString("60", 80, 153, 1);
        tft.drawString("80", 110, 153, 1);
        tft.drawString("100", 134, 153, 1);
        tft.drawString("120", 164, 153, 1);
        tft.drawString("%", 196, 153, 1);
        tft.drawString("M", 6, 136, 2);
        tft.drawString("PI:", 216, 195, 2);
        tft.drawString("PS:", 6, 195, 2);
        tft.drawString("PTY:", 6, 168, 2);
        tft.drawLine(20, 150, 200, 150, TFT_DARKGREY);

      }
      LowLevelInit = true;
    }

    if (SStatus / 10 <= LowLevelSet && band == 0) {
      if (LowLevelInit == true && menu == false && menu2 == false) {
        radio.clearRDS();
        if (screenmute == false) {
          tft.setTextColor(BackgroundColor);
          if (RDSClear == 1) {
            sprite.fillSprite(BackgroundColor);
            sprite.pushSprite(6, 220);
            tft.drawString(PSold, 38, 192, 4);
            tft.drawString(PIold, 244, 192, 4);
            tft.drawString(RTold, 6, 222, 2);
            tft.drawString(PTYold, 38, 168, 2);
            tft.drawString("TP", 190, 168, 2);
          }

          tft.fillRect(20, 139, 12, 8, GreyoutColor);
          tft.fillRect(34, 139, 12, 8, GreyoutColor);
          tft.fillRect(48, 139, 12, 8, GreyoutColor);
          tft.fillRect(62, 139, 12, 8, GreyoutColor);
          tft.fillRect(76, 139, 12, 8, GreyoutColor);
          tft.fillRect(90, 139, 12, 8, GreyoutColor);
          tft.fillRect(104, 139, 12, 8, GreyoutColor);
          tft.fillRect(118, 139, 12, 8, GreyoutColor);
          tft.fillRect(132, 139, 12, 8, GreyoutColor);
          tft.fillRect(146, 139, 12, 8, GreyoutColor);
          tft.fillRect(160, 139, 12, 8, GreyoutColor);
          tft.fillRect(174, 139, 12, 8, GreyoutColor);
          tft.fillRect(188, 139, 12, 8, GreyoutColor);
          tft.setTextColor(GreyoutColor);
          tft.drawString("20", 20, 153, 1);
          tft.drawString("40", 50, 153, 1);
          tft.drawString("60", 80, 153, 1);
          tft.drawString("80", 110, 153, 1);
          tft.drawString("100", 134, 153, 1);
          tft.drawString("120", 164, 153, 1);
          tft.drawString("%", 196, 153, 1);
          tft.drawString("M", 6, 136, 2);
          tft.drawString("PI:", 216, 195, 2);
          tft.drawString("PS:", 6, 195, 2);
          tft.drawString("PTY:", 6, 168, 2);
          tft.drawLine(20, 150, 200, 150, GreyoutColor);
          tft.drawBitmap(110, 5, RDSLogo, 67, 22, GreyoutColor);
        }
        LowLevelInit = false;
      }
      if (millis() >= lowsignaltimer + 300) {
        lowsignaltimer = millis();
        if (band == 0) {
          radio.getStatus(SStatus, USN, WAM, OStatus, BW, MStatus);
        } else {
          radio.getStatus_AM(SStatus, USN, WAM, OStatus, BW, MStatus);
        }
        if (screenmute == true) {
          readRds();
        }
        if (menu == false && menu2 == false) {
          doSquelch();
        }
      }
    } else {
      if (band == 0) {
        radio.getStatus(SStatus, USN, WAM, OStatus, BW, MStatus);
      } else {
        radio.getStatus_AM(SStatus, USN, WAM, OStatus, BW, MStatus);
      }
      if (menu == false && menu2 == false) {
        doSquelch();
        readRds();
        if (screenmute == false) {
          ShowModLevel();
        }
      }
    }

    if (menu == false && menu2 == false) {
      if (screenmute == false) {
        showPI();
        showTP();
        showPTY();
        showPS();
        showRadioText();
        //showPS();
        ShowBatteryLevel();
        ShowStereoStatus();
        ShowOffset();
        ShowSignalLevel();
        ShowBW();
      
      }
    }
    XDRGTKRoutine();
    XDRCommunication();
    XDRGTKWiFi();

    if (encoder.getPushButton() == true) {
      ButtonPress();
    }

    if (menu == true && menuopen == true && menuoption == 110) {
      if (band == 0) {
        radio.getStatus(SStatus, USN, WAM, OStatus, BW, MStatus);
      } else {
        radio.getStatus_AM(SStatus, USN, WAM, OStatus, BW, MStatus);
      }
      if (millis() >= lowsignaltimer + 500 || change2 == true) {
        lowsignaltimer = millis();
        change2 = false;
        if (SStatus > SStatusold || SStatus < SStatusold) {
          String count = String(SStatus / 10, DEC);
          if (screenmute == false) {
            tft.setTextColor(BackgroundColor);
            if (SStatusold >= 0) {
              if (SStatusoldcount <= 1) {
                tft.setCursor(100, 140);
              }
              if (SStatusoldcount == 2) {
                tft.setCursor(73, 140);
              }
              if (SStatusoldcount >= 3) {
                tft.setCursor(46, 140);
              }
            } else {
              if (SStatusoldcount <= 1) {
                tft.setCursor(100, 140);
              }
              if (SStatusoldcount == 2) {
                tft.setCursor(83, 140);
              }
              if (SStatusoldcount >= 3) {
                tft.setCursor(56, 140);
              }
            }
            tft.setTextFont(6);
            tft.print(SStatusold / 10);
            tft.print(".");
            if (SStatusold < 0) {
              String negative = String(SStatusold % 10, DEC);
              if (SStatusold % 10 == 0) {
                tft.print("0");
              } else {
                tft.print(negative[1]);
              }
            } else {
              tft.print(SStatusold % 10);
            }

            tft.setTextColor(PrimaryColor, BackgroundColor);
            if (SStatus >= 0) {
              if (count.length() == 1) {
                tft.setCursor(100, 140);
              }
              if (count.length() == 2) {
                tft.setCursor(73, 140);
              }
              if (count.length() == 3) {
                tft.setCursor(46, 140);
              }
            } else {
              if (count.length() == 1) {
                tft.setCursor(100, 140);
              }
              if (count.length() == 2) {
                tft.setCursor(83, 140);
              }
              if (count.length() >= 3) {
                tft.setCursor(56, 140);
              }
            }
            tft.setTextFont(6);
            tft.print(SStatus / 10);
            tft.print(".");
            if (SStatus < 0) {
              String negative = String(SStatus % 10, DEC);
              if (SStatus % 10 == 0) {
                tft.print("0");
              } else {
                tft.print(negative[1]);
              }
            } else {
              tft.print(SStatus % 10);
            }
            SStatusold = SStatus;
            SStatusoldcount = count.length();
          }
        }
      }
    }

    if (position < encoder.getPosition()) {
      if (rotarymode == 0 && menu != true && menu2 != true) {
        sprite.fillSprite(BackgroundColor);
        sprite.pushSprite(6, 220);
        radio.clearRDS();
        tft.setTextColor(BackgroundColor);
        tft.drawString(PSold, 38, 192, 4);
        tft.drawString(PIold, 244, 192, 4);
        tft.drawString(RTold, 6, 222, 2);
        tft.drawString(PTYold, 38, 168, 2);
        PSold = "";
        PTYold = "";
        PIold = "";
        RTold = "";
        KeyUp();
      } else {
        KeyDown();
      }
    }

    if (position > encoder.getPosition()) {
      if (rotarymode == 0 && menu != true && menu2 != true) {
        sprite.fillSprite(BackgroundColor);
        sprite.pushSprite(6, 220);
        KeyDown();
        radio.clearRDS();
        tft.setTextColor(BackgroundColor);
        tft.drawString(PSold, 38, 192, 4);
        tft.drawString(PIold, 244, 192, 4);
        tft.drawString(RTold, 6, 222, 2);
        tft.drawString(PTYold, 38, 168, 2);
        PSold = "";
        PTYold = "";
        PIold = "";
        RTold = "";
      } else {
        KeyUp();
      }
    }

    if (digitalRead(MODEBUTTON) == LOW && digitalRead(BWBUTTON) != LOW && screenmute == false) {
      ModeButtonPress();
    }

    if (digitalRead(BWBUTTON) == LOW && digitalRead(MODEBUTTON) != LOW && screenmute == false) {
      BWButtonPress();
    }

    if (store == true) {
      change++;
    }
    if (change > 200 && store == true) {
      detachInterrupt(digitalPinToInterrupt(ROTARY_PIN_A));
      /*      detachInterrupt(digitalPinToInterrupt(ROTARY_PIN_B));*/
      detachInterrupt(digitalPinToInterrupt(ROTARY_BUTTON));
      EEPROM.writeUInt(0, radio.getFrequency());
      EEPROM.writeUInt(47, radio.getFrequency_AM());
      EEPROM.writeByte(46, band);
      EEPROM.commit();
      store = false;
      attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_A), encoderISR, CHANGE);
      attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_B), encoderISR, CHANGE);
      attachInterrupt(digitalPinToInterrupt(ROTARY_BUTTON), encoderButtonISR, FALLING);
    }
  }
}

void ShowBatteryLevel() {
  if (millis() - VCLOCK >= interval) {
      VCLOCK = millis();
      if (WiFiSwitch == 0) {
      rawValue = analogRead(BATTERYPIN);
      voltageLevel = (rawValue / 4095.0) * 2 * 1.1 * 3.3 - 0.2;  // calculate voltage level
      tft.setTextColor(PrimaryColor);
      tft.fillRect(215, 173, 30, 12, BackgroundColor);  // clear the area before drawing
      tft.drawString(String(voltageLevel) + "V", 215, 173, 1);
    }
  }
}

void PWRButtonPress() {
  if (menu == false && menu2 == false) {
    unsigned long counterold = millis();
    unsigned long counter = millis();
    while (digitalRead(PWRBUTTON) == LOW && counter - counterold <= 1000) {
      counter = millis();
    }
    if (counter - counterold < 1000) {
      if (power == false) {
        ESP.restart();
      } else {
        if (band == 0) {
          band = 1;
        } else {
          band = 0;
        }
        StoreFrequency();
        SelectBand();
      }
    } else {
      if (power == false) {
        ESP.restart();
      } else {
        BuildMenu2();
        menu2 = true;
      }
    }
    while (digitalRead(PWRBUTTON) == LOW) {
      delay(50);
    }
    delay(100);
  }
}

void StoreFrequency() {
  EEPROM.writeUInt(0, radio.getFrequency());
  EEPROM.writeUInt(47, radio.getFrequency_AM());
  EEPROM.writeByte(46, band);
  EEPROM.commit();
}

void SelectBand() {
  if (band == 1) {
    seek = false;
    tunemode = false;
    BWreset = true;
    BWset = 2;
    if (radio.getFrequency_AM() > 0) {
      frequency_AM = radio.getFrequency_AM();
    }
    radio.setFrequency_AM(frequency_AM);
    freqold = frequency_AM;
    doBW;
    radio.getStatus_AM(SStatus, USN, WAM, OStatus, BW, MStatus);
    if (screenmute == false) {
      BuildDisplay();
    }
    tft.drawString("PI:", 216, 195, 2);
    tft.drawString("PS:", 6, 195, 2);
    tft.drawString("PTY:", 6, 168, 2);
    tft.drawBitmap(110, 5, RDSLogo, 67, 22, GreyoutColor);
  } else {
    LowLevelInit == false;
    BWreset = true;
    BWset = 0;
    radio.power(0);
    delay(50);
    radio.setFrequency(frequency, LowEdgeSet, HighEdgeSet);
    freqold = frequency_AM;
    doBW;
    radio.getStatus(SStatus, USN, WAM, OStatus, BW, MStatus);
    if (screenmute == false) {
      BuildDisplay();
    }
  }
}

void BWButtonPress() {
  if (menu == false && menu2 == false) {
    seek = false;
    unsigned long counterold = millis();
    unsigned long counter = millis();
    while (digitalRead(BWBUTTON) == LOW && counter - counterold <= 1000) {
      counter = millis();
    }
    if (counter - counterold < 1000) {
      BWset++;
      doBW();
    } else {
      doStereoToggle();
    }
  }
  while (digitalRead(BWBUTTON) == LOW) {
    delay(50);
  }
  delay(100);
}

void doStereoToggle() {
  if (StereoToggle == true) {
    if (screenmute == false) {
      tft.drawCircle(81, 15, 10, BackgroundColor);
      tft.drawCircle(81, 15, 9, BackgroundColor);
      tft.drawCircle(91, 15, 10, BackgroundColor);
      tft.drawCircle(91, 15, 9, BackgroundColor);
      tft.drawCircle(86, 15, 10, StereoColor);
      tft.drawCircle(86, 15, 9, StereoColor);
    }
    radio.setMono(2);
    StereoToggle = false;
  } else {
    if (screenmute == false) {
      tft.drawCircle(86, 15, 10, BackgroundColor);
      tft.drawCircle(86, 15, 9, BackgroundColor);
    }
    radio.setMono(0);
    Stereostatusold = false;
    StereoToggle = true;
  }
}

void ModeButtonPress() {
  if (menu == false && menu2 == false) {
    seek = false;
    unsigned long counterold = millis();
    unsigned long counter = millis();
    while (digitalRead(MODEBUTTON) == LOW && counter - counterold <= 1000) {
      counter = millis();
    }
    if (counter - counterold <= 1000) {
      doTuneMode();
    } else {
      if (USBstatus == true || WiFistatus == true) {
        ShowFreq(1);
        tft.setTextFont(4);
        tft.setTextColor(SecondaryColor, BackgroundColor);
        tft.setCursor(70, 60);
        tft.print("DISCONNECT USB");
        delay(1000);
        tft.setTextFont(4);
        tft.setTextColor(BackgroundColor);
        tft.setCursor(70, 60);
        tft.print("DISCONNECT USB");
        ShowFreq(0);
      } else {
        if (menu == false && menu2 == false) {
          BuildMenu();
          menu = true;
        }
      }
    }
  } else if (menu == false && menu2 == true) {
    OStatusold = 1000;
    Stereostatusold = false;
    SStatusold = 2000;
    BWOld = 0;
    radio.clearRDS();
    sprite.fillSprite(BackgroundColor);
    sprite.pushSprite(6, 220);
    RDSstatus = 0;
    BuildDisplay();
    ShowSignalLevel();
    ShowBW();
    menu2 = false;
    menuopen = false;
    LowLevelInit = true;
    EEPROM.writeInt(4, VolSet);
    EEPROM.writeInt(8, ConverterSet);
    EEPROM.writeInt(12, LowEdgeSet);
    EEPROM.writeInt(16, HighEdgeSet);
    EEPROM.writeInt(20, ContrastSet);
    EEPROM.writeInt(24, LevelOffset);
    EEPROM.writeInt(28, StereoLevel);
    EEPROM.writeInt(32, HighCutLevel);
    EEPROM.writeInt(36, HighCutOffset);
    EEPROM.writeByte(43, LowLevelSet);
    EEPROM.writeByte(56, SignalUnits);
    EEPROM.writeByte(57, RDSClear);
    EEPROM.writeByte(58, XDRGTKShutdown);
    EEPROM.writeInt(59, CurrentTheme);
    EEPROM.writeByte(63, WiFiSwitch);
    EEPROM.commit();
  } else {
    OStatusold = 1000;
    Stereostatusold = false;
    SStatusold = 2000;
    BWOld = 0;
    radio.clearRDS();
    sprite.fillSprite(BackgroundColor);
    sprite.pushSprite(6, 220);
    RDSstatus = 0;
    BuildDisplay();
    ShowSignalLevel();
    ShowBW();
    menu = false;
    menuopen = false;
    LowLevelInit = true;
    EEPROM.writeInt(4, VolSet);
    EEPROM.writeInt(8, ConverterSet);
    EEPROM.writeInt(12, LowEdgeSet);
    EEPROM.writeInt(16, HighEdgeSet);
    EEPROM.writeInt(20, ContrastSet);
    EEPROM.writeInt(24, LevelOffset);
    EEPROM.writeInt(28, StereoLevel);
    EEPROM.writeInt(32, HighCutLevel);
    EEPROM.writeInt(36, HighCutOffset);
    EEPROM.writeByte(43, LowLevelSet);
    EEPROM.writeByte(56, SignalUnits);
    EEPROM.writeByte(57, RDSClear);
    EEPROM.writeByte(58, XDRGTKShutdown);
    EEPROM.writeInt(59, CurrentTheme);
    EEPROM.writeByte(63, WiFiSwitch);
    EEPROM.commit();
  }
  while (digitalRead(MODEBUTTON) == LOW) {
    delay(50);
  }
  if (wifisetupopen == true) {
    wifisetupopen = false;
  }
  delay(100);
}

void ShowStepSize() {
  tft.fillRect(224, 38, 15, 4, GreyoutColor);
  tft.fillRect(193, 38, 15, 4, GreyoutColor);
  if (band == 0) {
    tft.fillRect(148, 38, 15, 4, GreyoutColor);
    tft.fillRect(117, 38, 15, 4, GreyoutColor);    
  } else {
    tft.fillRect(162, 38, 15, 4, GreyoutColor);
    tft.fillRect(131, 38, 15, 4, GreyoutColor);
  }
  if (stepsize == 1) {
    tft.fillRect(224, 38, 15, 4, TFT_GREEN);
  }
  if (stepsize == 2) {
    tft.fillRect(193, 38, 15, 4, TFT_GREEN);
  }
  if (stepsize == 3) {
    if (band == 0) {
      tft.fillRect(148, 38, 15, 4, TFT_GREEN);
    } else {
      tft.fillRect(162, 38, 15, 4, TFT_GREEN);
    }
  }
  if (stepsize == 4) {
    if (band == 0) {
      tft.fillRect(117, 38, 15, 4, TFT_GREEN);
    } else {
      tft.fillRect(131, 38, 15, 4, TFT_GREEN);
    }
  }
}

void RoundStep() {
  if (band == 0) {
    int freq = radio.getFrequency();
    if (freq % 10 < 3) {
      radio.setFrequency(freq - (freq % 10 - 5) - 5, LowEdgeSet, HighEdgeSet);
    } else if (freq % 10 > 2 && freq % 10 < 8) {
      radio.setFrequency(freq - (freq % 10 - 5), LowEdgeSet, HighEdgeSet);
    } else if (freq % 10 > 7) {
      radio.setFrequency(freq - (freq % 10 - 5) + 5, LowEdgeSet, HighEdgeSet);
    }
  } else {
    int freq = radio.getFrequency_AM();
    if (freq < 2000) {
      radio.setFrequency_AM((freq / 9) * 9);
    } else {
      radio.setFrequency_AM((freq / 5) * 5);
    }
  }
  while (digitalRead(ROTARY_BUTTON) == LOW) {
    delay(5);
  }
  if (band == 0) {
    EEPROM.writeUInt(0, radio.getFrequency());
  } else {
    EEPROM.writeUInt(47, radio.getFrequency_AM());
  }
  EEPROM.commit();
}

void ButtonPress() {
  if (menu == false && menu2 == false) {
    seek = false;
    unsigned long counterold = millis();
    unsigned long counter = millis();
    while (digitalRead(ROTARY_BUTTON) == LOW && counter - counterold <= 1000) {
      counter = millis();
    }
    if (counter - counterold < 1000) {
      if (tunemode == false) {
        stepsize++;
        if (stepsize > 4) {
          stepsize = 0;
        }
        if (screenmute == false) {
          ShowStepSize();
        }
        EEPROM.writeByte(42, stepsize);
        EEPROM.commit();
        if (stepsize == 0) {
          RoundStep();
          ShowFreq(0);
        }
      }
    } else {
      if (iMSEQ == 0) {
        iMSEQ = 1;
      }
      if (iMSEQ == 4) {
        iMSset = 0;
        EQset = 0;
        updateiMS();
        updateEQ();
        iMSEQ = 0;
      }
      if (iMSEQ == 3) {
        iMSset = 1;
        EQset = 0;
        updateiMS();
        updateEQ();
        iMSEQ = 4;
      }
      if (iMSEQ == 2) {
        iMSset = 0;
        EQset = 1;
        updateiMS();
        updateEQ();
        iMSEQ = 3;
      }
      if (iMSEQ == 1) {
        iMSset = 1;
        EQset = 1;
        updateiMS();
        updateEQ();
        iMSEQ = 2;
      }
      EEPROM.writeByte(44, iMSset);
      EEPROM.writeByte(45, EQset);
      EEPROM.commit();
    }
  } else if (menu == false && menu2 == true) {
    if (menuopen == false) {
      menuopen = true;
      tft.drawRoundRect(30, 40, 240, 160, 5, TFT_WHITE);
      tft.fillRoundRect(32, 42, 236, 156, 5, TFT_BLACK);
      switch (menuoption) {
        case 30:
          tft.setTextColor(TFT_WHITE);
          tft.drawCentreString("Color scheme:", 150, 70, 4);
          doTheme();
          tft.setTextColor(TFT_WHITE);
          tft.drawCentreString(CurrentThemeString, 150, 110, 4);
          break;

        case 50:
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("Signal units:", 150, 70, 4);
          if (SignalUnits == 1) {
            SignalUnitsString = "dBf";
          } else {
            SignalUnitsString = "dBuV";
          }
          tft.setTextColor(PrimaryColor);

          //          SignalUnitsString = String(SignalUnits, DEC);
          tft.drawCentreString(SignalUnitsString, 150, 110, 4);
          break;

        case 70:
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("Auto RDS clear:", 150, 70, 4);
          if (RDSClear == 1) {
            RDSClearString = "ON";
          } else {
            RDSClearString = "OFF";
          }
          tft.setTextColor(PrimaryColor);
          tft.drawRightString(RDSClearString, 165, 110, 4);
          break;

        case 90:
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("XDR-GTK shutdown", 150, 70, 4);
          if (XDRGTKShutdown == 1) {
            XDRGTKShutdownString = "ON";
          } else {
            XDRGTKShutdownString = "OFF";
          }
          tft.setTextColor(PrimaryColor);
          tft.drawCentreString(XDRGTKShutdownString, 165, 110, 4);
          break;
        case 110:
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("Wi-Fi:", 150, 70, 4);
          if (WiFiSwitch == 1) {
            WiFiSwitchString = "ON";
          } else {
            WiFiSwitchString = "OFF";
          }
          tft.setTextColor(PrimaryColor);
          tft.drawCentreString(WiFiSwitchString, 150, 110, 4);
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("Setting will apply after restart.", 150, 165, 2);
          break;

        case 130:
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("Wi-Fi SETUP", 150, 70, 4);
          wifisetupopen = true;
          tft.drawCentreString("Connect to this Wi-Fi network:", 150, 95, 2);
          tft.setTextColor(PrimaryColor);
          tft.drawCentreString("TEF6686 FM Tuner", 150, 110, 2);
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("And open this IP in your browser:", 150, 130, 2);
          tft.setTextColor(PrimaryColor);
          tft.drawCentreString("192.168.4.1", 150, 145, 2);
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("Then proceed in the web browser.", 150, 165, 2);
          WiFiSwitch = 1;
          EEPROM.writeByte(63, WiFiSwitch);
          WiFiSetup();
          break;

        case 150:
          if (power == false) {
            ESP.restart();
          } else {
            tft.setTextColor(SecondaryColor);
            tft.drawCentreString("Screen shutdown...", 150, 70, 4);
            tft.drawCentreString("Press blue menu button to wake up", 150, 95, 2);
            delay(1000);
            power = false;
            menu2 = false;
            analogWrite(CONTRASTPIN, 0);
            StoreFrequency();
          }
          break;
      }
    } else {
      menuopen = false;
      BuildMenu2();
    }
  } else {
    if (menuopen == false) {
      menuopen = true;
      tft.drawRoundRect(30, 40, 240, 160, 5, SecondaryColor);
      tft.fillRoundRect(32, 42, 236, 156, 5, BackgroundColor);
      switch (menuoption) {
        case 30:
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("Volume:", 150, 70, 4);
          tft.drawString("dB", 170, 110, 4);

          if (VolSet > 0) {
            VolString = (String) "+" + VolSet, DEC;
          } else {
            VolString = String(VolSet, DEC);
          }
          tft.setTextColor(PrimaryColor);
          tft.drawRightString(VolString, 165, 110, 4);
          break;

        case 50:
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("Converter:", 150, 70, 4);
          tft.drawString("MHz", 170, 110, 4);
          tft.setTextColor(PrimaryColor);

          ConverterString = String(ConverterSet, DEC);
          tft.drawRightString(ConverterString, 165, 110, 4);
          break;

        case 70:
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("Low bandedge:", 150, 70, 4);
          tft.drawString("MHz", 170, 110, 4);
          tft.setTextColor(PrimaryColor);
          LowEdgeString = String(LowEdgeSet + ConverterSet, DEC);
          tft.drawRightString(LowEdgeString, 165, 110, 4);
          break;

        case 90:
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("High bandedge:", 150, 70, 4);
          tft.drawString("MHz", 170, 110, 4);
          tft.setTextColor(PrimaryColor);
          HighEdgeString = String(HighEdgeSet + ConverterSet, DEC);
          tft.drawRightString(HighEdgeString, 165, 110, 4);
          break;

        case 110:
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("RF Level offset:", 150, 70, 4);
          tft.drawString("dB", 170, 110, 4);
          tft.drawString("dBuV", 190, 157, 4);
          if (LevelOffset > 0) {
            LevelOffsetString = (String) "+" + LevelOffset, DEC;
          } else {
            LevelOffsetString = String(LevelOffset, DEC);
          }
          tft.setTextColor(PrimaryColor);
          tft.drawRightString(LevelOffsetString, 165, 110, 4);
          SStatusold = 2000;
          change2 = true;
          break;

        case 130:
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("Stereo threshold:", 150, 70, 4);
          if (StereoLevel != 0) {
            tft.drawString("dBuV", 170, 110, 4);
          }
          tft.setTextColor(PrimaryColor);
          if (StereoLevel != 0) {
            StereoLevelString = String(StereoLevel, DEC);
          } else {
            StereoLevelString = String("Off");
          }
          tft.drawRightString(StereoLevelString, 165, 110, 4);
          break;

        case 150:
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("High Cut corner:", 150, 70, 4);
          if (HighCutLevel != 0) {
            tft.drawString("Hz", 170, 110, 4);
          }
          tft.setTextColor(PrimaryColor);
          if (HighCutLevel != 0) {
            HighCutLevelString = String(HighCutLevel * 100, DEC);
          } else {
            HighCutLevelString = String("Off");
          }
          tft.drawRightString(HighCutLevelString, 165, 110, 4);
          break;

        case 170:
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("Highcut threshold:", 150, 70, 4);
          if (HighCutOffset != 0) {
            tft.drawString("dBuV", 170, 110, 4);
          }
          tft.setTextColor(PrimaryColor);
          if (HighCutOffset != 0) {
            HighCutOffsetString = String(HighCutOffset, DEC);
          } else {
            HighCutOffsetString = String("Off");
          }
          tft.drawRightString(HighCutOffsetString, 165, 110, 4);
          break;

        case 190:
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("Low level threshold:", 150, 70, 4);
          tft.drawString("dBuV", 150, 110, 4);
          tft.setTextColor(PrimaryColor);
          LowLevelString = String(LowLevelSet, DEC);
          tft.drawRightString(LowLevelString, 145, 110, 4);
          break;

        case 210:
          tft.setTextColor(SecondaryColor);
          tft.drawCentreString("Contrast:", 150, 70, 4);
          tft.drawString("%", 170, 110, 4);
          tft.setTextColor(PrimaryColor);
          ContrastString = String(ContrastSet, DEC);
          tft.drawRightString(ContrastString, 165, 110, 4);
          break;
      }
    } else {
      menuopen = false;
      BuildMenu();
    }
  }
  while (digitalRead(ROTARY_BUTTON) == LOW) {
    delay(5);
  }
}

void KeyUp() {
  if (menu == false && menu2 == false) {

    tft.setTextColor(BackgroundColor);
    tft.drawString("TP", 190, 168, 2);

    if (tunemode == true) {
      direction = true;
      seek = true;
      Seek(direction);
    } else {
      if (band == 0) {
        frequency = radio.tuneUp(stepsize, LowEdgeSet, HighEdgeSet);
      } else {
        frequency_AM = radio.tuneUp_AM(stepsize);
      }
    }
    if (USBstatus == true) {
      Serial.print("T");
      if (band == 0) {
        Serial.println(frequency * 10);
      } else {
        Serial.println(frequency_AM);
      }
    }
    if (WiFistatus == true) {
      RemoteClient.print("T");
      if (band == 0) {
        RemoteClient.println(frequency * 10);
      } else {
        RemoteClient.println(frequency_AM);
      }
    }
    radio.clearRDS();
    sprite.fillSprite(BackgroundColor);
    sprite.pushSprite(6, 220);
    change = 0;
    ShowFreq(0);
    store = true;
  } else if (menu == false && menu2 == true) {
    if (menuopen == false) {
      tft.drawRoundRect(10, menuoption, 300, 18, 5, BackgroundColor);
      menuoption += 20;
      if (menuoption > 150) {
        menuoption = 30;
      }
      tft.drawRoundRect(10, menuoption, 300, 18, 5, SecondaryColor);
    } else {
      switch (menuoption) {
        case 30:
          tft.setTextColor(TFT_BLACK);
          tft.drawCentreString(CurrentThemeString, 150, 110, 4);
          CurrentTheme += 1;
          if (CurrentTheme > 7) {
            CurrentTheme = 0;
          }

          doTheme();
          tft.setTextColor(TFT_WHITE);
          tft.drawCentreString(CurrentThemeString, 150, 110, 4);
          break;

        case 50:
          tft.setTextColor(BackgroundColor);
          tft.drawCentreString(SignalUnitsString, 150, 110, 4);
          if (SignalUnits == 1) {
            SignalUnits = 0;
            SignalUnitsString = "dBuV";

          } else {
            SignalUnits = 1;
            SignalUnitsString = "dBf";
          }
          tft.setTextColor(PrimaryColor);
          tft.drawCentreString(SignalUnitsString, 150, 110, 4);
          break;

        case 70:
          tft.setTextColor(BackgroundColor);
          tft.drawRightString(RDSClearString, 165, 110, 4);
          if (RDSClear == 1) {
            RDSClear = 0;
            RDSClearString = "OFF";
          } else {
            RDSClear = 1;
            RDSClearString = "ON";
          }
          tft.setTextColor(PrimaryColor);
          tft.drawRightString(RDSClearString, 165, 110, 4);
          break;

        case 90:
          tft.setTextColor(BackgroundColor);
          tft.drawCentreString(XDRGTKShutdownString, 165, 110, 4);
          if (XDRGTKShutdown == 1) {
            XDRGTKShutdown = 0;
            XDRGTKShutdownString = "OFF";
          } else {
            XDRGTKShutdown = 1;
            XDRGTKShutdownString = "ON";
          }
          tft.setTextColor(PrimaryColor);
          tft.drawCentreString(XDRGTKShutdownString, 165, 110, 4);
          break;

        case 110:
          tft.setTextColor(BackgroundColor);
          tft.drawCentreString(WiFiSwitchString, 150, 110, 4);
          if (WiFiSwitch == 1) {
            WiFiSwitch = 0;
            WiFiSwitchString = "OFF";
          } else {
            WiFiSwitch = 1;
            WiFiSwitchString = "ON";
          }
          tft.setTextColor(PrimaryColor);
          tft.drawCentreString(WiFiSwitchString, 150, 110, 4);
          break;

        case 130:
          break;

        case 150:
          break;
      }
    }
  } else {
    if (menuopen == false) {
      tft.drawRoundRect(10, menuoption, 300, 18, 5, BackgroundColor);
      menuoption += 20;
      if (menuoption > 210) {
        menuoption = 30;
      }
      tft.drawRoundRect(10, menuoption, 300, 18, 5, SecondaryColor);
    } else {
      switch (menuoption) {
        case 30:
          tft.setTextColor(BackgroundColor);
          if (VolSet > 0) {
            VolString = (String) "+" + VolSet, DEC;
          } else {
            VolString = String(VolSet, DEC);
          }
          tft.drawRightString(VolString, 165, 110, 4);
          VolSet++;
          if (VolSet > 10) {
            VolSet = 10;
          }
          tft.setTextColor(PrimaryColor);
          if (VolSet > 0) {
            VolString = (String) "+" + VolSet, DEC;
          } else {
            VolString = String(VolSet, DEC);
          }
          tft.drawRightString(VolString, 165, 110, 4);
          radio.setVolume(VolSet);
          break;

        case 50:
          tft.setTextColor(BackgroundColor);
          ConverterString = String(ConverterSet, DEC);
          tft.drawRightString(ConverterString, 165, 110, 4);
          ConverterSet++;
          if (ConverterSet > 2400 || ConverterSet <= 200) {
            if (ConverterSet == 1) {
              ConverterSet = 200;
            } else {
              ConverterSet = 0;
            }
          }
          if (ConverterSet >= 200) {
            Wire.beginTransmission(0x12);
            Wire.write(ConverterSet >> 8);
            Wire.write(ConverterSet & (0xFF));
            Wire.endTransmission();
          }
          tft.setTextColor(PrimaryColor);
          ConverterString = String(ConverterSet, DEC);
          tft.drawRightString(ConverterString, 165, 110, 4);
          break;

        case 70:
          tft.setTextColor(BackgroundColor);
          LowEdgeString = String(LowEdgeSet + ConverterSet, DEC);
          tft.drawRightString(LowEdgeString, 165, 110, 4);
          LowEdgeSet++;
          if (LowEdgeSet > 107) {
            LowEdgeSet = 65;
          }
          tft.setTextColor(PrimaryColor);
          LowEdgeString = String(LowEdgeSet + ConverterSet, DEC);
          tft.drawRightString(LowEdgeString, 165, 110, 4);
          break;

        case 90:
          tft.setTextColor(BackgroundColor);
          HighEdgeString = String(HighEdgeSet + ConverterSet, DEC);
          tft.drawRightString(HighEdgeString, 165, 110, 4);
          HighEdgeSet++;
          if (HighEdgeSet > 108) {
            HighEdgeSet = 66;
          }
          tft.setTextColor(PrimaryColor);
          HighEdgeString = String(HighEdgeSet + ConverterSet, DEC);
          tft.drawRightString(HighEdgeString, 165, 110, 4);
          break;

        case 110:
          tft.setTextColor(BackgroundColor);
          if (LevelOffset > 0) {
            LevelOffsetString = (String) "+" + LevelOffset, DEC;
          } else {
            LevelOffsetString = String(LevelOffset, DEC);
          }
          tft.drawRightString(LevelOffsetString, 165, 110, 4);
          LevelOffset++;
          if (LevelOffset > 15) {
            LevelOffset = -25;
          }
          tft.setTextColor(PrimaryColor);
          if (LevelOffset > 0) {
            LevelOffsetString = (String) "+" + LevelOffset, DEC;
          } else {
            LevelOffsetString = String(LevelOffset, DEC);
          }
          tft.drawRightString(LevelOffsetString, 165, 110, 4);
          radio.setOffset(LevelOffset);
          change2 = true;
          break;

        case 130:
          tft.setTextColor(BackgroundColor);
          StereoLevelString = String(StereoLevel, DEC);
          tft.drawRightString(StereoLevelString, 165, 110, 4);
          StereoLevel++;
          if (StereoLevel > 60 || StereoLevel <= 30) {
            if (StereoLevel == 1) {
              StereoLevel = 30;
            } else {
              StereoLevel = 0;
            }
          }
          if (StereoLevel != 0) {
            StereoLevelString = String(StereoLevel, DEC);
          } else {
            StereoLevelString = String("Off");
          }
          tft.setTextColor(BackgroundColor);
          tft.drawRightString("Off", 165, 110, 4);
          tft.drawString("dBuV", 170, 110, 4);
          tft.setTextColor(SecondaryColor);
          if (StereoLevel != 0) {
            tft.drawString("dBuV", 170, 110, 4);
          }
          tft.setTextColor(PrimaryColor);
          tft.drawRightString(StereoLevelString, 165, 110, 4);
          radio.setStereoLevel(StereoLevel);
          break;

        case 150:
          tft.setTextColor(BackgroundColor);
          HighCutLevelString = String(HighCutLevel * 100, DEC);
          tft.drawRightString(HighCutLevelString, 165, 110, 4);
          HighCutLevel++;
          if (HighCutLevel > 70) {
            HighCutLevel = 15;
          }
          HighCutLevelString = String(HighCutLevel * 100, DEC);
          tft.setTextColor(PrimaryColor);
          tft.drawRightString(HighCutLevelString, 165, 110, 4);
          radio.setHighCutLevel(HighCutLevel);
          break;

        case 170:
          tft.setTextColor(BackgroundColor);
          HighCutOffsetString = String(HighCutOffset, DEC);
          tft.drawRightString(HighCutOffsetString, 165, 110, 4);
          HighCutOffset++;
          if (HighCutOffset > 60 || HighCutOffset <= 20) {
            if (HighCutOffset == 1) {
              HighCutOffset = 20;
            } else {
              HighCutOffset = 0;
            }
          }
          if (HighCutOffset != 0) {
            HighCutOffsetString = String(HighCutOffset, DEC);
          } else {
            HighCutOffsetString = String("Off");
          }
          tft.setTextColor(BackgroundColor);
          tft.drawRightString("Off", 165, 110, 4);
          tft.drawString("dBuV", 170, 110, 4);
          tft.setTextColor(SecondaryColor);
          if (HighCutOffset != 0) {
            tft.drawString("dBuV", 170, 110, 4);
          }
          tft.setTextColor(PrimaryColor);
          tft.drawRightString(HighCutOffsetString, 165, 110, 4);
          radio.setHighCutOffset(HighCutOffset);
          break;

        case 190:
          tft.setTextColor(BackgroundColor);
          LowLevelString = String(LowLevelSet, DEC);
          tft.drawRightString(LowLevelString, 145, 110, 4);
          LowLevelSet++;
          if (LowLevelSet > 40 || LowLevelSet <= 0) {
            LowLevelSet = 0;
          }
          tft.setTextColor(PrimaryColor);
          LowLevelString = String(LowLevelSet, DEC);
          tft.drawRightString(LowLevelString, 145, 110, 4);
          break;

        case 210:
          tft.setTextColor(BackgroundColor);
          ContrastString = String(ContrastSet, DEC);
          tft.drawRightString(ContrastString, 165, 110, 4);
          ContrastSet++;
          if (ContrastSet > 100) {
            ContrastSet = 1;
          }
          tft.setTextColor(PrimaryColor);
          ContrastString = String(ContrastSet, DEC);
          tft.drawRightString(ContrastString, 165, 110, 4);
          analogWrite(CONTRASTPIN, ContrastSet * 2 + 27);
          break;
      }
    }
  }
  position = encoder.getPosition();
}

void KeyDown() {
  if (menu == false && menu2 == false) {

    tft.setTextColor(BackgroundColor);
    tft.drawString("TP", 190, 168, 2);

    if (tunemode == true) {
      direction = false;
      seek = true;
      Seek(direction);
    } else {
      if (band == 0) {
        frequency = radio.tuneDown(stepsize, LowEdgeSet, HighEdgeSet);
      } else {
        frequency_AM = radio.tuneDown_AM(stepsize);
      }
    }
    if (USBstatus == true) {
      Serial.print("T");
      if (band == 0) {
        Serial.println(frequency * 10);
      } else {
        Serial.println(frequency_AM);
      }
    }
    if (WiFistatus == true) {
      RemoteClient.print("T");
      if (band == 0) {
        RemoteClient.println(frequency * 10);
      } else {
        RemoteClient.println(frequency_AM);
      }
    }
    radio.clearRDS();
    sprite.fillSprite(BackgroundColor);
    sprite.pushSprite(6, 220);
    change = 0;
    ShowFreq(0);
    store = true;
  } else if (menu == false && menu2 == true) {
    if (menuopen == false) {
      tft.drawRoundRect(10, menuoption, 300, 18, 5, BackgroundColor);
      menuoption -= 20;
      if (menuoption < 30) {
        menuoption = 150;
      }
      tft.drawRoundRect(10, menuoption, 300, 18, 5, SecondaryColor);
    } else {
      switch (menuoption) {
        case 30:
          tft.setTextColor(TFT_BLACK);
          tft.drawCentreString(CurrentThemeString, 150, 110, 4);
          CurrentTheme -= 1;
          if (CurrentTheme < 0) {
            CurrentTheme = 7;
          }

          doTheme();
          tft.setTextColor(TFT_WHITE);
          tft.drawCentreString(CurrentThemeString, 150, 110, 4);
          break;

        case 50:
          tft.setTextColor(BackgroundColor);
          tft.drawCentreString(SignalUnitsString, 150, 110, 4);
          if (SignalUnits == 1) {
            SignalUnits = 0;
            SignalUnitsString = "dBuV";
          } else {
            SignalUnits = 1;
            SignalUnitsString = "dBf";
          }
          tft.setTextColor(PrimaryColor);
          tft.drawCentreString(SignalUnitsString, 150, 110, 4);
          break;

        case 70:
          tft.setTextColor(BackgroundColor);
          tft.drawRightString(RDSClearString, 165, 110, 4);
          if (RDSClear == 1) {
            RDSClear = 0;
            RDSClearString = "OFF";
          } else {
            RDSClear = 1;
            RDSClearString = "ON";
          }
          tft.setTextColor(PrimaryColor);
          tft.drawRightString(RDSClearString, 165, 110, 4);
          break;

        case 90:
          tft.setTextColor(BackgroundColor);
          tft.drawCentreString(XDRGTKShutdownString, 165, 110, 4);
          if (XDRGTKShutdown == 1) {
            XDRGTKShutdown = 0;
            XDRGTKShutdownString = "OFF";
          } else {
            XDRGTKShutdown = 1;
            XDRGTKShutdownString = "ON";
          }
          tft.setTextColor(PrimaryColor);
          tft.drawCentreString(XDRGTKShutdownString, 165, 110, 4);
          break;

        case 110:
          tft.setTextColor(BackgroundColor);
          tft.drawCentreString(WiFiSwitchString, 150, 110, 4);
          if (WiFiSwitch == 1) {
            WiFiSwitch = 0;
            WiFiSwitchString = "OFF";
          } else {
            WiFiSwitch = 1;
            WiFiSwitchString = "ON";
          }
          tft.setTextColor(PrimaryColor);
          tft.drawCentreString(WiFiSwitchString, 150, 110, 4);
          break;
          break;

        case 130:
          break;

        case 150:
          break;
      }
    }
  } else {
    if (menuopen == false) {
      tft.drawRoundRect(10, menuoption, 300, 18, 5, BackgroundColor);
      menuoption -= 20;
      if (menuoption < 30) {
        menuoption = 210;
      }
      tft.drawRoundRect(10, menuoption, 300, 18, 5, SecondaryColor);
    } else {
      switch (menuoption) {
        case 30:

          tft.setTextColor(BackgroundColor);
          if (VolSet > 0) {
            VolString = (String) "+" + VolSet, DEC;
          } else {
            VolString = String(VolSet, DEC);
          }
          tft.drawRightString(VolString, 165, 110, 4);
          VolSet--;
          if (VolSet < -10) {
            VolSet = -10;
          }
          tft.setTextColor(PrimaryColor);
          if (VolSet > 0) {
            VolString = (String) "+" + VolSet, DEC;
          } else {
            VolString = String(VolSet, DEC);
          }
          tft.drawRightString(VolString, 165, 110, 4);
          radio.setVolume(VolSet);
          break;

        case 50:
          tft.setTextColor(BackgroundColor);
          ConverterString = String(ConverterSet, DEC);
          tft.drawRightString(ConverterString, 165, 110, 4);
          ConverterSet--;
          if (ConverterSet < 200) {
            if (ConverterSet < 0) {
              ConverterSet = 2400;
            } else {
              ConverterSet = 0;
            }
          }
          if (ConverterSet >= 200) {
            Wire.beginTransmission(0x12);
            Wire.write(ConverterSet >> 8);
            Wire.write(ConverterSet & (0xFF));
            Wire.endTransmission();
          }
          tft.setTextColor(PrimaryColor);
          ConverterString = String(ConverterSet, DEC);
          tft.drawRightString(ConverterString, 165, 110, 4);
          break;

        case 70:
          tft.setTextColor(BackgroundColor);
          LowEdgeString = String(LowEdgeSet + ConverterSet, DEC);
          tft.drawRightString(LowEdgeString, 165, 110, 4);
          LowEdgeSet--;
          if (LowEdgeSet < 65) {
            LowEdgeSet = 107;
          }
          tft.setTextColor(PrimaryColor);
          LowEdgeString = String(LowEdgeSet + ConverterSet, DEC);
          tft.drawRightString(LowEdgeString, 165, 110, 4);
          break;

        case 90:
          tft.setTextColor(BackgroundColor);
          HighEdgeString = String(HighEdgeSet + ConverterSet, DEC);
          tft.drawRightString(HighEdgeString, 165, 110, 4);
          HighEdgeSet--;
          if (HighEdgeSet < 66) {
            HighEdgeSet = 108;
          }
          tft.setTextColor(PrimaryColor);
          HighEdgeString = String(HighEdgeSet + ConverterSet, DEC);
          tft.drawRightString(HighEdgeString, 165, 110, 4);
          break;

        case 110:
          tft.setTextColor(BackgroundColor);
          if (LevelOffset > 0) {
            LevelOffsetString = (String) "+" + LevelOffset, DEC;
          } else {
            LevelOffsetString = String(LevelOffset, DEC);
          }
          tft.drawRightString(LevelOffsetString, 165, 110, 4);
          LevelOffset--;
          if (LevelOffset < -25) {
            LevelOffset = 15;
          }
          tft.setTextColor(PrimaryColor);
          if (LevelOffset > 0) {
            LevelOffsetString = (String) "+" + LevelOffset, DEC;
          } else {
            LevelOffsetString = String(LevelOffset, DEC);
          }
          tft.drawRightString(LevelOffsetString, 165, 110, 4);
          radio.setOffset(LevelOffset);
          change2 = true;
          break;

        case 130:
          tft.setTextColor(BackgroundColor);
          StereoLevelString = String(StereoLevel, DEC);
          tft.drawRightString(StereoLevelString, 165, 110, 4);
          StereoLevel--;
          if (StereoLevel < 30) {
            if (StereoLevel < 0) {
              StereoLevel = 60;
            } else {
              StereoLevel = 0;
            }
          }

          if (StereoLevel != 0) {
            StereoLevelString = String(StereoLevel, DEC);
          } else {
            StereoLevelString = String("Off");
          }
          tft.setTextColor(BackgroundColor);
          tft.drawRightString("Off", 165, 110, 4);
          tft.drawString("dBuV", 170, 110, 4);
          tft.setTextColor(SecondaryColor);
          if (StereoLevel != 0) {
            tft.drawString("dBuV", 170, 110, 4);
          }
          tft.setTextColor(PrimaryColor);
          tft.drawRightString(StereoLevelString, 165, 110, 4);
          radio.setStereoLevel(StereoLevel);
          break;

        case 150:
          tft.setTextColor(BackgroundColor);
          HighCutLevelString = String(HighCutLevel * 100, DEC);
          tft.drawRightString(HighCutLevelString, 165, 110, 4);
          HighCutLevel--;
          if (HighCutLevel < 15) {
            HighCutLevel = 70;
          }
          HighCutLevelString = String(HighCutLevel * 100, DEC);
          tft.setTextColor(PrimaryColor);
          tft.drawRightString(HighCutLevelString, 165, 110, 4);
          radio.setHighCutLevel(HighCutLevel);
          break;

        case 170:
          tft.setTextColor(BackgroundColor);
          HighCutOffsetString = String(HighCutOffset, DEC);
          tft.drawRightString(HighCutOffsetString, 165, 110, 4);
          HighCutOffset--;
          if (HighCutOffset < 20) {
            if (HighCutOffset < 0) {
              HighCutOffset = 60;
            } else {
              HighCutOffset = 0;
            }
          }

          if (HighCutOffset != 0) {
            HighCutOffsetString = String(HighCutOffset, DEC);
          } else {
            HighCutOffsetString = String("Off");
          }
          tft.setTextColor(BackgroundColor);
          tft.drawRightString("Off", 165, 110, 4);
          tft.drawString("dBuV", 170, 110, 4);
          tft.setTextColor(SecondaryColor);
          if (HighCutOffset != 0) {
            tft.drawString("dBuV", 170, 110, 4);
          }
          tft.setTextColor(PrimaryColor);
          tft.drawRightString(HighCutOffsetString, 165, 110, 4);
          radio.setHighCutOffset(HighCutOffset);
          break;

        case 190:
          tft.setTextColor(BackgroundColor);
          LowLevelString = String(LowLevelSet, DEC);
          tft.drawRightString(LowLevelString, 145, 110, 4);
          LowLevelSet--;
          if (LowLevelSet > 40) {
            LowLevelSet = 40;
          }
          tft.setTextColor(PrimaryColor);
          LowLevelString = String(LowLevelSet, DEC);
          tft.drawRightString(LowLevelString, 145, 110, 4);
          break;


        case 210:
          tft.setTextColor(BackgroundColor);
          ContrastString = String(ContrastSet, DEC);
          tft.drawRightString(ContrastString, 165, 110, 4);
          ContrastSet--;
          if (ContrastSet < 1) {
            ContrastSet = 100;
          }
          tft.setTextColor(PrimaryColor);
          ContrastString = String(ContrastSet, DEC);
          tft.drawRightString(ContrastString, 165, 110, 4);
          analogWrite(CONTRASTPIN, ContrastSet * 2 + 27);
          break;
      }
    }
  }
  if (optenc == 0) {
    while (digitalRead(ROTARY_PIN_A) == LOW || digitalRead(ROTARY_PIN_B) == LOW) {
      delay(5);
    }
  }
  position = encoder.getPosition();
}



void readRds() {
  if (band == 0) {
    radio.getRDS(&rdsInfo);
    RDSstatus = radio.readRDS(rdsB, rdsC, rdsD, rdsErr);
    ShowRDSLogo(RDSstatus);

    if (RDSstatus == 0) {
      if (RDSClear == 1) {
        tft.setTextColor(BackgroundColor);

        if (TPStatus == true) {
          tft.setTextColor(BackgroundColor);
          tft.drawString("TP", 190, 168, 2);
          TPStatus = false;
        }

      } else {
        sprite.fillSprite(BackgroundColor);
        sprite.pushSprite(6, 220);

        if (TPStatus == true) {
          tft.setTextColor(GreyoutColor);
          tft.drawString("TP", 190, 168, 2);
          TPStatus = false;
        }

        tft.setTextColor(GreyoutColor);
      }
      tft.drawString(PIold, 244, 192, 4);
      tft.drawString(PSold, 38, 192, 4);
      tft.drawString(RTold, 6, 222, 2);
      tft.drawString(PTYold, 38, 168, 2);
      strcpy(programServicePrevious, " ");
      strcpy(radioIdPrevious, " ");

      if (RDSClear == 1) {
        sprite.fillSprite(BackgroundColor);
        sprite.pushSprite(6, 220);
      }
    }

    if (RDSstatus == 1 && USBstatus == true && WiFistatus == false) {
      Serial.print("P");
      Serial.print(rdsInfo.programId);
      Serial.print("\nR");
      serial_hex(rdsB >> 8);
      serial_hex(rdsB);
      serial_hex(rdsC >> 8);
      serial_hex(rdsC);
      serial_hex(rdsD >> 8);
      serial_hex(rdsD);
      serial_hex(rdsErr >> 8);
      Serial.print("\n");
    }

    if (RDSstatus == 1 && USBstatus == false && WiFistatus == true) {
      RemoteClient.print("P");
      RemoteClient.print(rdsInfo.programId);
      RemoteClient.print("\nR");
      tcp_hex(rdsB >> 8);
      tcp_hex(rdsB);
      tcp_hex(rdsC >> 8);
      tcp_hex(rdsC);
      tcp_hex(rdsD >> 8);
      tcp_hex(rdsD);
      tcp_hex(rdsErr >> 8);
      RemoteClient.print("\n");
    }
  }
}

void showPI() {
  if ((RDSstatus == 1) && !strcmp(rdsInfo.programId, radioIdPrevious, 4)) {
    tft.setTextColor(BackgroundColor);
    tft.drawString(PIold, 244, 192, 4);
    tft.setTextColor(PrimaryColor);
    tft.drawString(rdsInfo.programId, 244, 192, 4);
    PIold = rdsInfo.programId;
    strcpy(radioIdPrevious, rdsInfo.programId);
  }
}

void showTP() {
  if (RDSstatus == 1 && TPStatus == true) {
    tft.setTextColor(PrimaryColor);
    tft.drawString("TP", 190, 168, 2);
  }
}

void showPTY() {
  if ((RDSstatus == 1) && !strcmp(rdsInfo.programType, programTypePrevious, 16)) {
    tft.setTextColor(BackgroundColor);
    tft.drawString(PTYold, 38, 168, 2);
    tft.setTextColor(PrimaryColor);
    tft.drawString(rdsInfo.programType, 38, 168, 2);
    PTYold = rdsInfo.programType;
    strcpy(programTypePrevious, rdsInfo.programType);
  }
}

void showPS() {
  if (SStatus / 10 > LowLevelSet) {
    if ((RDSstatus == 1) && (strlen(rdsInfo.programService) == 8) && !strcmp(rdsInfo.programService, programServicePrevious, 8)) {
      tft.setTextColor(BackgroundColor);
      tft.drawString(PSold, 38, 192, 4);
      tft.setTextColor(PrimaryColor, BackgroundColor);
      tft.drawString(rdsInfo.programService, 38, 192, 4);
      PSold = rdsInfo.programService;
      strcpy(programServicePrevious, rdsInfo.programService);
    }
  }
}

void showRadioText() {
  if ((RDSstatus == 1 && band == 0) /*&& !strcmp(rdsInfo.radioText, radioTextPrevious, 65)*/) {

    if (millis() - RTCLOCK >= 350) {
      // Update the position of the text
      xPos -= charWidth;
      if (xPos < -tft.textWidth(rdsInfo.radioText) + (charWidth * 42)) {
        xPos = 6;
      }

      sprite.fillSprite(BackgroundColor);
      sprite.setTextColor(PrimaryColor);
      sprite.drawString(rdsInfo.radioText, xPos, yPos, 2);
      sprite.pushSprite(6, 220);
      RTCLOCK = millis();
    }
    RTold = rdsInfo.radioText;
    strcpy(radioTextPrevious, rdsInfo.radioText);
  }
}

bool strcmp(char* str1, char* str2, int length) {
  for (int i = 0; i < length; i++) {
    if (str1[i] != str2[i]) {
      return false;
    }
  }
  return true;
}

void BuildMenu() {
  tft.fillScreen(BackgroundColor);
  tft.drawRect(0, 0, 320, 240, FrameColor);
  tft.drawLine(0, 23, 320, 23, FrameColor);
  tft.setTextColor(PrimaryColor);
  tft.drawString("PRESS MODE TO EXIT AND SAVE", 20, 4, 2);
  tft.setTextColor(SecondaryColor);
  tft.drawRightString(VERSION, 305, 4, 2);
  tft.drawRoundRect(10, menuoption, 300, 18, 5, SecondaryColor);
  tft.setTextColor(SecondaryColor);
  tft.drawRightString("dB", 305, 30, 2);
  tft.drawRightString("MHz", 305, 50, 2);
  tft.drawRightString("MHz", 305, 70, 2);
  tft.drawRightString("MHz", 305, 90, 2);
  tft.drawRightString("dB", 305, 110, 2);
  if (StereoLevel != 0) {
    tft.drawRightString("dBuV", 305, 130, 2);
  }
  if (HighCutLevel != 0) {
    tft.drawRightString("Hz", 305, 150, 2);
  }
  if (HighCutOffset != 0) {
    tft.drawRightString("dBuV", 305, 170, 2);
  }
  tft.drawRightString("dBuV", 305, 190, 2);
  tft.drawRightString("%", 305, 210, 2);
  tft.drawString("Set volume", 20, 30, 2);
  tft.drawString("Set converter offset", 20, 50, 2);
  tft.drawString("Set low bandedge", 20, 70, 2);
  tft.drawString("Set high bandedge", 20, 90, 2);
  tft.drawString("Set level offset", 20, 110, 2);
  tft.drawString("Set Stereo sep. threshold", 20, 130, 2);
  tft.drawString("Set high cut corner frequency", 20, 150, 2);
  tft.drawString("Set High cut threshold", 20, 170, 2);
  tft.drawString("Set low level threshold", 20, 190, 2);
  tft.drawString("Set Display brightness", 20, 210, 2);

  if (VolSet > 0) {
    VolString = (String) "+" + VolSet, DEC;
  } else {
    VolString = String(VolSet, DEC);
  }
  ConverterString = String(ConverterSet, DEC);
  LowEdgeString = String(LowEdgeSet + ConverterSet, DEC);
  HighEdgeString = String(HighEdgeSet + ConverterSet, DEC);
  LowLevelString = String(LowLevelSet, DEC);
  ContrastString = String(ContrastSet, DEC);

  if (StereoLevel != 0) {
    StereoLevelString = String(StereoLevel, DEC);
  } else {
    StereoLevelString = String("Off");
  }
  if (HighCutLevel != 0) {
    HighCutLevelString = String(HighCutLevel * 100, DEC);
  } else {
    HighCutLevelString = String("Off");
  }
  if (LevelOffset > 0) {
    LevelOffsetString = (String) "+" + LevelOffset, DEC;
  } else {
    LevelOffsetString = String(LevelOffset, DEC);
  }
  if (HighCutOffset != 0) {
    HighCutOffsetString = String(HighCutOffset, DEC);
  } else {
    HighCutOffsetString = String("Off");
  }
  tft.setTextColor(PrimaryColor);
  tft.drawRightString(VolString, 270, 30, 2);
  tft.drawRightString(ConverterString, 270, 50, 2);
  tft.drawRightString(LowEdgeString, 270, 70, 2);
  tft.drawRightString(HighEdgeString, 270, 90, 2);
  tft.drawRightString(LevelOffsetString, 270, 110, 2);
  tft.drawRightString(StereoLevelString, 270, 130, 2);
  tft.drawRightString(HighCutLevelString, 270, 150, 2);
  tft.drawRightString(HighCutOffsetString, 270, 170, 2);
  tft.drawRightString(LowLevelString, 270, 190, 2);
  tft.drawRightString(ContrastString, 270, 210, 2);
  analogWrite(SMETERPIN, 0);
}

void MuteScreen(int setting) {
  if (setting == 0) {
    screenmute = 0;
    setupmode = true;
    BuildDisplay();
    setupmode = false;
  } else {
    screenmute = 1;
    tft.fillScreen(BackgroundColor);
    tft.drawRect(0, 0, 320, 240, FrameColor);
    tft.setTextColor(SecondaryColor);
    tft.drawCentreString("Screen is muted!", 160, 30, 4);
    tft.drawCentreString("To unmute uncheck RF+ box", 160, 60, 2);
  }
}

void BuildMenu2() {
  tft.fillScreen(BackgroundColor);
  tft.drawRect(0, 0, 320, 240, FrameColor);
  tft.drawLine(0, 23, 320, 23, FrameColor);
  tft.drawLine(0, 205, 320, 205, FrameColor);
  tft.setTextColor(PrimaryColor);
  tft.drawString("PRESS MODE TO EXIT AND STORE", 20, 4, 2);
  tft.setTextColor(SecondaryColor);
  tft.drawRightString(VERSION, 305, 4, 2);
  tft.drawRoundRect(10, menuoption, 300, 18, 5, SecondaryColor);
  tft.setTextColor(SecondaryColor);

  tft.drawString("Color scheme", 20, 30, 2);
  tft.drawString("Signal units", 20, 50, 2);
  tft.drawString("Automatic RDS clear", 20, 70, 2);
  tft.drawString("XDR-GTK screen shutdown", 20, 90, 2);
  tft.drawString("Wi-Fi connectivity", 20, 110, 2);
  tft.drawString("Wi-Fi setup", 20, 130, 2);
  tft.drawString("Screen shutdown", 20, 150, 2);
  tft.setTextColor(PrimaryColor);

  tft.drawRightString(CurrentThemeString, 305, 30, 2);
  tft.drawRightString(SignalUnitsString, 305, 50, 2);
  tft.drawRightString(RDSClearString, 305, 70, 2);
  tft.drawRightString(XDRGTKShutdownString, 305, 90, 2);
  tft.drawRightString(WiFiSwitchString, 305, 110, 2);

  delay(100);

  tft.setTextColor(TFT_GREEN);
  if (batteryPercentage > 100) {
    batteryPercentage = 100;
  }

  if (batteryPercentage < 20) {
    tft.setTextColor(TFT_RED);
  }
  tft.drawString("Battery:", 20, 210, 1);

  if (WiFiSwitch == 0) {
    rawValue = analogRead(BATTERYPIN);
    voltageLevel = (rawValue / 4095.0) * 2 * 1.1 * 3.3 - 0.2;  // calculate voltage level
  }

  if (voltageLevel > 0.5) {
    tft.drawString(String(voltageLevel) + "V [" + String(rawValue) + "u]", 20, 219, 1);
    tft.drawString(String(batteryPercentage) + "%", 20, 228, 1);
  } else {
    tft.drawString("UNDETECTED", 20, 219, 1);
    tft.drawString(String(voltageLevel) + "V [" + String(rawValue) + "u]", 20, 228, 1);
  }

  /* Menu 2 Wi-Fi network info */

  tft.setTextColor(SecondaryColor);
  if (WiFiSwitch == 1) {
    tft.drawRightString("IP: " + ip, 305, 210, 1);
    tft.drawRightString("SSID: " + ssid, 305, 219, 1);
    tft.drawRightString("Signal: " + String(WiFi.RSSI()) + " dBm", 305, 228, 1);
  }
  analogWrite(SMETERPIN, 0);
}

void doTheme() {  // Use this to put your own colors in: http://www.barth-dev.de/online/rgb565-color-picker/
  switch (CurrentTheme) {
    case 0:  // Default PE5PVB theme
      PrimaryColor = 0xFFE0;
      SecondaryColor = 0xFFFF;
      FrequencyColor = 0xFFE0;
      FrameColor = 0x001F;
      GreyoutColor = 0x38E7;
      BackgroundColor = 0x0000;
      ActiveColor = 0xFFFF;
      OptimizerColor = 1;
      StereoColor = TFT_RED;
      RDSColor = TFT_SKYBLUE;
      CurrentThemeString = "Default";
      break;
    case 1:  // Cyan theme
      PrimaryColor = 0x0F3F;
      SecondaryColor = 0xFFFF;
      FrequencyColor = 0x0F3F;
      FrameColor = 0x0248;
      GreyoutColor = 0x4A69;
      BackgroundColor = 0x0000;
      ActiveColor = 0xFFFF;
      OptimizerColor = 1;
      StereoColor = 0xF3F;
      RDSColor = 0xFFFF;
      CurrentThemeString = "Cyan";
      break;
    case 2:  // Crimson theme
      PrimaryColor = 0xF8C3;
      SecondaryColor = 0xFFFF;
      FrequencyColor = 0xF8C3;
      FrameColor = 0x3800;
      GreyoutColor = 0x4A69;
      BackgroundColor = 0x0000;
      ActiveColor = 0xFFFF;
      OptimizerColor = 1;
      StereoColor = 0xF8C3;
      RDSColor = 0xFFFF;
      CurrentThemeString = "Crimson";
      break;
    case 3:  // Monochrome theme
      PrimaryColor = 0xFFFF;
      SecondaryColor = 0xFFFF;
      FrequencyColor = 0xFFFF;
      FrameColor = 0x2965;
      GreyoutColor = 0x4A69;
      BackgroundColor = 0x0000;
      ActiveColor = 0xFFFF;
      OptimizerColor = 1;
      StereoColor = 0xFFFF;
      RDSColor = 0xFFFF;
      CurrentThemeString = "Monochrome";
      break;
    case 4:  // Volcano theme
      PrimaryColor = 0xFC00;
      SecondaryColor = 0xFFFF;
      FrequencyColor = 0xFC00;
      FrameColor = 0x2965;
      GreyoutColor = 0x5140;
      BackgroundColor = 0x0806;
      ActiveColor = 0xFFFF;
      OptimizerColor = 1;
      StereoColor = 0xFC00;
      RDSColor = 0xFFFF;
      CurrentThemeString = "Volcano";
      break;
    case 5:  // Dendro theme
      PrimaryColor = TFT_GREEN;
      SecondaryColor = 0xFFFF;
      FrequencyColor = TFT_GREEN;
      FrameColor = 0x0200;
      GreyoutColor = 0x4A69;
      BackgroundColor = 0x0000;
      ActiveColor = 0xFFFF;
      OptimizerColor = 1;
      StereoColor = TFT_GREEN;
      RDSColor = TFT_YELLOW;
      CurrentThemeString = "Dendro";
      break;
    case 6:  // Sakura theme
      PrimaryColor = 0xF3D5;
      SecondaryColor = 0xFFFF;
      FrequencyColor = 0xF3D5;
      FrameColor = 0x3845;
      GreyoutColor = 0x4A69;
      BackgroundColor = 0x0000;
      ActiveColor = 0xFFFF;
      OptimizerColor = 1;
      StereoColor = 0xF3D5;
      RDSColor = TFT_WHITE;
      CurrentThemeString = "Sakura";
      break;
    case 7:  // Whiteout theme
      PrimaryColor = 0x0000;
      SecondaryColor = 0x0000;
      FrequencyColor = 0x18C3;
      FrameColor = 0x630C;
      GreyoutColor = 0x9492;
      BackgroundColor = 0xDFFC;  // FFFF White
      ActiveColor = 0x0000;
      OptimizerColor = 0xFFDF;
      StereoColor = 0x0000;
      RDSColor = 0x0000;
      CurrentThemeString = "Whiteout";
      break;
    default:
      PrimaryColor = 0xFFE0;
      SecondaryColor = 0xFFFF;
      FrequencyColor = 0xFFE0;
      FrameColor = 0x001F;
      GreyoutColor = 0x38E7;
      BackgroundColor = 0x0000;
      ActiveColor = 0xFFFF;
      OptimizerColor = 1;
      StereoColor = TFT_RED;
      RDSColor = TFT_SKYBLUE;
      CurrentThemeString = "Default";
      break;
  }
}

void BuildDisplay() {
  tft.fillScreen(BackgroundColor);
  tft.drawRect(0, 0, 320, 240, FrameColor);
  tft.drawLine(0, 30, 320, 30, FrameColor);
  tft.drawLine(0, 100, 320, 100, FrameColor);
  tft.drawLine(64, 30, 64, 0, FrameColor);
  tft.drawLine(210, 100, 210, 218, FrameColor);
  tft.drawLine(268, 30, 268, 0, FrameColor);
  tft.drawLine(0, 165, 210, 165, FrameColor);
  tft.drawLine(0, 187, 320, 187, FrameColor);
  tft.drawLine(0, 218, 320, 218, FrameColor);
  tft.drawLine(108, 30, 108, 0, FrameColor);
  tft.drawLine(174, 30, 174, 0, FrameColor);
  tft.drawLine(20, 120, 200, 120, TFT_DARKGREY);
  tft.drawLine(20, 150, 200, 150, TFT_DARKGREY);
  for (uint16_t segments = 0; segments < 94; segments++) {
    if (segments > 54) {
      if (((segments - 53) % 10) == 0)
        tft.fillRect(16 + (2 * segments), 117, 2, 3, TFT_RED);
    } else {
      if (((segments + 1) % 6) == 0)
        tft.fillRect(16 + (2 * segments), 117, 2, 3, TFT_GREEN);
    }
  }
  tft.setTextColor(SecondaryColor);
  tft.drawString("SQ:", 216, 155, 2);
  tft.drawString("S", 6, 106, 2);
  tft.drawString("M", 6, 136, 2);
  tft.drawString("PI:", 216, 195, 2);
  tft.drawString("PS:", 6, 195, 2);
  tft.drawString("PTY:", 6, 168, 2);
  tft.drawString("%", 196, 153, 1);
  tft.drawString("1", 24, 123, 1);
  tft.drawString("3", 48, 123, 1);
  tft.drawString("5", 72, 123, 1);
  tft.drawString("7", 96, 123, 1);
  tft.drawString("9", 120, 123, 1);
  tft.drawString("+10", 134, 123, 1);
  tft.drawString("+30", 174, 123, 1);
  tft.drawString("20", 20, 153, 1);
  tft.drawString("40", 50, 153, 1);
  tft.drawString("60", 80, 153, 1);
  tft.drawString("80", 110, 153, 1);
  tft.drawString("100", 134, 153, 1);
  tft.drawString("120", 164, 153, 1);
  tft.setTextColor(SecondaryColor);
  tft.drawString("kHz", 225, 6, 4);
  tft.setTextColor(SecondaryColor);
  if (band == 0) {
    tft.drawString("MHz", 256, 78, 4);
    tft.drawString("S/N", 250, 168, 2);
    tft.drawString("dB", 300, 168, 2);
  } else {
    tft.drawString("kHz", 256, 78, 4);
  }
  tft.setTextColor(SecondaryColor);
  if (SignalUnits == 1) {
    tft.drawString("dBf", 295, 155, 2);
  } else {
    tft.drawString("dBuV", 280, 155, 2);
    tft.drawPixel(295, 166, SecondaryColor);
    tft.drawPixel(295, 167, SecondaryColor);
    tft.drawPixel(295, 168, SecondaryColor);
    tft.drawPixel(295, 169, SecondaryColor);
    tft.drawPixel(295, 170, SecondaryColor);
  }
  RDSstatusold = false;
  Stereostatusold = false;
  ShowFreq(0);
  updateTuneMode();
  updateBW();
  ShowUSBstatus();
  ShowStepSize();
  updateiMS();
  updateEQ();
  Squelchold = -2;
  SStatusold = 2000;
  SStatus = 100;
  tft.drawCircle(81, 15, 10, GreyoutColor);
  tft.drawCircle(81, 15, 9, GreyoutColor);
  tft.drawCircle(91, 15, 10, GreyoutColor);
  tft.drawCircle(91, 15, 9, GreyoutColor);
  tft.drawBitmap(110, 5, RDSLogo, 67, 22, GreyoutColor);
  if (StereoToggle == false) {
    tft.drawCircle(86, 15, 10, PrimaryColor);
    tft.drawCircle(86, 15, 9, PrimaryColor);
  }
}

void ShowFreq(int mode) {
  if (setupmode == false) {
    if (band == 1) {
      if (freqold < 2000 && radio.getFrequency_AM() >= 2000 && stepsize == 0) {
        if (radio.getFrequency_AM() != 27000 && freqold != 144) {
          radio.setFrequency_AM(2000);
        }
      }

      if (freqold >= 2000 && radio.getFrequency_AM() < 2000 && stepsize == 0) {
        if (radio.getFrequency_AM() != 144 && freqold != 27000) {
          radio.setFrequency_AM(1998);
        }
      }
    }
  }

  if (screenmute == false) {
    /*    detachInterrupt(digitalPinToInterrupt(ROTARY_PIN_A));*/
    //detachInterrupt(digitalPinToInterrupt(ROTARY_PIN_B));
    if (band == 1) {
      unsigned int freq = radio.getFrequency_AM();
      String count = String(freq, DEC);
      if (count.length() != freqoldcount || mode != 0) {
        tft.setTextColor(BackgroundColor);
        tft.drawRightString(String(freqold), 248, 45, 7);
      }
      tft.setTextColor(FrequencyColor, BackgroundColor);
      tft.drawRightString(String(freq), 248, 45, 7);
      freqold = freq;
      freqoldcount = count.length();
    } else {
      unsigned int freq = radio.getFrequency() + ConverterSet * 100;
      String count = String(freq / 100, DEC);
      if (count.length() != freqoldcount || mode != 0) {
        tft.setTextColor(BackgroundColor);
        if (freqoldcount <= 2) {
          tft.setCursor(108, 45);
        }
        if (freqoldcount == 3) {
          tft.setCursor(76, 45);
        }
        if (freqoldcount == 4) {
          tft.setCursor(44, 45);
        }
        tft.setTextFont(7);
        tft.print(freqold / 100);
        tft.print(".");
        if (freqold % 100 < 10) {
          tft.print("0");
        }
        tft.print(freqold % 100);
      }

      tft.setTextColor(FrequencyColor, BackgroundColor);
      if (mode == 0) {
        if (count.length() <= 2) {
          tft.setCursor(108, 45);
        }
        if (count.length() == 3) {
          tft.setCursor(76, 45);
        }
        if (count.length() == 4) {
          tft.setCursor(44, 45);
        }
        tft.setTextFont(7);
        tft.print(freq / 100);
        tft.print(".");
        if (freq % 100 < 10) {
          tft.print("0");
        }
        tft.print(freq % 100);
        freqold = freq;
        freqoldcount = count.length();
      } else if (mode == 1) {
        tft.setTextColor(BackgroundColor);
        if (freqoldcount <= 2) {
          tft.setCursor(98, 45);
        }
        if (freqoldcount == 3) {
          tft.setCursor(71, 45);
        }
        if (freqoldcount == 4) {
          tft.setCursor(44, 45);
        }
        tft.setTextFont(1);
        tft.print(freqold / 100);
        tft.print(".");
        if (freqold % 100 < 10) {
          tft.print("0");
        }
        tft.print(freqold % 100);
      }
    }
    attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_A), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_B), encoderISR, CHANGE);
  }
}

void ShowSignalLevel() {
  if (band == 0) {
    SNR = int(0.46222375 * (float)(SStatus / 10) - 0.082495118 * (float)(USN / 10)) + 10;
  } else {
    SNR = -1;
    //SNR = -((int8_t)(USN / 10));
  }

  if ((SNR > (SNRold + 1) || SNR < (SNRold - 1)) && band == 0) {
    tft.setTextFont(2);
    tft.setCursor(280, 168);
    tft.setTextColor(BackgroundColor);
    if (SNRold == 99) {
      tft.print("--");
    } else {
      tft.print(SNRold);
    }
    tft.setCursor(280, 168);
    tft.setTextColor(PrimaryColor, BackgroundColor);
    if (tuned == true) {
      tft.print(SNR);
      SNRold = SNR;
    } else {
      tft.print("--");
      SNRold = 99;
    }
  }

  if (true) {
    int offset = (SStatus < 0) ? -5 : 5;
    // If the jump in S-meter value is less than 12dB, average the value; otherwise just use the new value as average

    if (abs((SAvg / 10) - SStatus) < 120) {
      SAvg = (((SAvg * 9) + offset) / 10) + SStatus;
      SStatus = (SAvg / 10);
    } else
      SAvg = SStatus * 10;
  }

  int16_t smeter = 0;
  int16_t segments;
  float sval = 0;
  if (SStatus > 0) {
    if (SStatus < 1000) {
      sval = 51 * ((pow(10, (((float)SStatus) / 1000))) - 1);
      smeter = int16_t(sval);
    } else {
      smeter = 511;
    }
  }
  if (menu == false && menu2 == false) {
    analogWrite(SMETERPIN, smeter);
  }
  if (SStatus > (SStatusold + 3) || SStatus < (SStatusold - 3)) {
    if (SStatus > 1200) {
      SStatus = 1200;
    }
    if (SStatus < -400) {
      SStatus = -400;
    }

    String count;

    if (SignalUnits == 1) {
      int16_t myVal = SStatus + 112;
      String count = String(abs(myVal / 10), DEC);
      tft.setTextColor(OptimizerColor, BackgroundColor);  //DSP; stupid compiler optimizes black/black away.
      tft.setCursor(213, 110);
      tft.setTextFont(6);
      // Using 'invisible' zero's to get the size 'right', ditto for spaces (half width,like '-' [alomost])
      if (myVal >= 0) {
        if (count.length() == 1) tft.print("00");
        if (count.length() == 2) tft.print("0");
      } else {
        if (count.length() == 1) tft.print("0 ");
        if (count.length() == 2) tft.print(" ");
      }
      tft.setTextColor(PrimaryColor, BackgroundColor);  //DSP
      if (myVal < 0) tft.print("-");
      tft.print(abs(myVal / 10));
      tft.setCursor(294, 110);
      tft.setTextFont(4);
      tft.print(".");
      if (myVal < 0) {
        String negative = String(myVal % 10, DEC);
        if (myVal % 10 == 0) {
          tft.print("0");
        } else {
          tft.print(negative[1]);
        }
      } else {
        tft.print(myVal % 10);
      }
    } else {
      String count = String(abs(SStatus / 10), DEC);
      tft.setTextColor(OptimizerColor, BackgroundColor);  //DSP; stupid compiler optimizes black/black away.
      tft.setCursor(213, 110);
      tft.setTextFont(6);
      // Using 'invisible' zero's to get the size 'right', ditto for spaces (half width,like '-' [alomost])
      if (SStatus >= 0) {
        if (count.length() == 1) tft.print("00");
        if (count.length() == 2) tft.print("0");
      } else {
        if (count.length() == 1) tft.print("0 ");
        if (count.length() == 2) tft.print(" ");
      }
      tft.setTextColor(PrimaryColor, BackgroundColor);  //DSP
      if (SStatus < 0) tft.print("-");
      tft.print(abs(SStatus / 10));
      tft.setCursor(294, 110);
      tft.setTextFont(4);
      tft.print(".");
      if (SStatus < 0) {
        String negative = String(SStatus % 10, DEC);
        if (SStatus % 10 == 0) {
          tft.print("0");
        } else {
          tft.print(negative[1]);
        }
      } else {
        tft.print(SStatus % 10);
      }
    }

    if (band == 0)
      segments = (SStatus + 200) / 10;
    else
      segments = (SStatus + 200) / 10;  // Correct the S meter value for AM, by adding a value of 20

    tft.fillRect(16, 109, 2 * constrain(segments, 0, 54), 8, TFT_GREEN);
    tft.fillRect(16 + 2 * 54, 109, 2 * (constrain(segments, 54, 94) - 54), 8, TFT_RED);
    tft.fillRect(16 + 2 * constrain(segments, 0, 94), 109, 2 * (94 - constrain(segments, 0, 94)), 8, GreyoutColor);

    SStatusold = SStatus;
    SStatusoldcount = count.length();
  }
}

void ShowRDSLogo(bool RDSstatus) {
  if (screenmute == false) {
    if (RDSstatus != RDSstatusold) {
      if (RDSstatus == true) {
        tft.drawBitmap(110, 5, RDSLogo, 67, 22, RDSColor);
      } else {
        tft.drawBitmap(110, 5, RDSLogo, 67, 22, GreyoutColor);
      }
      RDSstatusold = RDSstatus;
    }
  }
}

void ShowStereoStatus() {
  if (StereoToggle == true) {
    if (band == 0) {
      Stereostatus = radio.getStereoStatus();
    } else {
      Stereostatus = 0;
    }
    if (Stereostatus != Stereostatusold) {
      if (Stereostatus == true && screenmute == false) {
        tft.drawCircle(81, 15, 10, StereoColor);
        tft.drawCircle(81, 15, 9, StereoColor);
        tft.drawCircle(91, 15, 10, StereoColor);
        tft.drawCircle(91, 15, 9, StereoColor);
      } else {
        if (screenmute == false) {
          tft.drawCircle(81, 15, 10, GreyoutColor);
          tft.drawCircle(81, 15, 9, GreyoutColor);
          tft.drawCircle(91, 15, 10, GreyoutColor);
          tft.drawCircle(91, 15, 9, GreyoutColor);
        }
      }
      Stereostatusold = Stereostatus;
    }
  }
}

void ShowOffset() {
  if (OStatus != OStatusold) {
    if (band == 0) {
      if (OStatus < -500) {
        tft.fillTriangle(6, 8, 6, 22, 14, 14, GreyoutColor);
        tft.fillTriangle(18, 8, 18, 22, 26, 14, GreyoutColor);
        tft.fillCircle(32, 15, 3, GreyoutColor);
        tft.fillTriangle(38, 14, 46, 8, 46, 22, GreyoutColor);
        tft.fillTriangle(50, 14, 58, 8, 58, 22, TFT_RED);
        tuned = false;
      } else if (OStatus < -250 && OStatus > -500) {
        tft.fillTriangle(6, 8, 6, 22, 14, 14, GreyoutColor);
        tft.fillTriangle(18, 8, 18, 22, 26, 14, GreyoutColor);
        tft.fillCircle(32, 15, 3, GreyoutColor);
        tft.fillTriangle(38, 14, 46, 8, 46, 22, TFT_RED);
        tft.fillTriangle(50, 14, 58, 8, 58, 22, GreyoutColor);
        tuned = false;
      } else if (USN < 250 && WAM < 250 && OStatus > -250 && OStatus < 250 && SQ == false) {
        tft.fillTriangle(6, 8, 6, 22, 14, 14, GreyoutColor);
        tft.fillTriangle(18, 8, 18, 22, 26, 14, GreyoutColor);
        tft.fillCircle(32, 15, 3, TFT_GREEN);
        tft.fillTriangle(38, 14, 46, 8, 46, 22, GreyoutColor);
        tft.fillTriangle(50, 14, 58, 8, 58, 22, GreyoutColor);
        tuned = true;
      } else if (OStatus > 250 && OStatus < 500) {
        tft.fillTriangle(6, 8, 6, 22, 14, 14, GreyoutColor);
        tft.fillTriangle(18, 8, 18, 22, 26, 14, TFT_RED);
        tft.fillCircle(32, 15, 3, GreyoutColor);
        tft.fillTriangle(38, 14, 46, 8, 46, 22, GreyoutColor);
        tft.fillTriangle(50, 14, 58, 8, 58, 22, GreyoutColor);
        tuned = false;
      } else if (OStatus > 500) {
        tft.fillTriangle(6, 8, 6, 22, 14, 14, TFT_RED);
        tft.fillTriangle(18, 8, 18, 22, 26, 14, GreyoutColor);
        tft.fillCircle(32, 15, 3, GreyoutColor);
        tft.fillTriangle(38, 14, 46, 8, 46, 22, GreyoutColor);
        tft.fillTriangle(50, 14, 58, 8, 58, 22, GreyoutColor);
        tuned = false;
      } else {
        tft.fillTriangle(6, 8, 6, 22, 14, 14, GreyoutColor);
        tft.fillTriangle(18, 8, 18, 22, 26, 14, GreyoutColor);
        tft.fillCircle(32, 15, 3, GreyoutColor);
        tft.fillTriangle(38, 14, 46, 8, 46, 22, GreyoutColor);
        tft.fillTriangle(50, 14, 58, 8, 58, 22, GreyoutColor);
        tuned = false;
      }
    } else {
      if (OStatus <= -3) {
        tft.fillTriangle(6, 8, 6, 22, 14, 14, GreyoutColor);
        tft.fillTriangle(18, 8, 18, 22, 26, 14, GreyoutColor);
        tft.fillCircle(32, 15, 3, GreyoutColor);
        tft.fillTriangle(38, 14, 46, 8, 46, 22, GreyoutColor);
        tft.fillTriangle(50, 14, 58, 8, 58, 22, TFT_RED);
        tuned = false;
      } else if (OStatus < -2 && OStatus > -3) {
        tft.fillTriangle(6, 8, 6, 22, 14, 14, GreyoutColor);
        tft.fillTriangle(18, 8, 18, 22, 26, 14, GreyoutColor);
        tft.fillCircle(32, 15, 3, GreyoutColor);
        tft.fillTriangle(38, 14, 46, 8, 46, 22, TFT_RED);
        tft.fillTriangle(50, 14, 58, 8, 58, 22, GreyoutColor);
        tuned = false;
      } else if (OStatus > -2 && OStatus < 2 && SQ == false) {
        tft.fillTriangle(6, 8, 6, 22, 14, 14, GreyoutColor);
        tft.fillTriangle(18, 8, 18, 22, 26, 14, GreyoutColor);
        tft.fillCircle(32, 15, 3, TFT_GREEN);
        tft.fillTriangle(38, 14, 46, 8, 46, 22, GreyoutColor);
        tft.fillTriangle(50, 14, 58, 8, 58, 22, GreyoutColor);
        tuned = true;
      } else if (OStatus > 2 && OStatus < 3) {
        tft.fillTriangle(6, 8, 6, 22, 14, 14, GreyoutColor);
        tft.fillTriangle(18, 8, 18, 22, 26, 14, TFT_RED);
        tft.fillCircle(32, 15, 3, GreyoutColor);
        tft.fillTriangle(38, 14, 46, 8, 46, 22, GreyoutColor);
        tft.fillTriangle(50, 14, 58, 8, 58, 22, GreyoutColor);
        tuned = false;
      } else if (OStatus >= 3) {
        tft.fillTriangle(6, 8, 6, 22, 14, 14, TFT_RED);
        tft.fillTriangle(18, 8, 18, 22, 26, 14, GreyoutColor);
        tft.fillCircle(32, 15, 3, GreyoutColor);
        tft.fillTriangle(38, 14, 46, 8, 46, 22, GreyoutColor);
        tft.fillTriangle(50, 14, 58, 8, 58, 22, GreyoutColor);
        tuned = false;
      } else {
        tft.fillTriangle(6, 8, 6, 22, 14, 14, GreyoutColor);
        tft.fillTriangle(18, 8, 18, 22, 26, 14, GreyoutColor);
        tft.fillCircle(32, 15, 3, GreyoutColor);
        tft.fillTriangle(38, 14, 46, 8, 46, 22, GreyoutColor);
        tft.fillTriangle(50, 14, 58, 8, 58, 22, GreyoutColor);
        tuned = false;
      }
    }
    OStatusold = OStatus;
  }
}

void ShowBW() {
  if (millis() >= BWCLOCK + BWINTERVAL) {
    if (BW != BWOld || BWreset == true) {
      String BWString = String(BW, DEC);
      String BWOldString = String(BWOld, DEC);
      tft.setTextColor(BackgroundColor);
      tft.drawRightString(BWOldString, 218, 6, 4);
      if (BWset == 0) {
        tft.setTextColor(PrimaryColor);
      } else {
        tft.setTextColor(PrimaryColor);
      }
      tft.drawRightString(BWString, 218, 6, 4);
      BWOld = BW;
      BWreset = false;
    }
    BWCLOCK += BWINTERVAL;
  }
}

void ShowModLevel() {
  int segments;
  int color;
  int hold = 0;

  if (SQ == false) {
  } else {
    MStatus = 0;
    MStatusold = 1;
  }

  if (MStatus != MStatusold || MStatus < 10) {
    for (segments = 0; segments < 13; segments++) {
      color = TFT_GREEN;
      if (segments > 8)
        color = TFT_ORANGE;
      if (segments > 9)
        color = TFT_RED;
      if (MStatus > (segments + 1) * 10) {
        hold = segments;
        tft.fillRect(20 + segments * 14, 139, 12, 8, color);
      } else {
        if (segments != peakholdold)  // Only remove a segment if it's not the peak segment
          tft.fillRect(20 + segments * 14, 139, 12, 8, GreyoutColor);
      }
    }

    if (peakholdold < hold) {
      peakholdold = hold;
    }
    if (peakholdmillis > peakholdtimer + 3000) {
      peakholdtimer += 3000;
      peakholdold = hold;
    }
    peakholdmillis = millis();
    MStatusold = MStatus;
  }
}

void doSquelch() {
  if (USBstatus == false && WiFistatus == false) {
    Squelch = analogRead(PIN_POT) / 4 - 100;
    if (Squelch > 920) {
      Squelch = 920;
    }
    if (seek == false && menu == false && menu2 == false && Squelch != Squelchold) {
      tft.setTextFont(2);
      tft.setTextColor(BackgroundColor);
      tft.setCursor(239, 155);
      if (Squelchold == -100) {
        tft.print("OFF");
      } else if (Squelchold == 920) {
        tft.print("ST");
      } else {
        tft.print(Squelchold / 10);
      }
      tft.setTextColor(SecondaryColor);
      tft.setCursor(239, 155);
      if (Squelch == -100) {
        tft.print("OFF");
      } else if (Squelch == 920) {
        tft.print("ST");
      } else {
        tft.print(Squelch / 10);
      }
      Squelchold = Squelch;
    }
  }
  if (seek == false && USBstatus == true) {
    if (XDRMute == false) {
      if (Squelch != -1) {
        if (Squelch < SStatus || Squelch == -100) {
          radio.setUnMute();
          SQ = false;
        } else {
          radio.setMute();
          SQ = true;
        }
      } else {
        if (Stereostatus == true) {
          radio.setUnMute();
          SQ = false;
        } else {
          radio.setMute();
          SQ = true;
        }
      }
      if (screenmute == false) {
        if (Squelch != Squelchold) {
          tft.setTextFont(2);
          tft.setTextColor(BackgroundColor);
          tft.setCursor(239, 155);
          if (Squelchold == -1) {
            tft.print("ST");
          } else {
            tft.print(Squelchold / 10);
          }
          tft.setTextColor(SecondaryColor);
          tft.setCursor(239, 155);
          if (Squelch == -1) {
            tft.print("ST");
          } else {
            tft.print(Squelch / 10);
          }
          Squelchold = Squelch;
        }
      }
    }
  } else {
    if (seek == false && Squelch != 920) {
      if (Squelch < SStatus || Squelch == -100) {
        radio.setUnMute();
        SQ = false;
      } else {
        radio.setMute();
        SQ = true;
      }
    } else {
      if (seek == false && Stereostatus == true) {
        radio.setUnMute();
        SQ = false;
      } else {
        radio.setMute();
        SQ = true;
      }
    }
  }
  ShowSquelch();
}

void ShowSquelch() {
  if (menu == false && menu2 == false) {
    if (SQ == false) {
      tft.drawRoundRect(3, 79, 40, 20, 5, GreyoutColor);
      tft.setTextColor(GreyoutColor);
      tft.drawCentreString("MUTE", 24, 81, 2);
    } else {
      tft.drawRoundRect(3, 79, 40, 20, 5, SecondaryColor);
      tft.setTextColor(SecondaryColor);
      tft.drawCentreString("MUTE", 24, 81, 2);
    }
  }
}


void updateBW() {
  if (band == 0) {
    bool isAuto = (BWset == 0);
    if (screenmute == false) {
      uint16_t color = isAuto ? SecondaryColor : GreyoutColor;
      tft.drawRoundRect(249, 35, 68, 20, 5, color);
      tft.setTextColor(color);
      tft.drawCentreString("AUTO BW", 283, 37, 2);
    }
    if (isAuto) {
      radio.setFMABandw();
    }
  }
}

void updateiMS() {
  if (band == 0) {
    bool isSet = (iMSset == 0);
    if (screenmute == false) {
      uint16_t color = isSet ? SecondaryColor : GreyoutColor;
      tft.drawRoundRect(249, 56, 30, 20, 5, color);
      tft.setTextColor(color);
      tft.drawCentreString("iMS", 265, 58, 2);
    }
    radio.setiMS(isSet ? 1 : 0);
  }
}

void updateEQ() {
  if (band == 0) {
    bool isSet = (EQset == 0);
    if (screenmute == false) {
      uint16_t color = isSet ? SecondaryColor : GreyoutColor;
      tft.drawRoundRect(287, 56, 30, 20, 5, color);
      tft.setTextColor(color);
      tft.drawCentreString("EQ", 303, 58, 2);
    }
    radio.setEQ(isSet ? 1 : 0);
  }
}

void doBW() {
  if (band == 0) {
    if (BWset > 16) {
      BWset = 0;
    }

    switch (BWset) {
      case 1: radio.setFMBandw(56); break;
      case 2: radio.setFMBandw(64); break;
      case 3: radio.setFMBandw(72); break;
      case 4: radio.setFMBandw(84); break;
      case 5: radio.setFMBandw(97); break;
      case 6: radio.setFMBandw(114); break;
      case 7: radio.setFMBandw(133); break;
      case 8: radio.setFMBandw(151); break;
      case 9: radio.setFMBandw(168); break;
      case 10: radio.setFMBandw(184); break;
      case 11: radio.setFMBandw(200); break;
      case 12: radio.setFMBandw(217); break;
      case 13: radio.setFMBandw(236); break;
      case 14: radio.setFMBandw(254); break;
      case 15: radio.setFMBandw(287); break;
      case 16: radio.setFMBandw(311); break;
    }
  } else {
    if (BWset > 4) {
      BWset = 1;
    }

    switch (BWset) {
      case 1: radio.setAMBandw(3); break;
      case 2: radio.setAMBandw(4); break;
      case 3: radio.setAMBandw(6); break;
      case 4: radio.setAMBandw(8); break;
    }
  }
  updateBW();
  BWreset = true;
}

void doTuneMode() {
  if (band == 0) {
    if (tunemode == true) {
      tunemode = false;
    } else {
      tunemode = true;
    }
    updateTuneMode();
    if (stepsize != 0) {
      stepsize = 0;
      RoundStep();
      ShowStepSize();
      ShowFreq(0);
    }
  }
}

void updateTuneMode() {
  if (tunemode == true) {
    tft.drawRoundRect(3, 57, 40, 20, 5, SecondaryColor);
    tft.setTextColor(SecondaryColor);
    tft.drawCentreString("AUTO", 24, 59, 2);

    tft.drawRoundRect(3, 35, 40, 20, 5, GreyoutColor);
    tft.setTextColor(GreyoutColor);
    tft.drawCentreString("MAN", 24, 37, 2);
  } else {
    tft.drawRoundRect(3, 57, 40, 20, 5, GreyoutColor);
    tft.setTextColor(GreyoutColor);
    tft.drawCentreString("AUTO", 24, 59, 2);

    tft.drawRoundRect(3, 35, 40, 20, 5, SecondaryColor);
    tft.setTextColor(SecondaryColor);
    tft.drawCentreString("MAN", 24, 37, 2);
  }
}

void ShowUSBstatus() {
  if (USBstatus == true) {
    tft.drawBitmap(272, 6, USBLogo, 21, 21, ActiveColor);
    if (XDRGTKShutdown == 1 && analogRead(CONTRASTPIN) != 0) {
      analogWrite(CONTRASTPIN, 0);
    }
  } else {
    tft.drawBitmap(272, 6, USBLogo, 21, 21, GreyoutColor);
  }

  if (WiFi.status() == WL_CONNECTED && WiFistatus == false) {
    tft.drawBitmap(294, 6, WiFiLogo, 21, 21, PrimaryColor);
  } else if (WiFi.status() == WL_CONNECTED && WiFistatus == true) {
    tft.drawBitmap(294, 6, WiFiLogo, 21, 21, ActiveColor);
    if (XDRGTKShutdown == 1 && analogRead(CONTRASTPIN) != 0) {
      analogWrite(CONTRASTPIN, 0);
    }
  } else {
    tft.drawBitmap(294, 6, WiFiLogo, 21, 21, GreyoutColor);
  }
}

void passwordcrypt() {
  int generated = 0;
  while (generated < 16) {
    byte randomValue = random(0, 26);
    char letter = randomValue + 'a';
    if (randomValue > 26)
      letter = (randomValue - 26);
    saltkey.setCharAt(generated, letter);
    generated++;
  }
  salt = saltkey + password;
  cryptedpassword = String(sha1(salt));
}

void XDRGTKRoutine() {
  if (Serial.available() > 0) {
    buff[buff_pos] = Serial.read();
    if (buff[buff_pos] != '\n' && buff_pos != 16 - 1)
      buff_pos++;
    else {
      buff[buff_pos] = 0;
      buff_pos = 0;
      switch (buff[0]) {
        case 'x':
          Serial.println("OK");
          if (band != 0) {
            band = 0;
            SelectBand();
          }
          Serial.print("T" + String(frequency * 10));
          Serial.print("A0\n");
          Serial.print("D0\n");
          Serial.print("G00\n");
          USBstatus = true;
          ShowUSBstatus();
          if (menu == true) {
            ModeButtonPress();
          }
          if (Squelch != Squelchold) {
            if (screenmute == false) {
              tft.setTextFont(2);
              tft.setTextColor(BackgroundColor);
              tft.setCursor(240, 168);
              if (Squelchold == -100) {
                tft.print("OFF");
              } else if (Squelchold > 920) {
                tft.print("ST");
              } else {
                tft.print(Squelchold / 10);
              }
            }
          }
          break;

        case 'A':
          AGC = atol(buff + 1);
          Serial.print("A");
          Serial.print(AGC);
          Serial.print("\n");
          radio.setAGC(AGC);
          break;

        case 'C':
          byte scanmethod;
          scanmethod = atol(buff + 1);
          Serial.print("C");
          if (seek == false) {
            if (scanmethod == 1) {
              Serial.print("1\n");
              direction = true;
              seek = true;
              Seek(direction);
            }
            if (scanmethod == 2) {
              Serial.print("2\n");
              direction = false;
              seek = true;
              Seek(direction);
            }
          } else {
            seek = false;
          }
          Serial.print("C0\n");
          break;

        case 'N':
          doStereoToggle();
          break;

        case 'D':
          DeEmphasis = atol(buff + 1);
          Serial.print("D");
          Serial.print(DeEmphasis);
          Serial.print("\n");
          radio.setDeemphasis(DeEmphasis);
          break;

        case 'F':
          XDRBWset = atol(buff + 1);
          Serial.print("F");

          if (XDRBWset < 0) {
            XDRBWsetold = XDRBWset;
            BWset = 0;
          }

          switch (XDRBWset) {
            case 0:
              XDRBWsetold = XDRBWset;
              BWset = 1;
              break;
            case 1:
              XDRBWsetold = XDRBWset;
              BWset = 2;
              break;
            case 2:
              XDRBWsetold = XDRBWset;
              BWset = 3;
              break;
            case 3:
              XDRBWsetold = XDRBWset;
              BWset = 4;
              break;
            case 4:
              XDRBWsetold = XDRBWset;
              BWset = 5;
              break;
            case 5:
              XDRBWsetold = XDRBWset;
              BWset = 6;
              break;
            case 6:
              XDRBWsetold = XDRBWset;
              BWset = 7;
              break;
            case 7:
              XDRBWsetold = XDRBWset;
              BWset = 8;
              break;
            case 8:
              XDRBWsetold = XDRBWset;
              BWset = 9;
              break;
            case 9:
              XDRBWsetold = XDRBWset;
              BWset = 10;
              break;
            case 10:
              XDRBWsetold = XDRBWset;
              BWset = 11;
              break;
            case 11:
              XDRBWsetold = XDRBWset;
              BWset = 12;
              break;
            case 12:
              XDRBWsetold = XDRBWset;
              BWset = 13;
              break;
            case 13:
              XDRBWsetold = XDRBWset;
              BWset = 14;
              break;
            case 14:
              XDRBWsetold = XDRBWset;
              BWset = 15;
              break;
            case 15:
              XDRBWsetold = XDRBWset;
              BWset = 16;
              break;
            default: XDRBWset = XDRBWsetold; break;
          }
          doBW();
          Serial.print(XDRBWset);
          Serial.print("\n");
          break;

        case 'G':
          LevelOffset = atol(buff + 1);
          Serial.print("G");
          if (LevelOffset == 0) {
            MuteScreen(0);
            LowLevelSet = EEPROM.readByte(43);
            EEPROM.commit();
            Serial.print("00\n");
          }
          if (LevelOffset == 10) {
            MuteScreen(1);
            LowLevelSet = EEPROM.readByte(43);
            EEPROM.commit();
            Serial.print("10\n");
          }
          if (LevelOffset == 1) {
            MuteScreen(0);
            LowLevelSet = 120;
            Serial.print("01\n");
          }
          if (LevelOffset == 11) {
            LowLevelSet = 120;
            MuteScreen(1);
            Serial.print("11\n");
          }
          break;

        case 'M':
          byte XDRband;
          XDRband = atol(buff + 1);
          if (XDRband == 0) {
            band = 0;
            SelectBand();
            Serial.print("M0\n");
            Serial.print("T" + String(frequency * 10) + "\n");
          } else {
            band = 1;
            SelectBand();
            Serial.print("M1\n");
            Serial.print("T" + String(frequency_AM) + "\n");
          }
          break;

        case 'T':
          unsigned int freqtemp;
          freqtemp = atoi(buff + 1);
          if (seek == true) {
            seek = false;
          }
          if (freqtemp > 143 && freqtemp < 27001) {
            frequency_AM = freqtemp;
            if (band != 1) {
              band = 1;
              SelectBand();
            } else {
              radio.setFrequency_AM(frequency_AM);
            }
            Serial.print("M1\n");
          } else if (freqtemp > 64999 && freqtemp < 108001) {
            frequency = freqtemp / 10;
            if (band != 0) {
              band = 0;
              SelectBand();
              Serial.print("M0\n");
            } else {
              radio.setFrequency(frequency, 65, 108);
            }
          }
          if (band == 0) {
            Serial.print("T" + String(frequency * 10) + "\n");
          } else {
            Serial.print("T" + String(frequency_AM) + "\n");
          }
          radio.clearRDS();
          sprite.fillSprite(BackgroundColor);
          sprite.pushSprite(6, 220);
          ShowFreq(0);
          break;

        case 'S':
          if (buff[1] == 'a') {
            scanner_start = (atol(buff + 2) + 5) / 10;
          } else if (buff[1] == 'b') {
            scanner_end = (atol(buff + 2) + 5) / 10;
          } else if (buff[1] == 'c') {
            scanner_step = (atol(buff + 2) + 5) / 10;
          } else if (buff[1] == 'f') {
            scanner_filter = atol(buff + 2);
          } else if (scanner_start > 0 && scanner_end > 0 && scanner_step > 0 && scanner_filter >= 0) {
            frequencyold = radio.getFrequency();
            radio.setFrequency(scanner_start, 65, 108);
            Serial.print('U');
            switch (scanner_filter) {
              case -1: BWset = 0; break;
              case 0: BWset = 1; break;
              case 26: BWset = 2; break;
              case 1: BWset = 3; break;
              case 28: BWset = 4; break;
              case 29: BWset = 5; break;
              case 3: BWset = 6; break;
              case 4: BWset = 7; break;
              case 5: BWset = 8; break;
              case 7: BWset = 9; break;
              case 8: BWset = 10; break;
              case 9: BWset = 11; break;
              case 10: BWset = 12; break;
              case 11: BWset = 13; break;
              case 12: BWset = 14; break;
              case 13: BWset = 15; break;
              case 15: BWset = 16; break;
              default: BWset = 0; break;
            }
            doBW();
            if (screenmute == false) {
              ShowFreq(1);
              tft.setTextFont(4);
              tft.setTextColor(SecondaryColor, BackgroundColor);
              tft.setCursor(90, 60);
              tft.print("SCANNING...");
            }
            frequencyold = frequency / 10;
            for (freq_scan = scanner_start; freq_scan <= scanner_end; freq_scan += scanner_step) {
              radio.setFrequency(freq_scan, 65, 108);
              Serial.print(freq_scan * 10, DEC);
              Serial.print('=');
              delay(10);
              if (band == 0) {
                radio.getStatus(SStatus, USN, WAM, OStatus, BW, MStatus);
              } else {
                radio.getStatus_AM(SStatus, USN, WAM, OStatus, BW, MStatus);
              }
              Serial.print((SStatus / 10) + 10, DEC);
              Serial.print(',');
            }
            Serial.print('\n');
            if (screenmute == false) {
              tft.setTextFont(4);
              tft.setTextColor(BackgroundColor);
              tft.setCursor(90, 60);
              tft.print("SCANNING...");
            }
            radio.setFrequency(frequencyold, 65, 108);
            if (screenmute == false) {
              ShowFreq(0);
            }
            radio.setFMABandw();
          }
          break;

        case 'Y':
          VolSet = atoi(buff + 1);
          if (VolSet == 0) {
            radio.setMute();
            XDRMute = true;
            SQ = true;
          } else {
            radio.setVolume((VolSet - 70) / 10);
            XDRMute = false;
          }
          Serial.print("Y");
          Serial.print(VolSet);
          Serial.print("\n");
          break;

        case 'X':
          Serial.print("X\n");
          ESP.restart();
          break;

        case 'Z':
          byte iMSEQX;
          iMSEQX = atol(buff + 1);
          Serial.print("Z");
          if (iMSEQX == 0) {
            iMSset = 1;
            EQset = 1;
            iMSEQ = 2;
          }
          if (iMSEQX == 1) {
            iMSset = 0;
            EQset = 1;
            iMSEQ = 3;
          }
          if (iMSEQX == 2) {
            iMSset = 1;
            EQset = 0;
            iMSEQ = 4;
          }
          if (iMSEQX == 3) {
            iMSset = 0;
            EQset = 0;
            iMSEQ = 1;
          }
          updateiMS();
          updateEQ();
          Serial.print(iMSEQX);
          Serial.print("\n");
          break;
      }
    }
  }

  if (USBstatus == true) {
    Stereostatus = radio.getStereoStatus();
    Serial.print("S");
    if (StereoToggle == false) {
      Serial.print("S");
    } else if (Stereostatus == true && band == 0) {
      Serial.print("s");
    } else {
      Serial.print("m");
    }
    if (SStatus > (SStatusold + 10) || SStatus < (SStatusold - 10)) {
      Serial.print(((SStatus * 100) + 10875) / 1000);
      Serial.print(".");
      Serial.print(((SStatus * 100) + 10875) / 100 % 10);
    } else {
      Serial.print(((SStatusold * 100) + 10875) / 1000);
      Serial.print(".");
      Serial.print(((SStatusold * 100) + 10875) / 100 % 10);
    }
    Serial.print(',');
    Serial.print(WAM / 10, DEC);
    Serial.print(',');
    Serial.print(SNR, DEC);
    Serial.print("\n");
  }
}

void tcp_hex(uint8_t val) {
  RemoteClient.print((val >> 4) & 0xF, HEX);
  RemoteClient.print(val & 0xF, HEX);
}

void XDRCommunication() {

  if (server.hasClient()) {
    if (RemoteClient.connected()) {
      server.available().stop();
    } else {
      /* When first client connects */
      wificonnect = true;
      RemoteClient = server.available();
      passwordcrypt();
      RemoteClient.print(saltkey + "\n");
    }
  } else {
    if (server.hasClient()) {
      server.available().stop();
    }
  }
}

void XDRGTKWiFi() {
  if (RemoteClient.available() > 0) {
    buff[buff_pos] = RemoteClient.read();
    if (buff[buff_pos] != '\n' && buff_pos != 16 - 1)
      buff_pos++;
    else {
      buff[buff_pos] = 0;
      buff_pos = 0;
      switch (buff[0]) {
        case 'x':
          RemoteClient.println("OK");
          if (band != 0) {
            band = 0;
            SelectBand();
          }
          RemoteClient.print("T" + String(frequency * 10));
          RemoteClient.print("A0\n");
          RemoteClient.print("D0\n");
          RemoteClient.print("G00\n");
          WiFistatus = true;
          ShowUSBstatus();
          if (menu == true) {
            ModeButtonPress();
          }
          if (Squelch != Squelchold) {
            if (screenmute == false) {
              tft.setTextFont(2);
              tft.setTextColor(BackgroundColor);
              tft.setCursor(240, 168);
              if (Squelchold == -100) {
                tft.print("OFF");
              } else if (Squelchold > 920) {
                tft.print("ST");
              } else {
                tft.print(Squelchold / 10);
              }
            }
          }
          break;

        case 'A':
          AGC = atol(buff + 1);
          RemoteClient.print("A");
          RemoteClient.print(AGC);
          RemoteClient.print("\n");
          radio.setAGC(AGC);
          break;

        case 'C':
          byte scanmethod;
          scanmethod = atol(buff + 1);
          RemoteClient.print("C");
          if (seek == false) {
            if (scanmethod == 1) {
              RemoteClient.print("1\n");
              direction = true;
              seek = true;
              Seek(direction);
            }
            if (scanmethod == 2) {
              RemoteClient.print("2\n");
              direction = false;
              seek = true;
              Seek(direction);
            }
          } else {
            seek = false;
          }
          RemoteClient.print("C0\n");
          break;

        case 'N':
          doStereoToggle();
          break;

        case 'D':
          DeEmphasis = atol(buff + 1);
          RemoteClient.print("D");
          RemoteClient.print(DeEmphasis);
          RemoteClient.print("\n");
          radio.setDeemphasis(DeEmphasis);
          break;

        case 'F':
          XDRBWset = atol(buff + 1);
          RemoteClient.print("F");
          if (XDRBWset < 0) {
            XDRBWsetold = XDRBWset;
            BWset = 0;
          }

          switch (XDRBWset) {
            case 0:
              XDRBWsetold = XDRBWset;
              BWset = 1;
              break;
            case 1:
              XDRBWsetold = XDRBWset;
              BWset = 2;
              break;
            case 2:
              XDRBWsetold = XDRBWset;
              BWset = 3;
              break;
            case 3:
              XDRBWsetold = XDRBWset;
              BWset = 4;
              break;
            case 4:
              XDRBWsetold = XDRBWset;
              BWset = 5;
              break;
            case 5:
              XDRBWsetold = XDRBWset;
              BWset = 6;
              break;
            case 6:
              XDRBWsetold = XDRBWset;
              BWset = 7;
              break;
            case 7:
              XDRBWsetold = XDRBWset;
              BWset = 8;
              break;
            case 8:
              XDRBWsetold = XDRBWset;
              BWset = 9;
              break;
            case 9:
              XDRBWsetold = XDRBWset;
              BWset = 10;
              break;
            case 10:
              XDRBWsetold = XDRBWset;
              BWset = 11;
              break;
            case 11:
              XDRBWsetold = XDRBWset;
              BWset = 12;
              break;
            case 12:
              XDRBWsetold = XDRBWset;
              BWset = 13;
              break;
            case 13:
              XDRBWsetold = XDRBWset;
              BWset = 14;
              break;
            case 14:
              XDRBWsetold = XDRBWset;
              BWset = 15;
              break;
            case 15:
              XDRBWsetold = XDRBWset;
              BWset = 16;
              break;
            default: XDRBWset = XDRBWsetold; break;
          }

          doBW();
          RemoteClient.print(XDRBWset);
          RemoteClient.print("\n");
          break;

        case 'G':
          LevelOffset = atol(buff + 1);
          RemoteClient.print("G");
          if (LevelOffset == 0) {
            MuteScreen(0);
            LowLevelSet = EEPROM.readByte(43);
            EEPROM.commit();
            RemoteClient.print("00\n");
          }
          if (LevelOffset == 10) {
            MuteScreen(1);
            LowLevelSet = EEPROM.readByte(43);
            EEPROM.commit();
            RemoteClient.print("10\n");
          }
          if (LevelOffset == 1) {
            MuteScreen(0);
            LowLevelSet = 120;
            RemoteClient.print("01\n");
          }
          if (LevelOffset == 11) {
            LowLevelSet = 120;
            MuteScreen(1);
            RemoteClient.print("11\n");
          }
          break;

        case 'M':
          byte XDRband;
          XDRband = atol(buff + 1);
          if (XDRband == 0) {
            band = 0;
            SelectBand();
            RemoteClient.print("M0\n");
            RemoteClient.print("T" + String(frequency * 10) + "\n");
          } else {
            band = 1;
            SelectBand();
            RemoteClient.print("M1\n");
            RemoteClient.print("T" + String(frequency_AM) + "\n");
          }
          break;

        case 'T':
          unsigned int freqtemp;
          freqtemp = atoi(buff + 1);
          if (seek == true) {
            seek = false;
          }
          if (freqtemp > 143 && freqtemp < 27001) {
            frequency_AM = freqtemp;
            if (band != 1) {
              band = 1;
              SelectBand();
            } else {
              radio.setFrequency_AM(frequency_AM);
            }
            RemoteClient.print("M1\n");
          } else if (freqtemp > 64999 && freqtemp < 108001) {
            frequency = freqtemp / 10;
            if (band != 0) {
              band = 0;
              SelectBand();
              RemoteClient.print("M0\n");
            } else {
              radio.setFrequency(frequency, 65, 108);
            }
          }
          if (band == 0) {
            RemoteClient.print("T" + String(frequency * 10) + "\n");
          } else {
            RemoteClient.print("T" + String(frequency_AM) + "\n");
          }
          radio.clearRDS();
          sprite.fillSprite(BackgroundColor);
          sprite.pushSprite(6, 220);
          ShowFreq(0);
          break;

        case 'S':
          if (buff[1] == 'a') {
            scanner_start = (atol(buff + 2) + 5) / 10;
          } else if (buff[1] == 'b') {
            scanner_end = (atol(buff + 2) + 5) / 10;
          } else if (buff[1] == 'c') {
            scanner_step = (atol(buff + 2) + 5) / 10;
          } else if (buff[1] == 'f') {
            scanner_filter = atol(buff + 2);
          } else if (scanner_start > 0 && scanner_end > 0 && scanner_step > 0 && scanner_filter >= 0) {
            frequencyold = radio.getFrequency();
            radio.setFrequency(scanner_start, 65, 108);
            RemoteClient.print('U');
            switch (scanner_filter) {
              case -1: BWset = 0; break;
              case 0: BWset = 1; break;
              case 26: BWset = 2; break;
              case 1: BWset = 3; break;
              case 28: BWset = 4; break;
              case 29: BWset = 5; break;
              case 3: BWset = 6; break;
              case 4: BWset = 7; break;
              case 5: BWset = 8; break;
              case 7: BWset = 9; break;
              case 8: BWset = 10; break;
              case 9: BWset = 11; break;
              case 10: BWset = 12; break;
              case 11: BWset = 13; break;
              case 12: BWset = 14; break;
              case 13: BWset = 15; break;
              case 15: BWset = 16; break;
              default: BWset = 0; break;
            }
            doBW();
            if (screenmute == false) {
              ShowFreq(1);
              tft.setTextFont(4);
              tft.setTextColor(SecondaryColor, BackgroundColor);
              tft.setCursor(90, 60);
              tft.print("SCANNING...");
            }
            frequencyold = frequency / 10;
            for (freq_scan = scanner_start; freq_scan <= scanner_end; freq_scan += scanner_step) {
              radio.setFrequency(freq_scan, 65, 108);
              RemoteClient.print(freq_scan * 10, DEC);
              RemoteClient.print('=');
              delay(10);
              if (band == 0) {
                radio.getStatus(SStatus, USN, WAM, OStatus, BW, MStatus);
              } else {
                radio.getStatus_AM(SStatus, USN, WAM, OStatus, BW, MStatus);
              }
              RemoteClient.print((SStatus / 10) + 10, DEC);
              RemoteClient.print(',');
            }
            RemoteClient.print('\n');
            if (screenmute == false) {
              tft.setTextFont(4);
              tft.setTextColor(BackgroundColor);
              tft.setCursor(90, 60);
              tft.print("SCANNING...");
            }
            radio.setFrequency(frequencyold, 65, 108);
            if (screenmute == false) {
              ShowFreq(0);
            }
            radio.setFMABandw();
          }
          break;

        case 'Y':
          VolSet = atoi(buff + 1);
          if (VolSet == 0) {
            radio.setMute();
            XDRMute = true;
            SQ = true;
          } else {
            radio.setVolume((VolSet - 70) / 10);
            XDRMute = false;
          }
          RemoteClient.print("Y");
          RemoteClient.print(VolSet);
          RemoteClient.print("\n");
          break;

        case 'X':
          RemoteClient.print("X\n");
          ESP.restart();
          break;

        case 'Z':
          byte iMSEQX;
          iMSEQX = atol(buff + 1);
          RemoteClient.print("Z");
          if (iMSEQX == 0) {
            iMSset = 1;
            EQset = 1;
            iMSEQ = 2;
          }
          if (iMSEQX == 1) {
            iMSset = 0;
            EQset = 1;
            iMSEQ = 3;
          }
          if (iMSEQX == 2) {
            iMSset = 1;
            EQset = 0;
            iMSEQ = 4;
          }
          if (iMSEQX == 3) {
            iMSset = 0;
            EQset = 0;
            iMSEQ = 1;
          }
          updateiMS();
          updateEQ();
          RemoteClient.print(iMSEQX);
          RemoteClient.print("\n");
          break;
      }
    }
  }

  if (WiFistatus == true) {
    Stereostatus = radio.getStereoStatus();
    RemoteClient.print("S");
    if (StereoToggle == false) {
      RemoteClient.print("S");
    } else if (Stereostatus == true && band == 0) {
      RemoteClient.print("s");
    } else {
      RemoteClient.print("m");
    }
    if (SStatus > (SStatusold + 10) || SStatus < (SStatusold - 10)) {
      RemoteClient.print(((SStatus * 100) + 10875) / 1000);
      RemoteClient.print(".");
      RemoteClient.print(((SStatus * 100) + 10875) / 100 % 10);
    } else {
      RemoteClient.print(((SStatusold * 100) + 10875) / 1000);
      RemoteClient.print(".");
      RemoteClient.print(((SStatus * 100) + 10875) / 100 % 10);
    }
    RemoteClient.print(',');
    RemoteClient.print(WAM / 10, DEC);
    RemoteClient.print(',');
    RemoteClient.print(SNR, DEC);
    RemoteClient.print("\n");
  }
}

/*void XDRGTKWiFi() {
  RemoteClient = server.available();
  passwordcrypt();
  RemoteClient.print(saltkey + "\n");
}*/

void WiFiSetup() {
  // NULL sets an open Access Point
  WiFi.softAP("TEF6686 FM Tuner", NULL);

  IPAddress IP = WiFi.softAPIP();
  // Web Server Root URL
  setupserver.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html);
    //request->send(SPIFFS, "/wifimanager.html", "text/html");
  });

  //    setupserver.serveStatic("/wifimanager.html", SPIFFS, "/wifimanager.html");

  setupserver.on("/", HTTP_POST, [](AsyncWebServerRequest* request) {
    int params = request->params();
    for (int i = 0; i < params; i++) {
      AsyncWebParameter* p = request->getParam(i);
      if (p->isPost()) {
        // HTTP POST ssid value
        if (p->name() == PARAM_INPUT_1) {
          ssid = p->value().c_str();
          writeFile(SPIFFS, ssidPath, ssid.c_str());
        }
        // HTTP POST pass value
        if (p->name() == PARAM_INPUT_2) {
          pass = p->value().c_str();
          writeFile(SPIFFS, passPath, pass.c_str());
        }
        // HTTP POST ip value
        if (p->name() == PARAM_INPUT_3) {
          ip = p->value().c_str();
          writeFile(SPIFFS, ipPath, ip.c_str());
        }
        // HTTP POST gateway value
        if (p->name() == PARAM_INPUT_4) {
          gateway = p->value().c_str();
          writeFile(SPIFFS, gatewayPath, gateway.c_str());
        }
      }
    }
    request->send(200, "text/plain", "Settings saved. Tuner is now rebooting. Tuner's IP Address: " + ip);
    delay(2000);
    ESP.restart();
  });
  setupserver.begin();
}

void serial_hex(uint8_t val) {
  Serial.print((val >> 4) & 0xF, HEX);
  Serial.print(val & 0xF, HEX);
}

void Seek(bool mode) {
  if (band == 0) {
    radio.setMute();
    if (mode == false) {
      frequency = radio.tuneDown(stepsize, LowEdgeSet, HighEdgeSet);
    } else {
      frequency = radio.tuneUp(stepsize, LowEdgeSet, HighEdgeSet);
    }
    delay(50);
    ShowFreq(0);
    if (USBstatus == true) {
      if (band == 0) {
        Serial.print("T" + String(frequency * 10) + "\n");
      } else {
        Serial.print("T" + String(frequency_AM) + "\n");
      }
    }

    radio.getStatus(SStatus, USN, WAM, OStatus, BW, MStatus);

    if ((USN < 200) && (WAM < 230) && (OStatus < 80 && OStatus > -80) && (Squelch < SStatus || Squelch == 920)) {
      seek = false;
      radio.setUnMute();
      store = true;
    } else {
      seek = true;
    }
  }
}

void SetTunerPatch() {
  if (TEF != 101 && TEF != 102 && TEF != 205) {
    radio.init(102);
    uint16_t device;
    uint16_t hw;
    uint16_t sw;
    radio.getIdentification(device, hw, sw);
    TEF = highByte(hw) * 100 + highByte(sw);
    tft.fillScreen(BackgroundColor);
    tft.setTextColor(SecondaryColor);
    analogWrite(CONTRASTPIN, ContrastSet * 2 + 27);
    if (TEF == 0) {
      tft.drawCentreString("Tuner not detected", 150, 70, 4);
    } else {
      tft.drawCentreString(String("Tuner version set: v") + String(TEF), 150, 70, 4);
    }
    tft.drawCentreString("Please restart tuner", 150, 100, 4);
    EEPROM.writeByte(54, TEF);
    EEPROM.commit();
    while (true)
      ;
    for (;;)
      ;
  }
}
