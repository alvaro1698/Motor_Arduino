#include <DueTimer.h>
#include <math.h>
#define MASTER_CLOCK 84000000


//Variables y constantes


uint32_t clock_a = 42000000; // Sampling frequency in Hz
uint32_t p = 2100;
uint32_t d1;
uint32_t d5;

//Movimiento por tensión
int pinPWH1 = 42; //IN1A
int pinPWH5 = 44; //IN2A
int pinIntA = 6; //blanco
int pinIntB = 7; //amarillo
int pinEn = 2; 
int CLOCK_WISE=0;
int OTHER_WISE=0;
int cuenta = 0;
int currentState;
int prevState;

int pulse_muestras[1201];
float muestras_angulo[1201];
int pos = 0;
int timer = 0;

int imprimir = 0; //flag imprimir

//Diseño y movimiento por angulo
const float redr = 9.66667; //Relación reductora grande
const float r1vg = 3608;  //Cuentas por vuelta de reductora grande
const float r1vp = 464;  //Cuentas por vuelta de reductora pequeña
float y = 0;
float km = 1556.4;
float e;
float u;

//Valores de control

float Vreal = 1; //Tensión inicial
float v = Vreal; 

float Rreal = M_PI; //Angulo de referencia
float r = Rreal;

float kp = 2.5; //Ganancia sistema

int muestras = 1201;


//Metodos

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

void Muestras() { //Modelado
  // Interpin();
  // SetVoltaje(6);

  timer++;
  if (timer<600) {
    SetVoltaje(v);
    pulse_muestras[pos] = cuenta;
  }else if (timer >= 600 || timer <= 1200){
    SetVoltaje(0);
    pulse_muestras[pos] = cuenta;
  }

  pos ++;

  if (timer >= 1201) {
    SetVoltaje(0);
    Timer3.stop();
    pos = 0;
    timer = 0;
    imprimir = 1;
  }
}

void MuestrasError() { //Modelado

  //SetError(kp);
  timer++;
  if (timer<600) {
    SetError(kp);
    muestras_angulo[pos] = y;
  }else if (timer >= 600 || timer <= 1200){
    SetError(0);
    muestras_angulo[pos] = y;
  }

  pos ++;

  if (timer >= 1201) {
    SetError(0);
    Timer3.stop();
    pos = 0;
    timer = 0;
    imprimir = 1;
  }
}

void SetError(float kp){
  y = (cuenta*2*M_PI)/r1vp; //Dependiendo de la reductora usada, hay que usar o r1vp (reductora pequeña) o r1vg (reductora grande)
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
  
  //1º Control por voltaje (apartado a)
  //Descomentar y comentar ('2º Timer muestras' y '3º Timer radianes') y  poner valor deseado de tensión en Vreal en la sección de valores de control

  //SetVoltaje(v);

  //2º Timer muestras CPR (apartado b)
  //Descomentar y comentar ('1º Timer radianes' y '3º Timer radianes') y poner valor deseado de tensión en Vreal en la sección de valores de control

  // Timer3.attachInterrupt(Muestras); //Configuración del Timer3 llamando a la función 
  // Timer3.start(1000); //Establecimiento del Timer3 a 1 s

  //1º Timer radianes (apartado c)
  //Descomentar y comentar ('1º Timer radianes' y '3º Timer radianes') y poner el angulo de referencia en Rreal en la sección de valores de control

  Timer3.attachInterrupt(MuestrasError); //Configuración del Timer3 llamando a la función 
  Timer3.start(1000); //Establecimiento del Timer3 a 1 s
}

void loop(){

  // while(abs(e)>0.1){
  //   Serial.print(e);
  //   Serial.print(';');
  //   Serial.println(y);
  // }

 if(imprimir == 1) {

   for (int i=0; i<=1201; i++) {
     Serial.println(muestras_angulo[i]); //muestras_angulo -> apartado c //pulse muestras -> apartado a y b
   }
 }
 imprimir = 0; 
}
