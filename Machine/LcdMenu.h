#ifndef LcdMenu_h
#define LcdMenu_h

#include <Adafruit_RGBLCDShield.h> //Display text and change colors on LCD Display
#include "Arduino.h"

class LcdMenu{
  public:
    LcdMenu();
    int getStatus();
    uint8_t getButtonStatus();
    void print_progress(int percent);
    bool isRunning(int a_status);
    bool isNaAlgPumpRunning(int a_status);
    bool isCaInputPumpRunning(int a_status);
    bool isCaExitPumpRunning(int a_status);
    bool isSpoutPumpRunning(int a_status);
    void setMenu();
    void setupLcd();
    void set_to_idle();
    
  private:
    // These #defines make it easy to set the backlight color for LCD display
    #define RED 0x1
    //unused
    //#define YELLOW 0x3
    //#define GREEN 0x2
    //#define BLUE 0x4
    //#define VIOLET 0x5
    #define TEAL 0x6
    #define WHITE 0x7

    //store the state of machine: idle, running, or priming
    int currentStatus_;
    
    //keeps track of the state of the machine by the menu options
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
    
    Adafruit_RGBLCDShield lcd_;
    uint8_t buttons_;

    void print_percent_symb(int percent);
    void initial_progress();
    void check_status();
    void setRunningStatus();
    void primeStatus(String runPump, int a_status);
    void change_parent_menu_display(String parent, String child, int a_status);
    void change_child_menu_display(String parent,String child,int a_status);
};
#endif
