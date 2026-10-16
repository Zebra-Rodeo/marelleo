// Boite a histoires
// Badger une figurine lance sa liste de pistes, qui continue figurine retiree.
// Une autre figurine remplace la lecture. Bouton : pause et piste suivante.
// Potentiometre pour le volume.
// Une page web servie par la boite associe les histoires aux figurines.
#include <Arduino.h>
#include <Audio.h>
#include <LittleFS.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <atomic>
#include <vector>
#include "pn532_hsu.h"
#include "batterie.h"
#include "carillon.h"
#include "library.h"
#include "commandes.h"
#include "maj.h"
#include "reseau.h"
#include "serveur.h"
#include "version.h"

// --- le stockage
// Deux systemes de fichiers, deux roles. La memoire flash porte ce qui fait
// partie du reglage de CETTE boite : la configuration WiFi, envoyee avec
// `pio run -t uploadfs`. La carte SD porte le contenu : les histoires et la
// bibliotheque, alimentees par la page de configuration. La page elle-meme est
// compilee dans le micrologiciel, voir page.h.
//
// Si la carte ne monte pas, le contenu retombe sur la memoire flash, qui garde
// les histoires de test : la boite continue de jouer au lieu de rester muette.
//
// Les GPIO 11 a 14 sont sur l'ADC2, que le WiFi rend muet. Sans importance ici :
// le bus SPI s'en sert en numerique, jamais en analogique.
#ifdef CARTE_IMPRIMEE
constexpr uint8_t PIN_SD_CS = 11, PIN_SD_CLK = 13, PIN_SD_MOSI = 12, PIN_SD_MISO = 14;
#else
constexpr uint8_t PIN_SD_CS = 11, PIN_SD_CLK = 12, PIN_SD_MOSI = 13, PIN_SD_MISO = 14;
#endif
SPIClass spiSD(FSPI);
fs::FS*  contenu = nullptr;
bool     contenuSurSD = false;

bool monterStockage() {
  bool flash = LittleFS.begin();
  spiSD.begin(PIN_SD_CLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  // 4 MHz, 1 MHz en repli pour monter. Des ecritures echouent au hasard :
  // `sdCommand(): token error [13] 0x33`, une reponse corrompue a SEND_STATUS
  // juste apres l'ecriture des secteurs. Un bit se retourne sur le fil, le
  // pilote et la carte ne sont plus d'accord, et tout echoue ensuite jusqu'au
  // remontage : voir reparerStockage(). Monter la carte ne prouve rien sur la
  // fiabilite des ecritures.
  // Trois tentatives : le montage echoue une fois sur deux ou trois au
  // demarrage, et la boite repartait alors sur sa memoire interne, sans aucune
  // histoire. Couper l'alimentation logique de la carte entre deux essais la
  // remet dans son etat de depart, ce qu'une simple nouvelle tentative ne fait
  // pas.
  bool monte = false;
  for (int essai = 1; essai <= 3 && !monte; essai++) {
    monte = SD.begin(PIN_SD_CS, spiSD, 4000000) || SD.begin(PIN_SD_CS, spiSD, 1000000);
    if (monte) break;
    Serial.printf("carte SD : montage refuse, essai %d sur 3\n", essai);
    SD.end();
    spiSD.end();
    delay(200);
    spiSD.begin(PIN_SD_CLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  }
  if (monte) {
    contenu = &SD;       contenuSurSD = true;
  } else if (flash) {
    contenu = &LittleFS; contenuSurSD = false;
  }
  return contenu != nullptr;
}

// Deux cartes, deux brochages. Le prototype cable a la main garde le sien. La
// carte imprimee permute cinq signaux, audio et carte SD, pour que leurs pistes
// ne se croisent pas : le choix a ete calcule sur la geometrie reelle des
// pastilles. On l'active par -DCARTE_IMPRIMEE, voir l'environnement "carte"
// dans platformio.ini.
#ifdef CARTE_IMPRIMEE
constexpr uint8_t PIN_I2S_BCLK = 6, PIN_I2S_LRC = 7, PIN_I2S_DOUT = 5;
#else
constexpr uint8_t PIN_I2S_BCLK = 5, PIN_I2S_LRC = 6, PIN_I2S_DOUT = 7;
#endif
constexpr uint8_t PIN_AMP_SD   = 4;
constexpr uint8_t PIN_NFC_RX   = 8, PIN_NFC_TX = 9;
// La jauge n'existe que sur la carte imprimee : IO1 voit VSYS a travers un pont
// de deux 100 k, et IO2 recoit PGOOD du chargeur. Le prototype n'a ni l'un ni
// l'autre, d'ou 255, qui veut dire « pas de mesure » plutot qu'un chiffre faux.
#ifdef CARTE_IMPRIMEE
constexpr uint8_t PIN_BATTERIE = 1, PIN_USB = 2;
// Le verrou d'extinction : la grille de Q1, qui tire SYSOFF a la masse en
// parallele de l'interrupteur du potentiometre. Haut maintient la boite
// alimentee, bas la laisse s'eteindre.
constexpr uint8_t PIN_ARRET = 21;
#else
constexpr uint8_t PIN_BATTERIE = 255, PIN_USB = 255, PIN_ARRET = 255;
#endif
// Le potentiometre doit etre sur l'ADC1 (GPIO 1 a 10) : l'ADC2 de l'ESP32-S3
// devient muet des que le WiFi est actif.
constexpr uint8_t PIN_POT = 10, PIN_BOUTON = 18;   // GPIO 15, 16 et 17 libres
constexpr uint8_t PIN_LED      = 48;

// La mise en veille se decide sur le NIVEAU de volume, pas sur la valeur brute
// de l'ADC. En butee basse celle-ci flotte entre 0 et 80 selon la position
// exacte de la molette, mesure sur la carte : un seuil brut serre ne se
// declenche jamais, un seuil large mord sur le premier cran utile. Le niveau,
// lui, est deja stabilise par Potentiometre, qui exige trois echantillons
// d'accord avant de changer de cran.
//
// Molette au minimum vaut donc extinction, ce qui est deja le comportement du
// materiel sur batterie : l'interrupteur du potentiometre est juste au-dela.
constexpr uint32_t VEILLE_STABLE_MS = 1500;

// Trace continue de la valeur brute, pour recalibrer ADC_BAS et ADC_HAUT si le
// potentiometre change un jour. Bavarde : cinq lignes par seconde.
constexpr bool     TRACE_POT = false;
constexpr uint32_t POLL_MS = 250;
constexpr uint8_t  MISSES_BEFORE_REMOVED = 3;
constexpr uint8_t  VOLUME_MIN = 3, VOLUME_MAX = 21;  // 21 = plafond de la lib (gain unite)

void modeCharge(bool materielDemarre);   // defini plus bas, appele des le demarrage
void verrou(bool tenu);

// Survit a ESP.restart(), pas a une coupure de courant. Sert a ne pas dire au
// revoir deux fois, ni a saluer une boite qui vient de prendre conge : quand la
// molette etait seulement au minimum, et non au declic, la boite repart et se
// retrouve exactement dans l'etat qui l'avait fait sonner.
RTC_DATA_ATTR bool adieuDit = false;

Audio      audio;
Pn532Hsu   nfc(Serial1, PIN_NFC_RX, PIN_NFC_TX);
Library    biblio;
Commandes  commandes;
Batterie   batterie;
Reseau     reseau;
Serveur    serveur;

// --- la lecture, en mode pointeuse
// On badge une figurine et sa liste de pistes joue jusqu'au bout, meme figurine
// retiree. Badger une autre figurine remplace la lecture. Badger la meme reprend
// si elle etait en pause, et ne fait rien si elle joue deja : un enfant qui la
// repose dix fois ne doit pas relancer dix fois le debut. Le bouton devient la
// commande principale, pause et reprise en appui court, piste suivante en long.
enum class Lecture { Arret, Joue, Pause };
Lecture lecture = Lecture::Arret;

// Leve par la tache audio en fin de fichier, lu par la boucle principale. On ne
// relance rien depuis le rappel : il s'execute dans audio.loop(), et y appeler
// connecttoFS() rentrerait dans l'objet audio en pleine boucle.
std::atomic<bool> finDePiste{false};

String   fichierOuvert;  // fichier que le lecteur tient ouvert, vide sinon
String   uidCharge;      // figurine dont la liste est chargee, meme retiree
String   uidPose;        // figurine physiquement posee en ce moment, vide sinon
size_t   pisteCourante = 0;   // index de la piste jouee, dans la liste de la figurine

// L'ordre dans lequel on parcourt la liste. Il vaut 0,1,2... pour une figurine
// normale, et une permutation pour une figurine en lecture aleatoire. La liste
// elle-meme ne bouge jamais : celui qui la modifie dans la page doit la
// retrouver telle qu'il l'a laissee.
std::vector<uint16_t> ordre;
size_t   position = 0;        // ou l'on en est dans cet ordre
bool     figurinePosee = false;
String   dernierBadge;   // derniere figurine badgee, connue ou non : celle que la page configure
uint8_t  absences = 0;
uint32_t derniereScrutation = 0;
uint8_t  volume = 0;     // toujours dicte par la position du potentiometre

String enHex(const uint8_t* uid, uint8_t len) {
  char b[16] = {0};
  for (uint8_t i = 0; i < len && i < 7; i++) sprintf(b + i * 2, "%02X", uid[i]);
  return String(b);
}
void led(uint8_t r, uint8_t g, uint8_t b) { rgbLedWrite(PIN_LED, r, g, b); }
void tacheAudio(void*) { for (;;) { audio.loop(); vTaskDelay(1); } }

const char* nomLecture() {
  switch (lecture) {
    case Lecture::Joue:  return "joue";
    case Lecture::Pause: return "pause";
    default:             return "arret";
  }
}

void arreter(const char* raison) {
  audio.stopSong();
  digitalWrite(PIN_AMP_SD, LOW);
  lecture = Lecture::Arret;
  fichierOuvert = "";
  Serial.printf("  %s\n", raison);
}

// Une seule ecriture ratee condamne toutes les suivantes : la reponse corrompue
// a SEND_STATUS desynchronise le pilote et la carte, et rien ne les resynchronise
// tant que la carte n'est pas remontee. Mesure avant ce correctif, en envois de
// 1,3 Mo : 8 echecs sur 12, dont les six derniers d'affilee, puis dix d'affilee
// a la serie suivante. Les echecs ne sont donc pas independants, et toute mesure
// faite sans remonter entre deux essais surestime le taux d'erreur reel.
//
// Le remontage ferme le systeme de fichiers sous la tache audio : on arrete la
// lecture avant, sinon elle continue de lire dans un descripteur mort.
bool reparerStockage() {
  if (!contenuSurSD) return false;
  if (lecture != Lecture::Arret) arreter("carte remontee, lecture arretee");
  SD.end();
  spiSD.end();
  delay(20);
  spiSD.begin(PIN_SD_CLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  bool ok = SD.begin(PIN_SD_CS, spiSD, 4000000) || SD.begin(PIN_SD_CS, spiSD, 1000000);
  Serial.printf("carte SD : remontage %s\n", ok ? "reussi" : "echoue");
  return ok;
}

// Une piste est soit un fichier de la carte, soit une adresse a suivre en
// direct. La bibliotheque audio sait les deux, par deux portes voisines ; le
// reste du programme n'a pas a savoir laquelle est ouverte. Une figurine
// "radio" n'est donc pas un cas particulier : c'est une figurine dont la liste
// contient une adresse au lieu d'un nom de fichier.
bool estUnFlux(const String& piste) {
  return piste.startsWith("http://") || piste.startsWith("https://");
}

// Un direct ne se termine pas, il tombe. On le rattrape, mais une adresse morte
// ne doit pas faire tourner la boite en rond : cinq reprises, et on passe a la
// suite. Un flux qui a tenu ses trente secondes a fait ses preuves, son
// compteur repart de zero.
constexpr uint8_t  REPRISES_FLUX_MAX = 5;
constexpr uint32_t FLUX_TENU_MS = 30000;
uint32_t debutFlux = 0;
uint8_t  reprisesFlux = 0;

// Ouvre une piste, quelle que soit sa nature. Rend false si elle est illisible,
// et c'est a l'appelant de decider quoi faire : passer a la suite, ou reprendre.
bool ouvrirPiste(const String& piste) {
  finDePiste = false;   // une fin de piste en retard ne doit pas sauter celle-ci
  digitalWrite(PIN_AMP_SD, HIGH);
  if (estUnFlux(piste)) {
    // Sans reseau, inutile d'attendre le temps mort d'une resolution DNS.
    if (!reseau.connecte()) { Serial.println("  flux : pas de reseau"); return false; }
    if (!audio.connecttohost(piste.c_str())) return false;
    debutFlux = millis();
    return true;
  }
  if (!contenu) return false;
  return audio.connecttoFS(*contenu, piste.c_str());
}

// Joue la liste a partir d'un index. Une piste illisible est sautee plutot que
// de laisser la boite muette ; la boucle est bornee par la liste, jamais infinie.
// A appeler chaque fois qu'une figurine est chargee, ou rechargee apres une
// liste terminee : deux ecoutes de suite ne doivent pas donner le meme desordre.
void preparerOrdre(const Figurine& f) {
  ordre.resize(f.pistes.size());
  for (size_t i = 0; i < ordre.size(); i++) ordre[i] = (uint16_t)i;
  if (!f.aleatoire || ordre.size() < 2) return;
  // Melange de Fisher-Yates. esp_random() tire du bruit materiel, la ou random()
  // non initialise rendrait la meme suite a chaque demarrage.
  for (size_t i = ordre.size() - 1; i > 0; i--) {
    size_t j = esp_random() % (i + 1);
    std::swap(ordre[i], ordre[j]);
  }
}

// L'argument est une position dans l'ordre, pas un index de piste. Les deux se
// confondent hors lecture aleatoire.
void jouerDepuis(const Figurine& f, size_t depart) {
  if (ordre.size() != f.pistes.size()) preparerOrdre(f);
  for (size_t p = depart; p < ordre.size(); p++) {
    size_t i = ordre[p];
    if (i >= f.pistes.size()) continue;
    const String& chemin = f.pistes[i];
    Serial.printf("  %s [%u/%u]%s -> %s\n", f.nom.c_str(),
                  (unsigned)p + 1, (unsigned)f.pistes.size(),
                  f.aleatoire ? " melange" : "", chemin.c_str());
    if (ouvrirPiste(chemin)) {
      position = p;
      pisteCourante = i;
      lecture = Lecture::Joue;
      fichierOuvert = chemin;
      reprisesFlux = 0;
      return;
    }
    Serial.println("  illisible, piste suivante");
  }
  arreter("fin de la liste");
}

void mettreEnPause() {
  if (lecture != Lecture::Joue) return;
  audio.pauseResume();
  digitalWrite(PIN_AMP_SD, LOW);
  lecture = Lecture::Pause;
}

void reprendre() {
  if (lecture != Lecture::Pause) return;
  digitalWrite(PIN_AMP_SD, HIGH);
  audio.pauseResume();
  lecture = Lecture::Joue;
}

void surPose(const String& uid) {
  uidPose = uid;
  dernierBadge = uid;
  // Une figurine inconnue joue la figurine par defaut si library.json en definit
  // une, typiquement un message « pas encore d'histoire » : c'est un geste
  // volontaire de l'enfant, qui merite une reponse. Sans defaut, elle
  // n'interrompt pas l'histoire en cours.
  const Figurine* f = biblio.pour(uid);
  if (!f) { Serial.printf("  figurine inconnue : %s\n", uid.c_str()); return; }

  if (uid == uidCharge) {
    if (lecture == Lecture::Pause)     { Serial.println("  reprise"); reprendre(); }
    else if (lecture == Lecture::Joue) { Serial.println("  deja en lecture"); }
    else { preparerOrdre(*f); jouerDepuis(*f, 0); }   // liste finie : on recommence
    return;
  }
  uidCharge = uid;
  preparerOrdre(*f);
  jouerDepuis(*f, 0);
}

// Appui court, ou bouton lecture de la page : reprend une pause, et relance la
// liste de la figurine chargee si elle est finie ou arretee.
bool lancerOuReprendre() {
  if (lecture == Lecture::Pause) { reprendre(); return true; }
  if (lecture == Lecture::Joue)  return true;
  const Figurine* f = uidCharge.length() ? biblio.pour(uidCharge) : nullptr;
  if (!f) return false;
  preparerOrdre(*f);
  jouerDepuis(*f, 0);
  return true;
}

// Appui long, ou bouton suivante de la page. Reboucle, parce que c'est volontaire.
bool pisteSuivante() {
  const Figurine* f = uidCharge.length() ? biblio.pour(uidCharge) : nullptr;
  if (!f || f->pistes.size() < 2) return false;
  jouerDepuis(*f, (position + 1) % ordre.size());
  return true;
}

// En mode pointeuse, retirer la figurine ne touche pas a la lecture.
void surRetrait() {
  uidPose = "";
  Serial.println("  retiree");
  // En mode « la figurine doit rester posee », retirer la figurine met en
  // pause plutot que d'arreter : la reposer reprend ou l'on en etait, ce que
  // surPose() fait deja.
  if (biblio.poseRequise() && lecture == Lecture::Joue) {
    Serial.println("  pause, la figurine doit rester posee");
    mettreEnPause();
  }
}

// Fin de fichier signalee par la tache audio : piste suivante, ou fin de liste.
// Une liste qui s'acheve s'arrete, elle ne reboucle pas : c'est souvent l'heure
// du coucher. L'appui long, lui, reboucle, parce qu'il est volontaire.
void suivreFinDePiste() {
  if (!finDePiste.exchange(false) || lecture != Lecture::Joue) return;
  const String piste = fichierOuvert;
  fichierOuvert = "";   // la bibliotheque audio l'a deja ferme

  // Une radio qui se tait parce que le WiFi a hoquete doit revenir toute seule.
  if (estUnFlux(piste)) {
    if (millis() - debutFlux >= FLUX_TENU_MS) reprisesFlux = 0;
    if (reprisesFlux < REPRISES_FLUX_MAX) {
      reprisesFlux++;
      Serial.printf("  flux coupe, reprise %u sur %u\n", reprisesFlux, REPRISES_FLUX_MAX);
      delay(500);
      if (ouvrirPiste(piste)) { fichierOuvert = piste; return; }
    }
    Serial.println("  flux perdu, on passe a la suite");
  }

  const Figurine* f = uidCharge.length() ? biblio.pour(uidCharge) : nullptr;
  if (f) jouerDepuis(*f, position + 1);
  else   arreter("fin de la piste");
}

// Outils de developpement sur le port serie :
//   jouer /chemin.m4a   lit un fichier sans figurine, pour valider un format
//   jouer https://...   suit un flux en direct, pour valider une radio
//   badger <uid>        simule un badge, par le meme chemin que le lecteur NFC
//   retirer             simule le retrait de la figurine
//   wifi <res> <mdp>    enregistre un reseau et redemarre, secours sans l'app
// Ils permettent de tester la lecture de bout en bout sans poser de figurine.
// Banc d'essai d'ecriture, sans reseau. Un envoi HTTP fait ecrire la carte
// pendant que la radio WiFi recoit ; ce banc la fait ecrire seule, par blocs de
// 8 Ko. Si la carte n'a aucun refus ici alors qu'elle en a par le reseau, le
// fautif n'est ni la carte ni le module, mais ce que la radio fait a
// l'alimentation ou aux fils du bus. Le fichier d'essai est efface a la fin.
void bancSD(int mo) {
  if (!contenu || mo <= 0) { Serial.println("banc : usage, bancsd <Mo>"); return; }
  static uint8_t bloc[8192];
  for (size_t i = 0; i < sizeof bloc; i++) bloc[i] = (uint8_t)i;
  File f = contenu->open("/banc.bin", "w", true);
  if (!f) { Serial.println("banc : creation impossible"); return; }
  const int blocs = mo * 128;
  int refus = 0;
  uint32_t debut = millis();
  for (int i = 0; i < blocs; i++)
    if (f.write(bloc, sizeof bloc) != sizeof bloc) refus++;
  f.close();
  contenu->remove("/banc.bin");
  uint32_t duree = millis() - debut;
  Serial.printf("banc : %d Mo, %d blocs refuses sur %d, %lu ms, %lu Ko/s\n",
                mo, refus, blocs, (unsigned long)duree,
                duree ? (unsigned long)((uint64_t)blocs * 8 * 1000 / duree) : 0UL);
}

void lireCommandeSerie() {
  static String ligne;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c != '\n') { if (ligne.length() < 160) ligne += c; continue; }
    ligne.trim();
    if (ligne.startsWith("jouer ")) {
      String chemin = ligne.substring(6);
      uidCharge = "";   // hors figurine : rien a enchainer derriere
      Serial.printf("serie : jouer %s\n", chemin.c_str());
      lecture = ouvrirPiste(chemin) ? Lecture::Joue : Lecture::Arret;
      fichierOuvert = lecture == Lecture::Joue ? chemin : String();
      if (lecture == Lecture::Arret) Serial.println("serie : lecture impossible");
    } else if (ligne.startsWith("badger ")) {
      Serial.printf("serie : badge %s\n", ligne.substring(7).c_str());
      surPose(ligne.substring(7));
    } else if (ligne == "retirer") {
      Serial.println("serie : retrait");
      surRetrait();
    } else if (ligne.startsWith("bancsd ")) {
      bancSD(ligne.substring(7).toInt());
    } else if (ligne == "radio off") {
      // Couper la radio pour isoler le bus SPI ; elle revient au redemarrage.
      WiFi.disconnect(true, false);
      WiFi.mode(WIFI_OFF);
      Serial.println("serie : radio coupee, redemarre pour la retrouver");
    } else if (ligne.startsWith("wifi ")) {
      // wifi <reseau> <mot de passe> : secours sans l'app, reseau sans espace
      int espace = ligne.indexOf(' ', 5);
      if (espace < 0) Serial.println("serie : usage, wifi <reseau> <mot de passe>");
      else reseau.forcer(ligne.substring(5, espace), ligne.substring(espace + 1));
    }
    ligne = "";
  }
}

// Commandes venues de la page ou de l'app. Elles passent par la boucle
// principale, comme le lecteur NFC et le bouton : aucun partage entre taches.
bool commander(const String& action, const String& uid, String& erreur) {
  Serial.printf("commande : %s %s\n", action.c_str(), uid.c_str());
  if (action == "pause") {
    if (lecture != Lecture::Joue) { erreur = "rien ne joue"; return false; }
    mettreEnPause();
    return true;
  }
  if (action == "lecture") {
    if (!lancerOuReprendre()) { erreur = "aucune figurine chargee"; return false; }
    return true;
  }
  if (action == "suivante") {
    if (!pisteSuivante()) { erreur = "pas de piste suivante"; return false; }
    return true;
  }
  if (action == "arreter") {
    if (lecture == Lecture::Arret) { erreur = "deja arretee"; return false; }
    arreter("arretee depuis la page");
    return true;
  }
  if (action == "lire") {
    const Figurine* f = biblio.explicite(uid);
    if (!f) { erreur = "figurine inconnue"; return false; }
    uidCharge = uid;
    preparerOrdre(*f);
    jouerDepuis(*f, 0);
    return true;
  }
  erreur = "action inconnue : " + action;
  return false;
}

// La liste d'une figurine vient de changer. Si c'est celle qui est chargee, on
// ne continue pas l'ancien contenu : on relance le nouveau si elle jouait, et
// on l'arrete sinon, pour que la prochaine lecture parte bien de la nouvelle
// liste plutot que de reprendre un fichier qui n'en fait plus partie.
void surModification(const String& uid) {
  if (uid != uidCharge) return;
  const Figurine* f = biblio.explicite(uid);
  if (f && lecture == Lecture::Joue) {
    Serial.println("  liste modifiee, on relance");
    preparerOrdre(*f);
    jouerDepuis(*f, 0);
  }
  else if (lecture != Lecture::Arret) arreter("liste modifiee, lecture arretee");
}

// connue dit si la figurine est explicitement associee ; f est ce qu'elle joue,
// eventuellement la figurine par defaut.
void publierFigurine(JsonObject o, const String& uid, const Figurine* f) {
  o["uid"] = uid;
  o["connue"] = biblio.explicite(uid) != nullptr;
  if (!f) return;
  o["nom"] = f->nom;
  o["aleatoire"] = f->aleatoire;
  JsonArray pistes = o["pistes"].to<JsonArray>();
  for (const String& p : f->pistes) pistes.add(p);
}

// Ce que la page voit de la boite. La lire ne change rien a son comportement.
void publierEtat(JsonDocument& d) {
  JsonObject l = d["lecture"].to<JsonObject>();
  l["etat"] = nomLecture();
  if (uidCharge.length()) {
    publierFigurine(l["figurine"].to<JsonObject>(), uidCharge, biblio.pour(uidCharge));
    l["piste"] = pisteCourante;
  }
  if (fichierOuvert.length()) l["fichier"] = fichierOuvert;

  if (dernierBadge.length()) {
    JsonObject b = d["badge"].to<JsonObject>();
    publierFigurine(b, dernierBadge, biblio.explicite(dernierBadge));
    b["posee"] = dernierBadge == uidPose;
  } else {
    d["badge"] = nullptr;
  }

  d["poseRequise"] = biblio.poseRequise();
  d["molette"]  = commandes.potBrut();   // brut, pour regler les seuils de veille
  d["stockage"] = contenuSurSD ? "carte SD" : "mémoire flash";
  d["reseau"]   = reseau.description();
  // en Ko : ArduinoJson ne garantit pas les entiers 64 bits sur toutes les cibles
  uint64_t total   = contenuSurSD ? SD.totalBytes() : LittleFS.totalBytes();
  uint64_t utilise = contenuSurSD ? SD.usedBytes()  : LittleFS.usedBytes();
  d["libre"] = (uint32_t)((total - utilise) / 1024);
  d["total"] = (uint32_t)(total / 1024);

  // Absente sur le prototype, qui n'a pas de pont diviseur : la page n'affiche
  // alors rien, ce qui vaut mieux qu'une jauge inventee.
  if (batterie.presente()) {
    JsonObject b = d["batterie"].to<JsonObject>();
    b["volts"]   = roundf(batterie.volts() * 100) / 100.0f;
    b["secteur"] = batterie.surSecteur();
    if (batterie.chargeConnue()) {
      b["pourcent"] = batterie.pourcent();
    } else {
      b["pourcent"] = nullptr;
      // Faute de mieux : le dernier niveau vu sur batterie, clairement annonce
      // comme tel. La cellule n'est mesurable sur aucune broche pendant la charge.
      if (batterie.dernierConnu() != 255) b["dernier"] = batterie.dernierConnu();
    }
  } else {
    d["batterie"] = nullptr;
  }
}

const char* nomEvenement(Evenement e) {
  switch (e) {
    case Evenement::Volume:       return "molette";
    case Evenement::LecturePause: return "appui court";
    case Evenement::Suivant:      return "appui long";
    default:                      return "?";
  }
}

void surCommande(Evenement e) {
  Serial.printf("[%s] ", nomEvenement(e));
  switch (e) {
    case Evenement::Volume:
      volume = commandes.volume();
      audio.setVolume(volume);
      // La valeur brute reste affichee : c'est elle qui sert a caler ADC_BAS et
      // ADC_HAUT dans commandes.cpp, en butee basse puis en butee haute.
      Serial.printf("volume %u  [%u-%u]  brut %u\n",
                    volume, VOLUME_MIN, VOLUME_MAX, commandes.potBrut());
      break;
    case Evenement::LecturePause:
      if (lecture == Lecture::Joue) { mettreEnPause(); Serial.println("pause"); }
      else if (lancerOuReprendre())  Serial.println("lecture");
      else                           Serial.println("aucune figurine badgee");
      break;
    case Evenement::Suivant:
      if (pisteSuivante()) Serial.println("piste suivante");
      else                 Serial.println("pas de piste suivante");
      break;
    default: break;
  }
}

// Diagnostic de cablage. On force tour a tour un tirage interne au haut puis au
// bas et on mesure : une broche en l'air suit docilement le tirage, une broche
// reliee au curseur reste ou le potentiometre la tient. Deux mesures opposees
// qui se ressemblent prouvent donc que le fil du curseur est bon, et evitent de
// confondre un cablage absent avec une piste usee.
void sonderPot() {
  pinMode(PIN_POT, INPUT_PULLUP);   delay(20);
  int haut = analogReadMilliVolts(PIN_POT);
  pinMode(PIN_POT, INPUT_PULLDOWN); delay(20);
  int bas  = analogReadMilliVolts(PIN_POT);
  pinMode(PIN_POT, INPUT);          delay(20);
  Serial.printf("sonde curseur : tirage haut %d mV, tirage bas %d mV -> %s\n",
                haut, bas,
                (haut - bas > 800) ? "BROCHE EN L'AIR, curseur non relie"
                                   : "curseur relie");
}

void setup() {
  Serial.begin(115200); delay(700);
  Serial.printf("\n=== boite a histoires %s ===\n", MARELLEO_VERSION);
  sonderPot();   // toujours : deux liaisons coupees ont deja ete trouvees ainsi
  pinMode(PIN_AMP_SD, OUTPUT); digitalWrite(PIN_AMP_SD, LOW);

  if (!monterStockage()) {
    Serial.println("stockage : aucun systeme de fichiers");
  } else if (contenuSurSD) {
    Serial.printf("stockage : carte SD, %u Mo libres sur %u\n",
                  (unsigned)((SD.totalBytes() - SD.usedBytes()) >> 20),
                  (unsigned)(SD.totalBytes() >> 20));
  } else {
    Serial.printf("stockage : carte SD absente, repli sur la memoire flash (%u Ko sur %u)\n",
                  (unsigned)(LittleFS.usedBytes() / 1024),
                  (unsigned)(LittleFS.totalBytes() / 1024));
  }

  if (contenu) {
    if (biblio.charger(*contenu)) {
      Serial.printf("bibliotheque : %u figurine(s)\n", (unsigned)biblio.nombre());
    } else {
      Serial.printf("bibliotheque : %s\n", biblio.erreur().c_str());
      // On ne cree un fichier vide que s'il est ABSENT. Un JSON invalide reste
      // intact : l'ecraser detruirait des associations qu'on peut encore sauver.
      if (!contenu->exists("/library.json") && biblio.sauver(*contenu))
        Serial.println("bibliotheque : fichier vide cree, a remplir par la page de configuration");
    }
  }

  commandes.begin(PIN_POT, PIN_BOUTON, VOLUME_MIN, VOLUME_MAX);
  volume = commandes.volume();
  Serial.printf("volume : %u  [%u-%u]  brut %u\n",
                volume, VOLUME_MIN, VOLUME_MAX, commandes.potBrut());

  batterie.begin(PIN_BATTERIE, PIN_USB);
  if (batterie.presente()) {
    if (batterie.chargeConnue())
      Serial.printf("batterie : %.2f V, %u %%\n", batterie.volts(), batterie.pourcent());
    else
      Serial.printf("batterie : %.2f V, sur secteur, charge inconnue\n", batterie.volts());
  }

  // Branchee alors qu'elle est eteinte : elle n'a aucune raison de s'allumer
  // pour s'eteindre deux secondes plus tard. On decide avant d'avoir reveille
  // le lecteur, sonne le carillon et demarre la radio, sinon brancher un
  // chargeur sur une boite eteinte la fait chanter et clignoter pour rien.
  if (batterie.surSecteur() && volume == VOLUME_MIN) modeCharge(false);

  if (!nfc.begin()) Serial.println("PN532 injoignable");
  else              Serial.println("PN532 pret");

  // Le carillon apres : il installe son propre canal I2S et le rend, ce que la
  // bibliotheque audio ne permettrait plus une fois setPinout() appele.
  // L'amplificateur doit etre reveille pour qu'on l'entende, et rendormi apres.
  // Molette remontee : le dernier adieu ne compte plus. Toujours en bas : la
  // boite vient de saluer et de repartir, elle ne va pas dire bonjour par
  // dessus.
  if (volume > VOLUME_MIN) adieuDit = false;
  if (!adieuDit) {
    digitalWrite(PIN_AMP_SD, HIGH);
    carillon(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT, volume, VOLUME_MAX);
    digitalWrite(PIN_AMP_SD, LOW);
  }

  Audio::audio_info_callback = [](Audio::msg_t m) {
    if (m.e == Audio::evt_eof) finDePiste = true;
    // Les informations de format ne sortent qu'au demarrage d'une piste : utiles
    // pour verifier qu'un fichier est bien decode, sans bavarder le reste du temps.
    else if (m.e == Audio::evt_info && m.msg) Serial.printf("audio : %s\n", m.msg);
  };
  audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  audio.setVolume(volume);
  xTaskCreatePinnedToCore(tacheAudio, "audio", 8192, nullptr, 4, nullptr, 1);

  // Bouton maintenu trois secondes a l'allumage : on efface les identifiants
  // WiFi pour revenir au portail de configuration. La LED passe au bleu pendant
  // l'attente, et au blanc quand l'effacement est confirme.
  bool effacer = false;
  pinMode(PIN_BOUTON, INPUT_PULLUP);
  if (digitalRead(PIN_BOUTON) == LOW) {
    led(0, 0, 30);
    uint32_t debut = millis();
    while (digitalRead(PIN_BOUTON) == LOW && millis() - debut < 3000) delay(20);
    effacer = millis() - debut >= 3000;
    led(effacer ? 20 : 0, effacer ? 20 : 0, effacer ? 20 : 0);
  }
  // Avant le reseau : si on tourne sur une version fraichement gravee qui n'a
  // jamais reussi a se connecter, c'est ici qu'on fait demi-tour.
  maj.begin();
  reseau.begin(effacer);
  Lecteur lecteur;
  lecteur.etat      = publierEtat;
  lecteur.commander = commander;
  lecteur.modifiee  = surModification;
  lecteur.espaceLibre = []() -> uint64_t {
    return contenuSurSD ? SD.totalBytes() - SD.usedBytes() : LittleFS.totalBytes() - LittleFS.usedBytes();
  };
  lecteur.liberer   = [](const String& chemin) {
    if (chemin == fichierOuvert) arreter("fichier remplace ou supprime, lecture arretee");
  };
  lecteur.reparer   = []() { reparerStockage(); };
  serveur.begin(contenu, biblio, lecteur, reseau);
  Serial.println("Pose une figurine.\n");
}

// --- Mode charge -----------------------------------------------------------
//
// Eteindre au potentiometre ne coupe rien tant que l'USB alimente la carte :
// SYSOFF ne commande que le transistor de la batterie, et l'entree continue
// d'alimenter le systeme par un chemin que cet interrupteur ne touche pas. La
// boite reste donc allumee, elle consomme, et la moitie du courant de charge
// part dans le WiFi au lieu d'aller dans la cellule.
//
// Faute de pouvoir couper le materiel, on imite l'extinction : molette en butee
// basse et USB present veulent dire « l'utilisateur a eteint, et elle est au
// mur ». On arrete la lecture, on eteint la LED, on coupe la radio et on ralentit
// le processeur. Ne reste qu'une boucle qui surveille la molette.
//
// Le revers est assume : une boite en charge ne repond plus sur le reseau. C'est
// coherent avec le geste qui vient de l'eteindre.
// Le verrou d'extinction : Q1 tient SYSOFF a la masse en parallele de
// l'interrupteur de la molette. Tenu, la boite reste alimentee par la batterie
// meme apres le declic, ce qui lui laisse le temps de sonner. Lache, elle rend
// la main a l'interrupteur, et s'eteint donc pour de bon s'il est ouvert.
void verrou(bool tenu) {
  if (PIN_ARRET == 255) return;
  pinMode(PIN_ARRET, OUTPUT);
  digitalWrite(PIN_ARRET, tenu ? HIGH : LOW);
}

// Le geste d'extinction, entendu. L'amplificateur dort entre deux lectures : on
// le reveille le temps de la phrase, sinon elle part dans le vide.
void adieu() {
  if (lecture != Lecture::Arret) arreter("extinction");
  digitalWrite(PIN_AMP_SD, HIGH);
  carillonAdieu(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT, commandes.volume(), VOLUME_MAX);
  digitalWrite(PIN_AMP_SD, LOW);
}

// Molette descendue jusqu'en bas : la boite dit au revoir, puis rend la main a
// l'interrupteur. Si celui-ci est ouvert, le courant tombe ici meme, ce qui est
// exactement ce que le geste voulait dire. Sinon la molette n'etait qu'au
// minimum sans le declic, et la boite repart propre plutot que de tourner avec
// un bus audio dont le carillon a emporte le routage.
void eteindre() {
  adieu();
  if (batterie.surSecteur()) { modeCharge(true); return; }   // ne revient jamais
  Serial.println("extinction : verrou lache");
  verrou(false);
  delay(2000);
  ESP.restart();
}

void modeCharge(bool materielDemarre) {
  Serial.println("charge : molette en butee basse et USB present, mise en veille");
  led(0, 0, 0);
  // Appelee depuis le demarrage, rien n'est encore allume : l'audio n'existe
  // pas, et le PN532 dort deja puisqu'il sort de sa mise sous tension sans
  // qu'on lui ait parle. Il n'y a donc rien a eteindre.
  if (materielDemarre) {
    if (lecture != Lecture::Arret) arreter("mise en veille");
    // Le lecteur NFC ne sert a rien ici et consomme une dizaine de milliamperes
    // au repos. On l'endort. Sa LED, elle, restera probablement allumee : sur
    // ces modules elle est cablee sur l'alimentation, que la carte ne peut pas
    // couper.
    Serial.println(nfc.dormir() ? "charge : lecteur NFC endormi"
                                : "charge : le lecteur NFC n'a pas repondu");
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
  }

  // Le verrou d'extinction, enfin utilise. En butee basse, l'interrupteur lache
  // SYSOFF, et le chargeur suspend la charge en meme temps qu'il deconnecte la
  // batterie du systeme : mesure sur la carte, USB branche, 3,29 V molette haute
  // contre 3,11 V molette basse. On tient donc SYSOFF a la masse par Q1 tant que
  // l'USB est la, pour que la charge reprenne alors meme que la boite est
  // eteinte du point de vue de celui qui s'en sert.
  verrou(true);
  Serial.println("charge : verrou tenu, la charge peut reprendre");
  setCpuFrequencyMhz(80);

  uint32_t hautDepuis = 0;
  for (;;) {
    delay(50);
    commandes.lire();
    batterie.lire();   // se limite elle-meme a une mesure toutes les deux secondes

    // L'USB est parti : on lache le verrou et la boite s'eteint pour de bon,
    // ce que descendre la molette voulait dire. Sans cette ligne, une boite
    // debranchee resterait allumee jusqu'a vider sa cellule.
    if (!batterie.surSecteur()) {
      Serial.println("charge : USB retire, extinction");
      verrou(false);
      delay(2000);        // le temps que l'alimentation tombe
      ESP.restart();      // si elle n'est pas tombee, on repart proprement
    }

    if (commandes.volume() == VOLUME_MIN) { hautDepuis = 0; continue; }
    if (!hautDepuis) { hautDepuis = millis(); continue; }
    if (millis() - hautDepuis < 300) continue;

    // On redemarre au lieu de rallumer piece par piece. C'est exactement ce que
    // fait la boite sur batterie quand on remonte la molette, donc le
    // comportement devient le meme avec et sans USB. Et ca evite de remonter a
    // la main le WiFi, le mDNS, le serveur et le portail, chacun avec ses pieges.
    Serial.println("charge : reveil");
    verrou(false);                                        // l'interrupteur reprend la main
    delay(100);
    ESP.restart();
  }
}

void loop() {
  // Avant tout le reste : si la boite doit s'eteindre, rien d'autre n'a de sens.
  //
  // Le verrou est tenu des que la molette touche le bas, et lache une seconde
  // et demie plus tard, le carillon passe. C'est la seule fenetre ou la boite
  // peut survivre au declic de l'interrupteur, et elle est courte a dessein :
  // un micrologiciel bloque pendant qu'elle est ouverte garderait la batterie
  // connectee, molette eteinte.
  //
  // Sans verrou, sur le prototype, le declic coupe tout avant qu'on ait pu
  // dire quoi que ce soit : on laisse alors la boite tranquille.
  static uint32_t basDepuis = 0;
  const bool enBas = commandes.volume() == VOLUME_MIN;
  const bool peutSaluer = PIN_ARRET != 255 || batterie.surSecteur();
  if (enBas && peutSaluer) {
    if (!basDepuis) { basDepuis = millis(); verrou(true); }
    else if (!adieuDit && millis() - basDepuis > VEILLE_STABLE_MS) {
      adieuDit = true;
      eteindre();              // ne revient que si la boite est restee en vie
      basDepuis = 0;
    }
  } else {
    if (basDepuis && !batterie.surSecteur()) verrou(false);
    basDepuis = 0;
    adieuDit = false;
  }

  // Le reseau passe avant la scrutation NFC, qui ne tourne que toutes les
  // 250 ms : place apres, chaque requete web attendrait jusqu'a ce delai.
  reseau.loop();
  serveur.loop();
  batterie.lire();   // se limite elle-meme a une mesure toutes les deux secondes

  // Une version a l'essai est declaree bonne des qu'elle voit le reseau : c'est
  // la seule chose qu'on lui demande de prouver, parce que c'est la seule dont
  // depend la possibilite de la corriger.
  if (maj.enEssai() && reseau.connecte()) maj.valider();

  // Graver le micrologiciel pendant qu'une histoire joue, c'est demander a la
  // meme puce flash de servir deux maitres. On coupe le son, poliment.
  if (maj.occupee() && lecture != Lecture::Arret) arreter("mise a jour en cours");

  suivreFinDePiste();
  lireCommandeSerie();

  Evenement e = commandes.lire();
  if (e != Evenement::Rien) surCommande(e);

  if (TRACE_POT) {
    // Cadence fixe et inconditionnelle : c'est la seule facon de distinguer un
    // balayage regulier de la main d'un faux contact sur la piste. Une trace
    // declenchee par le changement melange les deux.
    static uint32_t trace = 0;
    if (millis() - trace > 200) {
      trace = millis();
      Serial.printf("brut %u\n", commandes.potBrut());
    }
  }
  if (millis() - derniereScrutation < POLL_MS) return;
  derniereScrutation = millis();

  uint8_t uid[7], len = 0;
  bool vue = nfc.lireTag(uid, len);
  if (vue) {
    absences = 0;
    if (!figurinePosee) { figurinePosee = true; surPose(enHex(uid, len)); }
  } else if (figurinePosee && ++absences >= MISSES_BEFORE_REMOVED) {
    figurinePosee = false; absences = 0; surRetrait();
  }

  if (lecture == Lecture::Joue)       led(0, 30, 0);
  else if (lecture == Lecture::Pause) led(25, 12, 0);
  else if (figurinePosee)       led(0, 0, 25);
  else                          led(6, 0, 0);
}
