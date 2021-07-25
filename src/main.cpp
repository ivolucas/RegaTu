/*
  Elements.ino, Example for the AutoConnect library.
  Copyright (c) 2020, Hieromon Ikasamo
  https://github.com/Hieromon/AutoConnect
  This software is released under the MIT License.
  https://opensource.org/licenses/MIT

  This example demonstrates the typical behavior of AutoConnectElement.
  It also represents a basic structural frame for saving and reusing
  values ​​entered in a custom web page into flash.
*/
#define AC_DEBUG
//#define AUTOCONNECT_NOUSE_JSON
#define SERIAL_DEBUG

extern "C" int rom_phy_get_vdd33();
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
using WebServerClass = WebServer;

#include <AutoConnect.h>
#include "time.h"
#include <ServoValve.h>
#include <CronTask.h>
/*
  AC_USE_SPIFFS indicates SPIFFS or LittleFS as available file systems that
  will become the AUTOCONNECT_USE_SPIFFS identifier and is exported as shown
  the valid file system. After including AutoConnect.h, the Sketch can determine
  whether to use FS.h or LittleFS.h by AUTOCONNECT_USE_SPIFFS definition.
*/
#include <FS.h>

#include <SPIFFS.h>
fs::SPIFFSFS &FlashFS = SPIFFS;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

#define VALVE_SERVO_CONFIG_FILE "/valve.struct"

#define CRON_TASK_CONFIG_FILE "/cron.struct"

ACText(header1, "<h1>Servo Settings</h1>");
ACText(saveResulteMessage, "");
ACText(caption1, "<h2>Pulse Width</h2>");
ACInput(minPulseWidth, "500", "Min", "\\d+", "", AC_Tag_BR, AC_Input_Number);
ACInput(maxPulseWidth, "2500", "Max", "\\d+", "", AC_Tag_BR, AC_Input_Number);
ACText(caption2, "<h2>Angle Position</h2>");
ACInput(openAngle, "0", "Open Angle", "\\d+", "", AC_Tag_BR, AC_Input_Number);
ACInput(closeAngle, "90", "Close Angle", "\\d+", "", AC_Tag_BR, AC_Input_Number);
ACInput(actionDuration, "2000", "Action Duraction", "\\d+", "", AC_Tag_BR, AC_Input_Number);
ACSubmit(valveServo_save, "Save", "/valveServo?action=save");
ACText(caption3, "<h2>Himidity logic converter</h2>");
ACInput(humidityLogicMin, "300", "Min logic level (Max H%)", "\\d+", "", AC_Tag_BR, AC_Input_Number);
ACInput(humidityLogicMax, "1200", "Max logic level (Min H%)", "\\d+", "", AC_Tag_BR, AC_Input_Number);

AutoConnectAux valveServoSetting("/valveServo", "Configuração de Parametros", true,
                                 {header1, saveResulteMessage, caption1, minPulseWidth, maxPulseWidth,
                                  caption2, openAngle, closeAngle, actionDuration, caption3, humidityLogicMin, humidityLogicMax, valveServo_save});

ACText(header, "<h1>Rega Tu</h1>");
ACText(timeText, "");
ACText(vccText, "");
ACText(hText, "");
ACButton(openValveButton, "Abrir", "window.fetch('/valveServo?action=open')");
ACButton(closeValveButton, "Fechar", "window.fetch('/valveServo?action=close')");
ACButton(setTimeButton, "Acertar a Hora", "window.fetch('/setTime?epoch='+(Date.now()/1000)).then(function(response) {location.reload();}).catch(function() { console.log('Error setting hour');})");
ACSubmit(sleepValveButton, "Dormir", "/sleep", AC_Tag_BR);
AutoConnectAux home("/", "Rega Tu", false,
                    {header, timeText, vccText, hText, openValveButton, closeValveButton,setTimeButton, sleepValveButton});

std::vector<String> hourArray = {String("0"), String("1"), String("2"), String("3"), String("4"), String("5"), String("6"), String("7"), String("8"), String("9"),
                                 String("10"), String("11"), String("12"), String("13"), String("14"), String("15"), String("16"), String("17"), String("18"),
                                 String("19"), String("20"), String("21"), String("22"), String("23")};

std::vector<String> minuteArray = {
    String("0"), String("1"), String("2"), String("3"), String("4"), String("5"), String("6"), String("7"), String("8"), String("9"),
    String("10"), String("11"), String("12"), String("13"), String("14"), String("15"), String("16"), String("17"), String("18"), String("19"),
    String("20"), String("21"), String("22"), String("23"), String("24"), String("25"), String("26"), String("27"), String("28"), String("29"),
    String("30"), String("31"), String("32"), String("33"), String("34"), String("35"), String("36"), String("37"), String("38"), String("39"),
    String("40"), String("41"), String("42"), String("43"), String("44"), String("45"), String("46"), String("47"), String("48"), String("49"),
    String("50"), String("51"), String("52"), String("53"), String("54"), String("55"), String("56"), String("57"), String("58"), String("59")};

std::vector<String> humidityArray = {
    String("0"), String("1"), String("2"), String("3"), String("4"), String("5"), String("6"), String("7"), String("8"), String("9"),
    String("10"), String("11"), String("12"), String("13"), String("14"), String("15"), String("16"), String("17"), String("18"), String("19"),
    String("20"), String("21"), String("22"), String("23"), String("24"), String("25"), String("26"), String("27"), String("28"), String("29"),
    String("30"), String("31"), String("32"), String("33"), String("34"), String("35"), String("36"), String("37"), String("38"), String("39"),
    String("40"), String("41"), String("42"), String("43"), String("44"), String("45"), String("46"), String("47"), String("48"), String("49"),
    String("50"), String("51"), String("52"), String("53"), String("54"), String("55"), String("56"), String("57"), String("58"), String("59"),
    String("60"), String("61"), String("62"), String("63"), String("64"), String("65"), String("66"), String("67"), String("68"), String("69"),
    String("70"), String("71"), String("72"), String("73"), String("74"), String("75"), String("76"), String("77"), String("78"), String("79"),
    String("80"), String("81"), String("82"), String("83"), String("84"), String("85"), String("86"), String("87"), String("88"), String("89"),
    String("90"), String("91"), String("92"), String("93"), String("94"), String("95"), String("96"), String("97"), String("98"), String("99"),
    String("100")};

ACText(header2, "<h1>Periodos de Rega</h1>");
ACText(saveResult2Message, "");
ACCheckbox(task1Active, "on", "Periodo 1", false, AC_Behind, AC_Tag_None);
ACSelect(task1Humidity, humidityArray, "Se humidade inferior a", 0, AC_Tag_None);
ACSelect(task1StartHour, hourArray, " Inicio", 0, AC_Tag_None);
ACSelect(task1StartMinute, minuteArray, ":", 0, AC_Tag_None);
ACSelect(task1EndHour, hourArray, " Fim", 0, AC_Tag_None);
ACSelect(task1EndMinute, minuteArray, ":", 0, AC_Tag_BR);
ACCheckbox(task2Active, "on", "Periodo 2", false, AC_Behind, AC_Tag_None);
ACSelect(task2Humidity, humidityArray, "Se humidade inferior a", 0, AC_Tag_None);
ACSelect(task2StartHour, hourArray, " Inicio", 0, AC_Tag_None);
ACSelect(task2StartMinute, minuteArray, ":", 0, AC_Tag_None);
ACSelect(task2EndHour, hourArray, " Fim", 0, AC_Tag_None);
ACSelect(task2EndMinute, minuteArray, ":", 0, AC_Tag_BR);
ACCheckbox(task3Active, "on", "Periodo 3", false, AC_Behind, AC_Tag_None);
ACSelect(task3Humidity, humidityArray, "Se humidade inferior a", 0, AC_Tag_None);
ACSelect(task3StartHour, hourArray, " Inicio", 0, AC_Tag_None);
ACSelect(task3StartMinute, minuteArray, ":", 0, AC_Tag_None);
ACSelect(task3EndHour, hourArray, " Fim", 0, AC_Tag_None);
ACSelect(task3EndMinute, minuteArray, ":", 0, AC_Tag_BR);
ACCheckbox(task4Active, "on", "Periodo 4", false, AC_Behind, AC_Tag_None);
ACSelect(task4Humidity, humidityArray, "Se humidade inferior a", 0, AC_Tag_None);
ACSelect(task4StartHour, hourArray, " Inicio", 0, AC_Tag_None);
ACSelect(task4StartMinute, minuteArray, ":", 0, AC_Tag_None);
ACSelect(task4EndHour, hourArray, " Fim", 0, AC_Tag_None);
ACSelect(task4EndMinute, minuteArray, ":", 0, AC_Tag_BR);
ACSelect(manualDuration, minuteArray, "Duração da rega manual:", 0, AC_Tag_BR);
ACSubmit(tasksSave, "Guardar", "/tasks?action=save");

AutoConnectAux tasksSetting("/tasks", "Rega", true,
                            {header2, saveResult2Message,
                             task1Active, task1Humidity, task1StartHour, task1StartMinute, task1EndHour, task1EndMinute,
                             task2Active, task2Humidity, task2StartHour, task2StartMinute, task2EndHour, task2EndMinute,
                             task3Active, task3Humidity, task3StartHour, task3StartMinute, task3EndHour, task3EndMinute,
                             task4Active, task4Humidity, task4StartHour, task4StartMinute, task4EndHour, task4EndMinute,
                             manualDuration, tasksSave});

WebServerClass server;
AutoConnect portal(server);
AutoConnectConfig config;

RTC_DATA_ATTR ValveServoSetting currentValveSettings = {500, 2400, 5, 90, 2000, 300, 1200, CLOSE};
ServoValve servoValve(GPIO_NUM_14, GPIO_NUM_33, GPIO_NUM_34, &currentValveSettings);

RTC_DATA_ATTR CronTaskSettings currentTaskSettings = {
    false,
    0,
    0,
    (short)100,
    false,
    0,
    0,
    (short)100,
    false,
    0,
    0,
    (short)100,
    false,
    0,
    0,
    (short)100,
    -1,
    -1,
    30};
CronTaskManager cronTaskManager(&currentTaskSettings);

time_t now;
struct tm timeinfo;

long last_press = 0;
long keep_waken = 0;
bool press = false;

// Display touchpad origin
void print_wakeup_touchpad()
{
  touch_pad_t touchPin;
  touchPin = esp_sleep_get_touchpad_wakeup_status();

  switch (touchPin)
  {
  case 0:
    Serial.println("Touch detected on GPIO 4");
    break;
  case 1:
    Serial.println("Touch detected on GPIO 0");
    break;
  case 2:
    Serial.println("Touch detected on GPIO 2");
    break;
  case 3:
    Serial.println("Touch detected on GPIO 15");
    break;
  case 4:
    Serial.println("Touch detected on GPIO 13");
    break;
  case 5:
    Serial.println("Touch detected on GPIO 12");
    break;
  case 6:
    Serial.println("Touch detected on GPIO 14");
    break;
  case 7:
    Serial.println("Touch detected on GPIO 27");
    break;
  case 8:
    Serial.println("Touch detected on GPIO 33");
    break;
  case 9:
    Serial.println("Touch detected on GPIO 32");
    break;
  default:
    Serial.println("Wakeup not by touchpad");
    break;
  }
}

// Display touchpad origin
void print_wakeup_ext1()
{
  uint64_t mask = 0x0000000000000001;
  uint64_t status = esp_sleep_get_ext1_wakeup_status();
  for (int i = 0; i < 39; ++i)
  {
    if ((mask & status) != 0x0)
      Serial.printf("GPIO %d cause wake up", i);
    mask = mask << 1;
  }
}

void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    print_wakeup_ext1();
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    print_wakeup_touchpad();
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  case ESP_SLEEP_WAKEUP_GPIO:
    Serial.println("Wakeup caused by GPIO");
    break;
  case ESP_SLEEP_WAKEUP_UART:
    Serial.println("Wakeup caused by UART");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

#define Threshold 40 /* Greater the value, more the sensitivity */

#define BUTTON_PIN_BITMASK 0x300000000

String getLocalTimeString()
{

  char timeStringBuff[70]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y/%m/%d %H:%M:%S %Z", &timeinfo);
//print like "const char*"
#ifdef SERIAL_DEBUG
  Serial.println(timeStringBuff);
#endif
  //Optional: Construct String object
  return String(timeStringBuff);
}

void loadValveConfigFromFile()
{
  FlashFS.begin(true);

  if (FlashFS.exists(VALVE_SERVO_CONFIG_FILE))
  {
    File myFile = FlashFS.open(VALVE_SERVO_CONFIG_FILE, "r");

    int readed = myFile.read((byte *)&currentValveSettings, sizeof(currentValveSettings));
    if (sizeof(currentValveSettings) != readed)
    {
      Serial.printf("Error reading file: readed %d expected %d!\n", readed, sizeof(currentValveSettings));
      currentValveSettings.minPulseWidth = 500;
      currentValveSettings.maxPulseWidth = 2400;
      currentValveSettings.openAngle = 0;
      currentValveSettings.closeAngle = 90;
      currentValveSettings.actionDuration = 2000;
      currentValveSettings.humidityLogicMin = 300;
      currentValveSettings.humidityLogicMax = 1200;
      currentValveSettings.state = CLOSE;
    }
    else
    {
      Serial.printf("Readed file: %d!", readed);
    }
    myFile.close();
  }

  FlashFS.end();
}

void saveValveConfigToFile()
{
  FlashFS.begin();
  File myFile = FlashFS.open(VALVE_SERVO_CONFIG_FILE, "w");
  myFile.write((byte *)&currentValveSettings, sizeof(currentValveSettings));
  myFile.close();
  FlashFS.end();
}

void loadCronTaskFromFile()
{
  FlashFS.begin();

  if (FlashFS.exists(CRON_TASK_CONFIG_FILE))
  {
    File myFile = FlashFS.open(CRON_TASK_CONFIG_FILE, "r");

    int readed = myFile.read((byte *)&currentTaskSettings, sizeof(currentTaskSettings));
    if (sizeof(currentTaskSettings) != readed)
    {
      Serial.printf("Error reading file: readed %d expected %d!\n", readed, sizeof(currentTaskSettings));
    }
    else
    {
      Serial.printf("Readed file: %d!\n", readed);
    }
    myFile.close();
  }

  FlashFS.end();
}

void saveCronTaskToFile()
{
  FlashFS.begin();
  File myFile = FlashFS.open(CRON_TASK_CONFIG_FILE, "w");
  myFile.write((byte *)&currentTaskSettings, sizeof(currentTaskSettings));
  myFile.close();
  FlashFS.end();
}

void keepWaken()
{
  keep_waken = millis();
}

void hibernate()
{
#ifdef SERIAL_DEBUG
  Serial.println("Going to sleep");
#endif
  digitalWrite(GPIO_NUM_26, LOW); // turn on led
  getLocalTimeString();
  ESP.deepSleep(30000000); //30sec
}

void touchAttachInterruptCallback()
{
  press = true;
}
void updateTime()
{
  time(&now);
  localtime_r(&now, &timeinfo);
}

boolean validWorkFuncion()
{
  updateTime();
  long t = millis();
  if (press)
  {
    if ((t - last_press) > 100)
    {
      Serial.println("Button togle");
      if (servoValve.togleValve())
      {
        cronTaskManager.scheduleManualClose(&timeinfo);
      }
      else
      {
        cronTaskManager.resetManualClose();
      }

      keepWaken();
    }
    press = false;
    last_press = t;
  }

  cronTaskManager.checkAction(&servoValve, &timeinfo);
  if (servoValve.tick() && (t - keep_waken > 60000)) //60sec 1min
  {
    hibernate();
  }
  return true;
}

void setup()
{
  setenv("TZ", "WET0WEST,M3.5.0/1,M10.5.0", 1);
  tzset();

  pinMode(GPIO_NUM_26, OUTPUT);
  digitalWrite(GPIO_NUM_26, HIGH); // turn on led
  servoValve.setupPwm();
  Serial.begin(115200);
  Serial.println();
#ifdef SERIAL_DEBUG
  getLocalTimeString();
  print_wakeup_reason();
#endif
  //Configure Touchpad as wakeup source
  esp_sleep_enable_touchpad_wakeup();
  touchAttachInterrupt(T0, touchAttachInterruptCallback, 50);
  //Configure mask as ext1 wake up source for HIGH logic level
  //esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  //Print the wakeup reason for ESP32 and touchpad too
  switch (esp_sleep_get_wakeup_cause())
  {
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
#ifdef SERIAL_DEBUG
    Serial.println("Init press");
#endif
    press = true;
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
#ifdef SERIAL_DEBUG
    Serial.println("WAKEUP_TIMER");
#endif
    updateTime();
    cronTaskManager.checkAction(&servoValve, &timeinfo);
    while (!servoValve.tick()) // wait action to end befor sleep
    {
      delay(100);
    }
    hibernate();

    break;
  default:
    loadValveConfigFromFile();
    loadCronTaskFromFile();
  }

  server.on("/sleep", []()
            {
              String content = "<h2>Sleep</h2>";
              server.send(200, "text/html", content);
              hibernate();
            });

  server.on("/setTime", []()
            {
              if (server.hasArg("epoch"))
              {                
                struct timeval tv;
                tv.tv_sec = server.arg("epoch").toInt();  // epoch time (seconds)
                tv.tv_usec = 0;    // microseconds
                settimeofday(&tv, NULL);
                server.send(200, "text/html", "Ok");
              } else {
                server.send(500, "text/html", "Missing Arg");
              }

            });

  // Load a custom web page described in JSON as PAGE_ELEMENT and
  // register a handler. This handler will be invoked from
  // AutoConnectSubmit named the valveServoSettingLoad defined on the same page.
  home.on([](AutoConnectAux &aux, PageArgument &arg)
          {
            keepWaken();
            aux["timeText"].value = getLocalTimeString();
            char buff[20];
            snprintf(buff, sizeof(buff), "Humidade: %d %%", servoValve.readHumidity());
            aux["hText"].value = String(buff);
            char buff2[30];
            snprintf(buff2, sizeof(buff2), "Bateria: %d %%", map(rom_phy_get_vdd33(), 6100, 7800, 0, 100));
            aux["vccText"].value = String(buff2);

            if (arg.hasArg("action"))
            {
              if (arg.arg("action") == "open")
              {
                servoValve.openValve();
                cronTaskManager.scheduleManualClose(&timeinfo);
              }
              else if (arg.arg("action") == "close")
              {
                servoValve.closeValve();
                cronTaskManager.resetManualClose();
              }
            }
            return String();
          });

  valveServoSetting.on([](AutoConnectAux &aux, PageArgument &arg)
                       {
                         keepWaken();
                         if (arg.hasArg("action"))
                         {
                           if (arg.arg("action") == "save")
                           {
                             currentValveSettings.minPulseWidth = aux["minPulseWidth"].value.toInt();
                             currentValveSettings.maxPulseWidth = aux["maxPulseWidth"].value.toInt();
                             currentValveSettings.openAngle = aux["openAngle"].value.toInt();
                             currentValveSettings.closeAngle = aux["closeAngle"].value.toInt();
                             currentValveSettings.actionDuration = aux["actionDuration"].value.toInt();
                             currentValveSettings.humidityLogicMin = aux["humidityLogicMin"].value.toInt();
                             currentValveSettings.humidityLogicMax = aux["humidityLogicMax"].value.toInt();
                             saveValveConfigToFile();
                             aux["saveResulteMessage"].value = String("Config Save");
                           }
                         }
                         else
                         {
                           aux["minPulseWidth"].value = String(currentValveSettings.minPulseWidth);
                           aux["maxPulseWidth"].value = String(currentValveSettings.maxPulseWidth);
                           aux["openAngle"].value = String(currentValveSettings.openAngle);
                           aux["closeAngle"].value = String(currentValveSettings.closeAngle);
                           aux["actionDuration"].value = String(currentValveSettings.actionDuration);
                           aux["humidityLogicMin"].value = String(currentValveSettings.humidityLogicMin);
                           aux["humidityLogicMax"].value = String(currentValveSettings.humidityLogicMax);
                         }
                         return String();
                       });

  tasksSetting.on([](AutoConnectAux &aux, PageArgument &arg)
                  {
                    keepWaken();
                    if (arg.hasArg("action"))
                    {
                      if (arg.arg("action") == "save")
                      {
                        currentTaskSettings.task1Active = task1Active.checked;
                        currentTaskSettings.task1MinuteOfDayStart = (task1StartHour.selected - 1) * 60 + task1StartMinute.selected - 1;
                        currentTaskSettings.task1MinuteOfDayStop = (task1EndHour.selected - 1) * 60 + task1EndMinute.selected - 1;
                        currentTaskSettings.task1MaxHumidity = task1Humidity.selected - 1;
                        ;
                        currentTaskSettings.task2Active = task2Active.checked;
                        currentTaskSettings.task2MinuteOfDayStart = (task2StartHour.selected - 1) * 60 + task2StartMinute.selected - 1;
                        currentTaskSettings.task2MinuteOfDayStop = (task2EndHour.selected - 1) * 60 + task2EndMinute.selected - 1;
                        currentTaskSettings.task2MaxHumidity = task2Humidity.selected - 1;
                        ;
                        currentTaskSettings.task3Active = task3Active.checked;
                        currentTaskSettings.task3MinuteOfDayStart = (task3StartHour.selected - 1) * 60 + task3StartMinute.selected - 1;
                        ;
                        currentTaskSettings.task3MinuteOfDayStop = (task3EndHour.selected - 1) * 60 + task3EndMinute.selected - 1;
                        ;
                        currentTaskSettings.task3MaxHumidity = task3Humidity.selected - 1;
                        ;
                        currentTaskSettings.task4Active = task4Active.checked;
                        currentTaskSettings.task4MinuteOfDayStart = (task4StartHour.selected - 1) * 60 + task4StartMinute.selected - 1;
                        ;
                        currentTaskSettings.task4MinuteOfDayStop = (task4EndHour.selected - 1) * 60 + task4EndMinute.selected - 1;
                        ;
                        currentTaskSettings.task4MaxHumidity = task4Humidity.selected - 1;
                        ;

                        currentTaskSettings.manualDuration = manualDuration.selected - 1;
                        ;

                        saveCronTaskToFile();
                        aux["saveResult2Message"].value = String("Config Save");
                      }
                    }
                    else
                    {
                      task1Active.checked = currentTaskSettings.task1Active;
                      task1StartHour.selected = 1 + currentTaskSettings.task1MinuteOfDayStart / 60;
                      task1StartMinute.selected = 1 + currentTaskSettings.task1MinuteOfDayStart % 60;
                      task1EndHour.selected = 1 + currentTaskSettings.task1MinuteOfDayStop / 60;
                      task1EndMinute.selected = 1 + currentTaskSettings.task1MinuteOfDayStop % 60;
                      task1Humidity.selected = 1 + currentTaskSettings.task1MaxHumidity;
                      task2Active.checked = currentTaskSettings.task2Active;
                      task2StartHour.selected = 1 + currentTaskSettings.task2MinuteOfDayStart / 60;
                      task2StartMinute.selected = 1 + currentTaskSettings.task2MinuteOfDayStart % 60;
                      task2EndHour.selected = 1 + currentTaskSettings.task2MinuteOfDayStop / 60;
                      task2EndMinute.selected = 1 + currentTaskSettings.task2MinuteOfDayStop % 60;
                      task2Humidity.selected = 1 + currentTaskSettings.task2MaxHumidity;
                      task3Active.checked = currentTaskSettings.task3Active;
                      task3StartHour.selected = 1 + currentTaskSettings.task3MinuteOfDayStart / 60;
                      task3StartMinute.selected = 1 + currentTaskSettings.task3MinuteOfDayStart % 60;
                      task3EndHour.selected = 1 + currentTaskSettings.task3MinuteOfDayStop / 60;
                      task3EndMinute.selected = 1 + currentTaskSettings.task3MinuteOfDayStop % 60;
                      task3Humidity.selected = 1 + currentTaskSettings.task3MaxHumidity;
                      task4Active.checked = currentTaskSettings.task4Active;
                      task4StartHour.selected = 1 + currentTaskSettings.task4MinuteOfDayStart / 60;
                      task4StartMinute.selected = 1 + currentTaskSettings.task4MinuteOfDayStart % 60;
                      task4EndHour.selected = 1 + currentTaskSettings.task4MinuteOfDayStop / 60;
                      task4EndMinute.selected = 1 + currentTaskSettings.task4MinuteOfDayStop % 60;
                      task4Humidity.selected = 1 + currentTaskSettings.task4MaxHumidity;
                      manualDuration.selected = 1 + currentTaskSettings.manualDuration;
                    }
                    return String();
                  });
  // Add the callback function to be called when the button is pressed.

  portal.join({home, valveServoSetting, tasksSetting});
  config.ticker = true;
  config.autoReconnect = true;
  config.beginTimeout = 100000; // 10s
  config.reconnectInterval = 6; // Attempt automatic reconnection.
  config.ota = AC_OTA_BUILTIN;

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  portal.config(config);
  portal.whileCaptivePortal(validWorkFuncion);
  portal.begin();
}

void loop()
{

  if (WiFi.status() == WL_IDLE_STATUS)
  {
#ifdef SERIAL_DEBUG
    Serial.println("WL_IDLE_STATUS =  ESP.restart(); ");
#endif
    ESP.restart();
    delay(1000);
  }

  validWorkFuncion();
  portal.handleClient(); // Need to handle AutoConnect menu.
}
