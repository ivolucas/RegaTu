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


#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ESP32Servo.h>
using WebServerClass = WebServer;

#include <AutoConnect.h>

/*
  AC_USE_SPIFFS indicates SPIFFS or LittleFS as available file systems that
  will become the AUTOCONNECT_USE_SPIFFS identifier and is exported as shown
  the valid file system. After including AutoConnect.h, the Sketch can determine
  whether to use FS.h or LittleFS.h by AUTOCONNECT_USE_SPIFFS definition.
*/
#include <FS.h>

#include <SPIFFS.h>
fs::SPIFFSFS &FlashFS = SPIFFS;


#define PAGE_ELEMENTS "/elements.json"
#define PAGE_SAVE "/save.json"

WebServerClass server;
AutoConnect portal(server);
AutoConnectConfig config;
AutoConnectAux elementsAux;
AutoConnectAux saveAux;
Servo servo; // Variável Servo


void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    case ESP_SLEEP_WAKEUP_GPIO : Serial.println("Wakeup caused by GPIO"); break;
    case ESP_SLEEP_WAKEUP_UART : Serial.println("Wakeup caused by UART"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}


#define Threshold 40    /* Greater the value, more the sensitivity */


#define BUTTON_PIN_BITMASK 0x300000000

// Display touchpad origin
void print_wakeup_touchpad(){
  touch_pad_t touchPin;
  touchPin = esp_sleep_get_touchpad_wakeup_status();

  switch(touchPin)
  {
    case 0  : Serial.println("Touch detected on GPIO 4"); break;
    case 1  : Serial.println("Touch detected on GPIO 0"); break;
    case 2  : Serial.println("Touch detected on GPIO 2"); break;
    case 3  : Serial.println("Touch detected on GPIO 15"); break;
    case 4  : Serial.println("Touch detected on GPIO 13"); break;
    case 5  : Serial.println("Touch detected on GPIO 12"); break;
    case 6  : Serial.println("Touch detected on GPIO 14"); break;
    case 7  : Serial.println("Touch detected on GPIO 27"); break;
    case 8  : Serial.println("Touch detected on GPIO 33"); break;
    case 9  : Serial.println("Touch detected on GPIO 32"); break;
    default : Serial.println("Wakeup not by touchpad"); break;
  }
}

// Display touchpad origin
void print_wakeup_ext1(){
  uint64_t mask = 0x0000000000000001;
  uint64_t status = esp_sleep_get_ext1_wakeup_status();
  for(int i = 0; i<39;++i){
    if ((mask & status) != 0x0)
      Serial.printf("GPIO %d cause wake up",i);
    mask = mask<<1;
  }
}

// Execute this function when Touch Pad in pressed
void callback() {
  Serial.println("Do something when Touch Pad is pressed");
}



void openValve(){
  servo.write(0);
  servo.attach(GPIO_NUM_12, 400, 2500);
  
  delay(1000);
  servo.detach();
}
void closeValve(){
  servo.write(90);
  servo.attach(GPIO_NUM_12, 400, 2500);
  
  delay(1000);
  servo.detach();
}



void setup()
{
  delay(1000);
  Serial.begin(115200);
  Serial.println();

  //Servo Setup
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  servo.setPeriodHertz(50);// Standard 50hz servo
  

  print_wakeup_reason();
  //Print the wakeup reason for ESP32 and touchpad too
  print_wakeup_touchpad();
  print_wakeup_ext1();

  //Setup interrupt on Touch Pad 4 (GPIO4)
  touchAttachInterrupt(T0, callback, Threshold);

  //Configure Touchpad as wakeup source
  esp_sleep_enable_touchpad_wakeup();
  
  //Configure mask as ext1 wake up source for HIGH logic level
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);


  // Responder of root page handled directly from WebServer class.
  server.on("/", []()
            {
              String content = "Place the root page with the sketch application.&ensp;";
              content += AUTOCONNECT_LINK(COG_24);
              server.send(200, "text/html", content);
            });

  server.on("/sleep", []()
            {
              String content = "<h2>Sleep</h2>";
              server.send(200, "text/html", content);
              
              ESP.deepSleep(600000000);//10minutos
            });

  server.on("/openValve", []()
            {
              String content = "<h2>openValve</h2>";
              server.send(200, "text/html", content);
              openValve();
            });
  server.on("/closeValve", []()
            {
              String content = "<h2>closeValve</h2>";
              server.send(200, "text/html", content);
              closeValve();
            });

  // Load a custom web page described in JSON as PAGE_ELEMENT and
  // register a handler. This handler will be invoked from
  // AutoConnectSubmit named the Load defined on the same page.
  FlashFS.begin();
  File page = FlashFS.open(PAGE_ELEMENTS, "r");
  elementsAux.load(page);
  page.close();
  page = FlashFS.open(PAGE_SAVE, "r");
  saveAux.load(page);
  page.close();
  FlashFS.end();
  saveAux.on([](AutoConnectAux &aux, PageArgument &arg)
             {
               // You can validate input values ​​before saving with
               // AutoConnectInput::isValid function.
               // Verification is using performed regular expression set in the
               // pattern attribute in advance.
               AutoConnectInput &input = elementsAux["input"].as<AutoConnectInput>();
               aux["validated"].value = input.isValid() ? String() : String("Input data pattern missmatched.");

               // The following line sets only the value, but it is HTMLified as
               // formatted text using the format attribute.
               aux["caption"].value = PAGE_ELEMENTS;

#if defined(ARDUINO_ARCH_ESP8266)
               FlashFS.begin();
#elif defined(ARDUINO_ARCH_ESP32)
               FlashFS.begin(true);
#endif
               File param = FlashFS.open(PAGE_ELEMENTS, "w");
               if (param)
               {
                 // Save as a loadable set for parameters.
                 elementsAux.saveElement(param);
                 param.close();
                 // Read the saved elements again to display.
                 param = FlashFS.open(PAGE_ELEMENTS, "r");
                 aux["echo"].value = param.readString();
                 param.close();
               }
               else
               {
                 aux["echo"].value = "Filesystem failed to open.";
               }
               FlashFS.end();
               return String();
             });
  portal.join({elementsAux, saveAux});
  config.ticker = true;
  portal.config(config);
  portal.begin();
}

void loop()
{
  server.handleClient();
  portal.handleRequest(); // Need to handle AutoConnect menu.
  if (WiFi.status() == WL_IDLE_STATUS)
  {
    ESP.restart();
    delay(1000);
  }
}

