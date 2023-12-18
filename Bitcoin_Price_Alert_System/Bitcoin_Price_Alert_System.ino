//Download all libraries from Libraries manager
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <Wire.h>
#include <HTTPClient.h>
//#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h> //upgrade to latest version of 6.x.x
#include <WiFi.h>
#include <WiFiClientSecure.h>

//Wifi details 
char wifiSSID[] = "Texoham" ;  // Enter your hotspot name 
char wifiPASS[] = "T3x0h@m@2k20"; // Enter your hotspot password
String on_currency = "BTCGBP";
String on_sub_currency = on_currency.substring(3);
char conversion[20];

const uint16_t WAIT_TIME = 1000;


#define HARDWARE_TYPE MD_MAX72XX::FC16_HW //If your LEDs look odd try replacing ICSTATION_HW with one of these GENERIC_HW, FC16_HW, PAROLA_HW .

//Connections and Pinout
#define MAX_DEVICES 4
#define CLK_PIN   18
#define DATA_PIN  23
#define CS_PIN    5
#define LED 2
#define BUZZER 4 

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// Scrolling parameters
uint8_t scrollSpeed = 50;    // default frame delay value
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 2000; // in milliseconds

// Global message buffers shared by Serial and Scrolling functions
#define  BUF_SIZE  75
char curMessage[BUF_SIZE] = { "" };
char newMessage[BUF_SIZE] = { "Hello! Enter new message?" };
bool newMessageAvailable = true;

///NEWCODE
const int httpsPort = 443;                                                          //Bitcoin price API powered by CoinDesk - https://www.coindesk.com/price/bitcoin
const String url = "http://api.coindesk.com/v1/bpi/currentprice/BTC.json";
const String historyURL = "http://api.coindesk.com/v1/bpi/historical/close.json";
const String cryptoCode = "BTC";

WiFiClient client;                                                            //Create a new WiFi client
HTTPClient http;

String formattedDate;                                                         //Create variables to store the date and time
String dayStamp;
String timeStamp;

double yesterdayPrice = 41090.00;
double prevPrice = 41066.00 ;
boolean priceChanged = false;
void setup(void)
{
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  //connect to local wifi
  WiFi.begin(wifiSSID, wifiPASS);
  Serial.println("Wifi Connecting ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("....");
  }
  Serial.println("Connected");
  P.begin();
  P.setIntensity(0);
  P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
}
int counta = 0;
void loop(void)
{
  if (P.displayAnimate()) {
    strcpy(curMessage, "Bitcoin Price Alert System");
    P.displayReset();
  }

  if (P.displayAnimate())
  { if (newMessageAvailable) {
      strcpy(curMessage, "Easy come, Rich Life");
      Serial.println("Bitcoin seems to be a very promising idea.");
    } P.displayReset();
  }

  ///NEW code
  Serial.print("Connecting to ");                                                       //Display url on Serial monitor for debugging
  Serial.println(url);

  http.begin(url);
  int httpCode = http.GET();                                                            //Get crypto price from API
  StaticJsonDocument <2000> doc;
  DeserializationError error = deserializeJson(doc, http.getString());

  if (error)                                                                            //Display error message if unsuccessful
  {
    Serial.print(F("deserializeJson Failed"));
    Serial.println(error.f_str());
    delay(2500);
    return;
  }

  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);

  String BTCUSDPrice = doc["bpi"]["USD"]["rate_float"].as<String>();                    //Store crypto price and update date in local variables
  String lastUpdated = doc["time"]["updated"].as<String>();
  http.end();

  Serial.print("Getting history...");
  StaticJsonDocument<2000> historyDoc;
  http.begin(historyURL);                                                               //Get historical crypto price from API
  int historyHttpCode = http.GET();
  DeserializationError historyError = deserializeJson(historyDoc, http.getString());

  if (historyError) {                                                                   //Display error message if unsuccessful
    Serial.print(F("deserializeJson(History) failed"));
    Serial.println(historyError.f_str());
    delay(2500);
    return;
  }

  Serial.print("History HTTP Status Code: ");
  Serial.println(historyHttpCode);
  JsonObject bpi = historyDoc["bpi"].as<JsonObject>();

  for (JsonPair kv : bpi) {
    yesterdayPrice = kv.value().as<double>();                                           //Store yesterday's crypto price
  }
  Serial.print("BTCUSD Price: ");                                                       //Display current price on serial monitor
  Serial.println(BTCUSDPrice.toDouble());
  Serial.print("Yesterday's Price: ");                                                  //Display yesterday's price on serial monitor
  Serial.println(yesterdayPrice);
  if (BTCUSDPrice.toDouble() != prevPrice) {
    priceChanged = true;
    //digitalWrite(BUZZER,LOW);
  }

 if (!priceChanged) {
  if (P.displayAnimate())
  {
      digitalWrite(LED, LOW);
      digitalWrite(BUZZER, LOW);
      char priceString[15]; // Adjust size as per your maximum expected double size
      dtostrf(BTCUSDPrice.toDouble(), 10, 2, priceString); // Convert the double to a string with 2 decimal places

      // Concatenate the static text and the double value
      strcpy(curMessage, "Bitcoin USD Price Now: ");
      strcat(curMessage, priceString); // Concatenate the double string

      Serial.println("Bitcoin USD Displaying");
      // Display the message or use it as needed
      // For example: displayMessage(curMessage);
      P.displayReset();
      delay(1000);
      digitalWrite(LED, HIGH);
    }
 }
 else {
  digitalWrite(BUZZER, HIGH);
    digitalWrite(LED, LOW);
      char priceString[15]  ; // Adjust size as per your maximum expected double size
      dtostrf(prevPrice, 10, 2, priceString); // Convert the double to a string with 2 decimal places
      // Concatenate the static text and the double value
      strcpy(curMessage, "Last Bitcoin Price: ");
      strcat(curMessage, priceString); // Concatenate the double string
      Serial.println("Previous Bitcoin USD Displaying");
      digitalWrite(LED, HIGH);
      P.displayReset();
      priceChanged = false;
      prevPrice = BTCUSDPrice.toDouble();
      //P.displayAnimate();
      delay(1000);
  }

    bool isUp = BTCUSDPrice.toDouble() >= yesterdayPrice;                                  //Check whether price has increased or decreased
    double percentChange;
    String dayChangeString = "24hr Change:";
    if (isUp)                                                                             //If price has increased from yesterday
    {
      percentChange = ((BTCUSDPrice.toDouble() - yesterdayPrice) / yesterdayPrice) * 100;
    }
    else                                                                                  //If price has decreased from yesterday
    {
      percentChange = ((yesterdayPrice - BTCUSDPrice.toDouble()) / yesterdayPrice) * 100;
      dayChangeString = dayChangeString + "-";
    }

    Serial.print("Percent Change: ");                                                     //Display the percentage change on the serial monitor
    Serial.println(percentChange);                                           //Display the current price

    //display.setTextSize(1);                                                               //Display the change percentage
    dayChangeString = dayChangeString + percentChange + "%";
    // printCenter(dayChangeString, 0, 55);
    // display.display();                                                                    //Execute the new display

    http.end();                                                                           //End the WiFi connection
    esp_sleep_enable_timer_wakeup(900000000);                                             //Sleep for 15 minutes
  }
