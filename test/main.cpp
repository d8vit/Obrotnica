 #include <Arduino.h>

 #include <Wire.h>                        
 #include <DS1307.h>
 #include <OneWire.h>
 #include <DS18B20.h>
 #include <LiquidCrystal.h>               
 #include <Encoder.h>
 #include <EEPROMex.h>
 #include <SolarPosition.h>

 //#define ENCODER_DO_NOT_USE_INTERRUPTS
 
 const byte ONEWIRE_PIN = 2;
 byte sensorAddress[8] = {0x28, 0xFF, 0x4B, 0x1A, 0x2, 0x17, 0x3, 0xDD};         //Sensor Adress
 OneWire onewire(ONEWIRE_PIN);           // 1-Wire object
 DS18B20 sensors(&onewire);              // DS18B20 sensors object
 
 //    Clock Configuration
 DS1307 rtc;
 uint8_t sec, min, hour, day, month;
 uint16_t year;


  //    LCD Configuration
 const int rs = 12, en = 9, d4 = 11, d5 = 8, d6 = 10, d7 = 7;
 LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

  //    Servo Config

 #define relay_1 A1
 #define relay_2 A2                         
 #define motor 6
 #define power A3
 #define potentiometer A0
 const  int brake = 150;
 const int brake_speed = 150;
 const int speed = 100;

 
 boolean buttonActive = false;
 boolean longPressActive = false;
 long buttonTimer = 0;
 unsigned long longPressTime = 850;
 int button = 5;
//#define sw 5
 Encoder myEnc(3, 4);
 long oldPosition  = -999;
 
boolean klik = 0;
boolean lklik = 0;
boolean state = 0;

int old_sec = 0;
int debug = 0;

int pot = analogRead(potentiometer);

long encoder = myEnc.read();

float temperature = sensors.readTemperature(sensorAddress);

int setPos = 0;
int old_setPos = 0;
int eep_array=10;
byte steps_num = 0;
byte timePosArray[35][3];


void setup() {
 Serial.begin(115200);
 //Lcd config
 lcd.begin(20, 4);                         
 
 Serial.println("Init RTC...");
  //only set the date+time one time
 //rtc.set(0, 15, 22, 23, 07, 2019); //sec, min, hour, day, month, year
  rtc.start();
   SolarPosition::setTimeProvider(rtc.get);
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  
  Serial.println("Init Temp Sensor");
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
 
 Serial.println("Program Startong...");
 int pot = analogRead(potentiometer);
 Serial.print("Potentiometer read: ");
 Serial.println(pot);
 digitalWrite(LED_BUILTIN,1);
 digitalWrite(relay_1,1);
 digitalWrite(relay_2,1);

 int eep_position=0;
 for(int i=0; i<35;i++){
   for(int j=0;j<3;j++){
    timePosArray[i][j] = EEPROM.read(eep_position);
    eep_position++;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("EEPROM READ");
    lcd.setCursor(9, 2);
    lcd.print(eep_position);
    lcd.setCursor(9, 3);
    lcd.print(timePosArray[i][j]);
    delay(10);
    
   }
 }
}

/********************************************MOTOR SPIN*******************************************************/

void motor_spin(int demandPosition){
  digitalWrite(relay_1,1);
  digitalWrite(relay_2,1);
  digitalWrite(LED_BUILTIN,1);
  pot = analogRead(potentiometer);
  boolean dir = 0;
  boolean work = 1;
  if (demandPosition > pot){
    dir = 1;
    digitalWrite(relay_1,1);
    digitalWrite(relay_2,0);
  }
  else if (demandPosition < pot) {
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
      Serial.println("Stop");
    }
    if (dir == 1 and pot >= demandPosition ){
      work = 0;
      Serial.println("Stop");
    }
  
  }
  analogWrite(motor, 255);
  delay(10);
  digitalWrite(relay_1,1);
  digitalWrite(relay_2,1);
  delay(10);
  analogWrite(motor, 0);
  delay(250);
  analogWrite(motor, 255);

  pot = analogRead(potentiometer);
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("POSITION SET");
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

void lcd_refresh (int potValue, float temp, long encoderValue){
   
  // Get time
  rtc.get(&sec, &min, &hour, &day, &month, &year);
  // Print LCD data
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(hour, DEC);
  lcd.print(":");
  lcd.print(min, DEC);
  lcd.print(":");
  lcd.print(sec, DEC);
  lcd.setCursor(13,0);
  lcd.print(temp);
  lcd.print(" C");
  lcd.setCursor(0,1);
  lcd.print("Servo: ");
  lcd.print(potValue);
  lcd.setCursor(0, 2);
  lcd.print("Encoder: ");
  lcd.print(encoderValue);

  debug = EEPROM.read(0);
  lcd.setCursor(0, 3);
  lcd.print("sizeof :");
  lcd.print(debug);
}


/**************************************************TABLE_HANDLER************************************************/

void table_write()
{
  int eep_pos = 0;
  for (int row = 0;row < 35;row++){
    for (int column = 0;column < 3;column++)
    {   
      EEPROM.write(eep_pos, timePosArray[row][column]);
      eep_pos++;
    }
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("EEPROM SAVED");
  delay(1500);
}



/*****************************************************   MENU   ************************************************************/

void menu() {
  rtc.get(&sec, &min, &hour, &day, &month, &year);
  lcd.clear();
  lcd.setCursor(7, 0);
  lcd.print("MENU");
  lcd.setCursor(5, 1);
  lcd.print("SET STEPS");
  lcd.setCursor(5, 2);
  lcd.print("SET TIME");
  lcd.setCursor(5, 3);
  lcd.print("MANUAL MODE");
  lcd.setCursor(1, 1);
  lcd.print(">");
  encoder = myEnc.read();
  long old_encoder = encoder;
  int menu_count = 0;
  int new_sec=sec, new_min=min, new_hour=hour, new_day=day, new_month=month, new_year=year;  
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
      if ( menu_count < 0) {menu_count = 2;}
      if ( menu_count > 2) {menu_count = 0;}
            
      lcd.clear();
      lcd.setCursor(7, 0);
      lcd.print("MENU");
      lcd.print(menu_count);
      lcd.setCursor(5, 1);
      lcd.print("SET STEPS");
      lcd.setCursor(5, 2);
      lcd.print("SET TIME");
      lcd.setCursor(5, 3);
      lcd.print("MANUAL MODE");  
       
        switch (menu_count){
          case 0:
            lcd.setCursor(1, 1);
            lcd.print(">");
          break;

          case 1:
            lcd.setCursor(1, 2);
            lcd.print(">");
          break;
        
          case 2:
            lcd.setCursor(1, 3);
            lcd.print(">");
          break;
        }
    }
        
  }
  
  klik=0;
  lklik=0;
  if (menu_count == 0){
    rtc.get(&sec, &min, &hour, &day, &month, &year);
    new_sec=sec, new_min=min, new_hour=hour;
    int position = analogRead(potentiometer); 
    int step = 0;
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("WORK SCHEDULE");
    lcd.setCursor(3, 1);
    lcd.print("STEP: ");
    lcd.print(step);
    lcd.setCursor(2, 2);
    lcd.print("POSITION: ");
    lcd.print(position);
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
            if (new_time > 254){new_time = 254;}
            timePosArray[step][0]=new_time;}
          if (sel == 2) {
            if (new_time > 24){new_time = 24;}
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
    rtc.get(&sec, &min, &hour, &day, &month, &year);
    new_sec=sec, new_min=min, new_hour=hour, new_day=day, new_month=month, new_year=year;        
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
            if (new_time > 24){new_time = 24;}
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
    rtc.set(new_sec, new_min, new_hour, new_day, new_month, new_year);
          
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

  lklik=0;
  klik=0;

}

/************************************************************LOOP************************************************************/
void loop() {
  
  // Read servo pot value
  pot = analogRead(potentiometer);
  
  // Get time
  rtc.get(&sec, &min, &hour, &day, &month, &year);
  
   // Requests sensor for measurement
  sensors.request(sensorAddress);
  
  // Waiting (block the program) for measurement reesults
  while(!sensors.available());
  
  // Reads the temperature from sensor
  temperature = sensors.readTemperature(sensorAddress);
  // Reads encoder
  encoder = myEnc.read();

  if (old_sec != sec){
    lcd_refresh(pot, temperature, encoder);
    old_sec = sec;
  }

  button_check();
  
  if (lklik == 1){
    lklik=0;
    menu();
  }
  
}

