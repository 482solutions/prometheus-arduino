#include <SPI.h>
#include <MFRC522.h> 
#include <iarduino_SensorPulse.h>
#include <Arduino.h>
#include <Wire.h>
#include <MicroLCD.h>

#define SS_PIN 10
#define RST_PIN 9
#define ALCO_PIN A0
#define PULSE_PIN A1
int Alcohol = 0;
boolean flag = true;


LCD_SSD1306 lcd;
iarduino_SensorPulse Pulse(PULSE_PIN);
MFRC522 mfrc522(SS_PIN, RST_PIN);

unsigned long uidDec, uidDecTemp;  // для храниения номера метки в десятичном формате



void setup() {
	Serial.begin(115200);
	
	//digitalWrite(3, LOW);
	lcd.begin();
	Pulse.begin();
	Serial.println("Waiting for card...");
	lcd.setCursor(0, 2);
	lcd.setFontSize(FONT_SIZE_MEDIUM);
	lcd.println("Waiting  card");
	SPI.begin();  // Init SPI bus.
	mfrc522.PCD_Init();     // Init MFRC522 card.
	
	while (flag == true)
	{
		Rfid_Run();
	}
}
void Alco_Run()
{
	//Serial.println(analogRead(A0));
		int Alcohol = analogRead(ALCO_PIN);
		Serial.print("Alcohol = ");
		Serial.println(Alcohol);
		if (Alcohol < 100)
		{			
			lcd.setCursor(0, 2);
			lcd.setFontSize(FONT_SIZE_SMALL);
			lcd.print("Alcohol = 0");
			lcd.setFontSize(FONT_SIZE_SMALL);
			lcd.print(Alcohol);
			delay(50);
		}
		else
		{
			lcd.setCursor(0, 2);
			lcd.setFontSize(FONT_SIZE_SMALL);
			lcd.print("Alcohol = ");
			lcd.setFontSize(FONT_SIZE_SMALL);
			lcd.print(Alcohol);
			delay(50);
		}
	}
	
void Pulse_Run()
{
	if (Pulse.check(ISP_VALID) == ISP_CONNECTED) 
	{
		int BMP = Pulse.check(ISP_PULSE);
		Serial.print("BMP = ");
		Serial.print(BMP);
		Serial.println();
		if (BMP < 100)
		{
			lcd.setCursor(0, 4);
			lcd.setFontSize(FONT_SIZE_SMALL);
			lcd.print("BMP = 0");
			lcd.setFontSize(FONT_SIZE_SMALL);
			lcd.print(BMP);
		}
		else
		{
			lcd.setCursor(0, 4);
			lcd.setFontSize(FONT_SIZE_SMALL);
			lcd.print("BMP = ");
			lcd.setFontSize(FONT_SIZE_SMALL);
			lcd.print(BMP);
		}
	}
	else
	{
		
		Serial.println("please put your finger on sensor");
		lcd.setCursor(0, 4);
		lcd.setFontSize(FONT_SIZE_SMALL);
		lcd.print("BMP = 000 ");
	}
}
void Rfid_Run()
{
	if (!mfrc522.PICC_IsNewCardPresent())
	{
		return;
	}
	// Выбор метки
	if (!mfrc522.PICC_ReadCardSerial())
	{
		return;
	}
	uidDec = 0;
	// Выдача серийного номера метки.
	for (byte i = 0; i < mfrc522.uid.size; i++)
	{
		uidDecTemp = mfrc522.uid.uidByte[i];
		uidDec = uidDec * 256 + uidDecTemp;
	}
	lcd.clear();
	//lcd.setCursor(0, 0);
	lcd.setFontSize(FONT_SIZE_SMALL);
	lcd.print("UID  ");
	//lcd.setCursor(0, 20);
	lcd.setFontSize(FONT_SIZE_SMALL);
	lcd.printLong(uidDec);
	Serial.println("Card UID: ");
	Serial.println(uidDec); // Выводим UID метки в консоль.
	if (uidDec == 3765941668) // Сравниваем Uid метки, если он равен заданому то поезд активируется
	{
		flag = false;
	}
}

void loop()
{	
	
	if (flag == false) 
	{
		Alco_Run();
		Pulse_Run();
	}

}