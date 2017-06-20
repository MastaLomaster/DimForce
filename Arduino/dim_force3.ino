// версия для геркона, двух диодов и пьезоизлучателя
int brightnessB=LOW, brightnessR=LOW;
int blinkingB=0; int blinkingR=0;

int speakerPin=8;
int BlueLED=10;
int RedLED=11;

int freq[]={3830,3038,2550,3038};
int cur_freq=0;
int buzzer_value=0;

volatile int stepper=0, old_stepper=0;

// функция обработки прерывания на pin 
void step()
{
  stepper=!stepper;

  Serial.print(stepper);
  Serial.flush();
}

void setup() {

pinMode(10,OUTPUT);
pinMode(11,OUTPUT);
pinMode(2,INPUT);

Serial.begin(9600);

//attachInterrupt(0,step,CHANGE);

}




void loop() 
{
 delay (200);

  // 1. Принимаем команду от компьютера о яркости светодиода
  while(Serial.available())
  {
    switch(Serial.read())
    {
     case '0': // красный и синий горят, сирена воет
      brightnessB=HIGH; blinkingB=0;
      brightnessR=HIGH; blinkingR=0;
      buzzer_value=1;
      break;

     case '1':
      // Синий и красный перемаргиваются. 
      brightnessB=LOW; blinkingB=1;
      brightnessR=HIGH; blinkingR=1;
      buzzer_value=0;
      break;
    
     case '2': // Синий моргает
      brightnessB=HIGH; blinkingB=1;
      brightnessR=LOW; blinkingR=0;
      buzzer_value=0;
      break;
    
     case '3':// Красный моргает
      brightnessB=LOW; blinkingB=0;
      brightnessR=HIGH; blinkingR=1;
      buzzer_value=0;
      break;

     case '4': // Красный горит
      brightnessB=LOW; blinkingB=0;
      brightnessR=HIGH; blinkingR=0;
      buzzer_value=0;
      break;

    case '5':
      // Тьма. 
     brightnessB=LOW;  blinkingB=0;
     brightnessR=LOW; blinkingR=0;
     buzzer_value=0;
     break;
    }
  } // Прочли все символы из буфера порта, поняли, какая яркость нам нужна


  if(1==blinkingB)
  {  
    if(HIGH==brightnessB) brightnessB=LOW; else brightnessB=HIGH;
  }

  if(1==blinkingR)
  {
    if(HIGH==brightnessR) brightnessR=LOW; else brightnessR=HIGH;
  }
    
  digitalWrite(RedLED,brightnessR);
  digitalWrite(BlueLED,brightnessB);

  if(1==buzzer_value)
  {
    tone(speakerPin, freq[cur_freq], 200);
    cur_freq++;
    if(cur_freq>3) {cur_freq=0;}
  }

  stepper=digitalRead(2);
  if(old_stepper!=stepper)
  {
     Serial.print(stepper);
     Serial.flush();
     old_stepper=stepper;
  }

}
