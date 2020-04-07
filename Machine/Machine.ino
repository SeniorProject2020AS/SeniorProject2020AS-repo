#include "LcdMenu.h"
#include <Wire.h> //Serial communication
#include <Adafruit_MotorShield.h> //Drive motors using Adafruit Motor Driver Board 
#include <Adafruit_RGBLCDShield.h> //Display text and change colors on LCD Display

////////////////////////////////////////////////
// Variables that affect the geometry of pearls:
////////////////////////////////////////////////

int highTime = 90; //Sets the frequency of steps of the stepper motor pump (30 being the fastest)
int lowTime = 20; //Sets the frequency of steps of the stepper motor pump (10 being the fastest)
int NaAlgPumpSpeed = 255; //Speed of Sodium Alginate pump
int BlenderStomachSpeed = 30; //Speed of Blender Stomach
int CalciumBathPumpSpeed = 255; //Speed of Calcium Bath pump
int CalciumBathExtractionPumpSpeed = 255; //Speed of Calcium Bath Extraction pump
int NaAlgPumpTime = 35; //Running time of Sodium Alginate pump
int BlenderStomachTime = 265; //Running time of Blender Stomach
int CalciumBathPumpTime = 267; //Running time of Calcium Bath pump
int CalciumBathExtractionPumpTime = 30; //Running time of Calcium Bath Extraction pump
int SpoutPumpStartTime = 265; //Time at which Spout pump starts
int SpoutPumpStopTime = 325; //Time at which Spout pump stops
int MachineStopTime = 325; //Time at which the machine stops
int NaAlgPumpStartTime = 0; //Time at which Sodium Alginate pump starts
int BlenderStomachStartTime = 5; //Time at which Blender Stomach starts
int CalciumBathPumpStartTime = 0; //Time at which Calcium Bath pump starts
int CalciumBathExtractionPumpStartTime = 0; //Time at which Calcium Bath Extraction pump starts

////////////////////////////////////////////////
// Do not change variables below this line!
////////////////////////////////////////////////

int SteppingMotorDirectionPin = 8; //Sets the direction (clockwise/counterclockwise) of the stepping motor
int SteppingMotorStepFrequencyPin = 9; //Used to set the frequency of steps for the stepping motor, important when regulating the size/shape of the pearls
int stepper = 0;
int currentpercentage = 0;

// Abort on Stepper 0 disable, 1 active (with the jump issue on stepper each second)
int AbortOnStepper = 0;

// Progress apprear 1 by 1 on Stepper stage OneByOneStepper=1 (with the jump issue on stepper each change of progress)
// Progress apprear 10 by 10 on Stepper stage OneByOneStepper=0
int OneByOneStepper = 1;

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

LcdMenu menu; //Object reference for the LCD manipulations

//int currentstatus;
int pressed = 0;
int startrun = 0;

void setUpPump(Adafruit_DCMotor *pump, int the_speed);
void set_vars_for_pumps();

void setup() {
  Serial.begin(9600); //Initialize serial data communication, set to baud rate of 9600
  AdafruitMotorDriverBoard.begin(); //Begin communication with Adafruit Motor Driver Board

  SetupPump(NaAlgPump, NaAlgPumpSpeed); //Setting up NaAlgPump, speed = 255
  SetupPump(BlenderStomach, BlenderStomachSpeed); //Setting up BlenderStomach, speed = 30
  SetupPump(CalciumBathPump, CalciumBathPumpSpeed); //Setting up CalciumBathPump, speed = 255
  SetupPump(CalciumBathExtractionPump, CalciumBathExtractionPumpSpeed); //Setting up CalciumBathExtractionPump, speed = 255

  pinMode(SteppingMotorDirectionPin, OUTPUT);
  pinMode(SteppingMotorStepFrequencyPin, OUTPUT);

  menu.setupLcd();
  menu.set_to_idle();
}

void loop()
{
  uint8_t buttons; //Buttons read from LCD

  /////////////////////////////////////////
  // Running
  /////////////////////////////////////////

  if (menu.isRunning(menu.getStatus())) {
    
    currentTime = millis();
    calculatedSecond = (unsigned long)(currentTime) / 1000 - rollover;

    int percentage = (int)( ((100 * calculatedSecond) / (265 + 60)));
    int updateprogress;
    if (OneByOneStepper == 1 || (OneByOneStepper == 0 && (calculatedSecond < 265 || calculatedSecond > (265 + 60)))) {
      // progress 1 by 1
      updateprogress = (currentpercentage != percentage);
    } else {
      // progress 10 by 10
      updateprogress = ((currentpercentage != percentage) && (percentage == 0 || percentage == 10 || percentage == 20 || percentage == 30 || percentage == 40 || percentage == 50 || percentage == 60 || percentage == 70 || percentage == 80 || percentage == 90 || percentage == 100));
    }
    if (updateprogress) {
      currentpercentage = percentage;
      menu.print_progress(currentpercentage);
    }
    if (calculatedSecond != currentSecond) {

      currentSecond = calculatedSecond;

      if (AbortOnStepper == 1 || (AbortOnStepper == 0 && (calculatedSecond < 265 || calculatedSecond > (265 + 60)))) {

        buttons = menu.getButtonStatus();

        if (buttons == 0  && pressed == 1) {
          if (startrun == 1) {
            //button released (to start the process
            pressed = 0;
            startrun = 0;
          }
          else if (startrun == 0) {
            //button released reset counter
            pressed = 0;
            startrun = 0;
            secondstoabort = 0;
            stepper = 0;
          }
        }
        if (buttons && pressed == 0 && secondstoabort == 0 && startrun == 0) {
          pressed = 1;
          secondstoabort = currentSecond;
        }
        else if (buttons && pressed == 1 && secondstoabort != 0 && currentSecond - secondstoabort > 3) {
          NaAlgPump->run(RELEASE);  //Stops motor
          BlenderStomach->run(RELEASE);  //Stops motor
          CalciumBathPump->run(RELEASE);  //Stops motor
          CalciumBathExtractionPump->run(RELEASE);  //Stops motor
          menu.set_to_idle();
          secondstoabort = 0;
        }
      }

      if (menu.isRunning(menu.getStatus())) {
        if (currentSecond == NaAlgPumpStartTime) {
          NaAlgPump->run(FORWARD);  //Runs motor forward
        } else if (currentSecond == (NaAlgPumpTime)) {
          NaAlgPump->run(RELEASE);  //Stops motor
        }

        if (currentSecond == BlenderStomachStartTime) {
          BlenderStomach->run(FORWARD);  //Runs motor forward
        } else if (currentSecond == (BlenderStomachTime)) {
          BlenderStomach->run(RELEASE);  //Stops motor
        }

        if (currentSecond == CalciumBathPumpStartTime) {
          CalciumBathPump->run(FORWARD);  //Runs motor forward
        } else if (currentSecond == (CalciumBathPumpTime)) {
          CalciumBathPump->run(RELEASE);  //Stops motor
        }

        if (currentSecond == CalciumBathExtractionPumpStartTime) {
          CalciumBathExtractionPump->run(FORWARD);  //Runs motor forward
        } else if (currentSecond == (CalciumBathExtractionPumpTime)) {
          CalciumBathExtractionPump->run(RELEASE);  //Stops motor
        }
      }
    }
    if (calculatedSecond >= SpoutPumpStartTime && calculatedSecond <= SpoutPumpStopTime)
    {
      if (stepper == 0) {
        stepper = 1;
      }
      RunSpoutPump();
    }

    if (currentSecond >= MachineStopTime) {
      delay(1000);
      menu.set_to_idle();
      secondstoabort = 0;
    }

    /////////////////////////////////////////
    // Prime Pumps
    /////////////////////////////////////////
    
  } else if (menu.isNaAlgPumpRunning(menu.getStatus()) || menu.isCaInputPumpRunning(menu.getStatus())
             || menu.isCaExitPumpRunning(menu.getStatus()) || menu.isSpoutPumpRunning(menu.getStatus())) {

    currentTime = millis();
    calculatedSecond = (unsigned long)(currentTime) / 1000 - rollover;
    if (menu.isNaAlgPumpRunning(menu.getStatus()) || menu.isCaInputPumpRunning(menu.getStatus()) || menu.isCaExitPumpRunning(menu.getStatus()) ||
        (menu.isSpoutPumpRunning(menu.getStatus()) && calculatedSecond != currentSecond)) {
      currentSecond = calculatedSecond;
      //buttons = lcd.readButtons();
      buttons = menu.getButtonStatus();

      //button pressed to start pump
      if (buttons == 0 && pressed == 1 && startrun == 1) {
        pressed = 0;
        startrun = 0;
      }

      //button pressed to stop pump
      if (buttons && pressed == 0 && startrun == 0) {
        if (menu.isNaAlgPumpRunning(menu.getStatus())) {
          NaAlgPump->run(RELEASE);  //Stops motor
        }
        else if (menu.isCaInputPumpRunning(menu.getStatus())) {
          CalciumBathPump->run(RELEASE);  //Stops motor
        }
        else if (menu.isCaExitPumpRunning(menu.getStatus())) {
          CalciumBathExtractionPump->run(RELEASE);  //Stops motor
        }
        menu.set_to_idle();
      }
    }
    if (menu.isSpoutPumpRunning(menu.getStatus())) {
      RunSpoutPump();
    }

    /////////////////////////////////////////
    // Menu navigation
    /////////////////////////////////////////

  } else {
    buttons = menu.getButtonStatus();
    //    buttons = lcd.readButtons();

    if (buttons && pressed == 0) {
      menu.setMenu();
      pressed = 1;
      //currentstatus = menu.getStatus();
      if (menu.isRunning(menu.getStatus())) {
        stepper = 0;
        set_vars_for_pumps();
        currentpercentage = 0;
      }
      else if (menu.isNaAlgPumpRunning(menu.getStatus())) {
        set_vars_for_pumps();
        NaAlgPump->run(FORWARD);  //Runs motor forward
      }
      else if (menu.isCaInputPumpRunning(menu.getStatus())) {
        set_vars_for_pumps();
        CalciumBathPump->run(FORWARD);
      }
      else if (menu.isCaExitPumpRunning(menu.getStatus())) {
        set_vars_for_pumps();
        CalciumBathExtractionPump->run(FORWARD);
      }
      else if (menu.isSpoutPumpRunning(menu.getStatus())) {
        set_vars_for_pumps();
      }
    }
    else if (buttons == 0  && pressed == 1) {
      // button released
      pressed = 0;
    }
  }
}

/*Set up a pump*/
void SetupPump(Adafruit_DCMotor *pump, int the_speed) {
  pump->setSpeed(the_speed);
  pump->run(FORWARD);//sets direction of motor
  pump->run(RELEASE);//stops motor
}

/*set proper variables when running pumps 1-4*/
void set_vars_for_pumps() {
  startrun = 1;
  currentTime = millis();
  rollover = (unsigned long)(currentTime) / 1000;
}

/*run the stepping motor / spout pump*/
void RunSpoutPump() {
  digitalWrite(SteppingMotorDirectionPin, LOW);
  digitalWrite(SteppingMotorStepFrequencyPin, HIGH);
  delayMicroseconds(highTime); // This sets the speed of the stepper motor pump (30 being the fastest)
  digitalWrite(SteppingMotorStepFrequencyPin, LOW);
  delayMicroseconds(lowTime); // This sets the speed of the stepper motor pump (10 being the fastest)
}
