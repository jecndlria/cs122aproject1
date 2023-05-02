// #include <SoftwareSerial.h>
// SoftwareSerial esp8266(3, 4);

#include <IRremote.h>
#include <Adafruit_ESP8266.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_ST77xx.h>
#include <EduIntro.h>

#define BUTTON_PIN 2
  bool humidifierOn = false;
#define WATER_PIN A1
  unsigned int lastSeenWater = 0;
  unsigned int waterLevel = 0;
#define numTasks 3
// #define numTasks 4
#define RELAY_PIN 6
#define IR_PIN 4
  IRrecv irrecv(IR_PIN);
  decode_results results;
  bool switchThresh = false;
  bool switchTimer = false;
  String humidityThresholdString = "";
  int humidityThreshold = 0;
  String humidifierTimerString = "";
  int humidifierTimer = 0;
  // Todo: State Machine for IR
    // Update bools to pass into hum state machine to know when to switch (then immediately set to false)
    // Press button mode, then input 2 numbers
      // Thresh: Hum %
      // Timer: # of minutes (use millis())
  // LCD prints status for timer and threshold mode
    // Print input as it comes in
    // Modify update humidifer status function to take a param to print current mode


typedef struct task {
  int state;
  unsigned long period;
  unsigned long elapsedTime;
  int (*TickFct)(int);
} task;

task tasks[numTasks];

/*
*
* DHT
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
* DHT
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
* LCD
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
  tft.print(waterLevel);
  tft.setCursor(84, 32);
  tft.print("%");
}

void lcdUpdateHumidifier(int mode = 0)
{
  tft.setTextColor(ST7735_WHITE);
  tft.fillRect(39, 48, 128-39, 15, ST7735_BLACK); // change to fill black
  tft.setCursor(40, 48);
  if (mode == 1)
  {
    tft.setTextColor(ST7735_GREEN);
    tft.println("THRESHOLD");
  }
  else if (mode == 2)
  {
    tft.setTextColor(ST7735_GREEN);
    tft.println("TIMER");
  }
  else if (humidifierOn)
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
* LCD
*
*/

/*
*
* Water Sensor
*
*/

enum WATER_STATES {WATER_INIT};

int waterTick(int state)
{
  switch(state)
  {
    case WATER_INIT:

      lastSeenWater = waterLevel;
      waterLevel = map(analogRead(WATER_PIN), 20, 700, 0, 100);
      if (lastSeenWater != waterLevel) lcdUpdateWaterLevel();
  }
  return state;
}

/*
*
* Water Sensor
*
*/

/*
*
* Humidifier
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
      if (digitalRead(BUTTON_PIN) == LOW) humidifierButtonHeld = false;        
      break;
    case HUM_ON:
      humidifierOn = true;
      if (digitalRead(BUTTON_PIN) == LOW) humidifierButtonHeld = false;
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
        digitalWrite(RELAY_PIN, HIGH);
        lcdUpdateHumidifier();

      }
      
      break;
    case HUM_ON:
      if (digitalRead(BUTTON_PIN) && !humidifierButtonHeld)
      {
        humidifierButtonHeld = true;
        state = HUM_OFF;
        digitalWrite(RELAY_PIN, LOW);
        lcdUpdateHumidifier();

      }
      break;
  }
  return state;
}

/*
*
* IR
*
*/

/* Deleted to save memory
long irLookup[10] = 
{0xEE110707,
0xFB040707,
0xFA050707,
0xF9060707,
0xF7080707,
0xF6090707,
0xF50A0707,
0xF30C0707,
0xF20D0707,
0xF10E0707};
*/

// Samsung TV remote

enum IR_STATES {IR_INIT, IR_WAIT, IR_READ_THRESH, IR_READ_TIMER};

#define IR0 0xEE110707
#define IR1 0xFB040707
#define IR2 0xFA050707
#define IR3 0xF9060707
#define IR4 0xF7080707
#define IR5 0xF6090707
#define IR6 0xF50A0707
#define IR7 0xF30C0707
#define IR8 0xF20D0707
#define IR9 0xF10E0707

#define thresholdModeButton 0xEC130707 // PRE-CH
#define timerModeButton 0xDC230707     // -

int irTick(int state)
{
  switch(state)
  {
    case IR_INIT:
      break;
    case IR_WAIT:
      break;
    case IR_READ_THRESH:
      if (irrecv.decode())
      {
        switch(irrecv.decodedIRData.decodedRawData)
        {
          case IR0:
            humidityThresholdString += "0";
            break;          
          case IR1:
            humidityThresholdString += "1";
            break;
          case IR2:
            humidityThresholdString += "2";
            break;
          case IR3:
            humidityThresholdString += "3";
            break;
          case IR4:
            humidityThresholdString += "4";
            break;
          case IR5:
            humidityThresholdString += "5";
            break;
          case IR6:
            humidityThresholdString += "6";
            break;
          case IR7:
            humidityThresholdString += "7";
            break;
          case IR8:
            humidityThresholdString += "8";
            break;
          case IR9:
            humidityThresholdString += "9";
            break;
           
        }
      }
      break;
    case IR_READ_TIMER:
      if (irrecv.decode())
      {
        switch(irrecv.decodedIRData.decodedRawData)
        {
          case IR0:
            humidifierTimerString += "0";
            break;          
          case IR1:
            humidifierTimerString += "1";
            break;
          case IR2:
            humidifierTimerString += "2";
            break;
          case IR3:
            humidifierTimerString += "3";
            break;
          case IR4:
            humidifierTimerString += "4";
            break;
          case IR5:
            humidifierTimerString += "5";
            break;
          case IR6:
            humidifierTimerString += "6";
            break;
          case IR7:
            humidifierTimerString += "7";
            break;
          case IR8:
            humidifierTimerString += "8";
            break;
          case IR9:
            humidifierTimerString += "9";
            break;
        }
      }
      break;
  }

  switch(state)
  {
    case IR_INIT:
      state = IR_WAIT;
      break;
    case IR_WAIT:
      if (irrecv.decode() && irrecv.decodedIRData.decodedRawData == thresholdModeButton) 
      {
        humidityThresholdString = "";
        state = IR_READ_THRESH;
      }
      else if (irrecv.decode() && irrecv.decodedIRData.decodedRawData == timerModeButton) 
      {
        humidifierTimerString = "";
        state = IR_READ_TIMER;
      }
      irrecv.resume();
      break;
    case IR_READ_THRESH:
      if (humidityThresholdString.length() == 2) 
      {
        state = IR_WAIT;
      }
      break;
    case IR_READ_TIMER:
      if (humidifierTimerString.length() == 2) 
      {
        state = IR_WAIT;
      }
      break;
  }
  return state;
}

/*
*
* IR
*
*/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  initializeScreen();
  irrecv.enableIRIn();
  irrecv.blink13(true);

  unsigned char i = 0;
  tasks[i].state = DHT_INIT;
  tasks[i].period = 100;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &dhtTick;
  i++;

  tasks[i].state = HUM_INIT;
  tasks[i].period = 100;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &humTick;
  i++;

  tasks[i].state = WATER_INIT;
  tasks[i].period = 1000;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &waterTick;
  i++;

  /*
  tasks[i].state = IR_INIT;
  tasks[i].period = 500;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &irTick;  
  */
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
