#include "LcdMenu.h"
#include <Wire.h> //Serial communication
#include <Adafruit_MotorShield.h> //Drive motors using Adafruit Motor Driver Board 
#include "utility/Adafruit_MS_PWMServoDriver.h"


int SteppingMotorDirectionPin = 8; //Sets the direction (clockwise/counterclockwise) of the stepping motor

int SteppingMotorStepFrequencyPin = 9; //Used to set the frequency of steps for the stepping motor, important when regulating the size/shape of the pearls
int stepper = 0;
int currentpercentage= 0;

// Abort on Stepper 0 disable, 1 active (with the jump issue on stepper each second) 
int AbortOnStepper=0;
 
// Progress apprear 1 by 1 on Stepper stage OneByOneStepper=1 (with the jump issue on stepper each change of progress)
// Progress apprear 10 by 10 on Stepper stage OneByOneStepper=0 
int OneByOneStepper=0;

unsigned long currentTime = millis();
unsigned long currentSecond = -1;
unsigned long calculatedSecond;
unsigned long rollover;
unsigned long secondstoabort = 0;

Adafruit_MotorShield AdafruitMotorDriverBoard = Adafruit_MotorShield();  // Creates Adafruit Motor Driver Board object, used to configure and drive the pumps and blender

Adafruit_DCMotor *NaAlgPump = AdafruitMotorDriverBoard.getMotor(1); //Creates an Adafruit DC Motor object for the Na-Alg pump, assigned to M1 on the Adafruit Motor Driver Board
Adafruit_DCMotor *BlenderStomach = AdafruitMotorDriverBoard.getMotor(2); //Creates an Adafruit DC Motor object for the Blender or Stomach, assigned to M2 on the Adafruit Motor Driver Board
Adafruit_DCMotor *CalciumBathPump = AdafruitMotorDriverBoard.getMotor(3); //Creates an Adafruit DC Motor object for the Calcium Bath pump, assigned to M3 on the Adafruit Motor Driver Board
Adafruit_DCMotor *CalciumBathExtractionPump = AdafruitMotorDriverBoard.getMotor(4); //Creates an Adafruit DC Motor object for the Calcium Bath Extraction pump, assigned to M4 on the Adafruit Motor Driver Board

int currentstatus;
int pressed = 0;
int startrun = 0;

LcdMenu menu;//object reference for the LCD manipulations

void setUpPump(Adafruit_DCMotor *pump, int the_speed);
void set_vars_for_pumps();

void setup() {
  Serial.begin(9600); //Initialize serial data communication, set to baud rate of 9600
  AdafruitMotorDriverBoard.begin();

  setUpPump(NaAlgPump, 255);
  setUpPump(BlenderStomach, 30);
  setUpPump(CalciumBathPump, 255);
  setUpPump(CalciumBathExtractionPump, 255);

  pinMode(SteppingMotorDirectionPin, OUTPUT);     
  pinMode(SteppingMotorStepFrequencyPin, OUTPUT);
  Serial.println("setup");
  
  menu.setupLcd();
  menu.set_to_idle();
  currentstatus = menu.getStatus();
//  lcd.begin(16, 2);// set up the LCD's number of columns and rows
//  lcd.print("Ready");
//  lcd.setBacklight(WHITE);
//  Serial.println("READY - DEFAULT");
}

void loop() 
{

  uint8_t buttons;
  currentstatus = menu.getStatus();

  /////////////////////////////////////////
  // Running
  /////////////////////////////////////////
if(menu.isRunning(currentstatus)) {
  
    currentTime = millis();
    calculatedSecond = (unsigned long)(currentTime)/1000-rollover;
  
  int percentage = (int)( ((100 * calculatedSecond) / (265+60)));
  int updateprogress;
  if (OneByOneStepper==1 || (OneByOneStepper==0 && (calculatedSecond <265 || calculatedSecond > (265+60)))) {
    // progress 1 by 1
    updateprogress=(currentpercentage!=percentage);
  } else {
    // progress 10 by 10
    updateprogress=((currentpercentage!=percentage) && (percentage==0 || percentage==10 || percentage==20 || percentage==30 || percentage==40 || percentage==50 || percentage==60 || percentage==70 || percentage==80 || percentage==90 || percentage==100));
  }
  if(updateprogress) {
      currentpercentage=percentage;
      menu.print_progress(currentpercentage);
  //        if(currentpercentage<10) {
  //         lcd.setCursor(10,0);
  //        } else if(currentpercentage<100) {
  //          lcd.setCursor(11,0);
  //        }
  //        lcd.print("%");
  }
    if(calculatedSecond!=currentSecond) {  
  
      currentSecond=calculatedSecond;
      Serial.print("Second:" );
      Serial.println(currentSecond);
      
      if (AbortOnStepper==1 || (AbortOnStepper==0 && (calculatedSecond <265 || calculatedSecond > (265+60)))){
        buttons = menu.getButtonStatus();
        
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
        } 
        else if (buttons && pressed==1 && secondstoabort!=0 && currentSecond-secondstoabort>3) {
          NaAlgPump->run(RELEASE);  //Stops motor 
          Serial.println("ABORT Motor 1 off");
          BlenderStomach->run(RELEASE);  //Stops motor 
          Serial.println("ABORT Motor 2 off");
          CalciumBathPump->run(RELEASE);  //Stops motor 
          Serial.println("ABORT Motor 3 off");
          CalciumBathExtractionPump->run(RELEASE);  //Stops motor 
          Serial.println("ABORT Motor 4 off");
          menu.set_to_idle();
        //            lcd.clear();
        //            lcd.setCursor(0,0);
        //            lcd.print("Ready");
        //            lcd.setBacklight(WHITE);
        //            currentstatus=READY;
        //            Serial.println("READY");
          secondstoabort=0;
        }
      }
      
      if(menu.isRunning(currentstatus)) {
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
      if(stepper == 0) {
        stepper=1;
        Serial.println("starting stepper motor pump");
      }
      digitalWrite(9, HIGH);
      delayMicroseconds(30); // This sets the speed of the stepper motor pump (30 being the fastest)
      digitalWrite(9, LOW);
      delayMicroseconds(30); // This sets the speed of the stepper motor pump (10 being the fastest)
    }
  
    if(currentSecond>=(265+60)) {
      delay(1000);
      Serial.println("END OF RUNNING");
      //Serial.println("exit");
      //exit(0);                //Exit loop
      menu.set_to_idle();
  //      lcd.clear();
  //      lcd.setCursor(0,0);
  //      lcd.print("Ready");
  //      lcd.setBacklight(WHITE);
  //      currentstatus=READY;
      secondstoabort=0;
      Serial.println("READY");
    }

    

/////////////////////////////////////////
// Prime Pumps
/////////////////////////////////////////
} else if(menu.isNaAlgPumpRunning(currentstatus) || menu.isCaInputPumpRunning(currentstatus) 
                  || menu.isCaExitPumpRunning(currentstatus) || menu.isSpoutPumpRunning(currentstatus)) {
    
    currentTime = millis();
    calculatedSecond = (unsigned long)(currentTime)/1000-rollover;
    if(menu.isNaAlgPumpRunning(currentstatus) || menu.isCaInputPumpRunning(currentstatus) || menu.isCaExitPumpRunning(currentstatus) || 
          (menu.isSpoutPumpRunning(currentstatus) && calculatedSecond!=currentSecond)) {
      currentSecond=calculatedSecond;
      //buttons = lcd.readButtons();
      buttons = menu.getButtonStatus();
    
      if (buttons==0 && pressed==1 && startrun==1) {
        // button released (to start the process
        pressed=0;
        startrun=0;
      }
    
      if(buttons && pressed==0 && startrun==0) {
        if (menu.isNaAlgPumpRunning(currentstatus)) {
          NaAlgPump->run(RELEASE);  //Stops motor 
          Serial.println("Manual Motor 1 off");
        } 
        else if (menu.isCaInputPumpRunning(currentstatus)) {
          CalciumBathPump->run(RELEASE);  //Stops motor
          Serial.println("Manual Motor 3 off");
        } 
        else if (menu.isCaExitPumpRunning(currentstatus)) {
          CalciumBathExtractionPump->run(RELEASE);  //Stops motor
          Serial.println("Manual Motor 4 off");
        }
       menu.set_to_idle();
    //           lcd.clear();
    //           lcd.setCursor(0,0);
    //           lcd.print("Ready");
    //           lcd.setBacklight(WHITE);
    //           currentstatus=READY;
    //           Serial.println("READY");
      }
    } 
    if(menu.isSpoutPumpRunning(currentstatus)){
        digitalWrite(9, HIGH);
        delayMicroseconds(30); // This sets the speed of the stepper motor pump (30 being the fastest)
        digitalWrite(9, LOW);
        delayMicroseconds(30); // This sets the speed of the stepper motor pump (10 being the fastest)
    }
    
    
/////////////////////////////////////////
// Menu navigation
/////////////////////////////////////////
}else {
    menu.getButtonStatus();
    //    buttons = lcd.readButtons();
    
    if (buttons && pressed==0) {
      menu.setMenu();
      pressed = 1;
      menu.getStatus();
      if(menu.isRunning(currentstatus)){
        stepper=0;
        startrun=1;
        currentTime = millis();
        currentpercentage = 0;
        rollover = (unsigned long)(currentTime)/1000;
      }
      else if(menu.isNaAlgPumpRunning(currentstatus)){
        set_vars_for_pumps();
        NaAlgPump->run(FORWARD);  //Runs motor forward          
      }
      else if(menu.isCaInputPumpRunning(currentstatus)){
        set_vars_for_pumps();
        CalciumBathPump->run(FORWARD); 
      }
      else if(menu.isCaExitPumpRunning(currentstatus)){
        set_vars_for_pumps();
        CalciumBathExtractionPump->run(FORWARD);
      }
      else if(menu.isSpoutPumpRunning(currentstatus)){
        set_vars_for_pumps();
        //pump is not run
        
      }
                
    //      if((currentstatus==START || currentstatus==SYSTEMSETTINGS) && buttons & BUTTON_LEFT) {
    //        lcd.clear();
    //        lcd.setCursor(0,0);
    //        lcd.print("Ready");
    //        lcd.setBacklight(WHITE);
    //        currentstatus=READY;
    //        Serial.println("READY");
    //        pressed=1;
    //      } else if((currentstatus==READY && (buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) || (currentstatus==SYSTEMSETTINGS && buttons & BUTTON_UP)) {
    //        lcd.clear();
    //        lcd.setCursor(0,0);
    //        lcd.print(">>Start");
    //        lcd.setCursor(0,1);
    //        lcd.print("System Settings");
    //        lcd.setBacklight(WHITE);
    //        currentstatus=START;
    //        Serial.println("START");
    //        pressed=1;
    //      } else if (currentstatus==START && (buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) {
    //        lcd.clear();
    //        lcd.setCursor(0,0);
    //        lcd.print("Progress 0%");
    //        lcd.setBacklight(TEAL);
    //        currentstatus=RUNNING;
    //        stepper=0;
    //        startrun=1;
    //        Serial.println("RUNNING");
    //        pressed=1;
    //        currentTime = millis();
    //        currentpercentage = 0;
    //        rollover = (unsigned long)(currentTime)/1000;
    //      } else if ((currentstatus==START && buttons & BUTTON_DOWN) || ((currentstatus==PRIMEPUMPS || currentstatus==CLEANSE) && buttons & BUTTON_LEFT)) {
    //        lcd.clear();
    //        lcd.setCursor(0,0);
    //        lcd.print("Start");
    //        lcd.setCursor(0,1);
    //        lcd.print(">>System Settings");
    //        lcd.setBacklight(WHITE);
    //        currentstatus=SYSTEMSETTINGS;
    //        Serial.println("SYSTEMSETTINGS");
    //        pressed=1;
    //      } else if ((currentstatus==SYSTEMSETTINGS && (buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) || (currentstatus==CLEANSE && buttons & BUTTON_UP)  || (currentstatus>=PUMP1 && buttons & BUTTON_LEFT)) {
    //        lcd.clear();
    //        lcd.setCursor(0,0);
    //        lcd.print(">>Prime Pumps");
    //        lcd.setCursor(0,1);
    //        lcd.print("Cleanse");
    //        lcd.setBacklight(WHITE);
    //        currentstatus=PRIMEPUMPS;
    //        Serial.println("PRIMEPUMPS");
    //        pressed=1;
    //      } else if (currentstatus==PRIMEPUMPS && buttons & BUTTON_DOWN) {
    //        lcd.clear();
    //        lcd.setCursor(0,0);
    //        lcd.print("Prime Pumps");
    //        lcd.setCursor(0,1);
    //        lcd.print(">>Cleanse");
    //        lcd.setBacklight(WHITE);
    //        currentstatus=CLEANSE;
    //        Serial.println("CLEANSE");
    //        pressed=1;
    //      } else if ((currentstatus==PRIMEPUMPS && (buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) || (currentstatus==PUMP2 && buttons & BUTTON_UP)) {
    //        lcd.clear();
    //        lcd.setCursor(0,0);
    //        lcd.print(">>Prime Na-alg Pump");
    //        lcd.setCursor(0,1);
    //        lcd.print("Prime Ca input Pump");
    //        lcd.setBacklight(WHITE);
    //        currentstatus=PUMP1;
    //        Serial.println("PUMP1");
    //        pressed=1;
    //      } else if ((currentstatus==PUMP1 && buttons & BUTTON_DOWN) || (currentstatus==PUMP3 && buttons & BUTTON_UP)) {
    //        lcd.clear();
    //        lcd.setCursor(0,0);
    //        lcd.print(">>Prime Ca input Pump");
    //        lcd.setCursor(0,1);
    //        lcd.print("Prime Ca exit Pump");
    //        lcd.setBacklight(WHITE);
    //        currentstatus=PUMP2;
    //        Serial.println("PUMP2");
    //        pressed=1;
    //      } else if ((currentstatus==PUMP2 && buttons & BUTTON_DOWN) || (currentstatus==PUMP4 && buttons & BUTTON_UP)) {
    //        lcd.clear();
    //        lcd.setCursor(0,0);
    //        lcd.print(">>Prime Ca exit Pump");
    //        lcd.setCursor(0,1);
    //        lcd.print("Prime spout Pump");
    //        lcd.setBacklight(WHITE);
    //        currentstatus=PUMP3;
    //        Serial.println("PUMP3");
    //        pressed=1;
    //      } else if (currentstatus==PUMP3 && buttons & BUTTON_DOWN) {
    //        lcd.clear();
    //        lcd.setCursor(0,0);
    //        lcd.print("Prime Ca exit Pump");
    //        lcd.setCursor(0,1);
    //        lcd.print(">>Prime spout Pump");
    //        lcd.setBacklight(WHITE);
    //        currentstatus=PUMP4;
    //        Serial.println("PUMP4");
    //        pressed=1;
    //      } else if (currentstatus==PUMP1 &&(buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) {
    //        lcd.clear();
    //        lcd.setCursor(0,0);
    //        lcd.print("Running Prime Na a");
    //        lcd.setBacklight(RED);
    //        currentstatus=RUNNING1;
    //        startrun=1;
    //        Serial.println("RUNNING1");
    //        pressed=1;
    //        NaAlgPump->run(FORWARD);  //Runs motor forward
    //        Serial.println("Manual Prime Na a started");
    //        currentTime = millis();
    //        rollover = (unsigned long)(currentTime)/1000;
    //      } else if (currentstatus==PUMP2 &&(buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) {
    //        lcd.clear();
    //        lcd.setCursor(0,0);
    //        lcd.print("Running Prime ca input pump");
    //        lcd.setBacklight(RED);
    //        currentstatus=RUNNING2;
    //        startrun=1;
    //        Serial.println("RUNNING2");
    //        pressed=1;
    //        CalciumBathPump->run(FORWARD);  //Runs motor forward
    //        Serial.println("Manual Prime ca input started");
    //        currentTime = millis();
    //        rollover = (unsigned long)(currentTime)/1000;
    //      } else if (currentstatus==PUMP3 &&(buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) {
    //        lcd.clear();
    //        lcd.setCursor(0,0);
    //        lcd.print("Running ca exit pump");
    //        lcd.setBacklight(RED);
    //        currentstatus=RUNNING3;
    //        startrun=1;
    //        Serial.println("RUNNING3");
    //        pressed=1;
    //        CalciumBathExtractionPump->run(FORWARD);  //Runs motor forward
    //        Serial.println("Manual ca exit pump started");
    //        currentTime = millis();
    //        rollover = (unsigned long)(currentTime)/1000;
    //      } else if (currentstatus==PUMP4 &&(buttons & BUTTON_RIGHT || buttons & BUTTON_SELECT)) {
    //        lcd.clear();
    //        lcd.setCursor(0,0);
    //        lcd.print("Running spout pump");
    //        lcd.setBacklight(RED);
    //        currentstatus=RUNNING4;
    //        startrun=1;
    //        Serial.println("RUNNING4");
    //        pressed=1;
    //        Serial.println("Manual spout pump started");
    //        currentTime = millis();
    //        rollover = (unsigned long)(currentTime)/1000;
    //      }
    } 
    else if (buttons==0  && pressed==1) {
    // button released
    pressed=0;
    }
  }

}

/*Set up a pump*/
void setUpPump(Adafruit_DCMotor *pump, int the_speed){
  pump->setSpeed(the_speed);
  pump->run(FORWARD);//sets direction of motor
  pump->run(RELEASE);//stops motor
}
/*set proper variables when running pumps 1-4*/
void set_vars_for_pumps(){
  startrun = 1;
  currentTime = millis();
  rollover = (unsigned long)(currentTime)/1000;
}
