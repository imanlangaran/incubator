#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "AHT20.h"
#include "RTClib.h"

#define buzzer 2
#define btn_up 5
#define btn_ok 6
#define btn_dn 7
#define fan 12
#define heat 11
#define steam 10
#define motor 9

#define msw_up 3
#define msw_dn 4


LiquidCrystal_I2C lcd(0x27, 16, 2);
AHT20 aht;
RTC_DS1307 rtc;

enum mode{main, menu} mode = main;

enum menuMode{t,h_min,h_max,rot_time,d_cntr,rot_now} menuMode = t;

enum rotate_dir{up,dn}rotate_dir = up;


byte temp_min = 30;
byte temp_max = 33;
byte hum_min = 40;
byte hum_max = 45;
byte rotation_time = 6;
byte day_counter = 1;

float hum , temp;

long end_buzz, steam_on, fan_off, heat_on;
bool buzzing = false;
bool change_disp = true;
long last_mili, now;

DateTime time;
DateTime next_rot_time;

void setup() {  
//  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  rtc.begin();

  aht.begin();
  
  time = rtc.now();
  next_rot_time = time + TimeSpan(0,rotation_time,0,0);
    
  pinMode(btn_up, INPUT_PULLUP);
  pinMode(btn_ok, INPUT_PULLUP);
  pinMode(btn_dn, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  pinMode(fan, OUTPUT);
  pinMode(heat, OUTPUT);
  pinMode(steam, OUTPUT);
  pinMode(motor, OUTPUT);
  
  digitalWrite(buzzer,0);
  digitalWrite(fan,0);
  digitalWrite(heat,0);
  digitalWrite(steam,0);
  digitalWrite(motor,0);

  pinMode(msw_up, INPUT_PULLUP);
  pinMode(msw_dn, INPUT_PULLUP);
}





void loop() {

  

  now = millis();
  if(now-last_mili > 1000){
    last_mili = now;
    time = rtc.now();
    aht.getSensor(&hum, &temp);
    hum = hum * 100;
    change_disp = true;
  }


  if(time >= next_rot_time){
    rotate();
  }


  if(hum < hum_min && steam_on == 0){
    digitalWrite(fan,1);
    steam_on = now + voltage_delay;
  }

  if(now > steam_on && steam_on != 0){
    digitalWrite(steam,1); 
    steam_on=0;
  }

  if(hum > hum_max){
    digitalWrite(steam,0);
    fan_off = now + voltage_delay;
  }

  if(temp < temp_min && heat_on == 0){
    digitalWrite(fan,1);
    heat_on = now + voltage_delay;
  }

  if(now > heat_on && heat_on != 0){
    digitalWrite(heat,1); 
    heat_on=0;
  }

  if(temp > temp_max){
    digitalWrite(heat,0);
    fan_off = now + voltage_delay;
  }

  if(now > fan_off && fan_off != 0){
    digitalWrite(fan,0); 
    fan_off = 0;
  }


  if(buzzing){
    long now = millis();
    if(now > end_buzz){
      digitalWrite(buzzer,0);
      buzzing = false;
    }
  }


  switch(mode){
    case menu:
      menu_handle();
      break;
    case main:
      main_handler();
      break;
    default:
      break;
  }
  


  if(digitalRead(btn_ok)==LOW){
    delay(20);
    while(digitalRead(btn_ok)==LOW);
    change_disp = true;
    if(mode == main){ 
      mode = menu;
      buzz(true);
    }
    else if(mode == menu){
      if(menuMode == t_min){
        menuMode = t_max;
        if(temp_min > temp_max) temp_max = temp_min+1;
        buzz(false);
      }
      else if(menuMode==t_max){
        menuMode = h_min;
        buzz(false);
      }
      else if(menuMode==h_min){
        menuMode = h_max;
        if(hum_min > hum_max) hum_max = hum_min+1;
        buzz(false);
      }
      else if(menuMode==h_max){
        menuMode = rot_time;
        buzz(false);
      }
      else if(menuMode==rot_time){
        next_rot_time = rtc.now()+TimeSpan(0,rotation_time,0,0);
        menuMode = d_cntr;
        buzz(false);
      }
      else if(menuMode==d_cntr){
        menuMode = rot_now;
        buzz(false);
      }
      else if(menuMode==rot_now){
        menuMode = t_min;
        mode = main;
        buzz(true);
      }
    }
    
  }

  if(digitalRead(btn_dn)==LOW && mode == menu){
    delay(20);
    while(digitalRead(btn_dn)==LOW);
    change_disp = true;
    switch(menuMode){
      case t_min:
        if(temp_min > 30){ temp_min--; buzz(false);}
        else buzz_err();
        break;
        
      case t_max:
        if(temp_max > temp_min+1){ temp_max--; buzz(false);}
        else buzz_err();
        break;

      case h_min:
        if(hum_min > 40){ hum_min--; buzz(false);}
        else buzz_err();
        break;

      case h_max:
        if(hum_max > hum_min+1){ hum_max--; buzz(false);}
        else buzz_err();
        break;

      case rot_time:
        if(rotation_time > 1){ rotation_time--; buzz(false);}
        else buzz_err();
        break;

      case d_cntr:
        if(day_counter > 0){ day_counter--; buzz(false);}
        else buzz_err();
        break;
      default:break;
    }
    
  }

  if(digitalRead(btn_up)==LOW && mode == menu){
    delay(20);
    while(digitalRead(btn_up)==LOW);
    change_disp = true;
    switch(menuMode){
      case t_min:
        if(temp_min < 45){ temp_min++; buzz(false);}
        else buzz_err();
        break;
        
      case t_max:
        if(temp_max < 47){ temp_max++; buzz(false);}
        else buzz_err();
        break;

      case h_min:
        if(hum_min < 90){ hum_min++; buzz(false);}
        else buzz_err();
        break;

      case h_max:
        if(hum_max < 95){ hum_max++; buzz(false);}
        else buzz_err();
        break;

      case rot_time:
        if(rotation_time < 9){ rotation_time++; buzz(false);}
        else buzz_err();
        break;

      case d_cntr:
        if(day_counter < 255){ day_counter++; buzz(false);}
        else buzz_err();
        break;
        
      case rot_now:
        rotate();
        break;
      default:break;
    }
    
  }


}

void buzz(bool lng){
  if(!buzzing){
    if(lng)
      end_buzz = millis() + 500;
    else
      end_buzz = millis() + 100;

    digitalWrite(buzzer,1);
    buzzing = true;
  }
}



byte h,m,s;

void main_handler(){
  if(change_disp){

    lcd.clear();
    lcd.home();
    lcd.print(time.month());
    lcd.print("/");
    lcd.print(time.day());

    lcd.print(" ");
    lcd.print(temp,1);
    lcd.print((char)223);
    lcd.print("C");
    lcd.print(" ");
    lcd.print(hum,0);
    lcd.print("%");

    h = time.hour();
    m = time.minute();
    s = time.second();
    
    lcd.setCursor(0,1);
    if(h < 10) lcd.print("0");
    lcd.print(h);
    lcd.print(":");
    if(m < 10) lcd.print("0");
    lcd.print(m);
    lcd.print(":");
    if(s < 10) lcd.print("0");
    lcd.print(s);

    lcd.print(" ");
    if(day_counter < 10) lcd.print(" ");
    lcd.print(day_counter);
    lcd.print(" ");
    lcd.print("day");
    if(day_counter > 1) lcd.print("s");
    
    change_disp = false;
  }
}

void menu_handle(){
  if(change_disp){
    switch(menuMode){
      case t_min:
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("min temp :");
        lcd.setCursor(0,1);
        lcd.print(temp_min);
        lcd.print(" ");
        lcd.print((char)223);
        lcd.print("C");
        break;
        
      case t_max:
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("max temp :");
        lcd.setCursor(0,1);
        lcd.print(temp_max);
        lcd.print(" ");
        lcd.print((char)223);
        lcd.print("C");
        break;
  
      case h_min:
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("min humidity :");
        lcd.setCursor(0,1);
        lcd.print(hum_min);
        lcd.print(" %");
        break;
  
      case h_max:
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("max humidity :");
        lcd.setCursor(0,1);
        lcd.print(hum_max);
        lcd.print(" %");
        break;
  
       case rot_time:
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("rotation time :");
        lcd.setCursor(0,1);
        lcd.print(rotation_time);
        lcd.print(" hour");
        break;

      case d_cntr:
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("day counter :");
        lcd.setCursor(0,1);
        lcd.print(day_counter);
        if(day_counter < 10) lcd.print(" ");
        lcd.print(" day");
        if(day_counter > 1) lcd.print("s");
        break;

     case rot_now:
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("press up to");
        lcd.setCursor(0,1);
        lcd.print("rotate now");
        
        break;
  
      default: break;
    }
  change_disp = false;
  }
}



void rotate(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("rotating");
  lcd.setCursor(0,1);
  lcd.print("Please Wait...");
  digitalWrite(buzzer,1);
  delay(100);
  digitalWrite(buzzer,0);

  digitalWrite(motor,1);

  if(rotate_dir == up){
    while(digitalRead(msw_up) == 1);
    rotate_dir = dn;
  }
  else{
    while(digitalRead(msw_dn) == 1);
    rotate_dir = up;
  }

  digitalWrite(motor,0);
  
  next_rot_time = rtc.now()+TimeSpan(0,rotation_time,0,0);
  
}


void buzz_err(){
  byte t = 60;
  digitalWrite(buzzer,1);
  delay(t);
  digitalWrite(buzzer,0);
  delay(t);
  digitalWrite(buzzer,1);
  delay(t);
  digitalWrite(buzzer,0);
  delay(t);
  digitalWrite(buzzer,1);
  delay(t);
  digitalWrite(buzzer,0);
  delay(t);
  digitalWrite(buzzer,1);
  delay(t);
  digitalWrite(buzzer,0);
  delay(t);
}
