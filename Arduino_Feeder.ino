/* Nunz Aquarium Feeder
  */

#include <AccelStepper.h>

#define FULLSTEP 4
#define HALFSTEP 8
                        
#define MotorPin1  8
#define MotorPin2  9
#define MotorPin3  10
#define MotorPin4  11

#define Port_Endstop 12
#define Port_Starter 7
#define Port_Led13 13

unsigned long FeedingActive = false;
unsigned long AktTime = 0;
unsigned long MeasureTime = 0;
int PositioningMode = 0;

AccelStepper Stepper1(FULLSTEP, MotorPin1, MotorPin3, MotorPin2, MotorPin4);

void setup() 
{
  Serial.begin(9600);

  pinMode(Port_Endstop, INPUT_PULLUP);
  pinMode(Port_Starter, INPUT);
  pinMode(Port_Led13, OUTPUT);
  digitalWrite(Port_Led13, LOW);
  
  Stepper1.setMaxSpeed(70.0);
  Stepper1.setAcceleration(1000.0);
  Stepper1.setSpeed(70);

  Stepper1.disableOutputs();

//  GotoStartPosition();
//  DoFeed();
} 

void loop() 
{
  AktTime = millis();
  
  // Das Startersignal muss mind eine 1/2 Sekunde lang auf HIGH sein bevors los geht
  if(digitalRead(Port_Starter) == HIGH)
  {
    if(MeasureTime == 0)
      MeasureTime = AktTime;
      
    if(MeasureTime > 0 && AktTime - MeasureTime > 1000 && FeedingActive == false)
    {
      FeedingActive = true;
      digitalWrite(Port_Led13, HIGH);
      // Es kann los gehen
      digitalWrite(Port_Led13, HIGH);
      Serial.println("Start");
      Stepper1.enableOutputs();
      if(GotoStartPosition() == true)
      {
        DoFeed();
        DoFeed();
      }
      digitalWrite(Port_Led13, LOW);
    }
  }
  else if(FeedingActive == true)  // Erst ein LOW beim Starter setzt alles zurück
  {
    FeedingActive = false;
    MeasureTime = 0;
    digitalWrite(Port_Led13, LOW);
    Serial.println("Reset");
  }
}

// Der Schlitten wird mittels des Endstops so positioniert, dass der 
// Schalter gerade eben nicht mehr schaltet, der Schlitten also ganz am Anfang steht
bool GotoStartPosition()
{
  unsigned long TimeOutTime = AktTime;

  Stepper1.setMaxSpeed(70.0);
  Stepper1.setAcceleration(1000.0);
  Stepper1.setSpeed(70);

  do
  {
    AktTime = millis();
    if(PositioningMode == 0)
    { 
      Stepper1.enableOutputs();
      Stepper1.moveTo(10000);
      PositioningMode = 1;
    }
  
    if(PositioningMode == 1)
    { 
      if(digitalRead(Port_Endstop) == LOW)
      { 
        PositioningMode = 2;
        Stepper1.stop();
        Serial.println("Stop1");
      }
    }
    if(PositioningMode == 2)
    { 
      Stepper1.moveTo(-10000);
      PositioningMode = 3;
    }
    if(PositioningMode == 3)
    { 
      if(digitalRead(Port_Endstop) == HIGH)
      { 
        unsigned WaitTime = millis();
        do
        {
          Stepper1.run();  
        } while(millis() - WaitTime < 400);
        PositioningMode = 4;
        Stepper1.stop();
        Serial.println("Stop2");
        break;
      }
    }
    if(PositioningMode > 0)
      Stepper1.run();  
  } while(PositioningMode < 4 && AktTime - TimeOutTime < 5000);  // falls es sich verklemmt und der Taster nicht erreicht werden kann

  if(PositioningMode < 4 && AktTime - TimeOutTime >= 5000)
  {
    Serial.println(PositioningMode);
    PositioningMode = 0;
    Serial.println("Timeout Positioning");
    return false;
  }

  Stepper1.setCurrentPosition(0);
  PositioningMode = 0;
  
  Serial.println("Positioned");
  return true;
}

// Einen Fütterungsvorgang einleiten
// Dabei wird davon ausgegangen dass der Schlitten schon korrekt am Ende sitzt
void DoFeed() 
{
  Stepper1.enableOutputs();
  Stepper1.setMaxSpeed(500.0);
  Stepper1.setAcceleration(2000.0);
  Stepper1.setSpeed(300);

  int Ausfahrweg = -720;
  
  // Zügig nach vorne fahren um Futter auszuwerfen
  Serial.println("Ausfahren");
  int CurrPos = Stepper1.currentPosition();
  Stepper1.moveTo(Ausfahrweg);
//  Serial.println(CurrPos);

  while(Stepper1.distanceToGo() != 0)
  {
//    Serial.println(Stepper1.currentPosition());
    Stepper1.run();
  }

  // Ein wenig hin und her wackeln damit das Futter auch fällt
  Stepper1.setAcceleration(20000.0);
  Serial.println("Wackeln");
  for(int t = 0; t< 3; t++)
  {
    Stepper1.moveTo(Ausfahrweg+50);
    while(Stepper1.distanceToGo() != 0)
        Stepper1.run();
    Stepper1.moveTo(Ausfahrweg);
    while(Stepper1.distanceToGo() != 0)
        Stepper1.run();
  }

  // Wieder einfahren um neu zu laden
  Serial.println(Stepper1.currentPosition());
  Stepper1.setAcceleration(2000.0);
  Serial.println("Einfahren");
  Stepper1.moveTo(0);
  while(Stepper1.distanceToGo() != 0)
  {
//    Serial.println(Stepper1.currentPosition());
      Stepper1.run();
  }
  Stepper1.disableOutputs();
//  Serial.println(Stepper1.currentPosition());
  Serial.println("Fertig");
}

