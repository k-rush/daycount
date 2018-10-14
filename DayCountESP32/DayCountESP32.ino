#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
#include <WiFi.h>
#include "time.h"
#include <Adafruit_GFX.h>
#include <RTClib.h>
#include "Adafruit_LEDBackpack.h"
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

FASTLED_USING_NAMESPACE

const char* ssid       = "****";
const char* password   = "****";
/*
const char* ssid       = "SoberAF";
const char* password   = "yoloyolo";
*/
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

#define DATA_PIN    12
//#define CLK_PIN   4
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    4
CRGB leds[NUM_LEDS];

// -- Task handles for use in the notifications
static TaskHandle_t FastLEDshowTaskHandle = 0;
static TaskHandle_t userTaskHandle = 0;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Set to false to display time in 12 hour format, or true to use 24 hour:
#define TIME_24_HOUR      false

// I2C address of the display.  Stick with the default address of 0x70
// unless you've changed the address jumpers on the back of the display.
#define DISPLAY_ADDRESS   0x70

#define BRIGHTNESS          60
#define FRAMES_PER_SECOND  120

// -- The core to run FastLED.show()
#define FASTLED_SHOW_CORE 0

// Create display and DS1307 objects.  These are global variables that
// can be accessed from both the setup and loop function below.
Adafruit_7segment clockDisplay = Adafruit_7segment();

//Sober date in GMT timestamp.
#define SOBER_DATE 1471590000

#define OFFSET -28800
#define MS_IN_DAY 86400

// Keep track of the hours, minutes, seconds displayed by the clock.
// Start off at 0:00:00 as a signal that the time should be read from
// the DS1307 to initialize it.
int hours = 0;
int minutes = 0;
int seconds = 0;

// Remember if the colon was drawn on the display so it can be blinked
// on and off every second.
bool blinkColon = true;
bool isDaylightTime = true;

int DAYLIGHT_OFFSET = isDaylightTime ? 3600 : 0;

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

uint8_t gHue = 0; // rotating "base color" used by many of the patterns
/** show() for ESP32
 *  Call this function instead of FastLED.show(). It signals core 0 to issue a show, 
 *  then waits for a notification that it is done.
 */
void FastLEDshowESP32()
{
    if (userTaskHandle == 0) {
        // -- Store the handle of the current task, so that the show task can
        //    notify it when it's done
        userTaskHandle = xTaskGetCurrentTaskHandle();

        // -- Trigger the show task
        xTaskNotifyGive(FastLEDshowTaskHandle);

        // -- Wait to be notified that it's done
        const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 200 );
        ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
        userTaskHandle = 0;
    }
}

/** show Task
 *  This function runs on core 0 and just waits for requests to call FastLED.show()
 */
void FastLEDshowTask(void *pvParameters)
{
    // -- Run forever...
    for(;;) {
        // -- Wait for the trigger
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // -- Do the show (synchronously)
        FastLED.show();

        // -- Notify the calling task
        xTaskNotifyGive(userTaskHandle);
    }
}


void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  clockDisplay.print((int)timeinfo.tm_hour*100+(int)timeinfo.tm_min, DEC);
  clockDisplay.writeDisplay();
}

void setup()
{  
  Serial.begin(115200);
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  //init and get the time
  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //printLocalTime();

  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(0);

  //disconnect WiFi as it's no longer needed
  //WiFi.disconnect(true);
  //WiFi.mode(WIFI_OFF);

  clockDisplay.begin(DISPLAY_ADDRESS);
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  // -- Create the FastLED show task
  xTaskCreatePinnedToCore(FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 2, &FastLEDshowTaskHandle, FASTLED_SHOW_CORE);
  
  
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  //init and get the time
  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //printLocalTime();

  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(0);

  //disconnect WiFi as it's no longer needed
  //WiFi.disconnect(true);
  //WiFi.mode(WIFI_OFF);

  clockDisplay.begin(DISPLAY_ADDRESS);
}

void loop()
{
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.print("DATE: ");
  Serial.println(dayStamp);
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.print("HOUR: ");
  Serial.println(timeStamp);
  delay(1000);
  //printLocalTime();


  //time_t soberdate = time(1471503600000);
  //time_t currentdate = time(timeClient.getEpochTime());

  Serial.println("TIME DIFF IN DAYS:");
  int soberdays = (int)((timeClient.getEpochTime() - SOBER_DATE)/MS_IN_DAY);
  Serial.println(soberdays);
  clockDisplay.print(soberdays, DEC);
  clockDisplay.writeDisplay();
  // send the 'leds' array out to the actual LED strip
  FastLEDshowESP32();
  // FastLED.show();
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 
  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}
