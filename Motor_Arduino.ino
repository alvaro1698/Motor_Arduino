#include <DueTimer.h>
#include <math.h>
#define MASTER_CLOCK 84000000

uint32_t clock_a = 42000000; // Sampling frequency in Hz
uint32_t p = 2100;
uint32_t d1;
uint32_t d5;

const float redr = 9.66667;
const float r1vg = 3608;
const float r1vp = 464; 

float v = 1; //Tensión inicial

int pinPWH1 = 42; //IN1A (reductora grande) //IN2A (reductora pequeña)
int pinPWH5 = 44; //IN2A (reductora grande) //IN1A (reductora pequeña)
int pinIntA = 6; //amarillo
int pinIntB = 7; //blanco
int pinEn = 2; 
int CLOCK_WISE=0;
int OTHER_WISE=0;
int cuenta = 0;
int currentState;
int prevState;

int pulse_muestras[1201];
int pos = 0;
int timer = 0;

int imprimir = 0; //flag imprimir

//Diseño
float r = 2*M_PI;
float y = 0;
float kp = 35;
float km = 1556.4;
float e;
float u;



//int pr = 1; //prueba

void SetPin(uint8_t pin)
{
  PIO_Configure(g_APinDescription[pin].pPort,
                PIO_PERIPH_B, 
                g_APinDescription[pin].ulPin,
                g_APinDescription[pin].ulPinConfiguration);

}

void SetVoltaje(float v){
  float vin = v;
  
  if (vin >= 12){
    vin = 12;
  }else if (vin <= -12){
    vin=-12;
  }

  if (vin>0 && vin<=12){ //SetDirection
    CLOCK_WISE=1;
    OTHER_WISE=0;
  }else if (vin<0 && vin>=-12){
    CLOCK_WISE=0;
    OTHER_WISE=1;
  }

  float vi = abs(vin); //DutyCycle

    if (CLOCK_WISE==1){
      d1 = (p*vi)/12;
      d5 = 0;
    }else if (OTHER_WISE==1){
      d1 = 0;
      d5 = (p*vi)/12;
    }

    PWMC_SetDutyCycle(PWM, 1, d1); // Channel: 1, Duty cycle: 50 %
    PWMC_SetDutyCycle(PWM, 5, d5); // Channel: 5, Duty cycle: d5
  
}

void Encoder(){
  pinMode(pinIntA, INPUT);
  pinMode(pinIntB, INPUT);

   if (digitalRead(pinIntA) == 1 && digitalRead(pinIntB) == 1){
   prevState=1;
 }
 else if(digitalRead(pinIntA) == 0 && digitalRead(pinIntB) == 1){
   prevState=2;
 }
 else if(digitalRead(pinIntA) == 0 && digitalRead(pinIntB) == 0){
   prevState=3;
 }
 else if(digitalRead(pinIntA) == 1 && digitalRead(pinIntB) == 0){
   prevState=4;
 }

  attachInterrupt(digitalPinToInterrupt(pinIntA), Lecture, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinIntB), Lecture, CHANGE);
}

void Lecture(){

 if (digitalRead(pinIntA) == 1 && digitalRead(pinIntB) == 1){
   currentState=1;
 }
 else if(digitalRead(pinIntA) == 0 && digitalRead(pinIntB) == 1){
   currentState=2;
 }
 else if(digitalRead(pinIntA) == 0 && digitalRead(pinIntB) == 0){
   currentState=3;
 }
 else if(digitalRead(pinIntA) == 1 && digitalRead(pinIntB) == 0){
   currentState=4;
 }


 if(currentState == 1 && prevState == 4){
   cuenta++;
 }else if(currentState == 2 && prevState == 1){
   cuenta++;
 }else if(currentState == 3 && prevState == 2){
   cuenta++;
 }else if(currentState == 4 && prevState == 3){
   cuenta++;
 }

 if(currentState == 1 && prevState == 2){
   cuenta--;
 }else  if(currentState == 2 && prevState == 3){
   cuenta--;
 }else if(currentState == 3 && prevState == 4){
   cuenta--;
 }else if(currentState == 4 && prevState == 1){
   cuenta--;
 }

 prevState=currentState;

}

// void Muestras() { //Modelado
//   // Interpin();
//   // SetVoltaje(6);

//   timer++;
//   if (timer<600) {
//     SetVoltaje(v);
//     pulse_muestras[pos] = cuenta;
//   }else if (timer >= 600 || timer <= 1200){
//     SetVoltaje(0);
//     pulse_muestras[pos] = cuenta;
//   }

//   pos ++;

//   if (timer >= 1201) {
//     SetVoltaje(0);
//     Timer3.stop();
//     pos = 0;
//     timer = 0;
//     imprimir = 1;
//   }
// }

void MuestrasError() { //Modelado

  SetError(kp);

}

// void Interpin() {
//    if (digitalRead(pr) == 1) {
//       digitalWrite(pr, LOW);
//     }else if (digitalRead(pr) == 0) {
//       digitalWrite(pr, HIGH);
//     }
// }

void SetError(float kp){
  y = (cuenta*2*M_PI)/r1vp;
  e = r-y;
  u = kp*e;
  SetVoltaje(u);
}

void setup(){
  SetPin(pinPWH1);// PWMH1
  SetPin(pinPWH5); // PWMH5
  SetPin(pinEn); // Enable
  digitalWrite(pinEn, HIGH);
  Serial.begin(115200);
  Encoder(); //Inicialización encoder

  pmc_enable_periph_clk(PWM_INTERFACE_ID);
  PWMC_ConfigureClocks(clock_a, 0, MASTER_CLOCK); // clock_b = 0

 //Channel1

  PWMC_ConfigureChannelExt(PWM,
                           1, // Channel: 1          
                           PWM_CMR_CPRE_CLKA, // Prescaler: use CLOCK_A
                           0, // Aligment: period is left aligned
                           0, // Polarity: output waveform starts at a low level
                           0, // Counter event: occurs at the end of the period
                           PWM_CMR_DTE, // Dead time generator is enabled
                           0, // Dead time PWMH output is not inverted    
                           0);  // Dead time PWML output is not inverted
  PWMC_SetPeriod(PWM, 1, p); // Channel: 1, Period: 1/(2100/42 Mhz) = ~20 kHz
  PWMC_SetDutyCycle(PWM, 1, 0); // Channel: 1, Duty cycle: 50 %
  //PWMC_SetDeadTime(PWM, 1, 42, 42); // Channel: 1, Rising and falling edge dead time: 42/42 Mhz = 1 us
  PWMC_EnableChannel(PWM, 1); // Channel: 1


  //Channel5

  PWMC_ConfigureChannelExt(PWM,
                           5, // Channel: 5          
                           PWM_CMR_CPRE_CLKA, // Prescaler: use CLOCK_A
                           0, // Aligment: period is left aligned
                           0, // Polarity: output waveform starts at a low level
                           0, // Counter event: occurs at the end of the period
                           PWM_CMR_DTE, // Dead time generator is enabled
                           0, // Dead time PWMH output is not inverted    
                           0);  // Dead time PWML output is not inverted
  PWMC_SetPeriod(PWM, 5, p); // Channel: 5, Period: 1/(2100/42 Mhz) = ~20 kHz
  PWMC_SetDutyCycle(PWM, 5, 0); // Channel: 5, Duty cycle: d5
  //PWMC_SetDeadTime(PWM, 5, 42, 42); // Channel: 5, Rising and falling edge dead time: 42/42 Mhz = 1 us
  PWMC_EnableChannel(PWM, 5); // Channel: 5

  // SetError(kp);
  
  //Timer muestras CPR

  // pinMode(pr, OUTPUT);
  // Timer3.attachInterrupt(Muestras); //Configuración del Timer3 llamando a la función 
  // Timer3.start(1000); //Establecimiento del Timer3 a 1 s

  //Timer radianes

  Timer3.attachInterrupt(MuestrasError); //Configuración del Timer3 llamando a la función 
  Timer3.start(1000); //Establecimiento del Timer3 a 1 s
  //SetError(kp);
}

void loop(){

  while(e>0){
    Serial.println(y);
  }


  // Serial.print(r);
  // Serial.print(';');
  // Serial.print(y);
  // Serial.print(';');
  // Serial.print(e);
  // Serial.print(';');
  // Serial.println(u);


  // Serial.println(cuenta);

//  if(imprimir == 1) {

//    for (int i=0; i<=1200; i++) {
//      Serial.println(pulse_muestras[i]);
//    }
//  }
//  imprimir = 0; 
}
