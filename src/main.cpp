// compile date 00:30

#include <Arduino.h>
#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <OneWire.h>
#include <DS18B20.h>
#include <LiquidCrystal.h>
#include <Encoder.h>
#include <EEPROMex.h>
#include <SolarPosition.h>

//   SOLAR position
SolarPosition my_position(49.551805, 20.566461); // naszacowice

//#define ENCODER_DO_NOT_USE_INTERRUPTS

const byte ONEWIRE_PIN = 2;
byte sensorAddress[8] = {0x28, 0xFF, 0x4B, 0x1A, 0x2, 0x17, 0x3, 0xDD}; //Sensor Adress
OneWire onewire(ONEWIRE_PIN);                                           // 1-Wire object
DS18B20 sensors(&onewire);                                              // DS18B20 sensors object

//    Clock Configuration
//DS1307 rtc;
tmElements_t tm;

//    LCD Configuration
const int rs = 12, en = 9, d4 = 11, d5 = 8, d6 = 10, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//    Servo Config

#define relay_1 A1
#define relay_2 A2
#define motor 6
#define power A3
#define potentiometer A0
int brake = 0;
int brake_speed = 0;
int speed = 0;
int servo_histereza = 0;

//    Button

boolean buttonActive = false;
boolean longPressActive = false;
long buttonTimer = 0;
unsigned long longPressTime = 850;
unsigned int button = 5;
boolean klik = 0;
boolean lklik = 0;
boolean state = 0;

//    Encoder

Encoder myEnc(3, 4);
long oldPosition = -999;
long encoder = myEnc.read();

//    Temperature sensor

float temperature = sensors.readTemperature(sensorAddress);

//    Variables

int temp = 0;
boolean temp_flag = 0;
int setPos = 0;
int old_setPos = 0;
int eep_array = 10;
int steps_num = 0;
int timePosArray[35][3];

int old_sec = 0;
int debug = 0;
int refresh_counter = 0;
int pot = analogRead(potentiometer);

int automatic_mode = 0;
int old_automatic_mode = 0;
int old_work_mode = 0;

int work_mode = 0; // data of work mode 0 stop, 1 time table, 2 full_auto
int temp_hist = 0;
int escape_direction = 0;
int transition_hour = 0;
int azimuth_min = 0;
int azimuth_max = 0;
int servo_min = 0;
int servo_max = 0;

int addressInt;
const int memBase = 10;
const int maxAllowedWrites = 80;

void setup()
{
  //Lcd config
  lcd.begin(20, 4);
  // the function to get the time from the RTC
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Unable to sync with the RTC");
    delay(2500);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RTC has set the system time");
  delay(250);

  SolarPosition::setTimeProvider(RTC.get);

  // Temperature sensor init
  sensors.begin();
  // I/O Configuration
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(potentiometer, INPUT);
  pinMode(3, INPUT);
  pinMode(4, INPUT);
  pinMode(button, INPUT);
  pinMode(relay_1, OUTPUT);
  pinMode(relay_2, OUTPUT);
  pinMode(power, OUTPUT);
  pinMode(motor, OUTPUT);
  // Init Hbridge
  digitalWrite(LED_BUILTIN, 1);
  digitalWrite(relay_1, 1);
  digitalWrite(relay_2, 1);

  //*********************EEPROM READ HANDLER****************

  EEPROM.setMemPool(memBase, 1024);

  addressInt = EEPROM.getAddress(sizeof(int));

  EEPROM.setMaxAllowedWrites(1024);
  delay(100);

  int eep_position = addressInt;

  /*
  For EEPROM clear uncoment
  //for(int i=0; i<1024;i++){
  //EEPROM.write(i,0);
  }
  */

  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("EEPROM READ");
  for (int i = 0; i < 35; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      timePosArray[i][j] = EEPROM.readInt(eep_position);
      eep_position = eep_position + 2;
    }
    delay(10);
  }
  temp_hist = timePosArray[30][2];
  transition_hour = timePosArray[31][0];
  temp = timePosArray[31][1];
  speed = timePosArray[32][2];
  brake = timePosArray[31][2];
  brake_speed = timePosArray[32][1];
  servo_histereza = timePosArray[32][0];
  azimuth_min = timePosArray[33][0];
  azimuth_max = timePosArray[33][1];
  servo_min = timePosArray[33][2];
  servo_max = timePosArray[34][0];
  work_mode = timePosArray[34][1]; // data of work mode 0 stop, 1 time table, 2 full_auto
  escape_direction = timePosArray[34][2];
}

/**************************************************TABLE_HANDLER************************************************/

void table_write()
{
  int eep_pos = addressInt;

  for (int row = 0; row < 35; row++)
  {
    for (int column = 0; column < 3; column++)
    {
      int write = timePosArray[row][column];
      EEPROM.updateInt(eep_pos, write);
      eep_pos = eep_pos + 2;
    }
  }
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("EEPROM SAVED");
  delay(1500);
}
//**********************************************TABLE_MODE^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^/
int table_mode()
{

  int now_min = minute();
  int now_hour = hour();
  int timeToProces = (now_hour * 100) + now_min;
  int oldtable_convert_time = 1000;
  int result = 0;

  for (int i = 0; i < 18; i++)
  {

    int table_convert_time = (timePosArray[i][1] * 100) + timePosArray[i][2];

    //table_convert_time = timeToProces - table_convert_time;
    //table_convert_time = abs(table_convert_time);
    table_convert_time = timeToProces - table_convert_time;
    if (table_convert_time <0){
      table_convert_time = 9999;
    }
    

    if (timePosArray[i][0] == 0 && timePosArray[i][1] == 0 && timePosArray[i][2] == 0)
    {
      i = 19;
    }
    if (table_convert_time < oldtable_convert_time)
    {
      result = timePosArray[i][0];
      oldtable_convert_time = table_convert_time;
    }
  }
  result = result;
  return result;
}

/****************************************************************BUTTON_CHECK***************************************/
void button_check()
{
  if (digitalRead(button) == LOW)
  {
    if (buttonActive == false)
    {

      buttonActive = true;
      buttonTimer = millis();
    }

    if ((millis() - buttonTimer > longPressTime) && (longPressActive == false))
    {

      longPressActive = true;
      lklik = !lklik;
    }
  }
  else
  {
    if (buttonActive == true)
    {
      if (longPressActive == true)
      {
        longPressActive = false;
      }
      else
      {
        klik = !klik;
      }
      buttonActive = false;
    }
  }
}
/********************************************MOTOR SPIN*******************************************************/

void motor_spin(int demandPosition)
{
  digitalWrite(relay_1, 1);
  digitalWrite(relay_2, 1);
  digitalWrite(motor, 1);
  boolean work = 0;

  pot = analogRead(potentiometer);
  int old_pot = pot;
  int histereza = demandPosition - pot;
  histereza = abs(histereza);
  if (histereza >= servo_histereza)
  {
    boolean dir = 0;
    work = 1;
    digitalWrite(power, 1);
    delay(250);

    if (demandPosition > pot)
    {
      dir = 1;
      digitalWrite(relay_1, 1);
      digitalWrite(relay_2, 0);
    }
    if (demandPosition < pot)
    {
      dir = 0;
      digitalWrite(relay_1, 0);
      digitalWrite(relay_2, 1);
    }

    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("MOTOR DIRECTION: ");
    lcd.setCursor(8, 1);

    if (dir == 0)
    {
      lcd.print("NORM");
    }

    if (dir == 1)
    {
      lcd.print("REV");
    }
    lcd.setCursor(1, 2);
    lcd.print("CURRENT:");
    lcd.setCursor(1, 3);
    lcd.print("DESIRABLE:");

    int work_sec = second();
    int old_work_sec = work_sec;
    int failsafe_counter = 0;
    analogWrite(motor, speed);
    button_check();
    while (work == 1)
    {
      if (lklik == 1)
      {
        work = 0;
      }
      button_check();
      work_sec = second();
      int pot = analogRead(potentiometer);
      lcd.setCursor(12, 2);
      lcd.print(pot, DEC);
      lcd.print("  ");
      lcd.setCursor(12, 3);
      lcd.print(demandPosition, DEC);
      lcd.print("  ");

      if (dir == 0 && pot <= demandPosition + brake)
      {
        analogWrite(motor, brake_speed);
      }
      if (dir == 1 && pot >= demandPosition - brake)
      {
        analogWrite(motor, brake_speed);
      }

      if (dir == 0 && pot <= demandPosition)
      {
        work = 0;
      }
      if (dir == 1 && pot >= demandPosition)
      {
        work = 0;
      }

      // Motor failsafe procedure for not changing pos in hardcoded time 15 sec
      if (work_sec != old_work_sec)
      {
        old_work_sec = work_sec;
        int failsafe_math = abs(pot - old_pot);

        if (failsafe_math <= 1)
        {
          failsafe_counter++;
        }
        old_pot = pot;
      }
      if (failsafe_counter >= 15)
      {

        pot = analogRead(potentiometer);
        LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
        lcd.begin(20, 4);
        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("FAILSAFE !!!");
        digitalWrite(power, 0);
        klik = 0;
        while (klik == 0)
        {
          if (digitalRead(button) == LOW)
          {
            klik = 1;
          }
        }
        work = 0;
      }
    }
    digitalWrite(motor, 1);
    delay(250);
    digitalWrite(relay_1, 1);
    digitalWrite(relay_2, 1);
    delay(10);

    pot = analogRead(potentiometer);
    LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
    lcd.begin(20, 4);
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("POSITION SET");
  }
  digitalWrite(power, 0);
  pot = +analogRead(potentiometer);
}

/****************************************************LCD_REFRESH**********************************/

void lcd_refresh(int potValue, float temp, long encoderValue, long azimuth, int elevation, int tpos)
{

  refresh_counter++;
  if (refresh_counter == 100)
  {
    LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
    lcd.begin(20, 4);
    delay(50);
    refresh_counter = 0;
  }
  // Get time
  // Print LCD data
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(hour(), DEC);
  lcd.print(":");
  lcd.print(minute(), DEC);
  lcd.print(":");
  lcd.print(second(), DEC);
  lcd.setCursor(13, 0);
  lcd.print(temp);
  lcd.print(" C");
  lcd.setCursor(1, 1);
  lcd.print("Servo: ");
  lcd.print(potValue);
  lcd.setCursor(1, 2);
  lcd.print("MODE: ");

  switch (work_mode)
  {
  case 0:
    lcd.print("STOP");
    break;
  case 1:
    lcd.print("TIME TABLE");
    break;
  case 2:
    lcd.print("AUTO");
    break;
  }

  lcd.setCursor(1, 3);
  lcd.print("AZIMUTH: ");
  lcd.print(azimuth);
  lcd.setCursor(12, 3);
}

/*****************************************************   MENU   ************************************************************/

void menu()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(">");
  lcd.setCursor(2, 0);
  lcd.print("SET STEPS");
  lcd.setCursor(15, 0);
  lcd.print("TEMP");
  lcd.setCursor(2, 1);
  lcd.print("SET TIME");
  lcd.setCursor(15, 1);
  lcd.print("MODE");
  lcd.setCursor(2, 2);
  lcd.print("MAN. MODE");
  lcd.setCursor(15, 2);
  lcd.print("SERVO");
  lcd.setCursor(2, 3);
  lcd.print("ADVENCED");
  lcd.setCursor(15, 3);
  lcd.print("EXIT");

  encoder = myEnc.read();
  long old_encoder = encoder;
  int menu_count = 0;
  int new_sec = second(), new_min = minute(), new_hour = hour(), new_day = day(), new_month = month(), new_year = year();
  delay(500);

  klik = 0;
  lklik = 0;

  while (klik == 0)
  {
    button_check();
    encoder = myEnc.read();

    if (4 <= abs(encoder - old_encoder))
    {
      if (encoder > old_encoder)
      {
        menu_count++;
        old_encoder = encoder;
      }
      if (encoder < old_encoder)
      {
        menu_count--;
        old_encoder = encoder;
      }

      if (menu_count < 0)
      {
        menu_count = 7;
      }
      if (menu_count > 7)
      {
        menu_count = 0;
      }

      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("SET STEPS");
      lcd.setCursor(15, 0);
      lcd.print("TEMP");
      lcd.setCursor(2, 1);
      lcd.print("SET TIME");
      lcd.setCursor(15, 1);
      lcd.print("MODE");
      lcd.setCursor(2, 2);
      lcd.print("MAN. MODE");
      lcd.setCursor(15, 2);
      lcd.print("SERVO");
      lcd.setCursor(2, 3);
      lcd.print("ADVENCED");
      lcd.setCursor(15, 3);
      lcd.print("EXIT");

      switch (menu_count)
      {
      case 0:
        lcd.setCursor(0, 0);
        lcd.print(">");
        break;

      case 1:
        lcd.setCursor(0, 1);
        lcd.print(">");
        break;

      case 2:
        lcd.setCursor(0, 2);
        lcd.print(">");
        break;

      case 3:
        lcd.setCursor(0, 3);
        lcd.print(">");
        break;

      case 4:
        lcd.setCursor(13, 0);
        lcd.print(">");
        break;

      case 5:
        lcd.setCursor(13, 1);
        lcd.print(">");
        break;

      case 6:
        lcd.setCursor(13, 2);
        lcd.print(">");
        break;
      case 7:
        lcd.setCursor(13, 3);
        lcd.print(">");
        break;
      }
    }
  }

  klik = 0;
  lklik = 0;
  //********************************TIME TABLE MENU**********************************
  if (menu_count == 0)
  {
    new_sec = second(), new_min = minute(), new_hour = hour();
    int step = 0;
    int new_time = 0;
    int sel = 0;
    klik = 0;
    lklik = 0;

    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("WORK SCHEDULE");
    lcd.setCursor(2, 1);
    lcd.print("STEP: ");
    lcd.print(step, DEC);
    lcd.setCursor(2, 2);
    lcd.print("POS : ");
    lcd.print(timePosArray[step][0], DEC);
    lcd.setCursor(2, 3);
    lcd.print("TIME:  ");
    lcd.print(timePosArray[step][1], DEC);
    lcd.print(":");
    lcd.print(timePosArray[step][2], DEC);
    lcd.setCursor(0, 1);
    lcd.print(">");

    while (lklik == 0)
    {
      button_check();
      encoder = myEnc.read();

      if (klik == 1)
      {
        sel++;
      }

      if (sel >= 6)
      {
        sel = 0;
      }

      if (sel == 0)
      {
        new_time = step;
      }

      if (sel == 1)
      {
        new_time = timePosArray[step][0];
      }

      if (sel == 3)
      {
        new_time = timePosArray[step][1];
      }

      if (sel == 4)
      {
        new_time = timePosArray[step][2];
      }

      if ((2 <= abs(encoder - old_encoder)) || klik == 1)
      {
        klik = 0;

        if (encoder > old_encoder)
        {
          new_time = new_time + 1;
          old_encoder = encoder;
        }

        if (encoder < old_encoder)
        {
          new_time = new_time - 1;
          old_encoder = encoder;
        }

        if (new_time < 0)
        {
          new_time = 0;
        }

        if (sel == 0)
        {
          if (new_time > 25)
          {
            new_time = 25;
          }
          step = new_time;
        }

        if (sel == 1)
        {
          if (new_time > 1000)
          {
            new_time = 1000;
          }
          timePosArray[step][0] = new_time;
        }

        if (sel == 3)
        {
          if (new_time > 23)
          {
            new_time = 23;
          }
          timePosArray[step][1] = new_time;
        }

        if (sel == 4)
        {
          if (new_time > 59)
          {
            new_time = 59;
          }
          timePosArray[step][2] = new_time;
        }

        lcd.clear();
        lcd.setCursor(3, 0);
        lcd.print("WORK SCHEDULE");
        lcd.setCursor(2, 1);
        lcd.print("STEP: ");
        lcd.print(step, DEC);
        lcd.setCursor(2, 2);
        lcd.print("POS : ");
        lcd.print(timePosArray[step][0], DEC);
        lcd.setCursor(2, 3);
        lcd.print("TIME:  ");
        lcd.print(timePosArray[step][1], DEC);
        lcd.print(":");
        lcd.print(timePosArray[step][2], DEC);

        if (sel == 2)
        {
          motor_spin(timePosArray[step][0]);
          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print("WORK SCHEDULE");
          lcd.setCursor(2, 1);
          lcd.print("STEP: ");
          lcd.print(step, DEC);
          lcd.setCursor(2, 2);
          lcd.print("POS : ");
          lcd.print(timePosArray[step][0], DEC);
          lcd.setCursor(2, 3);
          lcd.print("TIME:  ");
          lcd.print(timePosArray[step][1], DEC);
          lcd.print(":");
          lcd.print(timePosArray[step][2], DEC);
          sel = 3;
        }

        if (sel == 0)
        {
          lcd.setCursor(0, 1);
          lcd.print(">");
        }
        if (sel == 1)
        {
          lcd.setCursor(0, 2);
          lcd.print(">");
        }
        if (sel == 3)
        {
          lcd.setCursor(0, 3);
          lcd.print(">");
          lcd.setCursor(8, 3);
          lcd.print(">");
        }
        if (sel == 4)
        {
          lcd.setCursor(0, 3);
          lcd.print(">");
          lcd.setCursor(14, 3);
          lcd.print("<");
        }
        if (sel == 5)
        {
          lcd.setCursor(13, 1);
          lcd.print("save");
        }
      }
    }

    if (sel == 5)
    {

      table_write();
    }
  }
  //********************************SET TIME MENU**********************************
  if (menu_count == 1)
  {
    new_sec = second(), new_min = minute(), new_hour = hour(), new_day = day(), new_month = month(), new_year = year();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Time: ");
    lcd.print(new_hour, DEC);
    lcd.print(":");
    lcd.print(new_min, DEC);
    lcd.print(":");
    lcd.print(new_sec, DEC);
    lcd.setCursor(0, 2);
    lcd.print("Date: ");
    lcd.print(new_day, DEC);
    lcd.print("-");
    lcd.print(new_month, DEC);
    lcd.print("-");
    lcd.print(new_year, DEC);
    lcd.setCursor(6, 1);
    lcd.print("^");
    int new_time = 0;
    byte sel = 0;
    while (lklik == 0)
    {
      klik = 0;
      lklik = 0;
      button_check();
      encoder = myEnc.read();
      if (klik == 1)
      {
        sel++;
      }
      if (sel >= 6)
      {
        sel = 0;
      }

      if (sel == 0)
      {
        new_time = new_hour;
      }
      if (sel == 1)
      {
        new_time = new_min;
      }
      if (sel == 2)
      {
        new_time = new_sec;
      }
      if (sel == 3)
      {
        new_time = new_day;
      }
      if (sel == 4)
      {
        new_time = new_month;
      }
      if (sel == 5)
      {
        new_time = new_year;
      }

      if ((2 <= abs(encoder - old_encoder)) || klik == 1)
      {
        klik = 0;
        if (encoder > old_encoder)
        {
          new_time = new_time + 1;
          old_encoder = encoder;
        }
        if (encoder < old_encoder)
        {
          new_time = new_time - 1;
          old_encoder = encoder;
        }
        if (new_time < 0)
        {
          new_time = 0;
        }

        if (sel == 0)
        {
          if (new_time > 23)
          {
            new_time = 23;
          }
          new_hour = new_time;
        }
        if (sel == 1)
        {
          if (new_time > 59)
          {
            new_time = 59;
          }
          new_min = new_time;
        }
        if (sel == 2)
        {
          if (new_time > 59)
          {
            new_time = 59;
          }
          new_sec = new_time;
        }
        if (sel == 3)
        {
          if (new_time > 31)
          {
            new_time = 31;
          }
          new_day = new_time;
        }
        if (sel == 4)
        {
          if (new_time > 12)
          {
            new_time = 12;
          }
          new_month = new_time;
        }
        if (sel == 5)
        {
          new_year = new_time;
        }

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Time: ");
        lcd.print(new_hour, DEC);
        lcd.print(":");
        lcd.print(new_min, DEC);
        lcd.print(":");
        lcd.print(new_sec, DEC);
        lcd.setCursor(0, 2);
        lcd.print("Date: ");
        lcd.print(new_day, DEC);
        lcd.print("-");
        lcd.print(new_month, DEC);
        lcd.print("-");
        lcd.print(new_year, DEC);

        if (sel == 0)
        {
          lcd.setCursor(6, 1);
          lcd.print("^");
        }
        if (sel == 1)
        {
          lcd.setCursor(9, 1);
          lcd.print("^");
        }
        if (sel == 2)
        {
          lcd.setCursor(12, 1);
          lcd.print("^");
        }
        if (sel == 3)
        {
          lcd.setCursor(6, 3);
          lcd.print("^");
        }
        if (sel == 4)
        {
          lcd.setCursor(9, 3);
          lcd.print("^");
        }
        if (sel == 5)
        {
          lcd.setCursor(12, 3);
          lcd.print("^");
        }
      }
    }

    tm.Hour = new_hour;
    tm.Minute = new_min;
    tm.Second = new_sec;
    tm.Day = new_day;
    tm.Month = new_month;
    tm.Year = CalendarYrToTm(new_year);
    RTC.write(tm);
    setSyncProvider(RTC.get);
  }
  //********************************MANUAL MODE MENU**********************************
  if (menu_count == 2)
  {
    int new_motor = analogRead(potentiometer);
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("ACTUAL POSITION:");
    lcd.setCursor(7, 1);
    lcd.print(new_motor, DEC);
    lcd.setCursor(1, 2);
    lcd.print("DEMAND POSITION:");
    lcd.setCursor(7, 3);
    lcd.print(new_motor, DEC);

    while (lklik == 0)
    {
      button_check();
      encoder = myEnc.read();
      if (encoder != old_encoder)
      {
        if (encoder > old_encoder)
        {
          new_motor = new_motor + 5;
          old_encoder = encoder;
        }
        if (encoder < old_encoder)
        {
          new_motor = new_motor - 5;
          old_encoder = encoder;
        }
        if (new_motor > 1020)
        {
          new_motor = 1020;
        }
        if (new_motor < 0)
        {
          new_motor = 0;
        }

        lcd.setCursor(7, 3);
        lcd.print(new_motor, DEC);
        lcd.print("  ");
      }
      if (klik == 1)
      {
        motor_spin(new_motor);
        klik = 0;
        int new_motor = analogRead(potentiometer);
        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("ACTUAL POSITION:");
        lcd.setCursor(7, 1);
        lcd.print(new_motor, DEC);
        lcd.setCursor(1, 2);
        lcd.print("DEMAND POSITION:");
      }
    }
  }
  //********************************ADVENCED MENU**********************************
  if (menu_count == 3)
  {

    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Servo_min: ");
    lcd.print(servo_min);
    lcd.setCursor(1, 1);
    lcd.print("Servo_max: ");
    lcd.print(servo_max);
    lcd.setCursor(1, 2);
    lcd.print("Az_min: ");
    lcd.print(azimuth_min);
    lcd.setCursor(1, 3);
    lcd.print("Az_max: ");
    lcd.print(azimuth_max);
    lcd.setCursor(0, 0);
    lcd.print(">");

    int new_time = 0;
    int sel = 0;

    while (lklik == 0)
    {
      klik = 0;
      lklik = 0;
      button_check();
      encoder = myEnc.read();
      if (klik == 1)
      {
        sel++;
      }
      if (sel >= 4)
      {
        sel = 0;
      }

      if (sel == 0)
      {
        new_time = servo_min;
      }
      if (sel == 1)
      {
        new_time = servo_max;
      }
      if (sel == 2)
      {
        new_time = azimuth_min;
      }
      if (sel == 3)
      {
        new_time = azimuth_max;
      }

      if ((2 <= abs(encoder - old_encoder)) || klik == 1)
      {
        klik = 0;
        if (encoder > old_encoder)
        {
          new_time = new_time + 1;
          old_encoder = encoder;
        }
        if (encoder < old_encoder)
        {
          new_time = new_time - 1;
          old_encoder = encoder;
        }
        if (new_time < 0)
        {
          new_time = 0;
        }

        if (sel == 0)
        {
          if (new_time > 1000)
          {
            new_time = 1000;
          }
          servo_min = new_time;
        }
        if (sel == 1)
        {
          if (new_time < 500)
          {
            new_time = 500;
          }
          if (new_time > 1000)
          {
            new_time = 1000;
          }
          servo_max = new_time;
        }
        if (sel == 2)
        {
          if (new_time > 255)
          {
            new_time = 255;
          }
          azimuth_min = new_time;
        }
        if (sel == 3)
        {
          if (new_time > 510)
          {
            new_time = 510;
          }
          if (new_time < 155)
          {
            new_time = 155;
          }
          azimuth_max = new_time;
        }

        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("Servo_min: ");
        lcd.print(servo_min);
        lcd.setCursor(1, 1);
        lcd.print("Servo_max: ");
        lcd.print(servo_max);
        lcd.setCursor(1, 2);
        lcd.print("Az_min: ");
        lcd.print(azimuth_min);
        lcd.setCursor(1, 3);
        lcd.print("Az_max: ");
        lcd.print(azimuth_max);

        if (sel == 0)
        {
          lcd.setCursor(0, 0);
          lcd.print(">");
        }
        if (sel == 1)
        {
          lcd.setCursor(0, 1);
          lcd.print(">");
        }
        if (sel == 2)
        {
          lcd.setCursor(0, 2);
          lcd.print(">");
        }
        if (sel == 3)
        {
          lcd.setCursor(0, 3);
          lcd.print(">");
        }
      }
    }

    timePosArray[33][0] = azimuth_min;
    timePosArray[33][1] = azimuth_max;
    timePosArray[33][2] = servo_min;
    timePosArray[34][0] = servo_max;
    table_write();
  }
  //********************************TEMPERATURE MENU**********************************
  if (menu_count == 4)
  {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("MAX TEMP:");
    lcd.print(temp, DEC);
    lcd.print(" C");
    lcd.setCursor(1, 1);
    lcd.print("TRANS.HOUR: ");
    lcd.print(transition_hour);
    lcd.setCursor(1, 2);
    lcd.print("ESCAPE DIR: ");

    if (escape_direction == 0)
    {
      lcd.print("NORMAL");
    }

    if (escape_direction == 1)
    {
      lcd.print("REVERSE");
    }

    lcd.setCursor(1, 3);
    lcd.print("HISTERESIS: ");
    lcd.print(temp_hist);
    lcd.print(" C");

    int new_time = 0;
    int sel = 0;

    while (lklik == 0)
    {
      klik = 0;
      lklik = 0;
      button_check();
      encoder = myEnc.read();
      if (klik == 1)
      {
        sel++;
      }
      if (sel >= 4)
      {
        sel = 0;
      }

      if (sel == 0)
      {
        new_time = temp;
      }
      if (sel == 1)
      {
        new_time = transition_hour;
      }
      if (sel == 2)
      {
        new_time = escape_direction;
      }
      if (sel == 3)
      {
        new_time = temp_hist;
      }

      if ((2 <= abs(encoder - old_encoder)) || klik == 1)
      {
        klik = 0;

        if (encoder > old_encoder)
        {
          new_time = new_time + 1;
          old_encoder = encoder;
        }

        if (encoder < old_encoder)
        {
          new_time = new_time - 1;
          old_encoder = encoder;
        }
        if (new_time < 0)
        {
          new_time = 0;
        }

        if (sel == 0)
        {
          if (new_time > 95)
          {
            new_time = 95;
          }
          temp = new_time;
        }
        if (sel == 1)
        {
          if (new_time > 24)
          {
            new_time = 24;
          }
          transition_hour = new_time;
        }
        if (sel == 2)
        {
          if (new_time > 1)
          {
            new_time = 1;
          }
          escape_direction = new_time;
        }
        if (sel == 3)
        {
          if (new_time > 25)
          {
            new_time = 25;
          }
          temp_hist = new_time;
        }

        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("MAX TEMP:");
        lcd.print(temp, DEC);
        lcd.print(" C");
        lcd.setCursor(1, 1);
        lcd.print("TRANS.HOUR: ");
        lcd.print(transition_hour);
        lcd.setCursor(1, 2);
        lcd.print("ESCAPE DIR: ");

        if (escape_direction == 0)
        {
          lcd.print("NORMAL");
        }

        if (escape_direction == 1)
        {
          lcd.print("REVERSE");
        }

        lcd.setCursor(1, 3);
        lcd.print("HISTERESIS: ");
        lcd.print(temp_hist);
        lcd.print(" C");

        if (sel == 0)
        {
          lcd.setCursor(0, 0);
          lcd.print(">");
        }
        if (sel == 1)
        {
          lcd.setCursor(0, 1);
          lcd.print(">");
        }
        if (sel == 2)
        {
          lcd.setCursor(0, 2);
          lcd.print(">");
        }
        if (sel == 3)
        {
          lcd.setCursor(0, 3);
          lcd.print(">");
        }
      }
    }
    timePosArray[34][2] = escape_direction;
    timePosArray[31][0] = transition_hour;
    timePosArray[31][1] = temp;
    timePosArray[30][2] = temp_hist;
    table_write();
  }
  //********************************WORK MODE MENU**********************************
  if (menu_count == 5)
  {

    int work_menu = timePosArray[34][1];
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("Work mode: ");

    switch (work_menu)
    {
    case 0:
      lcd.setCursor(8, 2);
      lcd.print("STOP");
      break;
    case 1:
      lcd.setCursor(5, 2);
      lcd.print("TIME TABLE");
      break;
    case 2:
      lcd.setCursor(8, 2);
      lcd.print("AUTO");
      break;
    }

    klik = 0;
    lklik = 0;
    while (lklik == 0)
    {
      button_check();
      encoder = myEnc.read();

      if ((2 <= abs(encoder - old_encoder)) || klik == 1)
      {
        klik = 0;

        if (encoder > old_encoder)
        {
          work_menu = work_menu + 1;
          old_encoder = encoder;
        }
        if (encoder < old_encoder)
        {
          work_menu = work_menu - 1;
          old_encoder = encoder;
        }

        if (work_menu < 0)
        {
          work_menu = 0;
        }

        if (work_menu > 2)
        {
          work_menu = 2;
        }

        lcd.clear();
        lcd.setCursor(5, 0);
        lcd.print("Work mode: ");

        switch (work_menu)
        {
        case 0:
          lcd.setCursor(8, 2);
          lcd.print("STOP");
          break;
        case 1:
          lcd.setCursor(5, 2);
          lcd.print("TIME TABLE");
          break;
        case 2:
          lcd.setCursor(8, 2);
          lcd.print("AUTO");
          break;
        }
      }
    }

    timePosArray[34][1] = work_menu;
    work_mode = work_menu;
    table_write();
  }

  //********************************SERVO CONFIG MENU**********************************
  if (menu_count == 6)
  {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Speed: ");
    lcd.print(map(255 - speed, 0, 255, 0, 100));
    lcd.setCursor(1, 1);
    lcd.print("Brake S: ");
    lcd.print(map(255 - brake_speed, 0, 255, 0, 100));
    lcd.setCursor(1, 2);
    lcd.print("Hist: ");
    lcd.print(servo_histereza);
    lcd.setCursor(1, 3);
    lcd.print("Brake D: ");
    lcd.print(brake);

    int new_time = 0;
    int sel = 0;

    while (lklik == 0)
    {
      klik = 0;
      lklik = 0;
      button_check();
      encoder = myEnc.read();
      if (klik == 1)
      {
        sel++;
      }

      if (sel >= 4)
      {
        sel = 0;
      }

      if (sel == 0)
      {
        new_time = speed;
      }

      if (sel == 1)
      {
        new_time = brake_speed;
      }

      if (sel == 2)
      {
        new_time = servo_histereza;
      }

      if (sel == 3)
      {
        new_time = brake;
      }

      if ((2 <= abs(encoder - old_encoder)) || klik == 1)
      {
        klik = 0;
        if (encoder > old_encoder)
        {
          new_time = new_time + 1;
          old_encoder = encoder;
        }
        if (encoder < old_encoder)
        {
          new_time = new_time - 1;
          old_encoder = encoder;
        }
        if (new_time < 0)
        {
          new_time = 0;
        }

        if (sel == 0)
        {
          if (new_time > 255)
          {
            new_time = 255;
          }
          speed = new_time;
        }
        if (sel == 1)
        {
          if (new_time > 255)
          {
            new_time = 255;
          }
          brake_speed = new_time;
        }
        if (sel == 2)
        {
          if (new_time > 25)
          {
            new_time = 25;
          }
          servo_histereza = new_time;
        }
        if (sel == 3)
        {
          if (new_time > 100)
          {
            new_time = 100;
          }

          brake = new_time;
        }

        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("Speed: ");
        lcd.print(map(255 - speed, 0, 255, 0, 100));
        lcd.setCursor(1, 1);
        lcd.print("Brake S: ");
        lcd.print(map(255 - speed, 0, 255, 0, 100));
        lcd.setCursor(1, 2);
        lcd.print("Hist: ");
        lcd.print(servo_histereza);
        lcd.setCursor(1, 3);
        lcd.print("Brake D: ");
        lcd.print(brake);

        if (sel == 0)
        {
          lcd.setCursor(0, 0);
          lcd.print(">");
        }
        if (sel == 1)
        {
          lcd.setCursor(0, 1);
          lcd.print(">");
        }
        if (sel == 2)
        {
          lcd.setCursor(0, 2);
          lcd.print(">");
        }
        if (sel == 3)
        {
          lcd.setCursor(0, 3);
          lcd.print(">");
        }
      }
    }
    timePosArray[32][2] = speed;
    timePosArray[32][1] = brake_speed;
    timePosArray[32][0] = servo_histereza;
    timePosArray[31][2] = brake;
    table_write();
  }

  if (menu_count == 0)
  {
  }
  klik = 0;
  lklik = 0;
}

/************************************************************LOOP************************************************************/
void loop()
{

  SolarPosition_t pos = my_position.getSolarPosition();
  int azimuth = pos.azimuth;
  int elewati = pos.elevation;

  int work_table = table_mode();

  // Read potentiometer position 10 samples
  int position = 0;
  for (int i = 0; i <= 9; i++)
  {
    position = +analogRead(potentiometer);
  }
  position = position / 10;

  // Requests sensor for measurement
  sensors.request(sensorAddress);
  // Waiting (block the program) for measurement reesults
  while (!sensors.available())
    ;

  // Reads the temperature from sensor
  temperature = sensors.readTemperature(sensorAddress);

  // Thermostat function independent from work mode with escape strategy defined by user
  float math_temp = temp;
  if (temperature >= math_temp)
  {

    work_mode = 0;
    int now_hour = hour();

    if (now_hour < transition_hour)
    {
      if (escape_direction == 0)
      {
        motor_spin(servo_max);
      }
      if (escape_direction == 1)
      {
        motor_spin(servo_min);
      }
    }
    else if (now_hour >= transition_hour)
    {
      if (escape_direction == 0)
      {
        motor_spin(servo_min);
      }
      if (escape_direction == 1)
      {
        motor_spin(servo_max);
      }
    }
  }

  math_temp = math_temp - temp_hist;
  if (temperature < math_temp && work_mode != timePosArray[34][1])
  {
    work_mode = timePosArray[34][1];
  }

  if (work_mode == 1)
  {

    work_table = table_mode();
    //work_table = map(work_table, 0, 1024, servo_min, servo_max);
    if (work_table != position)
      ;
    motor_spin(work_table);
  }

  if (work_mode == 2)
  {
    // Calculate to servo pos
    automatic_mode = map(azimuth, azimuth_min, azimuth_max, servo_min, servo_max);

    if (automatic_mode <= servo_min)
    {
      automatic_mode = servo_min;
    }
    if (automatic_mode >= servo_max)
    {
      automatic_mode = servo_max;
    }
    //if difrent spin motor
    if (automatic_mode != position)
    {
      motor_spin(automatic_mode);
      old_automatic_mode = automatic_mode;
    }
  }

  // Reads encoder
  encoder = myEnc.read();

  if (old_sec != second())
  {
    lcd_refresh(pot, temperature, encoder, azimuth, elewati, work_table);
    old_sec = second();
  }

  button_check();

  if (lklik == 1)
  {
    lklik = 0;
    menu();
  }
}