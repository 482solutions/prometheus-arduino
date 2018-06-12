#include <SPI.h>
#include <Ethernet.h>
#include <MFRC522.h>
#include <ArduinoJson.h>

int lightSensorPin = 2;
int humiditySensorPin = 3;
int buttonPin = 5;
int RSTPin = 9;
int SSPin = 8;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
String host = "https://arduino-mijin-deaizwongu.now.sh";
String mainHostPath = "/device/signal";
String secondaryHostPath = "/buyer/prepay";

int lightSensorState = 0;
int humiditySensorState = 0;

MFRC522 mfrc522(SS_PIN, RST_PIN);
EthernetClient client;
StaticJsonBuffer<100> jsonBuffer;

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting serial communication...");
  delay(1000);
  Ethernet.begin(mac);
  Serial.println("Starting Ethernet сonnection...");
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
  SPI.begin();
  Serial.println("Starting SPI bus");
  mfrc522.PCD_Init();
  ShowReaderDetails();

  pinMode(lightSensorPin, INPUT);
  pinMode(humiditySensorPin, INPUT);
  pinMode(buttonPin, INPUT);
}

void loop()
{
  if (digitalRead(buttonPin) == HIGH)
  {
    createPOSTRequest(1, "button");
  }

  if (lightSensorState != digitalRead(lightSensorPin))
  {
    lightSensorState = digitalRead(lightSensorPin);
    Serial.print("LightSensorState: ");
    Serial.println(lightSensorState);
    createPOSTRequest(lightSensorState, "sensor1");
  }

  if (humiditySensorState != digitalRead(humiditySensorPin))
  {
    humiditySensorState = digitalRead(humiditySensorPin);
    Serial.print("HumiditySensorState: ");
    Serial.println(humiditySensorState);
    createPOSTRequest(humiditySensorState, "sensor2");
  }

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
  {
    delay(50);
    return;
  }

  createPOSTRequest(1, "sensor3");
  mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
}

void createPOSTRequest(int value, String sensorId)
{
  Serial.println("Creating request");
  jsonBuffer.clear();
  client.stop();

  JsonObject &root = jsonBuffer.createObject();
  root["value"] = value;
  root["sensorId"] = sensorId;
  root["deviceId"] = "arduinoUNO";
  root["shipmentId"] = "shrimp-25-05-18";
  root.prettyPrintTo(Serial);

  if (sensorId == "button")
  {
    if (client.connect(host, 80))
    {
      Serial.println("Hey mom");
      client.println("POST /" + secondaryHostPath + " HTTP/1.1");
      client.println("Host: " + host);
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
  else
  {
    if (client.connect(proxyServerAddress, proxyServerPort))
    {
      client.println("POST /" + mainHostPath + " HTTP/1.1");
      client.println("Host: " + host);
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
}

void ShowReaderDetails()
{
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (unknown)"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF))
  {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
  }
}