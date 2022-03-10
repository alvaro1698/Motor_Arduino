#define MASTER_CLOCK 84000000

uint32_t clock_a = 42000000; // Sampling frequency in Hz
uint32_t p = 2100;
uint32_t d1;
uint32_t d5;
int pinPWH1 = 42; //IN1A
int pinPWH5 = 44; //IN2A
int v = -12;
int CLOCK_WISE=0;
int OTHER_WISE=0;


void SetPin(uint8_t pin)
{
  PIO_Configure(g_APinDescription[pin].pPort,
                PIO_PERIPH_B, 
                g_APinDescription[pin].ulPin,
                g_APinDescription[pin].ulPinConfiguration);

}

void setDirection(int v){

  if (v>0 && v<=12){
    CLOCK_WISE=1;
    OTHER_WISE=0;
  }

  if (v<0 && v>=-12){
    CLOCK_WISE=0;
    OTHER_WISE=1;
  }

}

void DutyCycle(int v){
  int vi = abs(v);
    if (CLOCK_WISE==1){
      d1 = (p*vi)/12;
      d5 = 0;
    }else if (OTHER_WISE==1){
      d1 = 0;
      d5 = (p*vi)/12;
    }
}

void setup(){
  SetPin(pinPWH1);// PWMH1
  SetPin(pinPWH5); // PWMH5
  setDirection(v);
  DutyCycle(v);
  
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
  PWMC_SetDutyCycle(PWM, 1, d1); // Channel: 1, Duty cycle: 50 %
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
  PWMC_SetDutyCycle(PWM, 5, d5); // Channel: 5, Duty cycle: d5
  //PWMC_SetDeadTime(PWM, 5, 42, 42); // Channel: 5, Rising and falling edge dead time: 42/42 Mhz = 1 us
  PWMC_EnableChannel(PWM, 5); // Channel: 5
}

void loop(){}
