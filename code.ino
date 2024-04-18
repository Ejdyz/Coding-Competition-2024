#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>
#include <Arduino.h>

// created class for each pin
class PinClass {
  public:
    PinClass(int pin, int topValue, int bottomValue);
    int getPin();
    void setTop(int topValue);
    void setBottom(int bottomValue);
    int getTop();
    int getBottom();
  private:
    int pin;
    int topValue;
    int bottomValue;
};
// defined useful functions
PinClass::PinClass(int pin, int topValue, int bottomValue) {
  this->pin = pin;
  this->topValue = topValue;
  this->bottomValue = bottomValue;
}

void PinClass::setTop(int setTopValue) {
  this->topValue = setTopValue;
}

void PinClass::setBottom(int setBottomValue) {
  this->bottomValue = setBottomValue;
}

int PinClass::getPin() {
  return this->pin;
}

int PinClass::getTop() {
  return this->topValue;
}

int PinClass::getBottom() {
  return this->bottomValue;
}

//setting up display
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address 0x27, 16 column and 2 rows

//default value for the top and bottom 
int defaultTop = 1024 * 0.75;
int defaultBottom = 1024 * 0.25;
//created list of pins

PinClass availablePorts[]  = {PinClass(0, defaultTop,defaultBottom), PinClass(1, defaultTop,defaultBottom), PinClass(2, defaultTop,defaultBottom), PinClass(3, defaultTop,defaultBottom), PinClass(4, defaultTop,defaultBottom)};
//index of the selected pin
int selectedPortIndex = 0;

//defining buttons
int changePinButtonPin = 6;
int settingsButtonPin = 5;
int changeSetValueButtonPin = 11;
int changingRefreshRateButtonPin = 12;

//potenciometer pin
int potenciometerPin = A6;

//variables for behavoior (if the user is first setting top value or not, and if the user is setting refresh rate)
bool settingTopValue = true;
bool settingRefreshRate = false;


//defining led's
int dangerLedPin = 10;
int middleLedPin = 9;
int bottomLedPin = 8;
int settingsLedPin = 7;

// if the user is in setting
bool isInSettings = false;

//defining threads
void ButtonTask( void *pvParameters );
void MainTask( void *pvParameters );

//default refresh rate of the display
int refreshRate = 1000; //in ms

void setup() {
  //setup of the serial logging
  Serial.begin(9600);

  //creating task
  xTaskCreate(ButtonTask,"buttonTask" ,  128 ,  NULL ,  1 ,  NULL );
  xTaskCreate(MainTask,"MainTask" ,  128 ,  NULL ,  1 ,  NULL );

  //setting up the potenciometer
  pinMode(potenciometerPin, INPUT);

  //setting up buttons
  pinMode(changePinButtonPin, INPUT);
  pinMode(settingsButtonPin, INPUT);
  pinMode(changeSetValueButtonPin, INPUT);
  pinMode(changingRefreshRateButtonPin, INPUT);

  digitalWrite(changePinButtonPin, HIGH);
  digitalWrite(settingsButtonPin, HIGH);
  digitalWrite(changeSetValueButtonPin, HIGH);
  digitalWrite(changingRefreshRateButtonPin, HIGH);

  //setting up led's
  pinMode(dangerLedPin, OUTPUT);
  pinMode(middleLedPin, OUTPUT);
  pinMode(bottomLedPin, OUTPUT);
  pinMode(settingsLedPin, OUTPUT);
  
  // initialize the lcd
  lcd.init(); 
  lcd.backlight();
  
  //Start tasks
  vTaskStartScheduler();
}

void loop() {
}

//Thread for handling button states
void ButtonTask(void *pvParameters)  {
  while(1)
  {
    //Check if the button is pressed
    if (digitalRead(changePinButtonPin) == LOW)  
    {
      incrementPortIndex();    
      vTaskDelay( 250  / portTICK_PERIOD_MS );           
    }

    //Check if the button is pressed
    if (digitalRead(settingsButtonPin) == LOW)  
    {
      changeSettingState();    
      vTaskDelay( 250  / portTICK_PERIOD_MS );           
    }
    if (digitalRead(changeSetValueButtonPin) == LOW)  
    {
      changeSettableValue();    
      vTaskDelay( 250  / portTICK_PERIOD_MS );           
    }  
    if(digitalRead(changingRefreshRateButtonPin) == LOW){
      settingRefreshRate = true;
    }else{
      settingRefreshRate = false;
    }
    vTaskDelay( 250  / portTICK_PERIOD_MS ); 
  
  }

}

//Thread for handling main functionality
void MainTask(void *pvParameters)  {
  while(1)
  {
    refreshDisplay();
    setSelectedPin();
    if(!isInSettings){
      checkRange();
    }

    if(isInSettings){
      digitalWrite(dangerLedPin, LOW);
      digitalWrite(middleLedPin, LOW);
      digitalWrite(bottomLedPin, LOW);
      int value = analogRead(potenciometerPin);
      if(!settingRefreshRate){
        if(settingTopValue){
          if(value > availablePorts[selectedPortIndex].getBottom()){
            availablePorts[selectedPortIndex].setTop(value);
          }
        }else{
          if(value < availablePorts[selectedPortIndex].getTop()){
            availablePorts[selectedPortIndex].setBottom(value);
          }
        }
      }else{
        setRefreshRate(value);
      }
    }
    vTaskDelay( refreshRate  / portTICK_PERIOD_MS );
  }
}

//function for increasing selected index
void incrementPortIndex(){
  if (selectedPortIndex>=4){
    selectedPortIndex = 0;
  }else{
    selectedPortIndex++;
  }
}

//function for setting if the user is in settings
void changeSettingState(){
  isInSettings =!isInSettings;
  if(isInSettings)
  {
    digitalWrite(settingsLedPin, HIGH);
  }else{
    digitalWrite(settingsLedPin, LOW);
  }
}


//function refreshing and showing up to date values on the display
void refreshDisplay(){
  if(isInSettings){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("A" + getSelectedPinString() + " | ref :" + String(refreshRate));
    lcd.setCursor(0, 1);
    lcd.print("T:" + String(availablePorts[selectedPortIndex].getTop()) + " | B:"  + String(availablePorts[selectedPortIndex].getBottom()));  
    return;
  }
  Serial.println("Pin: A" + getSelectedPinString() + " | " + "Value: " + getSelectedPinValueString());
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PIn: A" + getSelectedPinString() + " |r:" + String(refreshRate));
  lcd.setCursor(3, 1);
  lcd.print("Value: " + getSelectedPinValueString());  
}


//function setting the pin that is now selected
void setSelectedPin(){
  pinMode(availablePorts[selectedPortIndex].getPin(), INPUT);
}


//functions getting useful information about currently set pin
String getSelectedPinValueString(){
  return String(analogRead(availablePorts[selectedPortIndex].getPin()));
}

int getSelectedPinValueInt(){
  return analogRead(availablePorts[selectedPortIndex].getPin());
}

PinClass getSelectedPin(){
  return availablePorts[selectedPortIndex];
}

int getSelectedPinInt(){
  return availablePorts[selectedPortIndex].getPin();
}

String getSelectedPinString(){
  return String(availablePorts[selectedPortIndex].getPin());
}

//function for changing which value is the user setting up
void changeSettableValue(){
  settingTopValue = !settingTopValue;
}

//function that lit up led's based on the value 
void checkRange(){
  if(!isInSettings){
    if(getSelectedPinValueInt() > getSelectedPin().getTop()){
      digitalWrite(dangerLedPin, HIGH);
      digitalWrite(middleLedPin, LOW);
      digitalWrite(bottomLedPin, LOW);
    }else if(getSelectedPinValueInt() < getSelectedPin().getTop() &&  getSelectedPinValueInt() > getSelectedPin().getBottom()){
      digitalWrite(dangerLedPin, LOW);
      digitalWrite(middleLedPin, HIGH);
      digitalWrite(bottomLedPin, LOW);
    }else{
      digitalWrite(dangerLedPin, LOW);
      digitalWrite(middleLedPin, LOW);
      digitalWrite(bottomLedPin, HIGH);
    }
  }else{
    digitalWrite(dangerLedPin, LOW);
    digitalWrite(middleLedPin, LOW);
    digitalWrite(bottomLedPin, LOW);
  }
}

//setting up refresh rate
void setRefreshRate(int value){
  refreshRate = value;
  // int refresLevel = map(value, 0,1023,1,20);
  // Serial.println(refresLevel);
}