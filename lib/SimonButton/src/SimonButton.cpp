#include "SimonButton.hpp"
#include "Bounce2.h"
#include <inttypes.h>

SimonButton::SimonButton(uint8_t buttonPin,uint8_t lightPin,uint16_t lightOnTime, uint16_t trackId){

  _button = new Bounce();
  _button->attach(buttonPin,INPUT);
  _button->interval(35);
  _lightPin=lightPin;
  _lightOnTime=lightOnTime;
  _trackId=trackId;
  _timeoutVariable = 1000;
  _timeOutGlobal = 1000;

  pinMode(_lightPin, OUTPUT);
  _lightState=lightStates::INIT;
}

void SimonButton::putLightOrder(lightOrders order=lightOrders::NOTHING){
  _lightOrder=order;
}

void SimonButton::update(){
   static unsigned long prevTime = 0;
   static unsigned long currentTimeout = 0;

    _button->update();
   unsigned long currentTime=millis();

    switch (_lightState) {
      case lightStates::INIT:{
        _lightState=lightStates::OFF;
        digitalWrite(_lightPin,false);
      }break;
      case lightStates::ON:{
        if(_lightOrder==lightOrders::OFF){
          _lightState=lightStates::OFF;
          digitalWrite(_lightPin,false);
        }
      }break;
      case lightStates::ON_TEMPORIZED:{
        if(currentTime-prevTime>currentTimeout){
          currentTimeout=0;
          _lightState=lightStates::OFF;
          digitalWrite(_lightPin,false);
        }
      }break;
      case lightStates::OFF_TEMPORIZED:{
        if(currentTime-prevTime>currentTimeout){
          currentTimeout=0;
          _lightState=lightStates::OFF;
          digitalWrite(_lightPin,false);
        }
      }break;
      case lightStates::OFF:{
        if(_lightOrder==lightOrders::ON){
          _lightState=lightStates::ON;
          digitalWrite(_lightPin,true);
        }else if(_lightOrder==lightOrders::ON_1SEG){
          digitalWrite(_lightPin,true);
          _lightState=lightStates::ON_TEMPORIZED;
          prevTime=millis();
          currentTimeout=1000;
        }else if(_lightOrder==lightOrders::ON_VARIABLE){
          digitalWrite(_lightPin,true);
          _lightState=lightStates::ON_TEMPORIZED;
          prevTime=millis();
          currentTimeout=_timeoutVariable;
          _timeoutVariable=_timeoutVariable-100;
          if(_timeoutVariable<300){
            _timeoutVariable=300;
          }
        }else if(_lightOrder==lightOrders::ON_GLOBAL){
          digitalWrite(_lightPin,true);
          _lightState=lightStates::ON_TEMPORIZED;
          prevTime=millis();
          currentTimeout=_timeOutGlobal;
        }
      }break;
      default:{

      }break;
    }
    if(_lightOrder==lightOrders::RESET_TIMER_VARIABLE){
      _timeoutVariable=1000;
    }
    _lightOrder=lightOrders::NOTHING;
}
uint16_t SimonButton::getLightOnTime(){
  return _lightOnTime;
}
SimonButton::lightStates SimonButton::getLightStatus(){
  return _lightState;
}

uint16_t SimonButton::getTrackId(){
  return _trackId;
}

uint8_t SimonButton::getLightPin(){
  return _lightPin;
}
void SimonButton::reset(){
  _timeoutVariable=1000;
  _timeOutGlobal=1000;
  putLightOrder(SimonButton::lightOrders::OFF);
  update();
}
void SimonButton::increaseTimeOutGlobal(){
  _timeOutGlobal=_timeOutGlobal+100;
  if(_timeOutGlobal>1000){
    _timeOutGlobal=1000;
  }
}
void SimonButton::decreaseTimeOutGlobal(){
  _timeOutGlobal=_timeOutGlobal-80;
  if(_timeOutGlobal<350){
    _timeOutGlobal=350;
  }
}
uint8_t SimonButton::getButtonPin(){
  return _button->getPin();
}

bool SimonButton::fell(){
    return _button->fell();
}
bool SimonButton::read(){
    return _button->read();
}
bool SimonButton::rose(){
    return _button->rose();
}
