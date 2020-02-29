#include <Arduino.h>
#include "LinkedList.h"
#include "Bounce2.h"
#include "SimonButton.hpp"
#include <SoftwareSerial.h>
#include "DFPlayerMini_Fast.h"

void gameStateMachine();
bool simonStateMachine(bool reset=false);
bool initialAnimationStateMachine();

//#define DEBUG_SERIAL 1

#define SOFTWARE_SERIAL_TX_PIN 9
#define SOFTWARE_SERIAL_RX_PIN 8
#define START_PIN 3
#define BUSY_PIN 12

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
#define GAME_VOLUME 10
#define GAME_RESET_TIME_MS 120000

SimonButton::lightOrders gameMode = SimonButton::lightOrders::ON_GLOBAL;

unsigned int counterWinGames = 0;

enum class gameStates{
  RESET,
  PLAYING,
  WIN
};

enum class simonStates{
  RESET,
  WAITING,
  INITIAL_ANIMATION,
  SHOWING_SEQUENCE,
  WAITING_SEQUENCE,
  WAIT_LAST,
  ERROR,
  WIN
};

enum class initialAnimationStates{
  RESET,
  RANDOMIZING,
  BLINKING_2_TIMES,
  OFF_500MS
};

initialAnimationStates initialAnimationState=initialAnimationStates::RESET;
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
  myMP3.flush();
  myMP3.wakeUp();
}
bool goToShow=false;

Bounce busySignal=Bounce();


void setup() {
  // put your setup code here, to run once:
  pinMode(LOCK_PIN, OUTPUT);
  digitalWrite(LOCK_PIN, LOCK_ENABLE);

  Serial.begin(115200);
  mySerial.begin(9600);
  myMP3.begin(mySerial);

  busySignal.attach(BUSY_PIN, INPUT);
  busySignal.interval(0);

  //Esto es asi porque funciona.
  unsigned int val = 0;
  for(int i=0;i<100;i++){
    val = val+analogRead(A7)*i/10;
  }
  for(int i=0;i<50;i++){
    val = val-analogRead(A7)*i/10;
  }
  #ifdef DEBUG_SERIAL
  Serial.print("Random seed value:");
  Serial.println(val*2);
  #endif

  randomSeed(val);
  /*
  Pin,
  tiempo ON luz,
  tiempo de encendido ON luz,
  trackId
  */
  redButton = new SimonButton(RED_BUTTON_PIN,RED_LIGHT_PIN,500,6);
  greenButton = new SimonButton(GREEN_BUTTON_PIN,GREEN_LIGHT_PIN,500,2);
  blueButton = new SimonButton(BLUE_BUTTON_PIN,BLUE_LIGHT_PIN,500,3);
  yellowButton = new SimonButton(YELLOW_BUTTON_PIN,YELLOW_LIGHT_PIN,500,4);

  gameButtonsList.add(redButton);
  gameButtonsList.add(greenButton);
  gameButtonsList.add(yellowButton);
  gameButtonsList.add(blueButton);

  //restartMP3();
  myMP3.volume(GAME_VOLUME);
  myMP3.playFromMP3Folder(7);
  delay(2000);
  myMP3.volume(GAME_VOLUME);
  myMP3.playFromMP3Folder(7);
/*
  Serial.println(1);
  myMP3.playFromMP3Folder(6);
  delay(T_D);
  Serial.println(2);
  myMP3.playFromMP3Folder(2);
  delay(T_D);
  Serial.println(3);
  myMP3.playFromMP3Folder(3);
  delay(T_D);
  Serial.println(4);
  myMP3.playFromMP3Folder(4);
  delay(T_D);
  Serial.println(5);
  myMP3.playFromMP3Folder(5);
  delay(T_D);
  */
}

bool anyRose = false;
bool anyFell = false;
int playingStatus = false;
bool lightOnPress = false;

void loop() {
  busySignal.update();
  if(busySignal.rose()){
    playingStatus--;
    #ifdef DEBUG_SERIAL
    Serial.print(F("Playing:"));
    Serial.println(playingStatus);
    #endif
  }

  for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
    SimonButton *currentButton = gameButtonsList.getNode(i)->data;
    currentButton->update();
    if(currentButton->fell()){
      anyFell=true;
    }else if(currentButton->rose()){
      anyRose=true;
    }

    if(currentButton->read()&&lightOnPress==true){
      currentButton->putLightOrder(SimonButton::lightOrders::ON);
    }else{
      if(simonState!=simonStates::INITIAL_ANIMATION){
        currentButton->putLightOrder(SimonButton::lightOrders::OFF);
      }
    }
  }

  gameStateMachine();
  anyRose=false;
  anyFell=false;
}


void gameStateMachine(){
  static unsigned long prevTimeState = 0;
  /*
  RESET,
  PLAYING,
  WIN
  */
  switch (gameState) {
    case gameStates::RESET:{
        #ifdef DEBUG_SERIAL
        Serial.println(F("gameStateMachine[RESET]:Going to PLAYING state"));
        #endif
        digitalWrite(LOCK_PIN, LOCK_ENABLE);
        gameState=gameStates::PLAYING;
        restartMP3();
        myMP3.volume(GAME_VOLUME);
    }break;
    case gameStates::PLAYING:{
      bool gameFinished=simonStateMachine();
      if(gameFinished){
        digitalWrite(LOCK_PIN, LOCK_DISABLE);
        counterWinGames++;
        if(counterWinGames%30==0){
          restartMP3();
          #ifdef DEBUG_SERIAL
          Serial.println(F("gameStateMachine[PLAYING]:MP3 restarted"));
          #endif
        }
        prevTimeState=millis();
        gameState=gameStates::WIN;
        #ifdef DEBUG_SERIAL
        Serial.println(F("gameStateMachine[PLAYING]:Game finish. Going to WIN state"));
        #endif
      }
    }break;
    case gameStates::WIN:{
      unsigned long currentTime = millis();
      if(currentTime-prevTimeState>GAME_RESET_TIME_MS){
        #ifdef DEBUG_SERIAL
        Serial.println(F("gameStateMachine[WIN]:Timeout completed"));
        Serial.println(F("gameStateMachine[WIN]:Game reset. Going to RESET state"));
        #endif
        bool reset = true;
        simonStateMachine(reset);
        gameState=gameStates::RESET;
      }
    }break;
    default:{

    }break;
  }
}

bool simonStateMachine(bool reset){
  static int currentShowingSequenceIndex = 0;
  static int currentWaitingSequenceIndex = 0;
  static SimonButton *firstButton;
  static bool orderSended = false;
  static unsigned long prevSimonTimeState = 0;

  /*
  RESET,
  WAITING,
  INITIAL_ANIMATION,
  SHOWING_SEQUENCE,
  WAITING_SEQUENCE,
  WAIT_LAST,
  ERROR,
  WIN
  */
  switch (simonState) {
    case simonStates::RESET:{
      #ifdef DEBUG_SERIAL
      Serial.println(F("simonStateMachine[RESET]:Going to WAITING state"));
      #endif
      simonState=simonStates::WAITING;
      lightOnPress=false;

      myMP3.volume(GAME_VOLUME);
    }break;
    case simonStates::WAITING:{
      SimonButton *currentButton;
      for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
        currentButton = gameButtonsList.getNode(i)->data;
        if(currentButton->rose()){

          #ifdef DEBUG_SERIAL
          Serial.print(F("simonStateMachine[WAITING]:Clearing "));
          Serial.print(gameSequenceList.size());
          Serial.println(F(" elements."));
          #endif
          gameSequenceList.clear();

          #ifdef DEBUG_SERIAL
          Serial.print(F("simonStateMachine[WAITING]:Button pressed:"));
          Serial.println(currentButton->getButtonPin());
          #endif

          gameSequenceList.add(currentButton);
          firstButton=currentButton;
          #ifdef DEBUG_SERIAL
          Serial.println(F("simonStateMachine[WAITING]:Game started"));
          Serial.println(F("simonStateMachine[WAITING]:Going to INITIAL_ANIMATION state"));
          #endif
          prevSimonTimeState=millis();
          simonState=simonStates::INITIAL_ANIMATION;
        }
      }
    }break;
    case simonStates::INITIAL_ANIMATION:{
      bool initialAnimationFinished = initialAnimationStateMachine();
      if(initialAnimationFinished){
        #ifdef DEBUG_SERIAL
        Serial.println(F("simonStateMachine[INITIAL_ANIMATION]:Animation finished"));
        #endif

        uint8_t buttonListSize = gameButtonsList.size();
        unsigned int randomButtonIndex = random(0,buttonListSize);
        SimonButton *buttonToAdd = gameButtonsList.get(randomButtonIndex);

        while(firstButton==buttonToAdd){
          randomButtonIndex = random(0,buttonListSize);
          buttonToAdd = gameButtonsList.get(randomButtonIndex);
          #ifdef DEBUG_SERIAL
          Serial.println(F("simonStateMachine[INITIAL_ANIMATION]:Wrong button generated"));
          #endif
        }
        #ifdef DEBUG_SERIAL
        Serial.print(F("simonStateMachine[INITIAL_ANIMATION]:Second button is "));
        Serial.println(buttonToAdd->getButtonPin());
        #endif
        gameSequenceList.add(buttonToAdd);

        #ifdef DEBUG_SERIAL
        Serial.println(F("simonStateMachine[INITIAL_ANIMATION]:Going to SHOWING_SEQUENCE state"));
        #endif
        currentShowingSequenceIndex=0;
        simonState=simonStates::SHOWING_SEQUENCE;
      }
    }break;
    case simonStates::SHOWING_SEQUENCE:{
      if(currentShowingSequenceIndex<gameSequenceList.size()){
        SimonButton *currentShowingButton = gameSequenceList.get(currentShowingSequenceIndex);
        if(currentShowingButton->getLightStatus()==SimonButton::lightStates::OFF){
          if(orderSended==false){
            currentShowingButton->putLightOrder(gameMode);
            orderSended=true;
            #ifdef DEBUG_SERIAL
            Serial.print(F("simonStateMachine[SHOWING_SEQUENCE]:"));
            Serial.println(currentShowingButton->getButtonPin());
            #endif
            myMP3.playFromMP3Folder(currentShowingButton->getTrackId());
            playingStatus++;
          }else{
            orderSended=false;
            currentShowingSequenceIndex++;
          }
        }
      }else{
        #ifdef DEBUG_SERIAL
        Serial.println(F("simonStateMachine[SHOWING_SEQUENCE]:Finished. Going to WAITING_SEQUENCE state"));
        #endif
        simonState=simonStates::WAITING_SEQUENCE;
        currentWaitingSequenceIndex=0;
        lightOnPress=true;
      }
    }break;
    case simonStates::WAITING_SEQUENCE:{
      SimonButton *expectedButton = gameSequenceList.get(currentWaitingSequenceIndex);
      for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
        SimonButton *currentButton = gameButtonsList.getNode(i)->data;
        if(currentButton->rose()){
          if(expectedButton==currentButton){
            #ifdef DEBUG_SERIAL
            Serial.print(F("simonStateMachine[WAITING_SEQUENCE]:Button pressed "));
            Serial.println(expectedButton->getButtonPin());
            #endif
            for (uint8_t j = 0; j < gameButtonsList.size(); j++) {
              SimonButton *auxButton = gameButtonsList.getNode(j)->data;
              auxButton->decreaseTimeOutGlobal();
            }
            myMP3.playFromMP3Folder(expectedButton->getTrackId());
            playingStatus++;
            currentWaitingSequenceIndex++;
            if(currentWaitingSequenceIndex==gameSequenceList.size()){
              #ifdef DEBUG_SERIAL
              Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Last button."));
              Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Going to WAIT_LAST state"));
              #endif
              simonState=simonStates::WAIT_LAST;
              prevSimonTimeState=millis();
            }
          }else{
            myMP3.playFromMP3Folder(5);
            playingStatus++;
            #ifdef DEBUG_SERIAL
            Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Button pressed ERROR"));
            Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Going to ERROR state"));
            #endif
            lightOnPress=false;
            simonState=simonStates::ERROR;
          }
        }
    }
    }break;
    case simonStates::WAIT_LAST:{
      unsigned long currentTime = millis();

      if(currentTime-prevSimonTimeState>500){
        lightOnPress=false;
        if(currentWaitingSequenceIndex==GAME_SEQUENCE_LENGHT){
          for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
            SimonButton *currentButton = gameButtonsList.getNode(i)->data;
            currentButton->putLightOrder(SimonButton::lightOrders::ON);
          }
          #ifdef DEBUG_SERIAL
          Serial.println(F("simonStateMachine[WAIT_LAST]:Game completed"));
          Serial.println(F("simonStateMachine[WAIT_LAST]:Going to WIN state"));
          #endif
          simonState=simonStates::WIN;
          return true;
        }else{

          #ifdef DEBUG_SERIAL
          Serial.println(F("simonStateMachine[WAIT_LAST]:Ready"));
          Serial.println(F("simonStateMachine[WAIT_LAST]:Adding new button.."));
          #endif

          uint8_t buttonListSize = gameButtonsList.size();
          unsigned int randomButtonIndex = random(0,buttonListSize);
          SimonButton *buttonToAdd = gameButtonsList.get(randomButtonIndex);
          SimonButton *prevButton = gameSequenceList.get(gameSequenceList.size()-1);
          SimonButton *prevPrevButton = gameSequenceList.get(gameSequenceList.size()-2);

          #ifdef DEBUG_SERIAL
          Serial.print(F("simonStateMachine[WAIT_LAST]:buttonToAdd is "));
          Serial.println(buttonToAdd->getButtonPin());
          #endif
          #ifdef DEBUG_SERIAL
          Serial.print(F("simonStateMachine[WAIT_LAST]:prevButton is "));
          Serial.println(prevButton->getButtonPin());
          #endif
          #ifdef DEBUG_SERIAL
          Serial.print(F("simonStateMachine[WAIT_LAST]:prevPrevButton is "));
          Serial.println(prevPrevButton->getButtonPin());
          #endif
          while((buttonToAdd==prevButton)&&(buttonToAdd==prevPrevButton)){
            randomButtonIndex = random(0,buttonListSize);
            buttonToAdd = gameButtonsList.get(randomButtonIndex);
            Serial.print(F("simonStateMachine[WAIT_LAST]:Wrong button generated"));
          }

          #ifdef DEBUG_SERIAL
          Serial.print(F("simonStateMachine[WAIT_LAST]:New button is "));
          Serial.println(buttonToAdd->getButtonPin());
          #endif

          gameSequenceList.add(buttonToAdd);
          currentShowingSequenceIndex=0;
          simonState=simonStates::SHOWING_SEQUENCE;

          #ifdef DEBUG_SERIAL
          Serial.println(F("simonStateMachine[WAIT_LAST]:Going to SHOWING_SEQUENCE state"));
          #endif
        }
      }
    }break;
    case simonStates::ERROR:{
      if(playingStatus==0) {
        #ifdef DEBUG_SERIAL
        Serial.println(F("simonStateMachine[ERROR]:Going to RESET state"));
        #endif
        simonState=simonStates::RESET;
        for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
          SimonButton *currentButton = gameButtonsList.get(i);
          currentButton->reset();
        }
        myMP3.volume(GAME_VOLUME);
      }
    }break;
    case simonStates::WIN:{
        if(reset){
          for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
            SimonButton *currentButton = gameButtonsList.get(i);
            currentButton->reset();
          }

          #ifdef DEBUG_SERIAL
          Serial.println(F("simonStateMachine[WIN]:Going to RESET state"));
          #endif

          simonState=simonStates::RESET;
          myMP3.volume(GAME_VOLUME);
        }
    }break;
    default:{
      return false;
    }break;
  }
  return false;
}

bool initialAnimationStateMachine(){
  static unsigned long prevTimeState=0;
  static unsigned long prevRandomTimeState=0;
  static unsigned long randomTime=0;
  static unsigned int lastButtonIndex=0;
  static unsigned int minRandom=50;
  static unsigned int maxRandom=100;
  static unsigned int counterState=0;
  static bool onOffState=false;

  switch (initialAnimationState) {
    case initialAnimationStates::RESET:{
      minRandom=50;
      minRandom=100;
      for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
        SimonButton *currentButton = gameButtonsList.get(i);
        currentButton->putLightOrder(SimonButton::lightOrders::OFF);
      }
      lastButtonIndex=random(0,gameButtonsList.size());

      gameButtonsList.get(lastButtonIndex)->putLightOrder(SimonButton::lightOrders::ON);

      prevTimeState=millis();
      prevRandomTimeState=millis();
      randomTime=random(minRandom,maxRandom);
      #ifdef DEBUG_SERIAL
      Serial.println(F("initialAnimationStateMachine[RESET]:Going to RANDOMIZING state"));
      #endif
      myMP3.playFromMP3Folder(7);
      playingStatus++;
      initialAnimationState=initialAnimationStates::RANDOMIZING;

    }break;
    case initialAnimationStates::RANDOMIZING:{
      unsigned long currentTime=millis();
      if(currentTime-prevRandomTimeState>1500){
        prevRandomTimeState=currentTime;
        gameButtonsList.get(lastButtonIndex)->putLightOrder(SimonButton::lightOrders::OFF);
        initialAnimationState=initialAnimationStates::BLINKING_2_TIMES;
        #ifdef DEBUG_SERIAL
        Serial.println(F("initialAnimationStateMachine[RANDOMIZING]:Going to BLINKING_2_TIMES state"));
        #endif
        prevTimeState=0;
        counterState=0;
      }else if(currentTime-prevTimeState>randomTime){
        prevTimeState=currentTime;
        gameButtonsList.get(lastButtonIndex)->putLightOrder(SimonButton::lightOrders::OFF);

        unsigned int currentButtonIndex=random(0,gameButtonsList.size());
        while(currentButtonIndex==lastButtonIndex){
          currentButtonIndex=random(0,gameButtonsList.size());
        }
        lastButtonIndex=currentButtonIndex;

        gameButtonsList.get(currentButtonIndex)->putLightOrder(SimonButton::lightOrders::ON);

        minRandom=minRandom+50;
        minRandom=maxRandom+50;
        randomTime=random(minRandom,maxRandom);
      }
    }break;
    case initialAnimationStates::BLINKING_2_TIMES:{
      unsigned long currentBlinkTime = millis();
      if(currentBlinkTime-prevTimeState>500){
        prevTimeState=currentBlinkTime;
        counterState++;
        onOffState=!onOffState;
        if(onOffState){
          for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
            SimonButton *currentButton = gameButtonsList.get(i);
            currentButton->putLightOrder(SimonButton::lightOrders::ON);
          }
        }else{
          for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
            SimonButton *currentButton = gameButtonsList.get(i);
            currentButton->putLightOrder(SimonButton::lightOrders::OFF);
          }
        }
        if(counterState==4){
          prevTimeState=millis();
          counterState=0;
          onOffState=false;
          #ifdef DEBUG_SERIAL
          Serial.println(F("initialAnimationStateMachine[BLINKING_2_TIMES]:Going to OFF_500MS state"));
          #endif
          initialAnimationState=initialAnimationStates::OFF_500MS;
        }
      }


    }break;
    case initialAnimationStates::OFF_500MS:{
      unsigned long currentTime=millis();
      if(currentTime-prevTimeState>500){
        #ifdef DEBUG_SERIAL
        Serial.println(F("initialAnimationStateMachine[OFF_500MS]:Going to RESET state"));
        #endif
        initialAnimationState=initialAnimationStates::RESET;
        return true;
      }
    }break;
    default:{

    }break;
  }
  return false;
}
