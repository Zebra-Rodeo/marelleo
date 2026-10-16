#include "commandes.h"

namespace {
  constexpr uint32_t STABLE_MS = 30;    // duree de stabilite exigee, bouton
  constexpr uint32_t LONG_MS   = 600;   // au-dela, c'est un appui long

  constexpr uint8_t  ECHANTILLONS  = 16;  // moyenne contre le bruit de conversion
  constexpr uint32_t PERIODE_MS    = 20;  // le volume n'a pas besoin d'aller plus vite
  constexpr uint32_t POT_STABLE_MS = 60;  // trois echantillons d'accord avant d'accepter

  // Calibrage du potentiometre, un B20K donc lineaire. Releve au banc sur un
  // balayage complet : butee basse a 15 environ, butee haute a 4095. Ces bornes
  // sont donc les bonnes telles quelles.
  //
  // L'ADC sature a 3157 mV alors que le curseur monte a 3300, ce qui rend muets
  // les 4 % du haut de la course, une douzaine de degres. Baisser ADC_HAUT
  // n'y changerait rien et agrandirait meme la zone morte : le maximum serait
  // simplement atteint plus tot. Le seul remede serait materiel, une resistance
  // de 1 kOhm entre le 3V3 et l'extremite haute. Ca n'en vaut pas la peine, et
  // arriver au maximum avant la butee est plutot agreable a l'usage.
  constexpr uint16_t ADC_BAS  = 0;
  constexpr uint16_t ADC_HAUT = 4095;
}

void Bouton::begin(uint8_t pin) {
  _pin = pin;
  pinMode(_pin, INPUT_PULLUP);   // ferme vers la masse, aucune resistance externe
  _brut = _stable = digitalRead(_pin);
  _depuis = millis();
}

Bouton::Appui Bouton::lire() {
  bool lu = digitalRead(_pin);
  if (lu != _brut) { _brut = lu; _depuis = millis(); return Appui::Aucun; }
  if (millis() - _depuis < STABLE_MS) return Appui::Aucun;

  if (_brut != _stable) {                       // changement d'etat confirme
    _stable = _brut;
    if (_stable == LOW) {                       // debut d'appui
      _appuyeDepuis = millis();
      _longEnvoye   = false;
      return Appui::Aucun;
    }
    // relachement : court seulement si le long n'a pas deja ete annonce
    return _longEnvoye ? Appui::Aucun : Appui::Court;
  }

  // maintenu : on annonce le long des le seuil franchi, pas au relachement.
  // Annoncer au relachement donnerait la sensation d'un objet qui traine.
  if (_stable == LOW && !_longEnvoye && millis() - _appuyeDepuis >= LONG_MS) {
    _longEnvoye = true;
    return Appui::Long;
  }
  return Appui::Aucun;
}

uint8_t Potentiometre::versNiveau(uint16_t brut) const {
  int32_t dans = constrain((int32_t)brut, (int32_t)ADC_BAS, (int32_t)ADC_HAUT) - ADC_BAS;
  int32_t plage = (int32_t)ADC_HAUT - ADC_BAS;
  if (plage <= 0) return _mini;
  // arrondi au plus proche plutot que troncature, sinon le dernier cran est
  // presque inatteignable et le premier occupe deux fois trop de course
  return _mini + (uint8_t)((dans * (_maxi - _mini) + plage / 2) / plage);
}

void Potentiometre::begin(uint8_t pin, uint8_t mini, uint8_t maxi) {
  _pin = pin; _mini = mini; _maxi = maxi;
  // Une premiere lecture attache la broche au peripherique ADC. Regler
  // l'attenuation avant cela echoue avec "Pin is not configured as analog
  // channel", et la valeur rendue au demarrage est alors fausse.
  analogRead(_pin);
  analogSetPinAttenuation(_pin, ADC_11db);   // pleine echelle jusqu'a environ 3,1 V

  uint32_t s = 0;
  for (uint8_t i = 0; i < ECHANTILLONS; i++) s += analogRead(_pin);
  _brut   = s / ECHANTILLONS;
  _niveau = _candidat = versNiveau(_brut);
  _depuis = _dernierEch = millis();
}

bool Potentiometre::lire() {
  if (millis() - _dernierEch < PERIODE_MS) return false;
  _dernierEch = millis();

  uint32_t s = 0;
  for (uint8_t i = 0; i < ECHANTILLONS; i++) s += analogRead(_pin);
  _brut = s / ECHANTILLONS;

  // Stabilite par integration, comme pour le bouton. Pose sur la frontiere entre
  // deux crans, le bruit residuel fait alterner les candidats, le compteur repart
  // a zero et le volume ne bouge pas : c'est exactement le comportement voulu.
  // Sans cela le niveau papilloterait tout seul, molette immobile.
  uint8_t n = versNiveau(_brut);
  if (n != _candidat) { _candidat = n; _depuis = millis(); return false; }
  if (_candidat == _niveau) return false;
  if (millis() - _depuis < POT_STABLE_MS) return false;

  _niveau = _candidat;
  return true;
}

void Commandes::begin(uint8_t pinPot, uint8_t pinBouton, uint8_t volMin, uint8_t volMax) {
  _bouton.begin(pinBouton);
  _pot.begin(pinPot, volMin, volMax);
}

Evenement Commandes::lire() {
  // L'anti-rebond doit tourner a CHAQUE appel, meme quand la molette bouge :
  // sinon un appui pendant une rotation est simplement perdu.
  Bouton::Appui a = _bouton.lire();

  // Priorite au bouton : perdre un rafraichissement de volume ne se voit pas,
  // perdre une pause se sent tout de suite.
  if (a == Bouton::Appui::Court) return Evenement::LecturePause;
  if (a == Bouton::Appui::Long)  return Evenement::Suivant;

  if (_pot.lire()) return Evenement::Volume;
  return Evenement::Rien;
}
