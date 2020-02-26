#include <Arduino.h>
#include "LinkedList.h"
#include "Bounce2.h"
#include "SimonButton.hpp"
#include <SoftwareSerial.h>
#include "DFPlayerMini_Fast.h"

void gameStateMachine();

#define DEBUG_SERIAL 1

#define SOFTWARE_SERIAL_TX_PIN 9
#define SOFTWARE_SERIAL_RX_PIN 8
#define START_PIN 3

#define RED_BUTTON_PIN 4
#define RED_LIGHT_PIN A0

#define GREEN_BUTTON_PIN 5
#define GREEN_LIGHT_PIN A2

#define BLUE_BUTTON_PIN 6
#define BLUE_LIGHT_PIN A3

#define YELLOW_BUTTON_PIN 7
#define YELLOW_LIGHT_PIN A1

#define LOCK_PIN A4
#define LOCK_DISABLE false
#define LOCK_ENABLE true

#define GAME_SEQUENCE_LENGHT 10
#define GAME_SPEED 10
#define GAME_VOLUME 15

SimonButton::lightOrders gameMode = SimonButton::lightOrders::ON_VARIABLE;

unsigned int counterWinGames = 0;

enum class gameStates{
  RESET,
  PLAYING,
  WIN
};

enum class simonStates{
  RESET,
  WAITING_FIRST_BUTTON,
  SHOWING_SEQUENCE,
  WAITING_SEQUENCE,
  ERROR,
  WIN
};

gameStates gameState=gameStates::RESET;
simonStates simonState=simonStates::RESET;

LinkedList<SimonButton*> gameSequenceList = LinkedList<SimonButton*>();
LinkedList<SimonButton*> gameButtonsList = LinkedList<SimonButton*>();

SimonButton *redButton;
SimonButton *greenButton;
SimonButton *blueButton;
SimonButton *yellowButton;

void restartMP3();

/***********************************************
            VARIABLES Y OBJETOS GLOBALES
 ***********************************************/

//Para la comunicacion del modulo DFPlayer mini y Araduino
SoftwareSerial mySerial(SOFTWARE_SERIAL_RX_PIN, SOFTWARE_SERIAL_TX_PIN); // RX, TX
DFPlayerMini_Fast myMP3;

void restartMP3(){
  myMP3.sleep();
  delay(50);
  myMP3.flush();
  delay(50);
  myMP3.wakeUp();
  delay(50);
}



void setup() {
  // put your setup code here, to run once:
  pinMode(LOCK_PIN, OUTPUT);
  digitalWrite(LOCK_PIN, LOCK_ENABLE);

  Serial.begin(9600);
  mySerial.begin(9600);
  myMP3.begin(mySerial);


  //Esto es asi porque funciona.
  unsigned int val = 0;
  for(int i=0;i<100;i++){
    val = val+analogRead(A7)*i/10;
  }
  for(int i=0;i<50;i++){
    val = val-analogRead(A7)*i/10;
  }

  Serial.print("Random seed value:");
  Serial.println(val*2);

  randomSeed(val);
  /*
  Pin,
  tiempo ON luz,
  tiempo de encendido ON luz,
  trackId
  */
  redButton = new SimonButton(RED_BUTTON_PIN,RED_LIGHT_PIN,500,5);
  greenButton = new SimonButton(GREEN_BUTTON_PIN,GREEN_LIGHT_PIN,500,2);
  blueButton = new SimonButton(BLUE_BUTTON_PIN,BLUE_LIGHT_PIN,500,3);
  yellowButton = new SimonButton(YELLOW_BUTTON_PIN,YELLOW_LIGHT_PIN,500,4);

  gameButtonsList.add(redButton);
  gameButtonsList.add(greenButton);
  gameButtonsList.add(yellowButton);
  gameButtonsList.add(blueButton);

  restartMP3();
  myMP3.volume(GAME_VOLUME);
}

bool anyRose = false;

void loop() {
  for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
    SimonButton *currentButton = gameButtonsList.getNode(i)->data;
    currentButton->update();
    if(currentButton->read()){
      currentButton->putLightOrder(SimonButton::lightOrders::ON);
    }else{
      currentButton->putLightOrder(SimonButton::lightOrders::OFF);
    }
    if(currentButton->rose()){
      randomSeed(millis());
      anyRose=true;
      myMP3.pause();
      myMP3.play(currentButton->getTrackId());
    }

  }
  gameStateMachine();
  anyRose=false;
}

bool simonStateMachine(){
  static int currentShowingSequence = 0;
  static int currentWaitingSequence = 0;
  static unsigned int prevTime = 0;
  static bool goToShow=false;
  static SimonButton *firstButton;

  /*
  RESET,
  WAITING_FIRST_BUTTON,
  SHOWING_SEQUENCE,
  WAITING_SEQUENCE,
  ERROR,
  WIN
  */
  switch (simonState) {
    case simonStates::RESET:{
      myMP3.volume(GAME_VOLUME);
      #ifdef DEBUG_SERIAL
      Serial.println(F("simonStateMachine[RESET]:Going to WAITING_FIRST_BUTTON state "));
      #endif
      simonState=simonStates::WAITING_FIRST_BUTTON;
      goToShow=false;
    }break;
    case simonStates::WAITING_FIRST_BUTTON:{
      SimonButton *currentButton;
      for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
        currentButton = gameButtonsList.getNode(i)->data;
        if(currentButton->rose()&&goToShow==false){
          #ifdef DEBUG_SERIAL
          Serial.print(F("simonStateMachine[WAITING_FIRST_BUTTON]:Button pressed:"));
          Serial.println(currentButton->getButtonPin());
          #endif
          goToShow=true;
          i=gameButtonsList.size()+1;
          prevTime=millis();
          firstButton=currentButton;
        }else if(currentButton->rose()&&goToShow==true){
          myMP3.volume(GAME_VOLUME);
          myMP3.pause();
          myMP3.play(1);
          #ifdef DEBUG_SERIAL
          Serial.print(F("simonStateMachine[WAITING_FIRST_BUTTON]:Button pressed ERROR:"));
          Serial.println(currentButton->getButtonPin());
          Serial.println(F("simonStateMachine[WAITING_FIRST_BUTTON]:Going to ERROR state"));
          #endif
          simonState=simonStates::ERROR;
        }
      }
      if(goToShow){
        unsigned int currentTime = millis();
        if(currentTime-prevTime>1000){
          #ifdef DEBUG_SERIAL
          Serial.print(F("simonStateMachine[WAITING_FIRST_BUTTON]:Clearing "));
          Serial.print(gameSequenceList.size());
          Serial.println(F(" elements."));

          Serial.print(F("simonStateMachine[WAITING_FIRST_BUTTON]:Selected button from list of "));
          Serial.print(gameButtonsList.size());
          Serial.println(F(" elements."));

          Serial.print(F("simonStateMachine[WAITING_FIRST_BUTTON]:First button is "));
          Serial.println(firstButton->getButtonPin());
          Serial.print(F("simonStateMachine[WAITING_FIRST_BUTTON]:First light is "));
          Serial.println(firstButton->getLightPin());

          Serial.println(F("simonStateMachine[WAITING_FIRST_BUTTON]:Going to SHOWING_SEQUENCE state"));
          #endif
          gameSequenceList.clear();
          gameSequenceList.add(firstButton);

          uint8_t buttonListSize = gameButtonsList.size();
          uint8_t randomButtonIndex = random(0,buttonListSize);
          SimonButton *buttonToAdd = gameButtonsList.get(randomButtonIndex);

          while(firstButton==buttonToAdd){
            randomButtonIndex = random(0,buttonListSize);
            buttonToAdd = gameButtonsList.get(randomButtonIndex);
            Serial.println("Di de baja uno.");
          }

          gameSequenceList.add(buttonToAdd);
          currentShowingSequence=0;
          #ifdef DEBUG_SERIAL
          Serial.print("gameSequenceList:");
          Serial.println(gameSequenceList.size());
          #endif
          simonState=simonStates::SHOWING_SEQUENCE;
        }
      }
    }break;
    case simonStates::SHOWING_SEQUENCE:{
      static bool orderSended = false;
      if(currentShowingSequence<gameSequenceList.size()){
        SimonButton *currentShowingButton = gameSequenceList.get(currentShowingSequence);
        if(currentShowingButton->getLightStatus()==SimonButton::lightStates::OFF&&orderSended==false){
          currentShowingButton->putLightOrder(gameMode);
          orderSended=true;
          //restartMP3();
          myMP3.pause();
          myMP3.play(currentShowingButton->getTrackId());
        }else{
          if(orderSended==true&&currentShowingButton->getLightStatus()==SimonButton::lightStates::OFF){
            orderSended=false;
            currentShowingSequence++;
          }
        }

      }else{

        #ifdef DEBUG_SERIAL
        Serial.println(F("simonStateMachine[SHOWING_SEQUENCE]:Going to WAITING_SEQUENCE state"));
        Serial.print("gameSequenceList:");
        Serial.println(gameSequenceList.size());
        #endif
        simonState=simonStates::WAITING_SEQUENCE;
        currentWaitingSequence=0;


      }
    }break;
    case simonStates::WAITING_SEQUENCE:{
      static unsigned int prevTime=0;
      SimonButton *expectedButton = gameSequenceList.get(currentWaitingSequence);
      for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
        SimonButton *currentButton = gameButtonsList.getNode(i)->data;
        if(currentButton->rose()){
            if(expectedButton==currentButton&&currentWaitingSequence<=gameSequenceList.size()){
            currentWaitingSequence++;
            prevTime=millis();
            #ifdef DEBUG_SERIAL
            Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Button pressed OK"));
            #endif
          }else{
            myMP3.volume(GAME_VOLUME);
            myMP3.pause();
            myMP3.play(1);
            #ifdef DEBUG_SERIAL
            Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Button pressed ERROR"));
            Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Going to ERROR state"));
            #endif
            simonState=simonStates::ERROR;
          }
        }
      }

      if(currentWaitingSequence==GAME_SEQUENCE_LENGHT){
          #ifdef DEBUG_SERIAL
          Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Going to WIN state"));
          #endif
          simonState=simonStates::WIN;
        }else if(currentWaitingSequence==gameSequenceList.size()){
          unsigned int currentTime = millis();
          if(currentTime-prevTime>=1000){
            prevTime=0;
            #ifdef DEBUG_SERIAL
            Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Adding new button."));
            #endif
            uint8_t buttonListSize = gameButtonsList.size();
            uint8_t randomButtonIndex = random(0,buttonListSize);
            SimonButton *buttonToAdd = gameButtonsList.get(randomButtonIndex);
            SimonButton *prevButton = gameButtonsList.get(gameSequenceList.size()-1);
            SimonButton *prevPrevButton = gameButtonsList.get(gameSequenceList.size()-2);

            while((buttonToAdd==prevButton)&&(buttonToAdd==prevPrevButton)){
              randomButtonIndex = random(0,buttonListSize);
              buttonToAdd = gameButtonsList.get(randomButtonIndex);
              Serial.println("Di de baja uno.");
            }

            gameSequenceList.add(buttonToAdd);
            currentShowingSequence=0;

            simonState=simonStates::SHOWING_SEQUENCE;

            #ifdef DEBUG_SERIAL
            Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Going to SHOWING_SEQUENCE state"));
            #endif
          }
      }
    }break;
    case simonStates::ERROR:{


      Serial.println(F("simonStateMachine[ERROR]:Going to RESET state"));
      simonState=simonStates::RESET;

      for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
        SimonButton *currentButton = gameButtonsList.get(i);
        currentButton->putLightOrder(SimonButton::lightOrders::RESET_TIMER_VARIABLE);
      }
      myMP3.volume(GAME_VOLUME);


    }break;
    case simonStates::WIN:{
        for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
          SimonButton *currentButton = gameButtonsList.get(i);
          currentButton->putLightOrder(SimonButton::lightOrders::RESET_TIMER_VARIABLE);
        }
        Serial.println(F("simonStateMachine[WIN]:Going to RESET state"));
        simonState=simonStates::RESET;
        myMP3.volume(GAME_VOLUME);
      return true;
    }break;
    default:{
      return false;
    }break;
  }
  return false;
}

void gameStateMachine(){
  static unsigned int prevTimeState = 0;
  /*
  RESET,
  IDLE,
  PLAYING,
  WIN
  */
  switch (gameState) {
    case gameStates::RESET:{
      bool goingToWaitingButtonCondition = false;
      goingToWaitingButtonCondition=true;
      if(goingToWaitingButtonCondition){
        #ifdef DEBUG_SERIAL
        Serial.println(F("gameStateMachine[RESET]:Going to PLAYING state"));
        #endif
        digitalWrite(LOCK_PIN, LOCK_ENABLE);
        gameState=gameStates::PLAYING;
      }
    }break;
    case gameStates::PLAYING:{
      bool gameFinished=simonStateMachine();
      if(gameFinished){
        #ifdef DEBUG_SERIAL
        Serial.println(F("gameStateMachine[PLAYING]:Game finish. Going to WIN state"));
        #endif
        gameState=gameStates::WIN;
        digitalWrite(LOCK_PIN, LOCK_DISABLE);
        prevTimeState=millis();
        counterWinGames++;
        if(counterWinGames%30==0){
          restartMP3();
        }
      }
    }break;
    case gameStates::WIN:{
      bool goingToResetCondition = false;
      unsigned int currenTime = millis();
      if(currenTime-prevTimeState>30000){
        goingToResetCondition=true;
      }else if(anyRose){
        goingToResetCondition=true;
      }
      if(goingToResetCondition){
        #ifdef DEBUG_SERIAL
        Serial.println(F("gameStateMachine[WIN]:Game reset. Going to RESET state"));
        #endif
        gameState=gameStates::RESET;
      }

    }break;
    default:{

    }break;
  }
}
