#include <Arduino.h>
#include "LinkedList.h"
#include "Bounce2.h"
#include "SimonButton.hpp"
#include <SoftwareSerial.h>
#include "DFMiniMp3.h"

void gameStateMachine();

#define DEBUG_SERIAL 1

#define SOFTWARE_SERIAL_TX_PIN 9
#define SOFTWARE_SERIAL_RX_PIN 8
#define START_PIN A5

#define RED_BUTTON_PIN 2
#define RED_LIGHT_PIN 8

#define GREEN_BUTTON_PIN 5
#define GREEN_LIGHT_PIN 6

#define BLUE_BUTTON_PIN 3
#define BLUE_LIGHT_PIN 11

#define YELLOW_BUTTON_PIN 4
#define YELLOW_LIGHT_PIN 7

#define LOCK_PIN A0
#define LOCK_DISABLE true
#define LOCK_ENABLE false

#define GAME_SEQUENCE_LENGHT 3
#define GAME_SPEED 10

enum class gameStates{
  RESET,
  WAITING_BUTTON,
  PLAYING,
  WIN
};

enum class simonStates{
  RESET,
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

//TODO desacoplar esta variable global
bool playFinished = false;
class Mp3Notify
{
  public:
    static void OnError(uint16_t errorCode)
    {
      // see DfMp3_Error for code meaning

#ifdef __DEBUG
      Serial.println();
      Serial.print("Com Error ");
      Serial.println(errorCode);
#endif

    }

    static void OnPlayFinished(DfMp3_PlaySources source,uint16_t globalTrack)
    {
#ifdef __DEBUG
      Serial.print("Play finished for #");
      Serial.println(globalTrack);
#endif
      //TODO desacoplar esto
      playFinished = true;

    }

    static void OnCardOnline(uint16_t code)
    {
#ifdef __DEBUG

      Serial.println();
      Serial.print("Card online ");
      Serial.println(code);
#endif

    }

    static void OnUsbOnline(uint16_t code)
    {
#ifdef __DEBUG

      Serial.println();
      Serial.print("USB Disk online ");
      Serial.println(code);
#endif

    }

    static void OnCardInserted(uint16_t code)
    {
#ifdef __DEBUG

      Serial.println();
      Serial.print("Card inserted ");
      Serial.println(code);
#endif

    }

    static void OnUsbInserted(uint16_t code)
    {
#ifdef __DEBUG

      Serial.println();
      Serial.print("USB Disk inserted ");
      Serial.println(code);
#endif

    }

    static void OnCardRemoved(uint16_t code)
    {
#ifdef __DEBUG

      Serial.println();
      Serial.print("Card removed ");
      Serial.println(code);
#endif

    }

    static void OnUsbRemoved(uint16_t code)
    {
#ifdef __DEBUG

      Serial.println();
      Serial.print("USB Disk removed ");
      Serial.println(code);
#endif

    }
};

//Para la comunicacion del modulo DFPlayer mini y Araduino
SoftwareSerial mySerial(SOFTWARE_SERIAL_RX_PIN, SOFTWARE_SERIAL_TX_PIN);
//Objeto para controlar el mp3
DFMiniMp3<SoftwareSerial, Mp3Notify> myMP3(mySerial);

void setup() {
  mySerial.begin(9600);

  #ifdef DEBUG_SERIAL
  Serial.begin(9600);
  Serial.println("Arduino reset!");

  myMP3.begin();
  myMP3.stop();

  uint16_t trackCount = myMP3.getTotalTrackCount(DfMp3_PlaySource_Sd);
  uint16_t volume = myMP3.getVolume();

  Serial.println("Reproductor incializado.");
  Serial.println("Starting...");
  String message = "Cantidad de canciones detectadas:" + String(trackCount);
  Serial.println(message);
  message = "Volumen inicial:" + String(volume);
  Serial.println(message);
  message = "Valor inicial random:" + String(random(1, trackCount + 1));
  Serial.println(message);

  #endif
}

void loop() {
  // put your main code here, to run repeatedly:
  gameStateMachine();
}

bool simonStateMachine(){
  static int currentShowingSequence = 0;
  static int currentWaitingSequence = 0;
  /*
  RESET,
  SHOWING_SEQUENCE,
  WAITING_SEQUENCE,
  ERROR,
  WIN
  */
  switch (simonState) {
    case simonStates::RESET:{
      randomSeed(analogRead(A1));
      uint8_t randomButtonIndex = random(0,gameButtonsList.size());
      #ifdef DEBUG_SERIAL
      Serial.print(F("simonStateMachine[RESET]:Clearing "));
      Serial.print(gameSequenceList.size());
      Serial.println(F(" elements."));

      Serial.print(F("simonStateMachine[RESET]:Selecting button from list of "));
      Serial.print(gameButtonsList.size());
      Serial.println(F(" elements."));

      Serial.print(F("simonStateMachine[RESET]:First button is "));
      Serial.println(gameButtonsList.get(randomButtonIndex)->getButtonPin());
      Serial.print(F("simonStateMachine[RESET]:First light is "));
      Serial.println(gameButtonsList.get(randomButtonIndex)->getLightPin());

      #endif
      gameSequenceList.clear();
      gameSequenceList.add(gameButtonsList.get(randomButtonIndex));
      currentShowingSequence=0;
      #ifdef DEBUG_SERIAL
      Serial.println(F("simonStateMachine[RESET]:Going to SHOWING_SEQUENCE state"));
      #endif
      simonState=simonStates::SHOWING_SEQUENCE;
    }break;
    case simonStates::SHOWING_SEQUENCE:{
      if(currentShowingSequence<gameSequenceList.size()){
        SimonButton *currentShowingButton = gameSequenceList.get(currentShowingSequence);
        uint8_t pin = currentShowingButton->getLightPin();
        uint16_t delayTime = currentShowingButton->getLightOnTime();
        #ifdef DEBUG_SERIAL
        Serial.print(F("simonStateMachine[SHOWING_SEQUENCE]:Showing "));
        Serial.print(pin);
        Serial.println(F(" light."));
        Serial.print(F("simonStateMachine[SHOWING_SEQUENCE]:Delay "));
        Serial.print(delayTime);
        Serial.println(F(" ms."));
        #endif
        digitalWrite(pin, true);
        delay(delayTime);
        digitalWrite(pin, false);
        delay(delayTime/2);
        currentShowingSequence++;
      }else{
        #ifdef DEBUG_SERIAL
        Serial.println(F("simonStateMachine[SHOWING_SEQUENCE]:Going to WAITING_SEQUENCE state"));
        #endif
        simonState=simonStates::WAITING_SEQUENCE;
        currentWaitingSequence=0;
      }
    }break;
    case simonStates::WAITING_SEQUENCE:{
      SimonButton *expectedButton = gameSequenceList.get(currentWaitingSequence);
      for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
        SimonButton *currentButton = gameButtonsList.getNode(i)->data;
        currentButton->update();
        if(currentButton->fell()){
            digitalWrite(currentButton->getLightPin(), true);
            if(expectedButton==currentButton){
            #ifdef DEBUG_SERIAL
            Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Button pressed OK"));
            #endif
            currentWaitingSequence++;
            if(currentWaitingSequence==GAME_SEQUENCE_LENGHT){
              #ifdef DEBUG_SERIAL
              Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Going to WIN state"));
              #endif
              bool realesed = false;
              while(realesed==false){
                currentButton->update();
                realesed=currentButton->rose();
              }
              digitalWrite(currentButton->getLightPin(), false);

              simonState=simonStates::WIN;
            }else if(currentWaitingSequence==gameSequenceList.size()){
              bool realesed = false;
              while(realesed==false){
                currentButton->update();
                realesed=currentButton->rose();
              }
              digitalWrite(currentButton->getLightPin(), false);

              delay(currentButton->getLightOnTime()/2);

              #ifdef DEBUG_SERIAL
              Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Adding new button."));
              #endif

              randomSeed(analogRead(A1));
              uint8_t randomButtonIndex = random(0,gameButtonsList.size());
              gameSequenceList.add(gameButtonsList.get(randomButtonIndex));
              currentShowingSequence=0;
              simonState=simonStates::SHOWING_SEQUENCE;

              #ifdef DEBUG_SERIAL
              Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Going to SHOWING_SEQUENCE state"));
              #endif
            }
          }else{
            #ifdef DEBUG_SERIAL
            Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Button pressed ERROR"));
            Serial.println(F("simonStateMachine[WAITING_SEQUENCE]:Going to ERROR state"));
            #endif

            bool realesed = false;
            while(realesed==false){
              currentButton->update();
              realesed=currentButton->rose();
            }
            digitalWrite(currentButton->getLightPin(), false);

            simonState=simonStates::ERROR;
          }
        }else if(currentButton->rose()){
          digitalWrite(currentButton->getLightPin(), false);
        }
      }
    }break;
    case simonStates::ERROR:{
      Serial.println(F("simonStateMachine[ERROR]:Going to RESET state"));
      simonState=simonStates::RESET;
    }break;
    case simonStates::WIN:{
      Serial.println(F("simonStateMachine[WIN]:Going to RESET state"));
      simonState=simonStates::RESET;
      return true;
    }break;
    default:{
      return false;
    }break;
  }
  return false;
}

void gameStateMachine(){
  /*
  RESET,
  IDLE,
  PLAYING,
  WIN
  */
  switch (gameState) {
    case gameStates::RESET:{
      bool goingToWaitingButtonCondition = false;
      delay(2000);
      goingToWaitingButtonCondition=true;
      if(goingToWaitingButtonCondition){
        #ifdef DEBUG_SERIAL
        Serial.println(F("gameStateMachine[RESET]:Going to WAITING_BUTTON state"));
        #endif
        digitalWrite(LOCK_PIN, LOCK_ENABLE);
        gameState=gameStates::WAITING_BUTTON;
      }
    }break;
    case gameStates::WAITING_BUTTON:{
      bool goToPlay=false;
      for (uint8_t i = 0; i < gameButtonsList.size(); i++) {
        SimonButton *currentButton = gameButtonsList.getNode(i)->data;
        currentButton->update();
        if(currentButton->fell()){
          #ifdef DEBUG_SERIAL
          Serial.print(F("gameStateMachine[IDLE]:Button pressed:"));
          Serial.println(currentButton->getButtonPin());
          #endif
          goToPlay=true;
          i=gameButtonsList.size()+1;
        }
      }
      if(goToPlay){
        #ifdef DEBUG_SERIAL
        Serial.println(F("gameStateMachine[IDLE]:Going to PLAYING state"));
        #endif
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
      }
    }break;
    case gameStates::WIN:{
      bool goingToResetCondition = false;
      delay(2000);
      goingToResetCondition=true;
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
