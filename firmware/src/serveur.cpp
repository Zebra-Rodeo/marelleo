#include "serveur.h"
#include <HTTPClient.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "distant.h"
#include "maj.h"
#include "portail.h"
#include "page.h"
#include "version.h"

namespace {
  WebServer srv(80);
  constexpr const char* DOSSIER      = "/histoires";
  // En-tete portant le nom du fichier envoye, encode par encodeURIComponent.
  constexpr const char* EN_TETE_NOM  = "X-Nom-Fichier";

  // On ne rapatrie que le debut d'un flux. Un flux de podcast fourni est
  // enorme, 11 Mo mesures chez Acast pour une emission quotidienne, alors que
  // la page n'affiche que les episodes recents, et qu'un flux RSS liste le plus
  // recent en premier. 512 Ko couvrent largement les dernieres dizaines
  // d'episodes et arrivent en deux secondes, la ou le flux entier bloquerait la
  // boite une minute. La page recoupe le XML tronque au dernier episode entier.
  constexpr size_t RELAIS_DEFAUT = 256 * 1024;
  constexpr size_t RELAIS_MAX    = 4 * 1024 * 1024;


  // Types acceptes par le relais : du texte, jamais un media. Un editeur qui
  // n'annonce rien passe, certains flux anciens omettent l'en-tete.
  bool typeTexte(const String& type) {
    if (!type.length()) return true;
    String t = type; t.toLowerCase();
    return t.startsWith("text/") || t.indexOf("xml") >= 0 || t.indexOf("json") >= 0;
  }

  void repondre(const JsonDocument& d, int code = 200) {
    String corps;
    serializeJson(d, corps);
    srv.send(code, "application/json", corps);
  }

  void refuser(int code, const String& message) {
    JsonDocument d;
    d["erreur"] = message;
    repondre(d, code);
  }

  // Les noms de fichiers viennent du telephone : accents, espaces, apostrophes.
  // On ne garde que des minuscules ASCII, des chiffres et des tirets, lisibles
  // dans la liste et surs sur FAT comme sur LittleFS. Les lettres accentuees
  // francaises sont ramenees a leur lettre de base plutot que remplacees.
  char sansAccent(uint8_t suite) {
    if ((suite >= 0x80 && suite <= 0x85) || (suite >= 0xA0 && suite <= 0xA5)) return 'a';
    if (suite == 0x87 || suite == 0xA7)                                        return 'c';
    if ((suite >= 0x88 && suite <= 0x8B) || (suite >= 0xA8 && suite <= 0xAB)) return 'e';
    if ((suite >= 0x8C && suite <= 0x8F) || (suite >= 0xAC && suite <= 0xAF)) return 'i';
    if ((suite >= 0x92 && suite <= 0x96) || (suite >= 0xB2 && suite <= 0xB6)) return 'o';
    if ((suite >= 0x99 && suite <= 0x9C) || (suite >= 0xB9 && suite <= 0xBC)) return 'u';
    return 0;
  }

  // Minuscules faites a la main et sur l'ASCII seul : tolower() sur un octet
  // d'accent, donc un char negatif, est un comportement indefini en C.
  char minuscule(char c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

  // L'extension audio acceptee, en minuscules, ou une chaine vide. MP3 pour les
  // histoires preparees au Mac, M4A pour l'AAC qu'iOS encode nativement.
  // Filtrer par extension protege aussi library.json et wifi.json : aucune
  // route ne peut les ecraser, les associer ni les supprimer.
  String extensionAudio(const String& chemin) {
    if (chemin.length() < 5) return "";
    const char* e = chemin.c_str() + chemin.length() - 4;
    if (e[0] != '.') return "";
    char ext[4] = {minuscule(e[1]), minuscule(e[2]), minuscule(e[3]), 0};
    return (!strcmp(ext, "mp3") || !strcmp(ext, "m4a")) ? String(".") + ext : String();
  }

  // Un chemin venu du client ne doit designer qu'un fichier audio, sans remonter.
  bool cheminAudioSur(const String& chemin) {
    return chemin.startsWith("/") && chemin.indexOf("..") < 0 && extensionAudio(chemin).length() > 0;
  }

  // Une piste peut aussi etre une adresse a suivre en direct : c'est ce qui
  // fait une figurine "radio". Rien a verifier sur la carte, mais l'adresse
  // part dans un en-tete HTTP, donc ni espace ni caractere de controle, et une
  // longueur bornee pour ne pas laisser remplir la bibliotheque.
  constexpr size_t FLUX_LONGUEUR_MAX = 300;

  bool fluxValide(const String& piste) {
    if (!piste.startsWith("http://") && !piste.startsWith("https://")) return false;
    if (piste.length() > FLUX_LONGUEUR_MAX) return false;
    for (size_t i = 0; i < piste.length(); i++) {
      char c = piste[i];
      if (c <= ' ' || c == 0x7f) return false;
    }
    return true;
  }

  bool estUneAdresse(const String& piste) {
    return piste.startsWith("http://") || piste.startsWith("https://");
  }

  String nomDeFichier(const String& brut) {
    String nom = brut.substring(max(brut.lastIndexOf('/'), brut.lastIndexOf('\\')) + 1);
    String ext = extensionAudio(nom);
    if (!ext.length()) return "";

    const char* fin = nom.c_str() + nom.length() - 4;   // sans l'extension
    String sortie;
    bool tiret = false;
    for (const char* p = nom.c_str(); p < fin; p++) {
      uint8_t c = minuscule(*p);
      char garde = 0;
      if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) garde = c;
      else if (c == 0xC3 && p + 1 < fin)                     garde = sansAccent(*++p);
      if (garde) { sortie += garde; tiret = false; }
      else if (!tiret && sortie.length()) { sortie += '-'; tiret = true; }
    }
    while (sortie.endsWith("-")) sortie.remove(sortie.length() - 1);
    // Couper par la fin mangeait le suffixe unique que l'app ajoute pour eviter
    // les collisions : deux episodes d'un podcast aux titres longs mais au debut
    // identique finissaient sur le meme nom, le second ecrasant le premier.
    // Observe sur la carte : « la-carte-du-monde-vous-ment-mercator-en-.m4a ».
    // On coupe donc au milieu et on garde les deux bouts.
    if (sortie.length() > 40)
      sortie = sortie.substring(0, 35) + sortie.substring(sortie.length() - 5);
    while (sortie.endsWith("-")) sortie.remove(sortie.length() - 1);
    if (!sortie.length()) sortie = "histoire";
    return sortie + ext;
  }
}

void Serveur::begin(fs::FS* contenu, Library& biblio, const Lecteur& lecteur, Reseau& reseau) {
  _contenu = contenu;
  _biblio  = &biblio;
  _lecteur = lecteur;
  _reseau  = &reseau;

  // La page est compilee dans le micrologiciel : pas de fichier a envoyer
  // separement, et rien qu'un `uploadfs` pourrait effacer.
  // Sans reseau connu, toute adresse demandee rend la page de reglage : c'est
  // ce que le telephone va chercher pour decider s'il y a un portail a ouvrir,
  // et il ne demande justement pas la racine.
  srv.on("/", HTTP_GET, [this]() {
    srv.send_P(200, "text/html; charset=utf-8", _reseau->enPortail() ? PORTAIL : PAGE);
  });

  // Les reseaux a portee, pour que la page de reglage ait quelque chose a
  // proposer. La scrutation bloque deux secondes, on ne la fait qu'ici.
  srv.on("/api/reseaux", HTTP_GET, [this]() {
    JsonDocument d;
    _reseau->reseauxAPortee(d);
    repondre(d);
  });

  srv.on("/api/etat", HTTP_GET, [this]() {
    JsonDocument d;
    _lecteur.etat(d);
    ajouterRecuperation(d);
    maj.ajouterEtat(d);
    repondre(d);
  });

  srv.on("/api/bibliotheque", HTTP_GET,  [this]() { publierBibliotheque(); });
  srv.on("/api/associer",     HTTP_POST, [this]() { associer(); });
  srv.on("/api/dissocier",    HTTP_POST, [this]() { dissocier(); });
  srv.on("/api/supprimer",    HTTP_POST, [this]() { supprimer(); });
  srv.on("/api/commande",     HTTP_POST, [this]() { commander(); });
  srv.on("/api/reglages",     HTTP_POST, [this]() { reglages(); });
  srv.on("/api/infos",        HTTP_GET,  [this]() { publierInfos(); });
  srv.on("/api/wifi",         HTTP_GET,  [this]() { publierWifi(); });
  srv.on("/api/wifi",         HTTP_POST, [this]() { changerWifi(); });
  srv.on("/api/relais",       HTTP_GET,  [this]() { relayer(); });
  srv.on("/api/recuperer",    HTTP_POST, [this]() { recuperer(); });

  // Mise a jour du micrologiciel. La consultation interroge le manifeste
  // distant et ne grave rien ; l'installation, elle, fait redemarrer la boite.
  srv.on("/api/maj", HTTP_GET, []() {
    JsonDocument d;
    String erreur;
    if (maj.consulter(erreur, d)) repondre(d);
    else {
      d["erreur"] = erreur;
      d["actuelle"] = MARELLEO_VERSION;
      repondre(d, 502);
    }
  });
  srv.on("/api/maj", HTTP_POST, []() {
    String erreur;
    if (!maj.installer(erreur)) { refuser(409, erreur); return; }
    JsonDocument d;
    d["lancee"] = true;
    repondre(d, 202);
  });
  srv.on("/api/televerser",   HTTP_POST,
         [this]() { terminerEnvoi(); },
         [this]() { recevoirMorceau(); });

  srv.onNotFound([this]() {
    if (_reseau->enPortail()) {
      srv.send_P(200, "text/html; charset=utf-8", PORTAIL);
      return;
    }
    refuser(404, "introuvable");
  });
  const char* enTetes[] = {EN_TETE_NOM};
  srv.collectHeaders(enTetes, 1);   // WebServer ignore tout en-tete non declare
  srv.begin();
}

void Serveur::loop() {
  srv.handleClient();
  if (_reparerDemande.load()) {
    if (_lecteur.reparer) _lecteur.reparer();
    _reparerDemande = false;
  }
}

// Demande a la boucle principale de remonter la carte, et attend qu'elle l'ait
// fait. Appelee depuis la tache de telechargement.
void Serveur::demanderReparation() {
  _reparerDemande = true;
  for (int i = 0; i < 100 && _reparerDemande.load(); i++) vTaskDelay(pdMS_TO_TICKS(100));
}

// Le fichier arrive en corps brut, pas en formulaire multipart. WebServer lit
// un envoi multipart octet par octet pour y chercher la frontiere entre les
// parties, ce qui plafonnait le debit a une centaine de Ko/s, veille WiFi coupee
// ou non. Un corps brut est lu par blocs.
//
// Le nom du fichier passe dans un en-tete et non dans l'adresse : sur le chemin
// du corps brut, WebServer ne decode jamais les parametres de l'URL, et
// srv.arg() y reste vide. Les en-tetes, eux, sont lus avant le corps.
//
// L'envoi bloque la boucle principale le temps du transfert : plus de lecture
// NFC ni de molette pendant quelques secondes. La tache audio, elle, continue :
// une histoire en cours n'est pas interrompue.
// Vide le tampon dans le fichier. Faux si la carte refuse : le message distingue
// une carte pleine d'une carte qui rate son ecriture, les deux rendant la meme
// ecriture courte.
bool Serveur::ecrireTampon() {
  if (!_dansTampon) return true;
  size_t n = _fichier.write(_tampon, _dansTampon);
  _ecrits += n;
  bool ok = n == _dansTampon;
  _dansTampon = 0;
  if (!ok) {
    // Une ecriture courte ne veut pas dire carte pleine : c'est aussi ce que
    // rend une erreur de la carte. Longtemps affiche « stockage plein » sur une
    // carte de 32 Go presque vide, ce qui a egare le diagnostic.
    if (_lecteur.espaceLibre() < 64 * 1024) { _erreur = "stockage plein"; _code = 507; }
    else { _erreur = "ecriture refusee par la carte SD, a reessayer"; _code = 503; }
  }
  return ok;
}

void Serveur::recevoirMorceau() {
  HTTPRaw& envoi = srv.raw();

  switch (envoi.status) {
    case RAW_START: {
      _erreur = ""; _code = 200; _ecrits = 0; _chemin = ""; _dansTampon = 0;
      if (!_contenu) { _erreur = "aucun stockage disponible"; _code = 503; return; }
      if (!_tampon) _tampon = (uint8_t*)malloc(TAMPON);
      if (!_tampon) { _erreur = "memoire insuffisante"; _code = 500; return; }
      String brut = WebServer::urlDecode(srv.header(EN_TETE_NOM));
      String nom  = nomDeFichier(brut);
      if (!nom.length()) { _erreur = "seuls les fichiers MP3 et M4A sont acceptes"; _code = 415; return; }
      _contenu->mkdir(DOSSIER);
      _chemin  = String(DOSSIER) + "/" + nom;
      _lecteur.liberer(_chemin);   // un renvoi du meme nom ecrase le fichier, peut-etre en lecture
      _fichier = _contenu->open(_chemin, "w", true);
      if (!_fichier) { _erreur = "creation du fichier impossible"; _code = 500; }
      Serial.printf("envoi : %s -> %s\n", brut.c_str(), _chemin.c_str());
      break;
    }
    case RAW_WRITE:
      // Apres une erreur on continue de recevoir sans ecrire : HTTP ne permet
      // pas d'interrompre proprement un envoi, le navigateur attend la fin.
      if (_fichier && !_erreur.length()) {
        const uint8_t* p = envoi.buf;
        size_t reste = envoi.currentSize;
        while (reste) {
          size_t place = TAMPON - _dansTampon;
          size_t n = reste < place ? reste : place;
          memcpy(_tampon + _dansTampon, p, n);
          _dansTampon += n; p += n; reste -= n;
          if (_dansTampon == TAMPON && !ecrireTampon()) break;
        }
      }
      break;
    case RAW_END:
      if (_fichier && !_erreur.length()) ecrireTampon();
      _dansTampon = 0;
      if (_fichier) _fichier.close();
      if (_erreur.length() && _chemin.length()) _contenu->remove(_chemin);   // pas de fichier tronque
      Serial.printf("envoi : %s, %u Ko\n",
                    _erreur.length() ? _erreur.c_str() : "termine", (unsigned)(_ecrits / 1024));
      // Une carte qui refuse une ecriture refusera aussi la suivante : le
      // dialogue SPI est desynchronise. On la remonte tout de suite pour que le
      // renvoi du client parte sur une carte saine, plutot qu'a la fin d'une
      // serie d'echecs. Apres la reponse aurait ete plus propre, mais WebServer
      // ne rappelle plus rien une fois terminerEnvoi() passe.
      if (_code == 503 && _lecteur.reparer) _lecteur.reparer();
      break;
    case RAW_ABORTED:
      _dansTampon = 0;
      if (_fichier) _fichier.close();
      if (_contenu && _chemin.length()) _contenu->remove(_chemin);
      _erreur = "envoi interrompu"; _code = 400;
      Serial.println("envoi : interrompu");
      break;
  }
}

void Serveur::terminerEnvoi() {
  if (_erreur.length()) { refuser(_code, _erreur); return; }
  JsonDocument d;
  d["chemin"] = _chemin;
  d["taille"] = _ecrits;
  repondre(d);
}

// Relaie une ressource texte distante. Un navigateur ne peut pas lire un flux
// RSS dont l'editeur n'autorise pas les autres sites : c'est la regle CORS,
// appliquee par le navigateur lui-meme. La boite, elle, n'est pas un navigateur
// et n'y est pas soumise. Elle va donc chercher le flux et le rend a la page.
//
// Le relais bloque la boucle principale le temps du transfert, comme un envoi :
// quelques secondes sans lecture NFC ni molette. La tache audio continue.
void Serveur::relayer() {
  String url = srv.arg("url");
  if (!url.length()) { refuser(400, "aucune adresse indiquee"); return; }

  size_t plafond = srv.hasArg("octets") ? (size_t)srv.arg("octets").toInt() : RELAIS_DEFAUT;
  if (!plafond || plafond > RELAIS_MAX) plafond = RELAIS_DEFAUT;

  WiFiClientSecure tls;
  WiFiClient clair;
  HTTPClient http;
  String erreur;
  if (!ouvrirDistant(http, tls, clair, url, erreur, "bytes=0-" + String((uint32_t)plafond - 1))) {
    refuser(erreur.startsWith("seules") ? 400 : 502, erreur);
    http.end();
    return;
  }

  String type = http.header("Content-Type");
  if (!typeTexte(type)) { refuser(415, "ce n'est pas un flux, mais " + type); http.end(); return; }
  int taille = http.getSize();

  // Longueur inconnue : le flux sera peut-etre coupe au plafond, et annoncer
  // une taille qu'on ne tiendrait pas casserait la reponse.
  srv.setContentLength(CONTENT_LENGTH_UNKNOWN);
  srv.send(200, type.length() ? type : "text/plain", "");

  WiFiClient* flux = http.getStreamPtr();
  uint8_t tampon[1024];
  size_t total = 0;
  uint32_t dernier = millis();
  while (http.connected() && total < plafond && (taille < 0 || total < (size_t)taille)) {
    size_t dispo = flux->available();
    if (!dispo) {
      if (millis() - dernier > 15000) break;
      delay(5);
      continue;
    }
    size_t n = flux->readBytes(tampon, dispo > sizeof tampon ? sizeof tampon : dispo);
    if (!n) continue;
    srv.sendContent((const char*)tampon, n);
    total += n;
    dernier = millis();
  }
  srv.sendContent("");
  http.end();
  Serial.printf("relais : %s, %u octets\n", url.c_str(), (unsigned)total);
}

void Serveur::ajouterRecuperation(JsonDocument& d) const {
  // Rien a dire tant qu'aucun episode n'a ete demande depuis l'allumage.
  if (!_recupActive && !_recupNom.length()) return;
  JsonObject r = d["telechargement"].to<JsonObject>();
  r["actif"] = _recupActive.load();
  r["nom"]   = _recupNom;
  r["recu"]  = _recupRecu.load();
  r["total"] = _recupTotal.load();
  if (!_recupActive && _recupErreur.length()) r["erreur"] = _recupErreur;
}

// Telecharge un episode vers la carte. La page n'a pas a rester ouverte : elle
// lance, puis suit la progression dans /api/etat.
void Serveur::recuperer() {
  if (!_contenu) { refuser(503, "aucun stockage disponible"); return; }
  if (_recupActive) { refuser(409, "un episode est deja en cours de recuperation"); return; }

  String url = srv.arg("url");
  String nom = nomDeFichier(srv.arg("nom"));
  if (!url.length()) { refuser(400, "aucune adresse indiquee"); return; }
  if (!nom.length()) { refuser(415, "seuls les fichiers MP3 et M4A sont acceptes"); return; }

  _recupUrl    = url;
  _recupNom    = nom;
  _recupChemin = String(DOSSIER) + "/" + nom;
  _recupErreur = "";
  _recupLibre  = _lecteur.espaceLibre();
  _recupRecu   = 0;
  _recupTotal  = 0;
  _recupActive = true;

  // Priorite 2 : au-dessus de la boucle principale, qui tourne en 1, et bien
  // en dessous de la tache audio, en 4, pour ne jamais lui voler son temps.
  // Sur le coeur 0, l'audio ayant le coeur 1 pour lui.
  if (xTaskCreatePinnedToCore(tacheRecuperation, "recup", 8192, this, 2, nullptr, 0) != pdPASS) {
    _recupActive = false;
    refuser(503, "memoire insuffisante pour telecharger");
    return;
  }
  Serial.printf("recuperation : %s -> %s (%llu octets libres)\n",
                url.c_str(), _recupChemin.c_str(), (unsigned long long)_recupLibre);

  JsonDocument d;
  d["chemin"] = _recupChemin;
  repondre(d, 202);
}

void Serveur::tacheRecuperation(void* arg) { static_cast<Serveur*>(arg)->telecharger(); }

// Une ecriture refusee desaccorde la carte : tout echoue ensuite jusqu'au
// remontage. Mesure faite sur un episode de 11 Mo : la carte tient sept a dix
// megaoctets d'affilee, puis refuse, ce qui explique aussi que les envois de
// 1,3 Mo passent presque toujours. On remonte donc la carte et on reprend la ou
// on en etait, par une requete de plage, au lieu de tout recommencer : sans ca,
// chaque essai retombe sur la meme limite et rien n'aboutit jamais.
void Serveur::telecharger() {
  String erreur;
  for (int essai = 1; essai <= 6; essai++) {
    if (tenterRecuperation(erreur, _recupRecu.load())) { erreur = ""; break; }
    if (!erreur.startsWith("ecriture")) break;
    Serial.printf("recuperation : %s a %u Ko, essai %d sur 6\n",
                  erreur.c_str(), (unsigned)(_recupRecu / 1024), essai);
    demanderReparation();
  }
  if (erreur.length() && _contenu) _contenu->remove(_recupChemin);
  Serial.printf("recuperation : %s, %u Ko\n",
                erreur.length() ? erreur.c_str() : "terminee", (unsigned)(_recupRecu / 1024));
  _recupErreur = erreur;
  _recupActive = false;
  vTaskDelete(nullptr);
}

bool Serveur::tenterRecuperation(String& erreur, uint32_t depuis) {
  erreur = "";
  WiFiClientSecure tls;
  WiFiClient clair;
  HTTPClient http;
  File f;
  int code = 0;
  // Meme lecon que les envois : on ecrit par blocs de 8 Ko, jamais par paquets
  // reseau, sans quoi la carte travaille en travers de ses secteurs.
  uint8_t* tampon = (uint8_t*)malloc(TAMPON);

  if (!tampon) {
    erreur = "memoire insuffisante";
  } else if (ouvrirDistant(http, tls, clair, _recupUrl, erreur,
                           depuis ? "bytes=" + String(depuis) + "-" : String(""), &code)) {
    int taille = http.getSize();
    // Reprise refusee : le serveur renvoie tout depuis le debut. On repart donc
    // du debut du fichier plutot que d'y coller un morceau en double.
    if (depuis && code != HTTP_CODE_PARTIAL_CONTENT) { depuis = 0; _recupRecu = 0; }
    if (taille > 0 && !depuis) _recupTotal = taille;
    if (taille > 0 && (uint64_t)taille + 64 * 1024 > _recupLibre) {
      erreur = "pas assez de place sur la carte";
    } else {
      _contenu->mkdir(DOSSIER);
      f = _contenu->open(_recupChemin, depuis ? "a" : "w", true);
      if (!f) erreur = "creation du fichier impossible";
    }
    if (f) {
      WiFiClient* flux = http.getStreamPtr();
      uint32_t recu = depuis, attendu = depuis + (taille > 0 ? (uint32_t)taille : 0), dernier = millis();
      while (http.connected() && (taille < 0 || recu < attendu)) {
        size_t dispo = flux->available();
        if (!dispo) {
          if (millis() - dernier > 20000) { erreur = "la source ne repond plus"; break; }
          vTaskDelay(pdMS_TO_TICKS(10));
          continue;
        }
        size_t n = flux->readBytes(tampon, dispo > TAMPON ? TAMPON : dispo);
        if (!n) continue;
        if (f.write(tampon, n) != n) { erreur = "ecriture refusee par la carte SD"; break; }
        recu += n;
        _recupRecu = recu;
        dernier = millis();
      }
      if (!erreur.length() && taille > 0 && recu < attendu) erreur = "telechargement interrompu";
    }
  }

  if (f) f.close();
  http.end();
  free(tampon);
  return !erreur.length();
}

// Les pistes arrivent comme parametres "piste" repetes, dans l'ordre de lecture :
// piste=/histoires/a.mp3&piste=/histoires/b.m4a. C'est ce que produit
// URLSearchParams.append() cote page ou cote app.
void Serveur::associer() {
  constexpr size_t PISTES_MAX = 64;
  if (!_contenu) { refuser(503, "aucun stockage disponible"); return; }
  String uid = srv.arg("uid");
  String nom = srv.arg("nom");
  nom.trim();
  if (nom.length() > 40) nom.remove(40);
  if (!uid.length()) { refuser(400, "aucune figurine indiquee"); return; }

  std::vector<String> pistes;
  for (int i = 0; i < srv.args(); i++) {
    if (srv.argName(i) != "piste") continue;
    String piste = srv.arg(i);
    if (estUneAdresse(piste)) {
      if (!fluxValide(piste)) { refuser(400, String("adresse invalide : ") + piste); return; }
    } else {
      if (!cheminAudioSur(piste))   { refuser(400, String("piste invalide : ") + piste); return; }
      if (!_contenu->exists(piste)) { refuser(404, String("introuvable sur la boite : ") + piste); return; }
    }
    if (pistes.size() == PISTES_MAX) { refuser(400, "trop de pistes"); return; }
    pistes.push_back(piste);
  }
  // Des fichiers s'enchainent, un direct ne se quitte pas : melanger les deux
  // sur une figurine donnait une lecture dont personne ne pouvait predire la
  // suite. La page ne le propose plus, l'app non plus, et ceci le garantit.
  bool flux = false, fichiers = false;
  for (const String& p : pistes) (estUneAdresse(p) ? flux : fichiers) = true;
  if (flux && fichiers) {
    refuser(400, "une figurine porte des histoires ou des radios, pas les deux");
    return;
  }

  // Une liste vide est un etat legitime : une figurine nommee qui attend son
  // contenu, ou dont on vient de retirer la derniere piste. Le refuser rendait
  // impossible de vider une figurine qui ne portait qu'une radio, et l'echec
  // ne se voyait nulle part. Pour retirer la figurine elle-meme, il y a
  // /api/dissocier.

  // Absent vaut faux : un client plus ancien qui n'envoie pas le champ ne doit
  // pas activer le melange par surprise.
  String melange = srv.arg("aleatoire");
  bool aleatoire = melange == "1" || melange == "true";

  _biblio->associer(uid, nom, pistes, aleatoire);
  if (!_biblio->sauver(*_contenu)) { refuser(500, _biblio->erreur()); return; }
  Serial.printf("configuration : %s (%s), %u piste(s)%s\n", uid.c_str(), nom.c_str(),
                (unsigned)pistes.size(), aleatoire ? ", aleatoire" : "");
  _lecteur.modifiee(uid);

  JsonDocument d;
  d["ok"] = true;
  repondre(d);
}

// Les reglages qui vivent dans library.json, a cote des figurines : ils
// decrivent le comportement de la boite, pas son materiel.
void Serveur::reglages() {
  if (!_contenu) { refuser(503, "aucun stockage disponible"); return; }
  if (srv.hasArg("pose_requise")) {
    String v = srv.arg("pose_requise");
    _biblio->poseRequise(v == "1" || v == "true");
    if (!_biblio->sauver(*_contenu)) { refuser(500, _biblio->erreur()); return; }
    Serial.printf("reglages : figurine posee %s\n",
                  _biblio->poseRequise() ? "exigee" : "non exigee");
  }
  JsonDocument d;
  d["poseRequise"] = _biblio->poseRequise();
  repondre(d);
}

void Serveur::dissocier() {
  if (!_contenu) { refuser(503, "aucun stockage disponible"); return; }
  String uid = srv.arg("uid");
  if (!_biblio->dissocier(uid))    { refuser(404, "figurine inconnue"); return; }
  if (!_biblio->sauver(*_contenu)) { refuser(500, _biblio->erreur()); return; }
  Serial.printf("configuration : %s dissociee\n", uid.c_str());
  _lecteur.modifiee(uid);

  JsonDocument d;
  d["ok"] = true;
  repondre(d);
}

void Serveur::supprimer() {
  if (!_contenu) { refuser(503, "aucun stockage disponible"); return; }
  String piste = srv.arg("piste");
  if (!cheminAudioSur(piste))   { refuser(400, "piste invalide"); return; }
  if (!_contenu->exists(piste)) { refuser(404, "introuvable sur la boite"); return; }

  // Refus si une figurine joue encore ce fichier : elle deviendrait muette sans
  // que rien ne le signale. On dit lesquelles, pour qu'on sache quoi modifier.
  String utilisatrices;
  _biblio->pourChaque([&](const String&, const Figurine& f) {
    for (const String& p : f.pistes) {
      if (p != piste) continue;
      if (utilisatrices.length()) utilisatrices += ", ";
      utilisatrices += f.nom;
      break;
    }
  });
  if (utilisatrices.length()) { refuser(409, String("encore jouee par ") + utilisatrices); return; }

  _lecteur.liberer(piste);
  if (!_contenu->remove(piste)) { refuser(500, "suppression impossible"); return; }
  Serial.printf("configuration : %s supprimee\n", piste.c_str());

  JsonDocument d;
  d["ok"] = true;
  repondre(d);
}

void Serveur::commander() {
  String erreur;
  if (!_lecteur.commander(srv.arg("action"), srv.arg("uid"), erreur)) { refuser(409, erreur); return; }
  JsonDocument d;
  _lecteur.etat(d);   // l'etat qui en resulte, pour que la page se mette a jour sans attendre
  repondre(d);
}

void Serveur::publierInfos() const {
  JsonDocument d;
  d["nom"]       = "Marelleo";
  d["version"]   = MARELLEO_VERSION;
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  d["id"]        = mac.substring(6);
  d["formats"].to<JsonArray>().add("mp3");
  d["formats"].add("m4a");
  d["pistesMax"] = 64;

  // De quoi retrouver la boite autrement que par son nom : l'adresse a taper,
  // celle que le routeur lui a donnee, et la MAC pour lui reserver un bail ou
  // la laisser passer un filtre. La page les range, elle ne les deballe pas.
  JsonObject r = d["reseau"].to<JsonObject>();
  r["hote"] = String("http://") + _reseau->hote();
  r["mac"]  = WiFi.macAddress();
  if (_reseau->connecte()) {
    r["ssid"]       = _reseau->ssid();
    r["ip"]         = _reseau->ip();
    r["passerelle"] = WiFi.gatewayIP().toString();
    r["rssi"]       = _reseau->rssi();
  } else {
    r["portail"] = _reseau->nomDuPortail();
  }
  repondre(d);
}

void Serveur::publierWifi() const {
  JsonDocument d;
  d["ssid"]         = _reseau->ssid();
  d["connecte"]     = _reseau->connecte();
  d["rssi"]         = _reseau->rssi();
  d["ip"]           = _reseau->ip();
  d["etat"]         = _reseau->description();
  d["modification"] = _reseau->modification();
  repondre(d);
}

// Le mot de passe n'est jamais renvoye par aucune route. La reponse part avant
// le changement de reseau : l'app sait que l'essai commence, puis retrouve la
// boite par Bonjour sur le nouveau reseau, ou sur l'ancien si l'essai echoue.
void Serveur::changerWifi() {
  String ssid = srv.arg("ssid");
  String mdp  = srv.arg("motdepasse");
  if (!ssid.length() || ssid.length() > 32)          { refuser(400, "nom de reseau invalide"); return; }
  if (mdp.length() && (mdp.length() < 8 || mdp.length() > 63)) {
    refuser(400, "un mot de passe WPA fait de 8 a 63 caracteres"); return;
  }
  _reseau->changer(ssid, mdp);
  JsonDocument d;
  d["modification"] = "en cours";
  repondre(d, 202);
}

void Serveur::publierBibliotheque() const {
  JsonDocument d;
  JsonArray figurines = d["figurines"].to<JsonArray>();
  _biblio->pourChaque([&](const String& uid, const Figurine& f) {
    JsonObject o = figurines.add<JsonObject>();
    o["uid"] = uid;
    o["nom"] = f.nom;
    o["aleatoire"] = f.aleatoire;
    JsonArray pistes = o["pistes"].to<JsonArray>();
    for (const String& p : f.pistes) pistes.add(p);
  });

  JsonArray histoires = d["histoires"].to<JsonArray>();
  if (_contenu) {
    listerHistoires("/", histoires);   // les histoires de test, a la racine
    listerHistoires(DOSSIER, histoires);
  }
  repondre(d);
}

void Serveur::listerHistoires(const char* dossier, JsonArray sortie) const {
  File rep = _contenu->open(dossier);
  if (!rep || !rep.isDirectory()) return;
  for (File f = rep.openNextFile(); f; f = rep.openNextFile()) {
    if (f.isDirectory()) continue;
    String chemin = f.path();
    if (!extensionAudio(chemin).length()) continue;
    JsonObject o = sortie.add<JsonObject>();
    o["chemin"] = chemin;
    o["taille"] = f.size();
  }
}
