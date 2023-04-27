#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include "ArduinoJson.h"

#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

#define USE_HSPI_FOR_EPD
#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#define GxEPD2_DISPLAY_CLASS GxEPD2_BW
#define GxEPD2_DRIVER_CLASS GxEPD2_420_M01  // GDEW042M01  400x300, UC8176 (IL0398)

// somehow there should be an easier way to do this
#define GxEPD2_BW_IS_GxEPD2_BW true
#define GxEPD2_3C_IS_GxEPD2_3C true
#define GxEPD2_7C_IS_GxEPD2_7C true
#define GxEPD2_1248_IS_GxEPD2_1248 true
#define IS_GxEPD(c, x) (c##x)
#define IS_GxEPD2_BW(x) IS_GxEPD(GxEPD2_BW_IS_, x)
#define IS_GxEPD2_3C(x) IS_GxEPD(GxEPD2_3C_IS_, x)
#define IS_GxEPD2_7C(x) IS_GxEPD(GxEPD2_7C_IS_, x)
#define IS_GxEPD2_1248(x) IS_GxEPD(GxEPD2_1248_IS_, x)

#if defined(ESP32)
#define MAX_DISPLAY_BUFFER_SIZE 65536ul  // e.g.
#if IS_GxEPD2_BW(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))
#elif IS_GxEPD2_3C(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8))
#elif IS_GxEPD2_7C(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2))
#endif
GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=*/15, /*DC=*/27, /*RST=*/26, /*BUSY=*/25));
#endif

#include "bitmaps/Bitmaps400x300.h"  // 4.2"  b/w

#if defined(ESP32) && defined(USE_HSPI_FOR_EPD)
SPIClass hspi(HSPI);
#endif

/* 1. Define the WiFi credentials */
#define WIFI_SSID <WIFI>
#define WIFI_PASSWORD <WIFI>

/* 2. Define the API Key */
#define API_KEY "AIzaSyA4ufCO69gmEE8pV51b3jh7eOAsWC7kwVU"

/* 3. Define the project ID */
#define FIREBASE_PROJECT_ID "esp-tester-c4a0c"

#define USER_EMAIL "floftus@hotmail.co.uk"
#define USER_PASSWORD "dragonshield"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

bool taskCompleted = false;
String contentArray[20];

unsigned long dataMillis = 0;
String firestoredata;
const char HelloWorld[] = "Hello World!";



// ------------------------------------------------------------------------
// arduino functions
void setup() {

  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  // see addons/TokenHelper.h

#if defined(ESP8266)
  // In ESP8266 required for BearSSL rx/tx buffer for large data handle, increase Rx size as needed.
  fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 2048 /* Tx buffer size in bytes from 512 - 16384 */);
#endif

  // Limit the size of response payload to be collected in FirebaseData
  fbdo.setResponseSize(2048);

  Firebase.begin(&config, &auth);

  Firebase.reconnectWiFi(true);

  //------------------------
  // setting up display
#if defined(ESP32) && defined(USE_HSPI_FOR_EPD)
  hspi.begin(13, 12, 14, 15);  // remap hspi for EPD (swap pins)
  display.epd2.selectSPI(hspi, SPISettings(4000000, MSBFIRST, SPI_MODE0));
#endif
  display.init(115200);
  printText("...waking up");
  delay(1000);
}

void loop() {

  // Firebase.ready() should be called repeatedly to handle authentication tasks.

  if (Firebase.ready() && (millis() - dataMillis > 80000 || dataMillis == 0)) {
    dataMillis = millis();

    String documentPath = "instructions/instruction1";
    String mask = "content2";

    Serial.print("Get a document... ");

    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), mask.c_str())) {
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());

      FirebaseJson payload;
      payload.setJsonData(fbdo.payload().c_str());

      // Get string data from FirebaseJson object
      // FirebaseJsonData jsonData;
      // payload.get(jsonData, "fields/content/stringValue", true);
      // Serial.println("the content value is: ");
      // Serial.println(jsonData.stringValue);

      int num_values = getArrayFromFirestore(payload, "fields/content2/arrayValue/values");
      Serial.print("one value in the array is: ");
      Serial.println(contentArray[0]);
      Serial.print("the number of values are: ");
      Serial.println(num_values);
      for (int i=0; i < num_values; i++) {
        printText(contentArray[i]);
        delay(1000);
      }
    } else {
      Serial.println(fbdo.errorReason());
    }
  }
}

void printText(String currentText) {
  Serial.print("in the print loop - the current value is: ");
  Serial.println(currentText);
  int text_len = currentText.length() + 1;
  char textArray[text_len];
  currentText.toCharArray(textArray, text_len);
  display.setRotation(0);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(currentText, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do {
    Serial.println("printing to the screen");
    display.fillScreen(GxEPD_WHITE); // refresh screen
    display.setCursor(x, y);
    display.print(currentText);
  } while (display.nextPage());
}

// ------------------------------------------------------------------------
// Function Declaration

int getArrayFromFirestore(FirebaseJson startingJson, String pathToArray) {
  FirebaseJsonData jsonData;
  startingJson.get(jsonData, pathToArray, true);
  FirebaseJsonArray arr;

  // //Get array data
  jsonData.get<FirebaseJsonArray>(arr);
  // String outputArray[arr.size()];
  Serial.print("The size of the array is: ");
  Serial.println(arr.size());
  Serial.println("the JSON array is: ");

  Serial.println("the array data is: ");
  //Call get with FirebaseJsonData to parse the array at defined index i
  for (int i = 0; i < arr.size(); i++) {
    //jsonData now used as temporary object to get the parse results
    FirebaseJsonData tempJson;
    String iterator_val = String(i);
    arr.get(tempJson, "/[" + iterator_val  + "]/stringValue");
    Serial.print("Array index: ");
    Serial.print(i);
    Serial.print(", type: ");
    Serial.print(tempJson.type);
    Serial.print(", value: ");
    Serial.println(tempJson.to<String>());
    contentArray[i] = tempJson.to<String>();
    tempJson.clear();
  }
  return arr.size();
}
