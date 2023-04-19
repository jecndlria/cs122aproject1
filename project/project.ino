#include <Adafruit_ST7735.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_ST77xx.h>
#include <EduIntro.h>


#define BUTTON_PIN 2
  bool humidifierOn = false;
#define WATER_PIN A1
  unsigned int waterLevel = 0;
#define numTasks 2

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
#define DHT_PIN A0

float fahrenheit;
float lastSeenFahrenheit;
int humidity; 
int lastSeenHumidity;

DHT11 dht11(DHT_PIN);
/*
*
* MOVE TO DHT.H
*
*/

enum DHT_STATES {DHT_INIT};
int dhtTick(int state)
{
  switch(state)
  {
    case DHT_INIT:
      dht11.update();
      lastSeenFahrenheit = fahrenheit;
      lastSeenHumidity = humidity;
      fahrenheit = dht11.readFahrenheit();
      humidity = dht11.readHumidity();
      if (lastSeenFahrenheit != fahrenheit) lcdUpdateTemp();
      if (lastSeenHumidity != humidity) lcdUpdateHumidity();

      break;      
  }
  return state;  
}

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
  tft.println("TEMPERATURE(F): ");

  tft.fillRect(0, 16, 128, 32, ST7735_BLACK);
  tft.setCursor(0, 16);
  tft.println("HUMIDITY: ");

  tft.fillRect(0, 32, 128, 48, ST7735_BLACK);
  tft.setCursor(0, 32);
  tft.println("WATER LEVEL: ");

  tft.fillRect(0, 48, 128, 64, ST7735_BLACK);
  tft.setCursor(0, 48);
  tft.println("STATUS: ");
  tft.setTextColor(ST7735_RED);
  tft.setCursor(40, 48);
  tft.println("OFF");
}


void lcdUpdateTemp()
{
  tft.setTextColor(ST7735_WHITE);
  tft.fillRect(88, 0, 128-88, 15, ST7735_BLACK); // change to fill black
  tft.setCursor(89, 0);
  tft.print(fahrenheit);

}

void lcdUpdateHumidity()
{
  tft.setTextColor(ST7735_WHITE);
  tft.fillRect(51, 16, 128-51, 15, ST7735_BLACK); // change to fill black
  tft.setCursor(52, 16);
  tft.print(humidity);
}

void lcdUpdateWaterLevel()
{
  tft.setTextColor(ST7735_WHITE);
  tft.fillRect(69, 32, 128-69, 15, ST7735_BLACK); // change to fill black
  tft.setCursor(70, 32);
  //tft.print(random() % 100);
}

void lcdUpdateHumidifier()
{
  tft.setTextColor(ST7735_WHITE);
  tft.fillRect(39, 48, 128-39, 15, ST7735_BLACK); // change to fill black
  tft.setCursor(40, 48);
  if (humidifierOn)
  {
    tft.setTextColor(ST7735_GREEN);
    tft.println("ON");
  }
  else
  {
    tft.setTextColor(ST7735_RED);
    tft.println("OFF");
  }
}

/* 
*
* MOVE TO LCD.H
*
*/

/*
*
* MOVE TO WATER.H
*
*/



/*
*
* MOVE TO WATER.H
*
*/

/*
*
* MOVE TO HUMIDIFIER.H
*
*/

enum HUM_STATES {HUM_INIT, HUM_OFF, HUM_ON};
bool humidifierButtonHeld = false;

int humTick(int state)
{
  switch(state)
  {
    case HUM_INIT:
      break;
    case HUM_OFF:
      humidifierOn = false;          
      break;
    case HUM_ON:
      humidifierOn = true;
      break;
  }
  switch(state)
  {
    case HUM_INIT:
      state = HUM_OFF;
      break;
    case HUM_OFF:
      if (digitalRead(BUTTON_PIN) && !humidifierButtonHeld)
      {
        humidifierButtonHeld = true;
        state = HUM_ON;
        lcdUpdateHumidifier();

      }
      else humidifierButtonHeld = false;
      break;
    case HUM_ON:
      if (digitalRead(BUTTON_PIN) && !humidifierButtonHeld)
      {
        humidifierButtonHeld = true;
        state = HUM_OFF;
        lcdUpdateHumidifier();

      }
      else humidifierButtonHeld = false;

      break;
  }
  return state;
}


/*
*
* MOVE TO HUMIDIFIER.H
*
*/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);
  initializeScreen();
  unsigned char i = 0;
  tasks[i].state = DHT_INIT;
  tasks[i].period = 10;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &dhtTick;
  i++;

  tasks[i].state = HUM_INIT;
  tasks[i].period = 10;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &humTick;
  i++;
}

void loop() {
  // put your main code here, to run repeatedly:
  
  unsigned char i;
  for (i = 0; i < numTasks; ++i)
  {
    if ((millis() - tasks[i].elapsedTime) >= tasks[i].period)
    {
      tasks[i].state = tasks[i].TickFct(tasks[i].state);
      tasks[i].elapsedTime = millis();
    }
  }
  
  
}
