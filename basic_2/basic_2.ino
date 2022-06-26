// main code

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"
#include "AHT20.h"

#define buzzer 2
#define heat 3
#define msw 4
#define btn_up  5
#define btn_ok  6
#define btn_dn  7
#define motor 8
#define steam 9
#define fan 10



LiquidCrystal_I2C lcd(0x27, 16, 2);
AHT20 aht;
RTC_DS1307 rtc;



bool D5_state = 1;
bool D6_state = 1;
bool D7_state = 1;
long prev_isr_timeD4, prev_isr_timeD5, prev_isr_timeD6;


enum bz_length{l100,l500} bz_length;
long end_bz;
bool bz = false;
bool change_disp = true;

DateTime time, last_btn_time, next_rot_time;

enum mode{main, menu, slp, mdl, rotate} mode = main;
enum menuMode{t,h,rot_time,d_cntr,middle,rot_now} menuMode = t;



float set_temp = 32.0;
byte set_humi = 75;
float env_temp;
float env_humi;
byte rotation_time = 6;
byte day_counter = 1;
byte hh,mm,ss;
long last_mili, now;
byte second_counter = 0;

bool motor_stt = true; // off
bool steam_stt = true; // off
long fan_millis;

float kp=3.5;         //Mine was 2.5
float ki=0.04;         //Mine was 0.06
float kd=1.0;         //Mine was 0.8
float PID_p, PID_i, PID_d, PID_total;
float now_pid_error, prev_pid_error;

byte mdl_state = 0;

byte pwm_cntr = 0;
long lastmlspwm;

void setup() {
  
  Serial.begin(9600);

  pinMode(msw, INPUT_PULLUP);
  pinMode(btn_up, INPUT_PULLUP); 
  pinMode(btn_dn, INPUT_PULLUP); 
  pinMode(btn_ok, INPUT_PULLUP); 
  PCICR |= B00000100;
  PCMSK2 |= B11100000;

  
  pinMode(buzzer, OUTPUT);
  pinMode(motor, OUTPUT);
  pinMode(steam, OUTPUT);
  pinMode(fan, OUTPUT);

//  TCCR2B = TCCR2B & B11111000 | B00000111;
  pinMode(heat, OUTPUT);


  digitalWrite(buzzer,0);
  digitalWrite(motor,1);
  digitalWrite(steam,1); // solid relay  // off
  digitalWrite(heat,1);


  last_btn_time = time;


  
  lcd.init();
  lcd.backlight();


  
  rtc.begin();
  aht.begin();
  time = rtc.now();

  next_rot_time = time + TimeSpan(0,rotation_time,0,0);
  
}

void loop() {

  now = millis();
  if(now-last_mili > 100){
    last_mili = now;
    aht.getSensor(&env_humi, &env_temp);
    env_humi = env_humi * 100;
    second_counter++;
//    PID_control();
  }
  
//  if(now - lastmlspwm > 10){
//    pwm_cntr++;
//    lastmlspwm = now;
//  }

  if(second_counter >= 10 && mode == main){
    change_disp = true;
    second_counter = 0;
    time = rtc.now();
  }

  if(time > next_rot_time && mdl_state == 0){
    mode = rotate;
    next_rot_time = time + TimeSpan(0,rotation_time,0,0);
    lcd.clear();
    lcd.print("rotating");
  }

  if(env_humi < (set_humi-0.1) && steam_stt){
    digitalWrite(steam,0); // on
    digitalWrite(fan,0);
    steam_stt = false;
  }
  else if(env_humi > (set_humi+0.1)){
    if(!steam_stt){
      steam_stt = true;
      digitalWrite(steam,1); // off
      fan_millis = millis() + 10000;
    }

    if(millis() > fan_millis){
      digitalWrite(fan,1); // off
    }
  }

  

  

  switch(mode){
    case menu:
      menu_handle();
      break;
    case main:
      main_handler();
      break;
    case rotate:
      rotate_handler();
      break;
    default:
      break;
  }



//  if(set_temp - env_temp > 5){
//    digitalWrite(heat,0);
//  }
//  else{
//    if(pwm_cntr < PID_total){
//      digitalWrite(heat,1);
//    }
//    else if(pwm_cntr < 101){
//      digitalWrite(heat,0);
//    }
//  }
//  if(pwm_cntr >= 101){
//      pwm_cntr = 0;
//  }

  if(env_temp < (set_temp-0.25)){
    digitalWrite(heat,0);
  }
  else if(env_temp > (set_temp-0.15)){
    digitalWrite(heat,1);
  }



  

  check_bz();
}



void PID_control(){
  now_pid_error = set_temp - env_temp;

  PID_p = kp * now_pid_error;
  PID_d = kd*((now_pid_error - prev_pid_error)/100);

  if(-3 < now_pid_error && now_pid_error < 3){
      PID_i = PID_i + (ki * now_pid_error);}
  else{PID_i = 0;}

  PID_total = PID_p + PID_i + PID_d;  
  PID_total = map(PID_total, 0, 150, 0, 100);

  if(PID_total < 0){PID_total = 0;}
  if(PID_total > 100) {PID_total = 100; } 


  PID_total = 100 - PID_total;


  prev_pid_error = now_pid_error;
}




void PID_control1(){
  now_pid_error = set_temp - env_temp;

  PID_p = kp * now_pid_error;
  PID_d = kd*((now_pid_error - prev_pid_error)/100);

  if(-3 < now_pid_error && now_pid_error < 3){
      PID_i = PID_i + (ki * now_pid_error);}
  else{PID_i = 0;}

  PID_total = PID_p + PID_i + PID_d;  
  PID_total = map(PID_total, 0, 150, 0, 255);

  if(PID_total < 0){PID_total = 0;}
  if(PID_total > 255) {PID_total = 255; } 

//  analogWrite(heat, 255-PID_total);
//  Serial.println(PID_total);

  prev_pid_error = now_pid_error;
}







//bool msw_state = false;
//void rotate_handler1(){
//  digitalWrite(motor,0);
//  if(digitalRead(msw) == 0 && msw_state == false){
//    msw_state = true;
//    msw_millis = millis() + 20;
//  }
//
//  if(msw_state == true && digitalRead(msw) == 1 && millis() >= msw_millis){
//    msw_state = false;
//    digitalWrite(motor,1);
//    mode = main;
//  }
//}




long msw_millis;
bool msw_state = false; // true=>pressed - false=>released
bool use_msw_millis = false;

void rotate_handler(){
  digitalWrite(motor,0);
  if(msw_state){
    if(digitalRead(msw) == 1 && !use_msw_millis){
      use_msw_millis = true;
      msw_millis = millis() + 20;
    }

    if(millis() > msw_millis && use_msw_millis){
      use_msw_millis = false;
      msw_state = false;
    }
  }

  else{
    if(digitalRead(msw) == 0 && !use_msw_millis){
//      digitalWrite(motor,0);
      use_msw_millis = true;
      msw_millis = millis() + 50;
    }

    if(millis() > msw_millis && use_msw_millis){
      digitalWrite(motor,1);
      use_msw_millis = false;
      msw_state = true;
      mode = main;
    }
    
  }
}

//void rotate_handler1(){ //main one
//  digitalWrite(motor,0);
//
//  if(msw_state && digitalRead(msw) == 1){
//    msw_millis = millis() + 20;
//    use_msw_millis = true;
//  }
//  if(msw_state && millis() >= msw_millis && use_msw_millis){
//    msw_state = false;
//    use_msw_millis = false;
//  }
//  
//  if(digitalRead(msw) == 0 && !msw_state){
//    msw_millis = millis() + 20;
//    use_msw_millis = true;
//  }
//
//  if(millis() >= msw_millis && !msw_state && use_msw_millis){
//    digitalWrite(motor,1);
//    mode = main;
//    msw_state = true;
//    use_msw_millis = false;
//  }
//}

void check_bz(){
  if(bz){
    digitalWrite(buzzer,1);
    bz = false;
    switch(bz_length){
      case l100:
        end_bz = millis()+50;
        break;
      case l500:
        end_bz = millis()+300;
        break;
        
      default:
        break;
    }
  }

  if(millis() > end_bz){
    digitalWrite(buzzer,0);
  }
}



void main_handler1(){
  if(change_disp){

    lcd.clear();
    lcd.home();

    lcd.print(PID_total);
    lcd.print(" - ");
    lcd.print(pwm_cntr);
    
    change_disp = false;
    lcd.display();
  }
}


void main_handler(){
  if(change_disp){

    lcd.clear();
    lcd.home();
    
    lcd.print(env_temp,2);
    lcd.print((char)223);
    lcd.print("C");
    lcd.print("   ");
    lcd.print(env_humi,2);
    lcd.print("%");

    hh = time.hour();
    mm = time.minute();
    ss = time.second();
    
    lcd.setCursor(0,1);
    if(hh < 10) lcd.print("0");
    lcd.print(hh);
    lcd.print(":");
    if(mm < 10) lcd.print("0");
    lcd.print(mm);

    lcd.print("    ");
    if(day_counter < 10) lcd.print(" ");
    lcd.print(day_counter);
    lcd.print(" ");
    lcd.print("day");
    if(day_counter > 1) lcd.print("s");
    
    change_disp = false;
    lcd.display();
  }
}

void menu_handle(){
  if(change_disp){
    switch(menuMode){
      case t:
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("temp :");
        lcd.setCursor(0,1);
        lcd.print(set_temp,1);
        lcd.print(" ");
        lcd.print((char)223);
        lcd.print("C");
        break;
        
      
  
      case h:
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("humidity :");
        lcd.setCursor(0,1);
        lcd.print(set_humi);
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

      case middle:
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("press up to go");
        lcd.setCursor(0,1);
        lcd.print("on middle");
        
        break;
  
      default: break;
    }
  change_disp = false;
  }
}

void btn(byte b){
//  last_btn_time = rtc.now() + TimeSpan(0,0,0,10);
//  last_btn_time = time + TimeSpan(0,0,0,10);
//  last_btn_time = time;
  
  
  if(b == btn_ok){
    change_disp = true;
    if(mode == main){ 
      mode = menu;
      bz = true;
      bz_length = l500;
    }
    else if(mode == menu){
      if(menuMode == t){
        menuMode = h;
        bz = true;
        bz_length = l100;
      }
      else if(menuMode==h){
        menuMode = rot_time;
        bz = true;
        bz_length = l100;
      }
      else if(menuMode==rot_time){
        next_rot_time = time + TimeSpan(0,rotation_time,0,0);
        menuMode = d_cntr;
        bz = true;
        bz_length = l100;
      }
      else if(menuMode==d_cntr){
        menuMode = middle;
        bz = true;
        bz_length = l100;
      }
      else if(menuMode==middle){
        motor_stt = true;
        digitalWrite(motor,1);
        menuMode = rot_now;
        bz = true;
        bz_length = l100;
      }
      else if(menuMode==rot_now){
        menuMode = t;
        mode = main;
        bz = true;
        bz_length = l500;
      }
    }

  }
  else if(b == btn_dn && mode == menu){
    change_disp = true;
    switch(menuMode){
      case t:
        if(set_temp > 20){
          set_temp-=0.1; 
          bz = true;
          bz_length = l100;
        }
        break;
        
      case h:
        if(set_humi > 10){
          set_humi--; 
          bz = true;
          bz_length = l100;
        }
        break;

      case rot_time:
        if(rotation_time > 1){
          rotation_time--; 
          bz = true;
          bz_length = l100;
        }
        break;

      case d_cntr:
        if(day_counter > 0){
          day_counter--; 
          bz = true;
          bz_length = l100;
        }
        break;
      default:break;
    }
  }

  else if(b == btn_up && mode == menu){
    change_disp = true;
    switch(menuMode){
      case t:
        if(set_temp < 50){
          set_temp+=0.1;
          bz = true;
          bz_length = l100;
        }
        break;

      case h:
        if(set_humi < 99){
          set_humi++;
          bz = true;
          bz_length = l100;
        }
        break;

      case rot_time:
        if(rotation_time < 24){
          rotation_time++;
          bz = true;
          bz_length = l100;
        } 
        break;

      case d_cntr:
        if(day_counter <= 30){
          day_counter++;
          bz = true;
          bz_length = l100;  
        }
        break;

      case middle:
        motor_stt = !motor_stt;
        digitalWrite(motor,motor_stt);
        mdl_state = 1;
        msw_state = !digitalRead(msw);
        bz = true;
        bz_length = l100;
        break;
        
      case rot_now:
//        rotate();
        next_rot_time = time;
        mdl_state = 0;
        msw_state = !digitalRead(msw);
        btn(btn_ok);
        break;
      default:break;
    }
  }
//  last_btn_time = time + TimeSpan(0,0,0,10);
//  if(mode == slp){
//    change_disp = true;
//    mode = main;
//    isOn = true;
//    lcd.backlight();
//    lcd.home();
//    lcd.print("ok");
//    lcd.display();
//  }
}



ISR (PCINT2_vect) 
{
  cli();  
  //1. Check D5 pin HIGH
  if(PIND & B00100000){ 
    if(D5_state == 0){
      D5_state = 1;
      prev_isr_timeD4 = millis();
    }
      
  }
  else if (D5_state == 1 && (millis() - prev_isr_timeD4 > 20)){
//    Serial.println("d5");
//    bz = true;
//    bz_length = l100;
    btn(btn_up);
    D5_state = 0;
  }


  //2. Check D6 pin HIGH
  if(PIND & B01000000){ 
    if(D6_state == 0){
      D6_state = 1;
      prev_isr_timeD5 = millis();
    }
      
  }
  else if (D6_state == 1 && (millis() - prev_isr_timeD5 > 20)){
//    Serial.println("d6");
//    bz = true;
//    bz_length = l500;
    btn(btn_ok);
    D6_state = 0;
  }




  //3. Check D7 pin HIGH
  if(PIND & B10000000){ 
    if(D7_state == 0){
      D7_state = 1;
      prev_isr_timeD6 = millis();
    }
      
  }
  else if (D7_state == 1 && (millis() - prev_isr_timeD6 > 20)){    
//    Serial.println("d7");
//    bz = true;
//    bz_length = l100;
    btn(btn_dn);
    D7_state = 0;
  }
  sei();

  
} 
