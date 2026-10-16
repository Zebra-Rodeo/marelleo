#include "pn532_hsu.h"

namespace {
  constexpr uint8_t TFI_HOTE = 0xD4;   // hote -> PN532
  constexpr uint8_t TFI_PUCE = 0xD5;   // PN532 -> hote
  const uint8_t ACK[6] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
}

bool Pn532Hsu::lireOctet(uint8_t& b, uint32_t tmo) {
  uint32_t t0 = millis();
  while (millis() - t0 < tmo) {
    if (_s.available()) { b = _s.read(); return true; }
  }
  return false;
}

void Pn532Hsu::reveiller() {
  // Le PN532 dort a la mise sous tension. Le preambule 0x55 le reveille.
  // Il doit partir dans la MEME rafale que la commande qui suit : si on
  // vide le tampon entre les deux, la puce se rendort et ne repond rien.
  const uint8_t r[] = {0x55, 0x55, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  _s.write(r, sizeof r);
}

bool Pn532Hsu::envoyer(const uint8_t* cmd, uint8_t len) {
  while (_s.available()) _s.read();          // on vide ce qui traine
  if (!_reveille) { reveiller(); _reveille = true; }   // meme rafale que la trame

  const uint8_t LEN = len + 1;               // TFI compris
  const uint8_t LCS = (uint8_t)(0x100 - LEN);
  uint8_t somme = TFI_HOTE;
  for (uint8_t i = 0; i < len; i++) somme += cmd[i];
  const uint8_t DCS = (uint8_t)(0x100 - somme);

  const uint8_t entete[5] = {0x00, 0x00, 0xFF, LEN, LCS};
  _s.write(entete, 5);
  _s.write(TFI_HOTE);
  _s.write(cmd, len);
  _s.write(DCS);
  _s.write((uint8_t)0x00);
  _s.flush();

  // le PN532 accuse reception par une trame ACK fixe de 6 octets
  uint8_t a[6], b; uint8_t n = 0;
  for (; n < 6; n++) {
    if (!lireOctet(b, 200)) break;
    a[n] = b;
  }
  return n == 6 && memcmp(a, ACK, 6) == 0;
}

int Pn532Hsu::recevoir(uint8_t* data, uint8_t max, uint32_t tmo) {
  // on se cale sur le preambule 00 00 FF, en ignorant les zeros de bourrage
  uint8_t b; uint8_t precedent = 0xAA;
  uint32_t t0 = millis();
  bool cale = false;
  while (millis() - t0 < tmo) {
    if (!lireOctet(b, tmo)) return -1;
    if (precedent == 0x00 && b == 0xFF) { cale = true; break; }
    precedent = b;
  }
  if (!cale) return -1;

  uint8_t LEN, LCS;
  if (!lireOctet(LEN, 100) || !lireOctet(LCS, 100)) return -1;
  if ((uint8_t)(LEN + LCS) != 0x00) return -1;          // somme de controle de longueur
  if (LEN < 1) return -1;

  uint8_t tfi;
  if (!lireOctet(tfi, 100) || tfi != TFI_PUCE) return -1;

  const uint8_t n = LEN - 1;                             // octets utiles
  if (n > max) return -1;
  uint8_t somme = tfi;
  for (uint8_t i = 0; i < n; i++) {
    if (!lireOctet(data[i], 100)) return -1;
    somme += data[i];
  }
  uint8_t dcs, postambule;
  if (!lireOctet(dcs, 100)) return -1;
  if ((uint8_t)(somme + dcs) != 0x00) return -1;         // somme de controle des donnees
  lireOctet(postambule, 50);
  return n;
}

bool Pn532Hsu::begin() {
  _s.end();
  _s.begin(115200, SERIAL_8N1, _rx, _tx);
  delay(50);

  // Le tout premier echange apres la mise sous tension rate souvent : la puce
  // n'a pas fini de se reveiller. On reessaie, en renvoyant le preambule.
  for (uint8_t essai = 0; essai < 6; essai++) {
    _reveille = false;
    uint8_t rep[8];

    // SAMConfiguration : mode normal, sans carte SAM
    const uint8_t sam[] = {0x14, 0x01, 0x14, 0x00};
    if (envoyer(sam, sizeof sam) && recevoir(rep, sizeof rep, 500) >= 0) {
      // RFConfiguration : limiter les tentatives d'activation, sinon
      // InListPassiveTarget bloque tant qu'aucune figurine n'est posee.
      const uint8_t rf[] = {0x32, 0x05, 0xFF, 0x01, 0x02};
      if (envoyer(rf, sizeof rf)) recevoir(rep, sizeof rep, 500);
      return true;
    }
    delay(120);
  }
  return false;
}

uint32_t Pn532Hsu::firmwareVersion() {
  const uint8_t cmd[] = {0x02};
  uint8_t rep[8];
  if (!envoyer(cmd, sizeof cmd)) return 0;
  int n = recevoir(rep, sizeof rep, 500);
  if (n < 5) return 0;                       // rep[0] = 0x03 (reponse a 0x02)
  return ((uint32_t)rep[1] << 24) | ((uint32_t)rep[2] << 16) |
         ((uint32_t)rep[3] << 8)  | rep[4];
}

// PowerDown. L'octet qui suit dit par quels moyens la puce accepte d'etre
// reveillee ; le bit 4 est la liaison serie, celle que nous utilisons. Sans lui
// elle ne se reveillerait jamais, et le lecteur serait perdu jusqu'a une coupure
// franche de l'alimentation.
bool Pn532Hsu::dormir() {
  const uint8_t cmd[] = {0x16, 0x10};
  bool ok = envoyer(cmd, sizeof cmd);
  _reveille = false;   // le prochain echange devra renvoyer le preambule
  return ok;
}

bool Pn532Hsu::lireTag(uint8_t* uid, uint8_t& len) {
  const uint8_t cmd[] = {0x4A, 0x01, 0x00};  // InListPassiveTarget, 1 cible, 106 kbps type A
  uint8_t rep[24];
  if (!envoyer(cmd, sizeof cmd)) return false;
  int n = recevoir(rep, sizeof rep, 300);
  // rep[0]=0x4B  rep[1]=NbTg  rep[2]=Tg  rep[3..4]=SENS_RES  rep[5]=SEL_RES
  // rep[6]=longueur de l'UID  rep[7..]=UID
  if (n < 7 || rep[1] == 0) return false;
  len = rep[6];
  if (len > 7) len = 7;
  memcpy(uid, &rep[7], len);
  return true;
}
