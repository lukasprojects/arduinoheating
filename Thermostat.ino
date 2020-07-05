#include "SparkFunBME280.h"
BME280 mySensor;
#include <Arduino.h>
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
#include "cactus_io_BME280_I2C.h"
#define heartbeat 17

BME280_I2C bme(0x76);
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
float istTemp, istHum;
int view = 1, sollTemp = 13, sollTemp_old = 0;
bool OK, OKflanke, UP, UPflanke, DO, DOflanke, changeflag=false, set = false, Fenster, heating, Fenst;
float userTemp[15] = {17.0, 17.5, 18.0, 18.5, 19.0, 19.5, 20.0, 20.5, 21.0, 21.5, 22.0, 22.5, 23.0, 23.5, 24.0};
volatile int Timer=0;
volatile bool pressed;


void setup(void) {
  u8g2.begin();
  Wire.begin();
  mySensor.setI2CAddress(0x76);
  pinMode(6, INPUT);
  pinMode(5, INPUT);
  pinMode(4, INPUT);
  pinMode(8, INPUT);
 

  if (mySensor.beginI2C() == false) //Begin communication over I2C
  {
    while (1); //Freeze
  }

noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;

  TCNT1 = 34286;            // preload timer 65536-16MHz/256/2Hz
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
  interrupts();  
  
}

void loop(void) {
  
  
    if(digitalRead(6)==LOW && pressed==false){                    //UP
      pressed=true;
    switch (view){
      case 1:
      if(sollTemp>=0 && sollTemp<14){           //Solltemp nicht über oder unter Max Wert des Arrays
      sollTemp=sollTemp+1;                      //Solltemp erhöhen
      }
      changeflag=true;                          //Flag setzten für anzeige solltemp
      displayTemp(userTemp[sollTemp], changeflag, false, false);    //Anzeige Solltemp
      Timer=0;                                  //Timer zurücksetzten damit solltemp 2 sek angezeigt
      }
     //UPflanke=UP;
    }


    if(digitalRead(4)==LOW && pressed==false){                    //DOWN
      pressed=true;
    switch (view){
      case 1:
      if(sollTemp>0 && sollTemp<=15){           //Solltemp nicht über oder unter Max Wert des Arrays
      sollTemp=sollTemp-1;                      //Solltemp verringern
      }
      changeflag=true;                          //Flag setzten für anzeige solltemp
      displayTemp(userTemp[sollTemp], changeflag,  false, false);    //Anzeige Solltemp
      Timer=0;                                  //Timer zurücksetzten damit solltemp 2 sek angezeigt
    }
    //  DOflanke=DO;
    }


    if(digitalRead(5)==LOW && pressed==false){                    //OK
      pressed=true;
      Timer=4;
      if(view==1){                              //TOGGLE zwischen TEMP und Luftfeuchte
        view=2;
      }
      if(view==2){
        view=1;
      }
    }
  


  if((istTemp+0.2)<userTemp[sollTemp] && digitalRead(8)==HIGH){       //Heizen wenn istTemp 0,5C unter sollTemp
    heating=true;
    Fenster=false;
    digitalWrite(7, HIGH);
  }

   if((istTemp-0.4)>userTemp[sollTemp] || digitalRead(8)==LOW){      //nicht mehr Heizen wenn istTemp 0,5C über sollTemp
    heating=false;
    digitalWrite(7, LOW);
  }

  if(digitalRead(8)==LOW){                                           //Anzeige Fenster,wenn Fenster offen
    Fenster=true;
  }

   if(digitalRead(8)==HIGH){                                           //Anzeige Fenster,wenn Fenster offen
    Fenster=false;
  }

  if(changeflag==true && Timer>5){
    Timer=0;
    changeflag=false;
  }
  

 if(Timer>5 && changeflag==false){        //Anzeigen und auslesen der Temperatur oder der Luftfeuchte alle 10sek
  Timer=0;                                      //Timer Rücksetzten
  istTemp = mySensor.readTempC();           //Sensor Auslesen und Offset Subtrahieren
  istHum = mySensor.readFloatHumidity();
  sendOLED();
  }

}

/*------------------------------------Funktionen------------------------------------------------*/

ISR(TIMER1_OVF_vect){                         //bei 250ms Timer Interrupt hochzählen von Timer
  Timer++;
  pressed=false;
  digitalWrite(heartbeat, digitalRead(heartbeat) ^ 1);
  TCNT1 = 34286;
}



void sendOLED() {                               //Bei Auswahl Temp, Anzeigen der Temperatur 
  switch (view) {
    case 1:
      displayTemp(istTemp, changeflag, Fenster, heating);
      break;
    case 2:
      displayHumidithy(istHum);
      break;
  }
}



void displayTemp(float Temp, bool changeflag, bool Fenster, bool heating) {
  u8g2.firstPage();
  do {
      u8g2.setFont(u8g2_font_logisoso38_tr);
      u8g2.setCursor(0, 60);
      u8g2.print(Temp, 1);
    if (changeflag == true) {
      u8g2.setFont(u8g2_font_t0_11_tf);
      u8g2.setCursor(5, 10);
      u8g2.print("SET Temp:");
    }
    else {
      u8g2.setCursor(100, 60);
      u8g2.print("C");
      u8g2.setFont(u8g2_font_t0_11_tf);
      u8g2.setCursor(95, 20);
      u8g2.print("o");
    }
      u8g2.setFont(u8g2_font_t0_11_tf);
    if (Fenster == true) {
      u8g2.setCursor(5, 10);
      u8g2.print("Fenster");
    }

     if (heating == true) {
      u8g2.setCursor(5, 10);
      u8g2.print("Heizt");
    }
  } while ( u8g2.nextPage() );

}


void displayHumidithy(float Hum) {


  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_logisoso38_tr);
    u8g2.setCursor(5, 60);
    u8g2.print(Hum, 0);
    u8g2.setCursor(70, 55);
    u8g2.print("%rH");
  } while ( u8g2.nextPage() );
}



//D6 D5 D4 Button Pins
//u8g2_font_logisoso34_tf
//u8g2_font_logisoso34_tn



/*ACHTUNG VIEW MUSS NOCH GEÄNDERT UND ZURÜCKGEGEBEN WERDEN
  void displayMenu(){                             //Menu Aufruf nach Longpress OK

  while(OK==HIGH){                        //Menu Anzeige endlos in schleife bis Exit über OK

  switch (view){
  case 1:
  u8g2.firstPage();
  do {
  u8g2.setFont(u8g2_font_logisoso38_tn);
  u8g2.setCursor(0, 55);
  u8g2.print("Temp");
  } while ( u8g2.nextPage() );
  break;

  case 2:
  u8g2.firstPage();
  do {
  u8g2.setFont(u8g2_font_logisoso38_tn);
  u8g2.setCursor(0, 55);
  u8g2.print("Hum");
  } while ( u8g2.nextPage() );
  break;
  }
  }

  }
*/
