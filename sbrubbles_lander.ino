#define ESP_DRD_USE_SPIFFS true

#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

#define JSON_CONFIG_FILE "/config.json"
#define TRIGGER_PIN 0

bool shouldSaveConfig = false;

char sbrubblesServer[80] = "sbrubbles-server";
int testNumber = 1234;
int timeout = 120;

WiFiManager wm;

void saveConfigFile() {
  Serial.println(F("Saving configuration..."));

  StaticJsonDocument<512> json;
  json["sbrubblesServer"] = sbrubblesServer;
  json["testNumber"] = testNumber;

  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  configFile.close();
}

void checkSpiffsInitialization() {
  Serial.println("SPIFFS Initialization: (First time run can last up to 30 sec - be patient)");

  boolean mounted = SPIFFS.begin();  // load config if it exists. Otherwise use defaults.
  if (!mounted) {
    Serial.println("FS not formatted. Doing that now... (can last up to 30 sec).");
    SPIFFS.format();
    Serial.println("FS formatted...");
    if (!SPIFFS.begin()) {
      Serial.println("Failed to mount FS");
    }
  }
}

bool loadConfigFile() {
  checkSpiffsInitialization();

  Serial.println("Mounting File System...");

  if (SPIFFS.exists(JSON_CONFIG_FILE)) {
    Serial.println("reading config file");
    File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");

    if (configFile) {
      Serial.println("Opened configuration file");
      StaticJsonDocument<512> json;
      DeserializationError error = deserializeJson(json, configFile);
      serializeJsonPretty(json, Serial);

      if (!error) {
        Serial.println("Parsing JSON");

        strcpy(sbrubblesServer, json["sbrubblesServer"]);
        testNumber = json["testNumber"].as<int>();

        return true;
      } else {
        Serial.println("Failed to load json config");
      }
    }
  }

  return true;
}


void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void configModeCallback(WiFiManager* myWiFiManager) {
  myWiFiManager->setHostname("sbrubbles-lander");
  myWiFiManager->setShowInfoErase(false);
  myWiFiManager->setShowInfoUpdate(false);


  Serial.println("Entered Configuration Mode");

  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());

  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void setup() {
  bool forceConfig = false;

  bool spiffsSetup = loadConfigFile();
  if (!spiffsSetup) {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname("sbrubbles-lander");

  Serial.begin(115200);
  delay(10);

  wm.resetSettings();
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setAPCallback(configModeCallback);

  WiFiManagerParameter custom_text_box("key_text", "Sbrubbles server address", sbrubblesServer, 80);

  char convertedValue[6];
  sprintf(convertedValue, "%d", testNumber);

  WiFiManagerParameter custom_text_box_num("key_num", "Enter your number here", convertedValue, 7);

  wm.addParameter(&custom_text_box);
  wm.addParameter(&custom_text_box_num);

  if (forceConfig) {
    if (!wm.startConfigPortal("sbrubbles_lander", "lander-wifi")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);

      ESP.restart();
      delay(5000);
    }
  } else {
    if (!wm.autoConnect("sbrubbles_lander", "lander-wifi")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  strncpy(sbrubblesServer, custom_text_box.getValue(), sizeof(sbrubblesServer));
  Serial.print("Sbrubbles Server: ");
  Serial.println(sbrubblesServer);

  testNumber = atoi(custom_text_box_num.getValue());
  Serial.print("testNumber: ");
  Serial.println(testNumber);

  if (shouldSaveConfig) {
    saveConfigFile();
  }

  pinMode(TRIGGER_PIN, INPUT_PULLUP);
}


void loop() {
  if (digitalRead(TRIGGER_PIN) == LOW) {
    wm.setConfigPortalTimeout(timeout);

    if (!wm.startConfigPortal("OnDemandAP")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
}