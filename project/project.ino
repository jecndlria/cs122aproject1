#include <Adafruit_ST7735.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_ST77xx.h>
#include <EduIntro.h>


#define BUTTON_PIN 2
  bool humidifierOn = false;
#define WATER_PIN A1
  unsigned int waterLevel = 0;
#define numTasks 4

typedef struct task {
  int state;
  unsigned long period;
  unsigned long elapsedTime;
  int (*TickFct)(int);
} task;

task tasks[numTasks];

/*
*
* MOVE TO DHT.H
*
*/
#define DHT_PIN A2

int celsius;
float farenheit;
int humidity; 

DHT11 dht11(DHT_PIN);
/*
*
* MOVE TO DHT.H
*
*/

/* 
*
* MOVE TO LCD.H
*
*/

// LCD Screen Pins
#define TFT_CS 10
#define TFT_RESET 8
#define TFT_A0 9
#define TFT_SDA 11
#define TFT_SCK 13

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_A0, TFT_SDA, TFT_SCK, TFT_RESET);

void initializeScreen()
{
  tft.initR(INITR_144GREENTAB);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);

  tft.fillRect(0, 0, 128, 16, ST7735_BLACK);
  tft.setCursor(0, 0);
  tft.println("TEMPERATURE: ");

  tft.fillRect(0, 16, 128, 32, ST7735_BLACK);
  tft.setCursor(0, 16);
  tft.println("HUMIDITY: ");

  tft.fillRect(0, 32, 128, 48, ST7735_BLACK);
  tft.setCursor(0, 32);
  tft.println("WATER LEVEL: ");

  tft.fillRect(0, 48, 128, 64, ST7735_BLACK);
  tft.setCursor(0, 48);
  tft.println("STATUS: ");
}


void lcdUpdateTemp()
{
  tft.setTextColor(ST7735_WHITE);
  tft.fillRect(69, 0, 128-69, 15, ST7735_BLACK); // change to fill black
  tft.setCursor(70, 0);
  tft.print(random() % 100);

}

void lcdUpdateHumidity()
{
  tft.setTextColor(ST7735_WHITE);
  tft.fillRect(51, 16, 128-51, 15, ST7735_BLACK); // change to fill black
  tft.setCursor(52, 16);
  tft.print(random() % 100);
}

void lcdUpdateWaterLevel()
{
  tft.setTextColor(ST7735_WHITE);
  tft.fillRect(69, 32, 128-69, 15, ST7735_BLACK); // change to fill black
  tft.setCursor(70, 32);
  tft.print(random() % 100);
}

void lcdUpdateHumidifier()
{
  tft.setTextColor(ST7735_WHITE);
  tft.fillRect(39, 48, 128-39, 15, ST7735_BLACK); // change to fill black
  tft.setCursor(40, 48);
  tft.print(random() % 100);
}
/* 
*
* MOVE TO LCD.H
*
*/


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);
  initializeScreen();

  //lcdUpdateReadings();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (digitalRead(BUTTON_PIN)) Serial.print("PRESSED\n");
  lcdUpdateTemp();
  lcdUpdateHumidity();
  lcdUpdateWaterLevel();
  lcdUpdateHumidifier();
  delay(50);
}
