#include <Wire.h> //Serial communication
#include <Adafruit_MotorShield.h> //Drive motors using Adafruit Motor Driver Board 
#include <Adafruit_RGBLCDShield.h> //Display text and change colors on LCD Display

int SteppingMotorDirectionPin = 8; //Sets the direction (clockwise/counterclockwise) of the stepping motor
int SteppingMotorStepFrequencyPin = 9; //Used to set the frequency of steps for the stepping motor, important when regulating the size/shape of the pearls
int stepper = 0;
int currentpercentage = 0;

// Abort on Stepper 0 disable, 1 active (with the jump issue on stepper each second) 
int AbortOnStepper = 0;
 
// Progress apprear 1 by 1 on Stepper stage OneByOneStepper=1 (with the jump issue on stepper each change of progress)
// Progress apprear 10 by 10 on Stepper stage OneByOneStepper=0 
int OneByOneStepper = 0;

unsigned long currentTime = millis();
unsigned long currentSecond = -1;
unsigned long calculatedSecond;
unsigned long rollover;
unsigned long secondstoabort = 0;

Adafruit_MotorShield AdafruitMotorDriverBoard = Adafruit_MotorShield();  // Creates Adafruit Motor Driver Board object, used to configure and drive the pumps and blender

Adafruit_DCMotor *NaAlgPump = AdafruitMotorDriverBoard.getMotor(3); //Creates an Adafruit DC Motor object for the Na-Alg pump, assigned to M1 on the Adafruit Motor Driver Board
Adafruit_DCMotor *BlenderStomach = AdafruitMotorDriverBoard.getMotor(2); //Creates an Adafruit DC Motor object for the Blender or Stomach, assigned to M2 on the Adafruit Motor Driver Board
Adafruit_DCMotor *CalciumBathPump = AdafruitMotorDriverBoard.getMotor(1); //Creates an Adafruit DC Motor object for the Calcium Bath pump, assigned to M3 on the Adafruit Motor Driver Board
Adafruit_DCMotor *CalciumBathExtractionPump = AdafruitMotorDriverBoard.getMotor(4); //Creates an Adafruit DC Motor object for the Calcium Bath Extraction pump, assigned to M4 on the Adafruit Motor Driver Board

Adafruit_RGBLCDShield LCD = Adafruit_RGBLCDShield(); //Creates an LCD object, used to display text and change colors on the LCD display

// Creates constants used for controlling the backlight color of the LCD display
#define RED 0x1
//unused
//#define YELLOW 0x3
//#define GREEN 0x2
//#define BLUE 0x4
//#define VIOLET 0x5
#define TEAL 0x6


#define WHITE 0x7

// Creates constants used to keep track of the current state of the machine, identifies the current menu option/selection from the user
#define READY 0
#define START 1
#define SYSTEMSETTINGS 2
#define RUNNING 11
#define PRIMEPUMPS 21
#define CLEANSE 22
#define PUMP1 211
#define PUMP2 212
#define PUMP3 213
#define PUMP4 214
#define RUNNING1 2111
#define RUNNING2 2121
#define RUNNING3 2131
#define RUNNING4 2141

int currentstatus = READY;
int pressed = 0;
int startrun = 0;

void setup() {
  Serial.begin(9600); //Initialize serial data communication, set to baud rate of 9600
  AdafruitMotorDriverBoard.begin(); //Begin communication with Adafruit Motor Driver Board
  
  SetupPump(NaAlgPump, 255); //Setting up NaAlgPump, speed = 255
  SetupPump(BlenderStomach, 30); //Setting up BlenderStomach, speed = 255
  SetupPump(CalciumBathPump, 255); //Setting up CalciumBathPump, speed = 255
  SetupPump(CalciumBathExtractionPump, 255); //Setting up CalciumBathExtractionPump, speed = 255

  pinMode(SteppingMotorDirectionPin, OUTPUT);
  pinMode(SteppingMotorStepFrequencyPin, OUTPUT);
  
  LCD.begin(16, 2); // Setting up the LCD's number of columns (16) and rows (2)
  LCD.print("Ready");
  LCD.setBacklight(WHITE);
}

void loop() 
{

  uint8_t buttons; //Buttons read from LCD

  /////////////////////////////////////////
  // Running
  /////////////////////////////////////////
  if(currentstatus==RUNNING) {

    currentTime = millis();
    calculatedSecond = (unsigned long)(currentTime)/1000-rollover;
  
  int percentage= (int)( ((100 * calculatedSecond) / (265+60)));
  int updateprogress;
  if (OneByOneStepper==1 || (OneByOneStepper==0 && (calculatedSecond <265 || calculatedSecond > (265+60)))) {
    // progress 1 by 1
    updateprogress=(currentpercentage!=percentage);
  } else {
    // progress 10 by 10
    updateprogress=((currentpercentage!=percentage) && (percentage==0 || percentage==10 || percentage==20 || percentage==30 || percentage==40 || percentage==50 || percentage==60 || percentage==70 || percentage==80 || percentage==90 || percentage==100));
  }
  if(updateprogress) {
  {
      currentpercentage=percentage;
            LCD.setCursor(9,0);
        LCD.print(currentpercentage);
        if(currentpercentage<10) {
         LCD.setCursor(10,0);
        } else if(currentpercentage<100) {
          LCD.setCursor(11,0);
        }
        LCD.print("%");
    }
  }
    if(calculatedSecond!=currentSecond) {  

      currentSecond=calculatedSecond;
      Serial.print("Second:" );
      Serial.println(currentSecond);

    if (AbortOnStepper==1 || (AbortOnStepper==0 && (calculatedSecond <265 || calculatedSecond > (265+60)))) 
    {
    buttons = LCD.readButtons();

    if (buttons==0  && pressed==1 && startrun==1) {
        // button released (to start the process
        pressed=0;
        startrun=0;
      }

      if (buttons==0  && pressed==1 && startrun==0) {
        // button released resert counter
        pressed=0;
        startrun=0;
        secondstoabort=0;
    stepper=0;
      }


      if(buttons && pressed==0 && secondstoabort==0 && startrun==0) {
        pressed=1;
        secondstoabort=currentSecond;
      } else if (buttons && pressed==1 && secondstoabort!=0 && currentSecond-secondstoabort>3) {
            NaAlgPump->run(RELEASE);  //Stops motor 
            Serial.println("ABORT Motor 1 off");
            BlenderStomach->run(RELEASE);  //Stops motor 
            Serial.println("ABORT Motor 2 off");
            CalciumBathPump->run(RELEASE);  //Stops motor 
            Serial.println("ABORT Motor 3 off");
            CalciumBathExtractionPump->run(RELEASE);  //Stops motor 
            Serial.println("ABORT Motor 4 off");
            LCD.clear();
            LCD.setCursor(0,0);
            LCD.print("Ready");
            LCD.setBacklight(WHITE);
            currentstatus=READY;
            Serial.println("READY");
            secondstoabort=0;
      }
    }

        if(currentstatus==RUNNING) {
          if (currentSecond==0) {
            NaAlgPump->run(FORWARD);  //Runs motor forward
            Serial.println("Na-Alg pump started");
          } else if(currentSecond==(0+35)) {
            NaAlgPump->run(RELEASE);  //Stops motor 
            Serial.println("Motor 1 off");
          }
    
          if (currentSecond==5) {
              BlenderStomach->run(FORWARD);  //Runs motor forward
              Serial.println("Blender started");
          } else if(currentSecond==(5+260)) {
              BlenderStomach->run(RELEASE);  //Stops motor
              Serial.println("Motor 2 off");
          }
  
          if (currentSecond==0) {
              CalciumBathPump->run(FORWARD);  //Runs motor forward
              Serial.println("Calcium pump started");
          } else if(currentSecond==(0+267)) {
              CalciumBathPump->run(RELEASE);  //Stops motor
              Serial.println("Motor 3 off");
          }
  
          if (currentSecond==0) {
              CalciumBathExtractionPump->run(FORWARD);  //Runs motor forward
              Serial.println("Ca exit pump started");
          } else if(currentSecond==(0+30)) {
              CalciumBathExtractionPump->run(RELEASE);  //Stops motor
              Serial.println("Motor 4 off");
          } 
      
      }
    }
  
    if (calculatedSecond >=265 && calculatedSecond <= (265+60)) 
    { 
    if(stepper==0) {
      stepper=1;
        Serial.println("starting stepper motor pump");
    }
      digitalWrite(SteppingMotorStepFrequencyPin, HIGH);
      delayMicroseconds(30); // This sets the speed of the stepper motor pump (30 being the fastest)
      digitalWrite(SteppingMotorStepFrequencyPin, LOW);
      delayMicroseconds(30); // This sets the speed of the stepper motor pump (10 being the fastest)
    }

    if(currentSecond>=(265+60)) {
      delay(1000);
      Serial.println("END OF RUNNING");
      //Serial.println("exit");
      //exit(0);                //Exit loop
      LCD.clear();
      LCD.setCursor(0,0);
      LCD.print("Ready");
      LCD.setBacklight(WHITE);
      currentstatus=READY;
      secondstoabort=0;
      Serial.println("READY");
    }

    

    /////////////////////////////////////////
    // Prime Pumps
    /////////////////////////////////////////
    } else if(currentstatus==RUNNING1 || currentstatus==RUNNING2 || currentstatus==RUNNING3 || currentstatus==RUNNING4) {
    
    currentTime = millis();
    calculatedSecond = (unsigned long)(currentTime)/1000-rollover;
    if(currentstatus==RUNNING1 || currentstatus==RUNNING2 || currentstatus==RUNNING3 || (currentstatus==RUNNING4 && calculatedSecond!=currentSecond)) {
      currentSecond=calculatedSecond;
      buttons = LCD.readButtons();

          if (buttons==0 && pressed==1 && startrun==1) {
            // button released (to start the process
            pressed=0;
            startrun=0;
          }
    
          if(buttons && pressed==0 && startrun==0) {
            if (currentstatus==RUNNING1) {
              NaAlgPump->run(RELEASE);  //Stops motor 
              Serial.println("Manual Motor 1 off");
            } else if (currentstatus==RUNNING2) {
              CalciumBathPump->run(RELEASE);  //Stops motor
              Serial.println("Manual Motor 3 off");
           } else if (currentstatus==RUNNING3) {
              CalciumBathExtractionPump->run(RELEASE);  //Stops motor
              Serial.println("Manual Motor 4 off");
           }
           LCD.clear();
           LCD.setCursor(0,0);
           LCD.print("Ready");
           LCD.setBacklight(WHITE);
           currentstatus=READY;
           Serial.println("READY");
      }
      } 
    if(currentstatus==RUNNING4){
        digitalWrite(SteppingMotorStepFrequencyPin, HIGH);
        delayMicroseconds(30); // This sets the speed of the stepper motor pump (30 being the fastest)
        digitalWrite(SteppingMotorStepFrequencyPin, LOW);
        delayMicroseconds(30); // This sets the speed of the stepper motor pump (10 being the fastest)
    }
    
    
    /////////////////////////////////////////
    // Menu navigation
    /////////////////////////////////////////
    } else {
    buttons = LCD.readButtons();

    if (buttons && pressed==0) {
      if((currentstatus==START || currentstatus==SYSTEMSETTINGS) && buttons & BUTTON_LEFT) {
        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print("Ready");
        LCD.setBacklight(WHITE);
        currentstatus=READY;
        Serial.println("READY");
        pressed=1;
      } else
      if((currentstatus==READY && (buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) || (currentstatus==SYSTEMSETTINGS && buttons & BUTTON_UP)) {
        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print(">>Start");
        LCD.setCursor(0,1);
        LCD.print("System Settings");
        LCD.setBacklight(WHITE);
        currentstatus=START;
        Serial.println("START");
        pressed=1;
      } else
      if (currentstatus==START && (buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) {
        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print("Progress 0%");
        LCD.setBacklight(TEAL);
        currentstatus=RUNNING;
    stepper=0;
        startrun=1;
        Serial.println("RUNNING");
        pressed=1;
        currentTime = millis();
    currentpercentage = 0;
        rollover = (unsigned long)(currentTime)/1000;
      } else
      if ((currentstatus==START && buttons & BUTTON_DOWN) || ((currentstatus==PRIMEPUMPS || currentstatus==CLEANSE) && buttons & BUTTON_LEFT)) {
        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print("Start");
        LCD.setCursor(0,1);
        LCD.print(">>System Settings");
        LCD.setBacklight(WHITE);
        currentstatus=SYSTEMSETTINGS;
        Serial.println("SYSTEMSETTINGS");
        pressed=1;
      } else
      if ((currentstatus==SYSTEMSETTINGS && (buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) || (currentstatus==CLEANSE && buttons & BUTTON_UP)  || (currentstatus>=PUMP1 && buttons & BUTTON_LEFT)) {
        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print(">>Prime Pumps");
        LCD.setCursor(0,1);
        LCD.print("Cleanse");
        LCD.setBacklight(WHITE);
        currentstatus=PRIMEPUMPS;
        Serial.println("PRIMEPUMPS");
        pressed=1;
      } else
      if (currentstatus==PRIMEPUMPS && buttons & BUTTON_DOWN) {
        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print("Prime Pumps");
        LCD.setCursor(0,1);
        LCD.print(">>Cleanse");
        LCD.setBacklight(WHITE);
        currentstatus=CLEANSE;
        Serial.println("CLEANSE");
        pressed=1;
      } else
      if ((currentstatus==PRIMEPUMPS && (buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) || (currentstatus==PUMP2 && buttons & BUTTON_UP)) {
        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print(">>Prime Na-alg Pump");
        LCD.setCursor(0,1);
        LCD.print("Prime Ca input Pump");
        LCD.setBacklight(WHITE);
        currentstatus=PUMP1;
        Serial.println("PUMP1");
        pressed=1;
      } else
      if ((currentstatus==PUMP1 && buttons & BUTTON_DOWN) || (currentstatus==PUMP3 && buttons & BUTTON_UP)) {
        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print(">>Prime Ca input Pump");
        LCD.setCursor(0,1);
        LCD.print("Prime Ca exit Pump");
        LCD.setBacklight(WHITE);
        currentstatus=PUMP2;
        Serial.println("PUMP2");
        pressed=1;
      } else
      if ((currentstatus==PUMP2 && buttons & BUTTON_DOWN) || (currentstatus==PUMP4 && buttons & BUTTON_UP)) {
        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print(">>Prime Ca exit Pump");
        LCD.setCursor(0,1);
        LCD.print("Prime spout Pump");
        LCD.setBacklight(WHITE);
        currentstatus=PUMP3;
        Serial.println("PUMP3");
        pressed=1;
      } else
      if (currentstatus==PUMP3 && buttons & BUTTON_DOWN) {
        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print("Prime Ca exit Pump");
        LCD.setCursor(0,1);
        LCD.print(">>Prime spout Pump");
        LCD.setBacklight(WHITE);
        currentstatus=PUMP4;
        Serial.println("PUMP4");
        pressed=1;
      } else
      if (currentstatus==PUMP1 &&(buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) {
        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print("Running Prime Na a");
        LCD.setBacklight(RED);
        currentstatus=RUNNING1;
        startrun=1;
        Serial.println("RUNNING1");
        pressed=1;
        NaAlgPump->run(FORWARD);  //Runs motor forward
        Serial.println("Manual Prime Na a started");
        currentTime = millis();
        rollover = (unsigned long)(currentTime)/1000;
      } else
      if (currentstatus==PUMP2 &&(buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) {
        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print("Running Prime ca input pump");
        LCD.setBacklight(RED);
        currentstatus=RUNNING2;
        startrun=1;
        Serial.println("RUNNING2");
        pressed=1;
        CalciumBathPump->run(FORWARD);  //Runs motor forward
        Serial.println("Manual Prime ca input started");
        currentTime = millis();
        rollover = (unsigned long)(currentTime)/1000;
      } else
      if (currentstatus==PUMP3 &&(buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) {
        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print("Running ca exit pump");
        LCD.setBacklight(RED);
        currentstatus=RUNNING3;
        startrun=1;
        Serial.println("RUNNING3");
        pressed=1;
        CalciumBathExtractionPump->run(FORWARD);  //Runs motor forward
        Serial.println("Manual ca exit pump started");
        currentTime = millis();
        rollover = (unsigned long)(currentTime)/1000;
      } else
      if (currentstatus==PUMP4 &&(buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) {
        LCD.clear();
        LCD.setCursor(0,0);
        LCD.print("Running spout pump");
        LCD.setBacklight(RED);
        currentstatus=RUNNING4;
        startrun=1;
        Serial.println("RUNNING4");
        pressed=1;
        Serial.println("Manual spout pump started");
        currentTime = millis();
        rollover = (unsigned long)(currentTime)/1000;
      }
    } else
    if (buttons==0  && pressed==1) {
      // button released
      pressed=0;
    }
  }
}

void SetupPump(Adafruit_DCMotor *pump, int speed) {
  pump->setSpeed(speed); // Sets speed of motor
  pump->run(FORWARD); // Sets direction of motor
  pump->run(RELEASE); // Stops motor
}
