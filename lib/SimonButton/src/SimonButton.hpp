#include "Bounce2.h"
#include <inttypes.h>

class SimonButton{
public:
  enum class lightStates{
    RESET,
    INIT,
    ON,
    ON_TEMPORIZED,
    OFF_TEMPORIZED,
    OFF
  };
  enum class lightOrders{
    NOTHING,
    ON,
    OFF,
    ON_1SEG,
    ON_VARIABLE,
    ON_GLOBAL,
    RESET_TIMER_VARIABLE,
    ON_05SEG,
    OFF_05SEG
  };
  SimonButton(uint8_t buttonPin,uint8_t lightPin,uint16_t lightOnTime, uint16_t trackId);
  bool fell();
  bool rose();
  bool read();
  uint16_t getLightOnTime();
  uint8_t getLightPin();
  uint8_t getButtonPin();
  uint16_t getTrackId();
  void update();
  void putLightOrder(lightOrders);
  SimonButton::lightStates getLightStatus();
  void reset();
  void decreaseTimeOutGlobal();
  void increaseTimeOutGlobal();

private:
  Bounce *_button;
  uint8_t _lightPin;
  uint16_t _lightOnTime;
  lightStates _lightState;
  uint8_t _trackId;
  lightOrders _lightOrder;
  unsigned int _timeoutVariable;
  unsigned int _timeOutGlobal;
};
