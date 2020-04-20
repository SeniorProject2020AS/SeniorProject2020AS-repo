#include "LcdMenu.h"

/*construct adafruit lcd and map*/
LcdMenu::LcdMenu(){
    lcd_= Adafruit_RGBLCDShield();
}

/*Get the status of the machine, which is referenced by the menu*/
int LcdMenu::getStatus(){
  return currentStatus_;
}

/*check if status is on running*/
bool LcdMenu::isRunning(int a_status){
  if(a_status == RUNNING){
    return true;
  }
  else{
    return false;
  }
}

/*check if status is on running NaAlg pump*/
bool LcdMenu::isNaAlgPumpRunning(int a_status){
  if(a_status == RUNNING1){
    return true;
  }
  else{
    return false;
  }
}

/*check if status is on running Ca input pump*/
bool LcdMenu::isCaInputPumpRunning(int a_status){
  if(a_status == RUNNING2){
    return true;
  }
  else{
    return false;
  }
}

/*check if status is on running Ca exit pump*/
bool LcdMenu::isCaExitPumpRunning(int a_status){
  if(a_status == RUNNING3){
    return true;
  }
  else{
    return false;
  }
}

/*check if status is on running spout pump*/
bool LcdMenu::isSpoutPumpRunning(int a_status){
  if(a_status == RUNNING4){
    return true;
  }
  else{
    return false;
  }
}

/*Change the status of the machine, which refers to Ready or running a particular pump*/
void LcdMenu::set_to_idle(){
  lcd_.clear();
  lcd_.setCursor(0, 0);
  lcd_.print("Ready");
  lcd_.setBacklight(WHITE);
  currentStatus_ = READY;  
}

/*Tells you which button was pressed on LCD*/
uint8_t LcdMenu::getButtonStatus(){

  buttons_ = lcd_.readButtons();

  return buttons_;
}

/*Sets up the number of columns and rows required for LCD
can place in constructor if it works*/
void LcdMenu::setupLcd(){

  lcd_.begin(16, 2);
  
}

/*prints the percent progress on LCD*/
void LcdMenu::print_progress(int percent){
  lcd_.setCursor(9, 0);
  lcd_.print(percent);
  print_percent_symb(percent);
}

/*set cursor to correct position to place % after progress number*/
void LcdMenu::print_percent_symb(int percent){
  if(percent < 10){
    lcd_.setCursor(10, 0);
  }
  else if(percent < 100){
    lcd_.setCursor(11, 0);
  }
  lcd_.print("%");
}

/*display on LCD the initial progress*/
void LcdMenu::initial_progress(){
  lcd_.clear();
  lcd_.setCursor(0,0);
  lcd_.print("Progress 0%");
  lcd_.setBacklight(TEAL);
  currentStatus_ = RUNNING;
}

/*display on LCD which pump is running*/
void LcdMenu::primeStatus(String runPump, int a_status){
  lcd_.clear();
  lcd_.setCursor(0, 0);
  lcd_.print(runPump);
  lcd_.setBacklight(RED);
  currentStatus_ = a_status;
}

/*set up the menu on display by user input*/
void LcdMenu::setMenu(){
  check_status();
}

/*check status before displaying the menu on LCD*/
void LcdMenu::check_status(){
  buttons_ = getButtonStatus();
  if((currentStatus_ == START || currentStatus_ == SYSTEMSETTINGS) && buttons_ & BUTTON_LEFT){
    set_to_idle();
  }
  else if((currentStatus_ == READY && (buttons_ & BUTTON_RIGHT || buttons_ & BUTTON_SELECT)) || 
              (currentStatus_ == SYSTEMSETTINGS && buttons_ & BUTTON_UP)){
    change_parent_menu_display(">>Start", "System Settings", START);
  }
  else if(currentStatus_ == START && (buttons_ & BUTTON_RIGHT || buttons_ & BUTTON_SELECT)){
    initial_progress();
  }
  else if((currentStatus_ == START && buttons_ & BUTTON_DOWN) || 
              ((currentStatus_ == PRIMEPUMPS || currentStatus_ == CLEANSE) && buttons_ & BUTTON_LEFT)){
    change_child_menu_display("Start", ">>System Settings", SYSTEMSETTINGS);                  
  }
  else if((currentStatus_ == SYSTEMSETTINGS && (buttons_ & BUTTON_RIGHT || buttons_ & BUTTON_SELECT)) || 
            (currentStatus_ == CLEANSE && buttons_ & BUTTON_UP) || 
                (currentStatus_ >= PUMP1 && buttons_ & BUTTON_LEFT)){
     change_parent_menu_display(">>Prime Pumps", "Cleanse", PRIMEPUMPS);       
  }
  else if(currentStatus_ == PRIMEPUMPS && buttons_ & BUTTON_DOWN){
    change_child_menu_display("Prime Pumps",">>Cleanse", CLEANSE);
  }
  else if((currentStatus_ == PRIMEPUMPS && (buttons_ & BUTTON_RIGHT || buttons_ & BUTTON_SELECT)) || 
            (currentStatus_ == PUMP2 && buttons_ & BUTTON_UP)){
     change_parent_menu_display(">>Prime Na-Alg Pump", "Prime Ca input Pump", PUMP1);           
  }
  else if((currentStatus_ == PUMP1 && buttons_ & BUTTON_DOWN) || (currentStatus_ == PUMP3 && buttons_ & BUTTON_UP)){
    change_parent_menu_display(">>Prime Ca input Pump", "Prime Ca exit pump", PUMP2); 
  }
  else if((currentStatus_ == PUMP2 && buttons_ & BUTTON_DOWN) || (currentStatus_ == PUMP4 && buttons_ & BUTTON_UP)){
    change_parent_menu_display(">>Prime Ca exit pump", "Prime Spout Pump", PUMP3); 
  }
  else if(currentStatus_ == PUMP3 && buttons_ & BUTTON_DOWN){
    change_child_menu_display("Prime Ca exit pump", ">>Prime Spout Pump", PUMP4);
  }
  else if(currentStatus_ == PUMP1 && (buttons_ & BUTTON_RIGHT || buttons_ & BUTTON_SELECT)){
    primeStatus("Running Prime Na Alg", RUNNING1);
  }
  else if(currentStatus_ == PUMP2 && (buttons_ & BUTTON_RIGHT || buttons_ & BUTTON_SELECT)){
    primeStatus("Running Prime Ca Input Pump", RUNNING2);
  }
  else if(currentStatus_ == PUMP3 && (buttons_ & BUTTON_RIGHT || buttons_ & BUTTON_SELECT)){
    primeStatus("Running Ca Exit Pump", RUNNING3);
  }
  else if(currentStatus_ == PUMP4 && (buttons_ & BUTTON_RIGHT || buttons_ & BUTTON_SELECT)){
    primeStatus("Running Spout Pump", RUNNING4);
  }
}
/*Change the display text to show parent menu selected and child menu is not*/
void LcdMenu::change_parent_menu_display(String parent_menu, String child_menu, int a_status){

  lcd_.clear();
  lcd_.setCursor(0, 0);
  lcd_.print(parent_menu);
  lcd_.setCursor(0, 1);
  lcd_.print(child_menu);
  lcd_.setBacklight(WHITE);
  currentStatus_ = a_status;  
}

/*Change the display text to show child menu selected and parent menu is not*/
void LcdMenu::change_child_menu_display(String parent_menu, String child_menu, int a_status){
  lcd_.clear();
  lcd_.setCursor(0, 0);
  lcd_.print(parent_menu);
  lcd_.setCursor(0, 1);
  lcd_.print(child_menu);
  lcd_.setBacklight(WHITE);
  currentStatus_ = a_status;
}
