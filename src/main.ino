/*
  It's finally here!
  Edit by f0x, original code from Fastled, Sebastius, Juerd
*/

#include <EEPROM.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>

#include <PubSubClient.h> //https://github.com/knolleary/pubsubclient/releases/tag/2.4

#include "FastLED.h"
FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

void onMqttMessage(char* topic, byte * payload, unsigned int length);
boolean reconnect();
void mqtt_publish (String topic, String message);
void rainbow();
void juggle();
void confetti();
void nextPattern();

#define DATA_PIN    5
#define CLK_PIN     6
#define LED_TYPE    APA102
#define COLOR_ORDER BGR
#define NUM_LEDS    11
CRGB leds[NUM_LEDS];

#define BRIGHTNESS        255
#define FRAMES_PER_SECOND 1000
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

bool spacestate = LOW;
bool klok_ok = false;
bool switchstate;
bool zapgevaar = LOW;
bool iemand_kijkt = LOW;
static uint8_t hue = 0;
int x = 0;

// WiFi settings
char ssid[] = "revspace-pub-2.4ghz"; // your network SSID (name)
char pass[] = "";                    // your network password

// MQTT Server settings and preparations
const char* mqtt_server = "mosquitto.space.revspace.nl";
WiFiClient espClient;

PubSubClient client(mqtt_server, 1883, onMqttMessage, espClient);
long lastReconnectAttempt = 0;

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

boolean reconnect() {
    if (client.connect("SAMLA")) {
        client.publish("f0x/samla", "hello world");
        client.subscribe("revspace/cams");
        client.loop();
        client.subscribe("revspace/button/nomz");
        client.loop();
        client.subscribe("revspace/button/doorbell");
        client.loop();
    }
    return client.connected();
}


void onMqttMessage(char* topic, byte * payload, unsigned int length) {
    uint16_t spaceCnt;
    uint8_t numCnt = 0;
    char bericht[50] = "";

    Serial.print("received topic: ");
    Serial.println(topic);
    Serial.print("length: ");
    Serial.println(length);
    Serial.print("payload: ");
    for (uint8_t pos = 0; pos < length; pos++) {
        bericht[pos] = payload[pos];
    }
    Serial.println(bericht);
    Serial.println();

    if (strcmp(topic, "revspace/cams") == 0) {
        Serial.print("cams:");
        Serial.println(bericht[8]);

        uint8_t spatieteller = 3;
        uint8_t positie = 0;
        
        while (spatieteller) if (bericht[positie++] == ' ') spatieteller--;
        
        Serial.print("positie");
        Serial.println(bericht[positie], DEC);
        iemand_kijkt = (bericht[positie] != '0');
        if (iemand_kijkt) {
            Serial.println("Iemand kijkt op cam3");
        }
    }

    if (strcmp(topic, "revspace/button/nomz") == 0) {
        Serial.println("NOMZ!");
        for (uint8_t tel = 0; tel < 150; tel++) {
            for (uint8_t vuller = 0; vuller < NUM_LEDS; vuller++) {
                leds[vuller] = CRGB::Orange;
            }
            FastLED.show();
            delay(150);

            for (uint8_t vuller = 0; vuller < NUM_LEDS; vuller++) {
                leds[vuller] = CRGB::Blue;
            }
            FastLED.show();
            delay(100);
        }

    }

    // DOORBELL
    if (strcmp(topic, "revspace/button/doorbell") == 0) {
        Serial.println("Doorbell!");
        for (uint8_t tel = 0; tel < 10; tel++) {
            for (uint8_t vuller = 0; vuller < NUM_LEDS; vuller++) {
                leds[vuller] = CRGB::Yellow;
            }
            FastLED.show();
            delay(1000);

            for (uint8_t vuller = 0; vuller < NUM_LEDS; vuller++) {
                leds[vuller] = CRGB::Black;
            }
            FastLED.show();
            delay(1000);
        }
    }
    Serial.println();
}

void mqtt_publish (String topic, String message) {
    Serial.println();
    Serial.print("Publishing ");
    Serial.print(message);
    Serial.print(" to ");
    Serial.println(topic);

    char t[100], m[100];
    topic.toCharArray(t, sizeof t);
    message.toCharArray(m, sizeof m);

    client.publish(t, m, /*retain*/ 0);
}

void setup() {
    WiFi.mode(WIFI_STA);
    Serial.begin(115200);
    Serial.println();
    Serial.println(ESP.getChipId());
    // tell FastLED about the LED strip configuration
    FastLED.addLeds<LED_TYPE, DATA_PIN, CLK_PIN, BGR,DATA_RATE_MHZ(2)>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    // set master brightness control
    FastLED.setBrightness(255);

    // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);

    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(50);
        leds[i] = CRGB::Red;
        leds[(i+NUM_LEDS-1)%NUM_LEDS] = CRGB::Black;
        FastLED.show();
        i = (i+1)%NUM_LEDS;
        Serial.print(".");
    }
    for (int i=0; i<NUM_LEDS; i++) {
      leds[i] = CRGB::Black;
    }
    FastLED.show();
    Serial.println("");

    Serial.print("WiFi connected");
    Serial.println(ssid);

    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    ArduinoOTA.setPassword("329euaidjk");

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
        FastLED.showColor(CRGB::Black);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
        FastLED.showColor(CRGB::Green);
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        leds[x] = CRGB::Blue;
        leds[(x+NUM_LEDS-1)%NUM_LEDS] = CRGB::Black;
        FastLED.show();
        x = (x+1)%NUM_LEDS;
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();
}


void loop() {
    if (!client.connected()) {
        long verstreken = millis();
        if (verstreken - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = verstreken;
            // Attempt to reconnect
            if (reconnect()) {
                lastReconnectAttempt = 0;
            }
        }
    } else {
        // Client connected
        client.loop();
    }
    
    ArduinoOTA.handle();
    //fill_rainbow( leds, NUM_LEDS, gHue, 7);

    if (iemand_kijkt) {
      FastLED.showColor(CRGB::Red);
    } else {
      FastLED.showColor(CHSV(hue, 255, 200)); 
    }

    FastLED.show();
    EVERY_N_MILLISECONDS( 100 ) {
        hue++;    // slowly cycle the "base color" through the rainbow
    }
    if (hue>255) {
      hue=0;
    }
}

