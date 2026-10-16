// Pilote PN532 minimal, en liaison serie (HSU).
// Le protocole tient en une page : on echange des trames encadrees par
// 00 00 FF, avec deux sommes de controle. Rien de plus.
#pragma once
#include <Arduino.h>

class Pn532Hsu {
public:
  Pn532Hsu(HardwareSerial& ser, uint8_t rxPin, uint8_t txPin)
    : _s(ser), _rx(rxPin), _tx(txPin) {}

  bool     begin();                 // reveille, configure, prepare la scrutation
  uint32_t firmwareVersion();       // 0 si pas de reponse
  bool     lireTag(uint8_t* uid, uint8_t& len);  // true si une figurine est posee

  // Endort la puce : elle tombe a quelques microamperes au lieu de la dizaine
  // de milliamperes qu'elle consomme au repos. Elle ne repond plus tant qu'on
  // ne lui a pas renvoye le preambule de reveil, ce que begin() fait.
  bool     dormir();

private:
  HardwareSerial& _s;
  uint8_t _rx, _tx;
  bool _reveille = false;

  void reveiller();
  bool envoyer(const uint8_t* cmd, uint8_t len);          // trame + attente de l'ACK
  int  recevoir(uint8_t* data, uint8_t max, uint32_t tmo); // longueur utile, -1 si echec
  bool lireOctet(uint8_t& b, uint32_t tmo);
};
