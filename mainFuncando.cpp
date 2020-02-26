#include <Arduino.h>
#include "LinkedList.h"
#include "Bounce2.h"
#include "SimonButton.hpp"
#include <SoftwareSerial.h>
#include "DFMiniMp3.h"


#define __DEBUG //Para tener salida por el puerto serie
//#define WITH_LCD //Para tener salida por el LCD 16x2 conectado a I2C


#ifdef WITH_LCD
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
#endif



/***********************************************
              CLASES PROPIAS
************************************************/
//TODO desacoplar esta varaible global
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
      delay(150);

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

/***********************************************
                     DEFINES
************************************************/
#define NEXT_PIN 10
#define PLAY_PAUSE_PIN 11
#define PREV_PIN 12

#define DEFAULT_VOLUME 20
#define MAX_PLAYLIST_SIZE 10

#define RXPIN 8
#define TXPIN 9

/***********************************************
                FUNCIONES PROPIAS
 ***********************************************/
void mainStateMachine(void);

/***********************************************
              TIPOS DE DATOS
************************************************/

enum reprodutorStates {
  RESET,
  PAUSING,
  PLAYING
};


/***********************************************
            VARIABLES Y OBJETOS GLOBALES
 ***********************************************/

//Para la comunicacion del modulo DFPlayer mini y Araduino
SoftwareSerial mySerial(RXPIN, TXPIN);
//Objeto para controlar el mp3
DFMiniMp3<SoftwareSerial, Mp3Notify> myMP3(mySerial);

//Objetos para el antirrebote de los botones
Bounce playPauseButton = Bounce();
Bounce nextButton = Bounce();
Bounce prevButton = Bounce();

reprodutorStates reproductorState = RESET;
uint16_t trackCount = 0;

LinkedList<int> playList = LinkedList<int>();
unsigned int currentTrackInPlaylist = -1;

unsigned int currentShowMessageTime = 0;
unsigned int prevShowMessageTime = 0;
bool showingMessage = false;

#ifdef __DEBUG
//Si el debuggin esta activo toggleamos el led cada vez que se presiona un boton
bool statudLed = false;
#endif


void setup() {

  mySerial.begin(9600);

  myMP3.begin();
  myMP3.stop();

  randomSeed(analogRead(A0));
  trackCount = myMP3.getTotalTrackCount(DfMp3_PlaySource_Sd);
  uint16_t volume = myMP3.getVolume();

#ifdef __DEBUG
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, statudLed);
  Serial.println("Reproductor incializado.");
  Serial.println("Starting...");
  String message = "Cantidad de canciones detectadas:" + String(trackCount);
  Serial.println(message);
  message = "Volumen inicial:" + String(volume);
  Serial.println(message);
  message = "Valor inicial random:" + String(random(1, trackCount + 1));
  Serial.println(message);

#endif

#ifdef WITH_LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Inicializando..");
  delay(1500);
  lcd.clear();
  lcd.print("Reproductor MP3");
  lcd.setCursor(0, 1);
  lcd.print("@arduino.school ");
  prevShowMessageTime = millis();
#endif


}

void loop() {

  playPauseButton.update();
  nextButton.update();
  prevButton.update();

#ifdef DEBUG
  //Si cualquiera de los botones se presiona, toggleamos el led onboard
  if (playPauseButton.fell() || nextButton.fell() || prevButton.fell() ) {
    statudLed = !statudLed;
    digitalWrite(LED_BUILTIN, statudLed);
  }
#endif


#ifdef WITH_LCD
  currentShowMessageTime = millis();
  if (currentShowMessageTime - prevShowMessageTime > 2000 && showingMessage == true) {
    prevShowMessageTime = currentShowMessageTime;
    showingMessage = false;
    lcd.setCursor(0, 1);
    switch (reproductorState) {
      case RESET: {
          lcd.print("@arduino.school ");
        }
        break;
      case PAUSING: {
          String mensaje = "-->Pausa        ";
          lcd.print(mensaje);
        }
        break;
      case PLAYING: {
          String mensaje = "-->Reproduciendo";
          lcd.print(mensaje);
        }
        break;
    }
  }
#endif
  mainStateMachine();
}




void mainStateMachine() {

  /*
    RESET,
    PAUSING,
    PLAYING
  */
  switch (reproductorState) {
    case RESET: {
        if (playPauseButton.fell()||true) {

          unsigned int newTrack = random(1, trackCount + 1);

          playList.add(newTrack);
          myMP3.setVolume(5);
          myMP3.playGlobalTrack(newTrack);

          currentTrackInPlaylist++;
          reproductorState = PLAYING;

#ifdef __DEBUG
          Serial.println("Saliendo del estado RESET");
          Serial.print("Tema inicial:");
          Serial.println(newTrack);
#endif

#ifdef WITH_LCD
          lcd.setCursor(0, 1);
          String mensaje = "-->Reproduciendo";
          lcd.print(mensaje);
#endif

        } else if ( nextButton.fell() ) {

          uint16_t volume = myMP3.getVolume();
          volume++;
          myMP3.setVolume(volume);

#ifdef __DEBUG
          String message = "Aumentando volumen, nuevo valor:" + String(volume);
          Serial.println(message);
#endif
#ifdef WITH_LCD
          lcd.setCursor(0, 1);
          String mensaje = "--> Volumen:" + String(volume) + "  ";
          lcd.print(mensaje);
          showingMessage = true;
          prevShowMessageTime = millis();
#endif

        } else if (prevButton.fell()) {

          uint16_t volume = myMP3.getVolume();
          volume--;
          myMP3.setVolume(volume);

#ifdef __DEBUG
          String message = "Bajando volumen, nuevo valor:" + String(volume);
          Serial.println(message);
#endif
#ifdef WITH_LCD
          lcd.setCursor(0, 1);
          String mensaje = "--> Volumen:" + String(volume) + "  ";
          lcd.print(mensaje);
          showingMessage = true;
          prevShowMessageTime = millis();
#endif
        }
      }
      break;
    case PAUSING: {
        if (playPauseButton.fell()) {
          myMP3.start();
          reproductorState = PLAYING;
#ifdef __DEBUG
          Serial.println("Saliendo del estado PAUSING");
#endif

#ifdef WITH_LCD
          lcd.setCursor(0, 1);
          String mensaje = "-->Reproduciendo";
          lcd.print(mensaje);
#endif
        }
      }
      break;
    case PLAYING: {
        if (playPauseButton.fell()) {
          myMP3.pause();
          reproductorState = PAUSING;
#ifdef __DEBUG
          Serial.println("Saliendo del estado PLAYING");
#endif

#ifdef WITH_LCD
          lcd.setCursor(0, 1);
          String mensaje = "-->Pausa        ";
          lcd.print(mensaje);
#endif
        } else if (nextButton.fell() || playFinished == true) {
          playFinished = false;
          unsigned int lastTrack = playList.get(currentTrackInPlaylist);
          unsigned int playListSize = playList.size();

          currentTrackInPlaylist++;

          //Si es un nuevo tema hay que agregarlo
          if (currentTrackInPlaylist >= playListSize) {
            uint16_t newTrack = random(1, trackCount + 1);
            unsigned int intents = 0;

            //Intento generar un tema nuevo, distinto al anterior
            //Al menos lo intentamos 40 veces
            while (newTrack == lastTrack || intents < 40) {
              newTrack = random(1, trackCount + 1);
              intents++;
            }

            //Si todavia no llegamos al maximo de temas lo sumamos
            if (playListSize <= MAX_PLAYLIST_SIZE) {
              playList.add(newTrack);
            } else {
              //Sino antes removemos el primero que habia
              unsigned int trackRemoved = playList.shift();
              currentTrackInPlaylist--;
              playList.add(newTrack);

#ifdef __DEBUG
              Serial.print("Removed track:");
              Serial.println(trackRemoved);
#endif

            }

          }

          unsigned int currentTrack = playList.get(currentTrackInPlaylist);

#ifdef __DEBUG
          String message = "Siguiente tema: Playlist[" + String(currentTrackInPlaylist) + "]:" + String(currentTrack);
          Serial.println(message);
#endif

#ifdef WITH_LCD
          lcd.setCursor(0, 1);
          lcd.print("                ");
          lcd.setCursor(0, 1);
          String messageLcd = "--> Siguiente:" + String(currentTrack) + "";
          lcd.print(messageLcd);
          showingMessage = true;
          prevShowMessageTime = millis();
#endif
          //Nos comunicamos con el mp3 despues de mandar todos los mensajes
          myMP3.playGlobalTrack(currentTrack);

        } else if (prevButton.fell()) {

          if (currentTrackInPlaylist != 0) {
            currentTrackInPlaylist--;
          }

          unsigned int currentTrack = playList.get(currentTrackInPlaylist);

#ifdef __DEBUG
          String message = "Anterior tema: Playlist[" + String(currentTrackInPlaylist) + "]:" + String(currentTrack);
          Serial.println(message);
#endif
#ifdef WITH_LCD
          lcd.setCursor(0, 1);
          lcd.print("                ");
          lcd.setCursor(0, 1);
          String messageLcd = "--> Anterior:" + String(currentTrack) + "";
          lcd.print(messageLcd);
          showingMessage = true;
          prevShowMessageTime = millis();
#endif
          //Nos comunicamos con el mp3 despues de mandar todos los mensajes
          myMP3.playGlobalTrack(currentTrack);
        }

        //Esto se ejecuta siempre en el estado
        myMP3.loop();
      }
      break;
  }
}
