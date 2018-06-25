#include <WiFiEsp.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <Keypad.h>
#include <HX711.h>
#include <Ethernet.h>
#include "SoftwareSerial.h"

#define RST_PIN 49
#define SS_PIN 53

const byte ROWS = 3;
const byte COLS = 3;

char hexaKeys[ROWS][COLS] = {
    {'1', '4', '7'},
    {'2', '5', '8'},
    {'3', '6', '9'}};

const byte humiditySensorPin = 48;
const byte rowPins[ROWS] = {47, 46, 45};
const byte colPins[COLS] = {44, 43, 42};

const String accessPassword = "1234";
const String PID = "D5 50 7E 63";
const float calibrationFactor = -14.70;
const float grammCoeficient = 0.035274;
const byte weightError = 10;
const short accessWeight = 300;
const char ssid[] = "BlockChainHub";
const char wifiPassword[] = "";
const char host[] = "46.101.30.8";
const short port = 3000;

WiFiEspClient client;
MFRC522 mfrc522(SS_PIN, RST_PIN);
Keypad keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
StaticJsonBuffer<200> jsonBuffer;
HX711 scale(2, 3);

short status = WL_IDLE_STATUS;
short weight;
char key = 0;
String password;

void setup()
{
  Serial.begin(19200);
  Serial1.begin(115200);
  Serial.println("Starting serial communication...");

  WiFi.init(&Serial1);

  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, wifiPassword);
  }

  printWifiStatus();

  scale.tare();
  scale.set_scale(calibrationFactor);

  SPI.begin();
  Serial.println("Starting SPI bus");

  mfrc522.PCD_Init();

  pinMode(humiditySensorPin, INPUT);
}

void loop()
{

  char key = keypad.getKey();

  if (key != NO_KEY)
  {
    Serial.println(key);
    password += key;
  }

  if (password.length() >= 4)
  {
    if (accessPassword.equals(password))
    {
      sendRequest(createRequestBody(1, "button", true));
      password = "";
    }
    else
    {
      sendRequest(createRequestBody(0, "button", "Wrong Password", true));
      password = "";
    }
  }
  weight = scale.get_units() * grammCoeficient;
  if (weight<accessWeight + weightError & weight> accessWeight - weightError)
  {
    Serial.println("WeightSensorState: " + weight);

    sendRequest(createRequestBody(1, "sensor1", false));
    while (weight<accessWeight + weightError & weight> accessWeight - weightError)
    {
      weight = scale.get_units(10) * grammCoeficient;
    }
  }

  if (digitalRead(humiditySensorPin) == LOW)
  {
    Serial.println("HumiditySensorState: LOW");

    sendRequest(createRequestBody(1, "sensor2", false));
  }

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
  {
    delay(50);
    return;
  }

  Serial.print("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  if (content.substring(1) == PID)
  {
    sendRequest(createRequestBody(1, "sensor3", false));
  }
  else
  {
    Serial.println("Access denied");
    sendRequest(createRequestBody(0, "sensor3", "Wrong card with PID: " + content.substring(1), false));
  }
}

JsonObject &createRequestBody(int value, String sensorId, boolean primary)
{
  return createRequestBody(value, sensorId, "", primary);
}

JsonObject &createRequestBody(int value, String sensorId, String message, boolean primary)
{
  jsonBuffer.clear();
  JsonObject &root = jsonBuffer.createObject();
  root["sensorId"] = sensorId;
  root["deviceId"] = "arduinoUNO";
  root["shipmentId"] = "shrimp-25-05-18";

  if (message != "")
  {
    root["status"] = message;
    root["value"] = 1;
  }
  else
  {
    root["status"] = "Success";
    root["value"] = value;
  }
  return root;
}

void sendRequest(JsonObject &root)
{
  Serial.println("Creating request");
  client.stop();
  root.prettyPrintTo(Serial);
  Serial.println();

  if (client.connect(host, port))
  {
    if (root["sensorId"] == "button")
    {
      client.println("POST /buyer/prepay HTTP/1.1");
    }
    else
    {
      client.println("POST /device/signal HTTP/1.1");
    }
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(root.measurePrettyLength());
    client.println();
    root.prettyPrintTo(client);
    client.println();
  }
  else
  {
    Serial.println("Connection failed");
  }
}

void printWifiStatus()
{
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}