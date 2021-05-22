#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <FastLED.h>
#include <PubSubClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
// #include <NTPClient.h>

#define NUM_LEDS 512
#define DATA_PIN 0

CRGB leds[NUM_LEDS];

const String FirmwareVer = {"4.4"};
#define URL_fw_Version "https://raw.githubusercontent.com/Sthira-Nusantara/iot-rgb-firmware/master/version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/Sthira-Nusantara/iot-rgb-firmware/master/firmware.bin"

String URL_register = "https://api.rupira.com/v1/iot/request";

const char *mqtt_server = "mqtt.rupira.com";
const char *mqtt_username = "sthirasystemiot";
const char *mqtt_password = "Ud3^fn*mpWWKaLw8oRUBG5FY&NQ2A@cK^iVW*7FZCRqCgnKYEQ6wBHh5K%J9uwCBW%KGBrg$Kv5Bgx$yCBGR3UuhSZq%89XFHf@6^cJjRamB7FntyAG#78%wdYhN!YYX";
// const char *ssid = "IOT_STHIRA";
// const char *password = "IotSthiraNusantara@2712";
const char *ssid = "IOT_STHIRA";
const char *password = "IotSthiraNusantara@2712";

String MacAdd = String(WiFi.macAddress());

String COMPANY = "sthira";
String DEVICE = "controller";
String UNUM = MacAdd;

String prefix = COMPANY + "/" + DEVICE + "/" + UNUM;
// ----- Subscribe Init
String pinmode = prefix + "/pinmode";
String trigger = prefix + "/changemode";
String setvalue = prefix + "/setvalue";
String lastmode = prefix + "/lastmode";
String correctionmode = prefix + "/correct";
String update = prefix + "/update";
String restart = prefix + "/restart";
String testSubs = prefix + "/test";
String checkConnection = prefix + "/check";
String yesConnect = prefix + "/yesConnect";

int lastCodeColor = 1;
uint8_t lastBrightness = 255;

// const long utcOffsetInSeconds = 25200;

// char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

CRGB::HTMLColorCode colors[7] = {CRGB::Red, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Magenta, CRGB::White, CRGB::Black};

WiFiClientSecure deviceClient;
WiFiClient espClient;
// WiFiUDP ntpUDP;
// NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
PubSubClient client(espClient);

void changeColor(CRGB color)
{
  for (int i = 0; i <= NUM_LEDS; i++)
  {
    leds[i] = color;
  }
  FastLED.show();
}

void changeBrightness(uint8_t value)
{

  // Val = 255 , Last = 128;
  if (value > lastBrightness)
  {
    for (uint8_t i = lastBrightness; i < value; i++)
    {
      FastLED.setBrightness(i);
      FastLED.show();
      delay(2);
    }
  }

  // Val = 125, Last = 255
  if (value < lastBrightness)
  {
    for (uint8_t i = lastBrightness; i > value; i--)
    {
      FastLED.setBrightness(i);
      FastLED.show();
      delay(2);
    }
  }
}

void blinkingColor(CRGB color, int times, int ms)
{
  for (int i = 0; i <= times; i++)
  {
    changeColor(color);
    delay(ms);
    changeColor(CRGB::Black);
    delay(ms);
  }

  changeColor(color);
}

void FirmwareUpdate()
{
  Serial.println("Preparing update firmware");

  if ((WiFi.status() == WL_CONNECTED))
  {

    deviceClient.setInsecure();

    //    deviceClient.addHeader("Authorization", "Bearer affd6c0995d4b701ce6e67b2531eb368177f3e7f");

    HTTPClient https;

    Serial.print("[HTTPS] begin...\n");
    if (https.begin(deviceClient, URL_fw_Version))
    { // HTTPS
      //      https.addHeader("Authorization", "Bearer affd6c0995d4b701ce6e67b2531eb368177f3e7f");

      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0)
      {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
          String payload = https.getString();
          payload.trim();

          Serial.print("Latest Version: ");
          Serial.println(payload);

          if (payload.equals(FirmwareVer))
          {
            Serial.println("Device already on latest firmware version");
          }
          else
          {
            Serial.println("New firmware detected");
            ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

            Serial.println("Device ready to update");

            t_httpUpdate_return ret = ESPhttpUpdate.update(deviceClient, URL_fw_Bin);

            switch (ret)
            {
            case HTTP_UPDATE_FAILED:
              Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
              break;

            case HTTP_UPDATE_NO_UPDATES:
              Serial.println("HTTP_UPDATE_NO_UPDATES");
              break;

            case HTTP_UPDATE_OK:
              Serial.println("Update Done well");
              Serial.println("HTTP_UPDATE_OK");
              break;
            }
          }
        }
      }
      else
      {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    }
    else
    {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }
}

void registerDevice()
{

  deviceClient.setInsecure();
  HTTPClient https;

  https.begin(deviceClient, URL_register.c_str()); //HTTP
  https.addHeader("Content-Type", "application/json");

  Serial.println("Try to register device");
  int httpCode = https.POST("{\"UNUM\": \"" + MacAdd + "\",\"requestCategory\":\"Controller\"}");

  if (httpCode > 0)
  {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK)
    {
      const String &payload = https.getString();
      Serial.println("received payload:\n<<");
      Serial.println(payload);
      Serial.println(">>");
    }
    else
    {
      const String &payload = https.getString();
      Serial.println("received payload:\n<<");
      Serial.println(payload);
      Serial.println(">>");
    }
  }
  else
  {
    Serial.printf("[HTTP] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
  }

  //  https.end();
}

void callback(char *topic, byte *payload, unsigned int length)
{

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }

  Serial.println();

  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length);

  if (strcmp(topic, trigger.c_str()) == 0)
  {
    Serial.print("Device : ");
    const char *device = doc["device"];
    Serial.print(device);
    Serial.print("");

    if (strcmp(device, "rgb") == 0)
    {
      const uint8_t pinChoose = doc["pin"][0];
      int valueTrigger = doc["value"][0];
      if (valueTrigger > 7)
      {
        valueTrigger = 7;
      }
      else if (valueTrigger < 1)
      {
        valueTrigger = 1;
      }
      else
      {
        valueTrigger = doc["value"][0];
      }
      changeColor(colors[valueTrigger - 1]);
      lastCodeColor = valueTrigger;

      char JSON[40];

      sprintf(JSON, "{\"pin\":[\"%d\"],\"value\": [\"%d\"]}", pinChoose, valueTrigger);
      client.publish(setvalue.c_str(), JSON);
    }

    if (strcmp(device, "brightness") == 0)
    {
      Serial.println("Set Brightness");

      const uint8_t pinChoose = doc["pin"][0];
      uint8_t valueTrigger = doc["value"][0];
      if (valueTrigger > 255)
      {
        valueTrigger = 255;
      }
      else if (valueTrigger < 0)
      {
        valueTrigger = 0;
      }
      else
      {
        valueTrigger = doc["value"][0];
      }
      changeBrightness(valueTrigger);
      lastBrightness = valueTrigger;

      char JSON[40];

      sprintf(JSON, "{\"pin\":[\"%d\"],\"value\": [\"%d\"]}", pinChoose, valueTrigger);
      client.publish(setvalue.c_str(), JSON);
    }
  }

  if (strcmp(topic, correctionmode.c_str()) == 0)
  {
    Serial.print("Device : ");
    const char *device = doc["device"];
    Serial.print(device);
    Serial.print("");

    if (strcmp(device, "rgb") == 0)
    {
      int valueTrigger = doc["value"][0];
      if (valueTrigger > 7)
      {
        valueTrigger = 7;
      }
      else if (valueTrigger < 1)
      {
        valueTrigger = 1;
      }
      else
      {
        valueTrigger = doc["value"][0];
      }
      changeColor(colors[valueTrigger - 1]);
      lastCodeColor = valueTrigger;
    }

    if (strcmp(device, "brightness") == 0)
    {
      uint8_t valueTrigger = doc["value"][0];
      if (valueTrigger > 255)
      {
        valueTrigger = 255;
      }
      else if (valueTrigger < 0)
      {
        valueTrigger = 0;
      }
      else
      {
        valueTrigger = doc["value"][0];
      }
      changeBrightness(valueTrigger);
      lastBrightness = valueTrigger;
    }
  }

  if (strcmp(topic, testSubs.c_str()) == 0)
  {
    blinkingColor(colors[lastCodeColor - 1], 3, 500);
  }

  if (strcmp(topic, update.c_str()) == 0)
  {
    Serial.println("Checking firmware update");
    FirmwareUpdate();
  }

  if (strcmp(topic, restart.c_str()) == 0)
  {
    ESP.restart();
  }

  if (strcmp(topic, checkConnection.c_str()) == 0)
  {
    client.publish(yesConnect.c_str(), "1");
  }

  Serial.println();
  //  return;
}

String scanNetwork(const char *ssid)
{
  Serial.println("scan start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
  {
    Serial.println("no networks found");
  }
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    int myVal = WiFi.RSSI(0);
    String BSSIDvalue;
    for (int i = 0; i < n; ++i)
    {
      Serial.print(WiFi.SSID(i));
      Serial.print(" - ");
      Serial.print(WiFi.BSSIDstr(i));
      Serial.print("(");
      Serial.print(WiFi.RSSI(i));
      Serial.print(") - ");
      Serial.println(WiFi.channel(i));
      if (WiFi.SSID(i) == ssid)
      {
        if (WiFi.RSSI(i) > myVal)
        {
          myVal = WiFi.RSSI(i);
          BSSIDvalue = WiFi.BSSIDstr(i);
        }
      }
    }

    Serial.println();
    Serial.println(BSSIDvalue);

    return BSSIDvalue;
  }

  return "";
}

int getChannel(String bssid)
{
  Serial.println("Find BSSID channel");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan network for channel done");
  if (n == 0)
  {
    Serial.println("no networks found");
  }
  else
  {
    for (int i = 0; i < n; ++i)
    {
      if (WiFi.BSSIDstr(i) == bssid)
      {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network

  String BSSIDnetwork = scanNetwork(ssid);
  int chan = getChannel(BSSIDnetwork);

  int n = BSSIDnetwork.length();

  char char_array[n + 1];

  strcpy(char_array, BSSIDnetwork.c_str());

  uint16_t num = strtoul(char_array, nullptr, 16);

  uint8_t bssid[sizeof(num)];
  memcpy(bssid, &num, sizeof(num));

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.print("Connecting to BSSID ");
  Serial.println(BSSIDnetwork);
  Serial.print("Connecting to Channel ");
  Serial.println(chan);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    delay(500);
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

boolean reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("connected");
      client.publish("connected", MacAdd.c_str());
      client.publish(lastmode.c_str(), "1");

      // //      Your Subs
      client.subscribe(pinmode.c_str());
      client.subscribe(trigger.c_str());
      client.subscribe(correctionmode.c_str());
      client.subscribe(testSubs.c_str());
      client.subscribe(update.c_str());
      client.subscribe(restart.c_str());
      client.subscribe(checkConnection.c_str());
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  return client.connected();
}

void setup()
{
  Serial.begin(115200);

  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("=========================================");
  Serial.println("============STHIRA NUSANTARA=============");
  Serial.println("=========================================");

  Serial.print("Current Firmware Version: ");
  Serial.println(FirmwareVer);

  SPI.begin(); // Init SPI bus
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS); // GRB ordering is typical
                                                           // FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  delay(500);
  changeColor(colors[0]);
  delay(500);
  changeColor(colors[1]);
  delay(500);
  changeColor(colors[2]);
  delay(500);
  changeColor(colors[3]);
  delay(500);
  changeColor(colors[4]);

  delay(500);

  changeColor(colors[3]);
  FastLED.setBrightness(255);
  FastLED.show();
  delay(500);
  Serial.println("Setup done");

  Serial.println(MacAdd);

  setup_wifi();

  registerDevice();

  FirmwareUpdate();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop()
{

  if (WiFi.status() != WL_CONNECTED)
  {
    setup_wifi();
  }

  // timeClient.update();

  // int thisHour = timeClient.getHours();
  // int thisMinute = timeClient.getMinutes();
  // int thisSecond = timeClient.getSeconds();

  // if (thisHour == 0 && thisMinute == 1 && thisSecond == 0)
  // {
  //   Serial.println("Checking firmware update");
  //   FirmwareUpdate();
  // }

  // if (thisHour == 1 && thisMinute == 1 && thisSecond == 0)
  // {
  //   Serial.println("Checking firmware update");
  //   FirmwareUpdate();
  // }

  // if (thisHour == 2 && thisMinute == 1 && thisSecond == 0)
  // {
  //   Serial.println("Checking firmware update");
  //   FirmwareUpdate();
  // }

  // if (thisHour == 19 && thisMinute == 1 && thisSecond == 0)
  // {
  //   Serial.println("Checking firmware update");
  //   FirmwareUpdate();
  // }

  // if (thisHour == 20 && thisMinute == 1 && thisSecond == 0)
  // {
  //   Serial.println("Checking firmware update");
  //   FirmwareUpdate();
  // }

  // if (thisHour == 21 && thisMinute == 1 && thisSecond == 0)
  // {
  //   Serial.println("Checking firmware update");
  //   FirmwareUpdate();
  // }

  if (!client.connected())
  {
    Serial.println("MQTT is NOT connected");
    int lastReconnectAttempt = 0;
    long now = millis();
    if (now - lastReconnectAttempt > 5000)
    {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect())
      {
        lastReconnectAttempt = 0;
      }
    }
  }
  else
  {
    client.loop();
  }

  delay(1000);
}
