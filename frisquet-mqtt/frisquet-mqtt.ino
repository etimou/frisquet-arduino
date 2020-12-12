/*

  Frisquet - MQTT
  This sketch allows communication between an esp8266 and a Frisquet heating boiler.
  The esp8266 replaces the original Eco Radio System receiver.

  Wiring :
  ESP8266              BOILER
  D2 (configurable)    yellow wire
  Gnd                  black wire

  To send a command to the boiler : publish Mode,Value on topic inTopic (e.g. 3,20 for Comfort Mode and Heating value 20)

  To receive the water temperature : subcribe the outTopic topic. It makes use of a ds18b20 sensor.

*/
/********************************************************************/
// First we include the libraries
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoOTA.h>
/********************************************************************/
// Data wire is plugged into pin 10 on the Arduino
#define ONE_WIRE_BUS D3
#define ERS_pin D2
/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
/********************************************************************/

int long_pulse = 825;
byte message[17] = {0x00, 0x00, 0x00, 0x7E, 0xBE, 0xE7, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFD, 0x00, 0xFF, 0x00};
// replace 0xBE and 0xE7 with your ID



byte old_state, num_byte;
byte bitstuff = 0;

byte heatingValue = 0;
byte preHeatingValue = 0;
int delayCycleCmd; //  This variable contains the delay for the next command to the boiler (if no command is received via MQTT)
#define DELAY_CYCLE_CMD 240000 // delay between 2 commands (4min)
#define DELAY_CYCLE_CMD_INIT 240000// delay for the 1st command after startup (4min)
#define DELAY_REPEAT_CMD 20000 // when a new command is issued, it is repeated after this delay (20s)
#define DELAY_TIMEOUT_CMD_MQTT 900000 // 15min Max delay without Mqtt msg ---PROTECTION OVERHEATING ---- (Same as remote) - 0 to deactivate
#define DELAY_CYCLE_MSG 60000 // reports temperature every minute



#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
long lastCmd = 0;
long lastCmdMQTT = 0;
char msg[50];
int value = 0;

void writeBit(bool Bit) {
  old_state = !old_state;
  digitalWrite(ERS_pin, old_state);
  delayMicroseconds(long_pulse);

  if (Bit) {
    old_state = !old_state;
    digitalWrite(ERS_pin, old_state);
  }
  delayMicroseconds(long_pulse);

}

void conversion(byte input) {
  for (byte n = 0; n < 8; n++) { // boucle pour chaque bit
    writeBit(bitRead(input, n));
    if (num_byte >= 4 && num_byte <= 14) {
      if (bitRead(input, n) == 1) bitstuff++;  // incrémente le compteur bitstuffing
    }
    if (bitRead(input, n) == 0) bitstuff = 0;

    if (bitstuff >= 5) {
      writeBit(0);
      bitstuff = 0;
    }
  }
}

void commande(byte prechauffage, byte chauffage) {
  Serial.print("Nouvelle commande de chauffage : (");
  Serial.print(prechauffage);
  Serial.print(",");
  Serial.print(chauffage);
  Serial.println(")");
  Serial.flush();

  if ((chauffage <= 100) and ((prechauffage == 0) or (prechauffage == 3) or (prechauffage == 4))) {
    for (byte x = 0; x < 3; x++) { // boucle de 3 messages
        old_state = 0;
        message[9] = x;  // numero message : 0 à 2
        if (x == 2) {
          message[10] = prechauffage;
        } else {
          message[10] = prechauffage + 0x80;
        }
        message[11] = chauffage;

        int checksum = 0;
        for (int i = 4; i <= 12; i++) {
          checksum -= message[i];
        }
        message[13] = highByte(checksum);
        message[14] = lowByte(checksum);

        for (num_byte = 1; num_byte < 17; num_byte++) { // boucle de 16 bytes
          conversion(message[num_byte]);
        }
        //digitalWrite(ERS_pin, HIGH);
        digitalWrite(ERS_pin, LOW);
        delay(33);
      }
      digitalWrite(ERS_pin, LOW);

    } else {
      return;
    }
  }



  void setup_wifi() {

    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    randomSeed(micros());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  void callback(char* topic, byte * payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    /*for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
      }
      Serial.println();*/
    char str[length + 1];
    memcpy(str, payload, length);
    str[length] = 0;

    char * strToken = strtok ( str, "," );
    preHeatingValue = atoi(strToken);
    strToken = strtok ( NULL, "," );
    heatingValue = atoi(strToken);

    if ((heatingValue <= 100) and ((preHeatingValue == 0) or (preHeatingValue == 3) or (preHeatingValue == 4))) {

      commande(preHeatingValue, heatingValue);  // reduit 0, confort 3, hors gel 4, chauffage 0 à 100
      lastCmd = millis();
      lastCmdMQTT = lastCmd;
      delayCycleCmd = DELAY_REPEAT_CMD;
    }



    /*
      // Switch on the LED if an 1 was received as first character
      if ((char)payload[0] == '1') {
      digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
      // but actually the LED is on; this is because
      // it is active low on the ESP-01)
      } else {
      digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
      }
    */

  }

  void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      // Create a random client ID
      String clientId = "ESP8266Client-";
      clientId += String(random(0xffff), HEX);
      // Attempt to connect
      if (client.connect(clientId.c_str())) {
        Serial.println("connected");
        // Once connected, publish an announcement...
        //client.publish("outTopic", "hello world");
        // ... and resubscribe
        client.subscribe("inTopic");
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
  }

  void setup() {
    pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
    pinMode(ERS_pin, OUTPUT);
    digitalWrite(ERS_pin, LOW);
    digitalWrite(LED_BUILTIN, HIGH);
    delayCycleCmd = DELAY_CYCLE_CMD_INIT; // init
    Serial.begin(115200);
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    // Start up the library
    sensors.begin();

    // OTA
    ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else { // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });
    ArduinoOTA.begin();
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

  }

  void loop() {

    if (WiFi.status() != WL_CONNECTED) {
      //WifiReconnect();
    }

    ArduinoOTA.handle();

    if (!client.connected()) {
      reconnect();
    }
    client.loop();


    long now = millis();
    if (now - lastMsg > DELAY_CYCLE_MSG) {
      // call sensors.requestTemperatures() to issue a global temperature
      // request to all devices on the bus
      /********************************************************************/
      Serial.print(" Requesting temperatures...");
      sensors.requestTemperatures(); // Send the command to get temperature readings
      Serial.println("DONE");
      /********************************************************************/
      Serial.print("Temperature is: ");
      float temp = sensors.getTempCByIndex(0);
      Serial.println(temp); // Why "byIndex"?
      // You can have more than one DS18B20 on the same bus.
      // 0 refers to the first IC on the wire

      lastMsg = now;
      ++value;
      snprintf (msg, 50, "%.2f,%d,0,0\n", temp, heatingValue);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish("outTopic", msg);


    }
    if (now - lastCmd > delayCycleCmd) {
      if ((now - lastCmdMQTT < DELAY_TIMEOUT_CMD_MQTT) || (DELAY_TIMEOUT_CMD_MQTT == 0)) {
        commande(preHeatingValue, heatingValue);  // reduit 0, confort 3, hors gel 4, chauffage 0 à 100
      }
      lastCmd = now;
      delayCycleCmd = DELAY_CYCLE_CMD;
    }
  }
