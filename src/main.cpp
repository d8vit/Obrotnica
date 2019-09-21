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
   SolarPosition my_position (49.551805, 20.566461); // naszacowice

 //#define ENCODER_DO_NOT_USE_INTERRUPTS
 
  const byte ONEWIRE_PIN = 2;
  byte sensorAddress[8] = {0x28, 0xFF, 0x4B, 0x1A, 0x2, 0x17, 0x3, 0xDD};         //Sensor Adress
  OneWire onewire(ONEWIRE_PIN);           // 1-Wire object
  DS18B20 sensors(&onewire);              // DS18B20 sensors object
 
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
  const  int brake = 5;
  const int brake_speed = 150;
  const int speed = 150;
  const int servo_histereza = 7; 

  //    Button 

  boolean buttonActive = false;
  boolean longPressActive = false;
  long buttonTimer = 0;
  unsigned long longPressTime = 850;
  int button = 5;
  boolean klik = 0;
  boolean lklik = 0;
  boolean state = 0;

  //    Encoder

  Encoder myEnc(3, 4);
  long oldPosition  = -999; 
  long encoder = myEnc.read();

  //    Temperature sensor

  float temperature = sensors.readTemperature(sensorAddress);

  //    Variables

  int setPos = 0;
  int old_setPos = 0;
  int eep_array=10;
  byte steps_num = 0;
  uint16_t timePosArray[35][3];

  int old_sec = 0;
  int debug = 0;
  int refresh_counter = 0;
  int pot = analogRead(potentiometer);

  int automatic_mode = 0;
  int old_automatic_mode = 0;

  int work_mode = 1;       // data of work mode 0 stop, 1 time table, 2 full_auto
  int azimuth_min = 0; 
  int azimuth_max = 0;
  int servo_min = 0;
  int servo_max = 0;

  int addressInt;
  const int memBase = 10;
  const int maxAllowedWrites = 80;

void setup() {
 Serial.begin(115200);
 //Lcd config
 lcd.begin(20, 4);                         

  //only set the date+time one time
  //rtc.set(0, 15, 22, 23, 07, 2019); //sec, min, hour, day, month, year
  //rtc.start();
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if(timeStatus()!= timeSet) 
     Serial.println("Unable to sync with the RTC");
  else
     Serial.println("RTC has set the system time");      
  
  SolarPosition::setTimeProvider(RTC.get);

  sensors.begin();
 
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(potentiometer, INPUT);
  pinMode(3, INPUT);
  pinMode(4, INPUT);
  //pinMode(sw, INPUT);
  pinMode(button, INPUT);
  pinMode(relay_1, OUTPUT);
  pinMode(relay_2, OUTPUT);
  pinMode(power, OUTPUT);
  pinMode(motor, OUTPUT);
  
  digitalWrite(LED_BUILTIN,1);
  digitalWrite(relay_1,1);
  digitalWrite(relay_2,1);




//*********************EEPROM READ HANDLER****************

 EEPROM.setMemPool(memBase, 1024);

  addressInt = EEPROM.getAddress(sizeof(int));
 
  
  EEPROM.setMaxAllowedWrites(1024);
  delay(100);

  int eep_position=addressInt;
  
  //for(int i=0; i<1024;i++){
  //EEPROM.write(i,0);
  //}

 for(int i=0; i<35;i++){
   for(int j=0;j<3;j++){
    timePosArray[i][j] = EEPROM.readInt(eep_position);
    eep_position=eep_position+2;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("EEPROM READ");
    lcd.setCursor(9, 2);
    lcd.print(eep_position);
    lcd.setCursor(9, 3);
    lcd.print(timePosArray[i][j]);
    }

    delay(10);

  }

  azimuth_min = timePosArray[33][0]; 
  azimuth_max = timePosArray[33][1];
  servo_min = timePosArray[33][2];
  servo_max = timePosArray[34][0];
  work_mode = timePosArray[34][1];       // data of work mode 0 stop, 1 time table, 2 full_auto
}

/**************************************************TABLE_HANDLER************************************************/

void table_write()
{
 
  int eep_pos=addressInt;

  for (int row = 0;row < 35;row++){
    for (int column = 0;column < 3;column++)
    {   
      int write = timePosArray[row][column];
      EEPROM.writeInt(eep_pos, write);  
      eep_pos=eep_pos+2;
    }
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("EEPROM SAVED");
  delay(1500);
}
//**********************************************TABLE_MODE^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^/
int table_mode() 
{ 
  int tableTimePosArray[30][3];
  int eep_position = 120;
  for(int i=0; i<30;i++){
   for(int j=0;j<3;j++){
    tableTimePosArray[i][j] = timePosArray[i][j];
    eep_position++;
   }
  }
  int now_min = minute();
  int now_hour = hour();
  int timeToProces = (now_hour * 100) + now_min;
  int oldtable_convert_time =  1000;
  int result = 0;

  for (int i = 0; i<18; i++){

     
      int table_convert_time = (tableTimePosArray[i][1] * 100) + timePosArray[i][2];

      
      table_convert_time = timeToProces - table_convert_time;
      table_convert_time = abs (table_convert_time);

      if (tableTimePosArray[i][0] == 0 and tableTimePosArray[i][1] == 0 and timePosArray[i][2] == 0){
      i = 19;
      }
      if (table_convert_time < oldtable_convert_time) {
        result = tableTimePosArray[i][0];
        oldtable_convert_time = table_convert_time;
      }

      
   
  }
  result = result;
  return result;

}


/********************************************MOTOR SPIN*******************************************************/

void motor_spin(int demandPosition){
  
  digitalWrite(relay_1,1);
  digitalWrite(relay_2,1);
  digitalWrite(motor, 1);
  boolean work = 0;
  
    pot = analogRead(potentiometer);
    int histereza = demandPosition - pot;
    histereza = abs (histereza);
  if (histereza >= servo_histereza){
    boolean dir = 0;
    work = 1;
      digitalWrite(power, 1);
      delay(250);

    if (demandPosition > pot){
      dir = 1;
      digitalWrite(relay_1,1);
      digitalWrite(relay_2,0);
    }
    if (demandPosition < pot) {
      dir = 0;
      digitalWrite(relay_1,0);
      digitalWrite(relay_2,1);
    }
    

    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("MOTOR DIRECTION: ");
    lcd.print(dir);
    lcd.setCursor(1, 2);
    lcd.print("CURRENT:");
    lcd.setCursor(1, 3);
    lcd.print("DESIRABLE:");
    
    analogWrite(motor, speed);
    
    while (work == 1) {
      
      int pot = analogRead(potentiometer);
      lcd.setCursor(12, 2);
      lcd.print(pot, DEC);
      lcd.setCursor(12, 3);
      lcd.print(demandPosition, DEC);

      if (dir == 0 and pot <= demandPosition + brake ){
        analogWrite(motor, brake_speed);
      }
      if (dir == 1 and pot >= demandPosition - brake){
        analogWrite(motor, brake_speed);
      }
      
      if (dir == 0 and pot <= demandPosition ){
        work = 0;

      }
      if (dir == 1 and pot >= demandPosition ){
        work = 0;
      }
    
    }
    digitalWrite(motor, 1);
    delay(250);
    digitalWrite(relay_1,1);
    digitalWrite(relay_2,1);
    delay(10);
    
    pot = analogRead(potentiometer);
    LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
    lcd.begin(20, 4); 
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("POSITION SET");
  }
  digitalWrite(power, 0);
  pot =+ analogRead(potentiometer);
}


/****************************************************************BUTTON_CHECK***************************************/


void button_check() {
if (digitalRead(button) == LOW) {

    if (buttonActive == false) {

      buttonActive = true;
      buttonTimer = millis();

    }

    if ((millis() - buttonTimer > longPressTime) && (longPressActive == false)) {

      longPressActive = true;
      lklik = !lklik;

    }

  } else {

    if (buttonActive == true) {

      if (longPressActive == true) {

        longPressActive = false;

      } else {

        klik = !klik;

      }

      buttonActive = false;

    }

  }
}


/****************************************************LCD_REFRESH**********************************/

void lcd_refresh (int potValue, float temp, long encoderValue, long azimuth, int tpos){

  refresh_counter++;
  if (refresh_counter == 100){
    LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
    lcd.begin(20, 4); 
    delay(50);
    refresh_counter = 0;
  }
  // Get time
  // Print LCD data
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(hour(), DEC);
  lcd.print(":");
  lcd.print(minute(), DEC);
  lcd.print(":");
  lcd.print(second(), DEC);
  lcd.setCursor(13,0);
  lcd.print(temp);
  lcd.print(" C");
  lcd.setCursor(0,1);
  lcd.print("Servo: ");
  lcd.print(potValue);
  lcd.setCursor(11,1);
  lcd.print("Tp: ");
  lcd.print(tpos);
  lcd.setCursor(0, 2);
  lcd.print("MODE: ");

  switch (work_mode){
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

  lcd.setCursor(0, 3);
  lcd.print("AZIMUTH: ");
  lcd.print(azimuth);

}

/*****************************************************   MENU   ************************************************************/

void menu() {
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
  lcd.setCursor(2, 3);
  lcd.print("ADVENCED"); 
  lcd.setCursor(0, 0);
  lcd.print(">");
  encoder = myEnc.read();
  long old_encoder = encoder;
  int menu_count = 0;
  int new_sec=second(), new_min=minute(), new_hour=hour(), new_day=day(), new_month=month(), new_year=year();  
  delay(1500);
  lcd.setCursor(1, 1);
  klik=0;
  lklik=0;
  
  while (klik==0) {

    button_check();
    encoder = myEnc.read();

    if (encoder != old_encoder){
      if ( encoder > old_encoder) {menu_count++; old_encoder = encoder;}
      if ( encoder < old_encoder) {menu_count--; old_encoder = encoder;}
        if ( menu_count < 0) {menu_count = 0;}
        if ( menu_count > 5) {menu_count = 5;}

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
        lcd.setCursor(2, 3);
        lcd.print("ADVENCED"); 
        
        
          switch (menu_count){
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
              lcd.setCursor(13,0);
              lcd.print(">");
            break;

            case 5:
              lcd.setCursor(13,1);
              lcd.print(">");
            break;
          }
      }
          
    }
    
    klik=0;
    lklik=0;
    if (menu_count == 0){
      new_sec=second(), new_min=minute(), new_hour=hour();
      int step = 0;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("WORK SCHEDULE");
      lcd.setCursor(3, 1);
      lcd.print("STEP: ");
      lcd.print(step);
      lcd.setCursor(2, 2);
      lcd.print("POSITION: ");
      lcd.print(timePosArray[step][0], DEC);
      lcd.setCursor(2, 3);
      lcd.print("TIME: ");
      lcd.print(new_hour, DEC);
      lcd.print(":");
      lcd.print(new_min, DEC);
      int new_time=0;
      byte sel=0;  
      klik=0;
      lklik=0;
      while (lklik==0) {
          
          button_check();
          encoder = myEnc.read();
          if (klik == 1) {sel++;}
          if(sel >=5) {sel=0;}

          if (sel == 0) {new_time=step;}
          if (sel == 1) {new_time=timePosArray[step][0];}
          if (sel == 2) {new_time=timePosArray[step][1];}
          if (sel == 3) {new_time=timePosArray[step][2];}

          if (encoder != old_encoder or klik == 1){
            klik=0;
            if ( encoder > old_encoder) {new_time=new_time+1; old_encoder = encoder;}
            if ( encoder < old_encoder) {new_time=new_time-1; old_encoder = encoder;}
            if (new_time < 0) {new_time = 0;}

            if (sel == 0) {
              if (new_time > 35){new_time = 35;}
              step=new_time;
              }
            if (sel == 1) {
              if (new_time > 1000){new_time = 1000;}
              timePosArray[step][0]=new_time;}
            if (sel == 2) {
              if (new_time > 23){new_time = 23;}
              timePosArray[step][1]=new_time;
            }
            if (sel == 3) {
              if (new_time > 59){new_time = 59;}
              timePosArray[step][2]=new_time;
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

            if (sel == 0) {lcd.setCursor(0, 1); lcd.print(">");}
            if (sel == 1) {lcd.setCursor(0, 2); lcd.print(">");}
            if (sel == 2) {lcd.setCursor(0, 3); lcd.print(">"); lcd.setCursor(8, 3); lcd.print(">");}
            if (sel == 3) {lcd.setCursor(0, 3); lcd.print(">"); lcd.setCursor(14, 3); lcd.print("<");}
            if (sel == 4) {lcd.setCursor(13, 1); lcd.print("save");}    
          }
      }

      if (sel == 4){
        
        table_write(); 
      }
      
    } 
    if (menu_count == 1){
      new_sec=second(), new_min=minute(), new_hour=hour(), new_day=day(), new_month=month(), new_year=year();        
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
      int new_time=0;
      byte sel=0;      
      while (lklik==0) {
          klik=0;
          lklik=0;
          button_check();
          encoder = myEnc.read();
          if (klik == 1) {sel++;}
          if(sel >=6) {sel=0;}

          if (sel == 0) {
            new_time=new_hour;
            }
          if (sel == 1) {
            new_time=new_min;
            }
          if (sel == 2) {
            new_time=new_sec;
            }
          if (sel == 3) {
            new_time=new_day;
            }
          if (sel == 4) {
            new_time=new_month;
            }
          if (sel == 5) {new_time=new_year;}

          if (encoder != old_encoder or klik == 1){
            klik=0;
            if ( encoder > old_encoder) {new_time=new_time+1; old_encoder = encoder;}
            if ( encoder < old_encoder) {new_time=new_time-1; old_encoder = encoder;}
            if (new_time < 0) {new_time = 0;}

            if (sel == 0) {
              if (new_time > 23){new_time = 23;}
              new_hour=new_time;
              }
            if (sel == 1) {
              if (new_time > 59){new_time = 59;}
              new_min=new_time;
              }
            if (sel == 2) {
              if (new_time > 59){new_time = 59;}
              new_sec=new_time;
              }
            if (sel == 3) {
              if (new_time > 31){new_time = 31;}
              new_day=new_time;
              }
            if (sel == 4) {
              if (new_time > 12){new_time = 12;}
              new_month=new_time;
              }
            if (sel == 5) {new_year=new_time;}
                  
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

            if (sel == 0) {lcd.setCursor(6, 1); lcd.print("^");}
            if (sel == 1) {lcd.setCursor(9, 1); lcd.print("^");}
            if (sel == 2) {lcd.setCursor(12, 1); lcd.print("^");}
            if (sel == 3) {lcd.setCursor(6, 3); lcd.print("^");}
            if (sel == 4) {lcd.setCursor(9, 3); lcd.print("^");}
            if (sel == 5) {lcd.setCursor(12, 3); lcd.print("^");}
                
          }
      }
      //rtc.set time
      tm.Hour = new_hour;
      tm.Minute = new_min;
      tm.Second = new_sec;
      tm.Day = new_day;
      tm.Month = new_month;
      tm.Year = CalendarYrToTm(new_year); 
      RTC.write(tm); 
      setSyncProvider(RTC.get);

    }
            
    if (menu_count == 2){   
      int new_motor = analogRead(potentiometer);   
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("ACTUAL POSITION:");
      lcd.setCursor(10, 1);
      lcd.print(new_motor, DEC);
      lcd.setCursor(3, 2);
      lcd.print("DEMAND POSITION:");
      lcd.print(new_motor, DEC);

      while (lklik == 0) {
        button_check();
        encoder = myEnc.read();
        if (encoder != old_encoder){
          if ( encoder > old_encoder) {new_motor=new_motor+1; old_encoder = encoder;}
          if ( encoder < old_encoder) {new_motor=new_motor-1; old_encoder = encoder;} 
          lcd.setCursor(8, 3);
          lcd.print(new_motor, DEC);
        }
        if (klik == 1){
          motor_spin(new_motor); 
          klik = 0;
          int new_motor = analogRead(potentiometer);   
          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print("ACTUAL POSITION:");
          lcd.setCursor(10, 1);
          lcd.print(new_motor, DEC);
          lcd.setCursor(3, 2);
          lcd.print("DEMAND POSITION:");
        }       
      }
              
            
    }

    if (menu_count == 3){     
      
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
      
      int new_time=0;
      byte sel=0;  

      while (lklik==0) {
          klik=0;
          lklik=0;
          button_check();
          encoder = myEnc.read();
          if (klik == 1) {sel++;}
          if(sel >=4) {sel=0;}

          if (sel == 0) {
            new_time=servo_min;
            }
          if (sel == 1) {
            new_time=servo_max;
            }
          if (sel == 2) {
            new_time=azimuth_min;
            }
          if (sel == 3) {
            new_time=azimuth_max;
            }
          
          if (encoder != old_encoder or klik == 1){
            klik=0;
            if ( encoder > old_encoder) {new_time=new_time+1; old_encoder = encoder;}
            if ( encoder < old_encoder) {new_time=new_time-1; old_encoder = encoder;}
            if (new_time < 0) {new_time = 0;}

            if (sel == 0) {
              if (new_time > 1000){new_time = 1000;}
              servo_min = new_time;
              }
            if (sel == 1) {
              if (new_time < 500){new_time = 500;}
              if (new_time > 1000){new_time = 1000;}
              servo_max = new_time;
              }
            if (sel == 2) {
              if (new_time > 255){new_time = 255;}
              azimuth_min = new_time;
              }
            if (sel == 3) {
              if (new_time > 510){new_time = 510;}
              if (new_time < 155){new_time = 155;}
              azimuth_max  = new_time;
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

            if (sel == 0) {lcd.setCursor(0, 0); lcd.print(">");}
            if (sel == 1) {lcd.setCursor(0, 1); lcd.print(">");}
            if (sel == 2) {lcd.setCursor(0, 2); lcd.print(">");}
            if (sel == 3) {lcd.setCursor(0, 3); lcd.print(">");}
                
          }
              
            
        }

      timePosArray[33][0] = azimuth_min;
      timePosArray[33][1] = azimuth_max;
      timePosArray[33][2] = servo_min;
      timePosArray[34][0] = servo_max;
      table_write();

  }

  if (menu_count == 4){
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("MAX TEMP:");
      lcd.setCursor(11, 1);
      lcd.print("give a varible idiot");
      delay(5600);


  }

  if (menu_count == 5){     
      int work_menu = timePosArray[34][1];
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("Work mode: ");
      lcd.print(work_menu);
      lcd.setCursor(2, 0);
      switch (work_menu){
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
      while (lklik==0) {
          klik=0;
          lklik=0;
          button_check();
          encoder = myEnc.read();
          
          if (encoder != old_encoder or klik == 1){
            klik=0;
            if ( encoder > old_encoder) {work_menu=work_menu+1; old_encoder = encoder;}
            if ( encoder < old_encoder) {work_menu=work_menu-1; old_encoder = encoder;}
            if ( work_menu < 0) { work_menu = 0;}
            if ( work_menu > 2) { work_menu = 2;}
              
            lcd.clear();
            lcd.setCursor(1, 0);
            lcd.print("Work mode: ");
            lcd.print(work_menu);
            lcd.setCursor(0, 2);
            switch (work_menu){
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
            
                
          }
              
            
        }

   
      timePosArray[34][1] = work_menu;
      work_mode = work_menu;
      table_write();

  }

  
  klik=0;
  lklik=0;
}

/************************************************************LOOP************************************************************/
void loop() {
  
  
  SolarPosition_t pos = my_position.getSolarPosition();
  int azimuth = pos.azimuth;

  int work_table = table_mode();
  int position = 0; 
  for (int i=0; i <= 9; i++){
     position =+ analogRead(potentiometer);
   }
   position = position / 10;

  if (work_mode == 1){

    work_table = table_mode();
    //work_table = map(work_table, 0, 1024, servo_min, servo_max);
    if (work_table != position);
    motor_spin(work_table);

  }
  
  if (work_mode == 2){
      // Calculate to servo pos
      automatic_mode = map(azimuth, azimuth_min, azimuth_max, servo_min, servo_max);

      if (automatic_mode <= servo_min) {
        automatic_mode = servo_min;
      }
      if (automatic_mode >= servo_max) {
        automatic_mode = servo_max;
      }
      //if difrent spin motor
      if (automatic_mode != position){
        motor_spin(automatic_mode);
        old_automatic_mode = automatic_mode;
      }
  }
  
   // Requests sensor for measurement
  sensors.request(sensorAddress);
  // Waiting (block the program) for measurement reesults
  while(!sensors.available());
  
  // Reads the temperature from sensor
  temperature = sensors.readTemperature(sensorAddress);

  // Calculate azimuth
  
  // Reads encoder
  encoder = myEnc.read();

  if (old_sec != second()){
    lcd_refresh(pot, temperature, encoder, azimuth, work_table);
    old_sec = second();
  }

  button_check();
  
  if (lklik == 1){
    lklik=0;
    menu();
  }
  
}

