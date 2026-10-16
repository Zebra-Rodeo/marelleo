#include "maj.h"
#include "distant.h"
#include "version.h"
#include <Preferences.h>
#include <Update.h>
#include <esp_ota_ops.h>

Maj maj;

namespace {
  constexpr uint8_t     ESSAIS_MAX = 3;   // demarrages sans reseau avant retour arriere
  constexpr size_t      BLOC = 4096;      // meme bon sens que pour la carte SD
  Preferences           nvs;
}

// --- periode d'essai -------------------------------------------------------

void Maj::begin() {
  nvs.begin("maj", false);
  uint8_t essais = nvs.getUChar("essais", 0);
  if (!essais) { nvs.end(); return; }   // version installee de longue date

  _enEssai = true;
  if (essais > ESSAIS_MAX) {
    // Trois demarrages sans jamais voir le reseau : cette version ne sait pas
    // se connecter, et sans reseau on ne peut plus la remplacer. Demi-tour.
    const esp_partition_t* precedente = esp_ota_get_next_update_partition(nullptr);
    Serial.printf("maj : %u demarrages sans reseau, retour a la version precedente\n",
                  (unsigned)essais - 1);
    nvs.putUChar("essais", 0);
    nvs.end();
    if (precedente && esp_ota_set_boot_partition(precedente) == ESP_OK) {
      delay(200);
      ESP.restart();
    }
    return;
  }
  nvs.putUChar("essais", essais + 1);
  nvs.end();
  Serial.printf("maj : version %s a l'essai, tentative %u sur %u\n",
                MARELLEO_VERSION, (unsigned)essais, (unsigned)ESSAIS_MAX);
}

void Maj::valider() {
  if (!_enEssai) return;
  _enEssai = false;
  nvs.begin("maj", false);
  nvs.putUChar("essais", 0);
  nvs.end();
  Serial.printf("maj : version %s validee, elle sait se connecter\n", MARELLEO_VERSION);
}

// --- ou aller chercher -----------------------------------------------------

// L'adresse est compilee, et elle ne se change pas depuis le reseau. Ce n'est
// pas de la rigidite : une boite qui accepterait qu'on lui dise ou chercher son
// micrologiciel accepterait qu'on lui en fasse graver n'importe lequel, et
// quelqu'un sur le meme WiFi suffirait. Pour changer d'hebergeur on change
// l'adresse derriere, jamais dans la boite.
String Maj::source() const {
  return String(MAJ_SOURCE_DEFAUT);
}

// --- consultation du manifeste ---------------------------------------------

bool Maj::consulter(String& erreur, JsonDocument& reponse) {
  String url = source();
  if (!url.length()) { erreur = "aucune adresse de mise a jour enregistree"; return false; }

  WiFiClientSecure tls;
  WiFiClient clair;
  HTTPClient http;
  if (!ouvrirDistant(http, tls, clair, url, erreur)) { http.end(); return false; }

  String texte = http.getString();
  http.end();

  JsonDocument m;
  if (deserializeJson(m, texte)) { erreur = "le manifeste n'est pas du JSON"; return false; }
  const char* version = m["version"];
  const char* image   = m["url"];
  if (!version || !image) { erreur = "le manifeste ne dit ni version ni url"; return false; }

  reponse["actuelle"]  = MARELLEO_VERSION;
  reponse["proposee"]  = version;
  reponse["url"]       = image;
  reponse["taille"]    = m["taille"] | 0;
  reponse["notes"]     = m["notes"] | "";
  reponse["nouvelle"]  = String(version) != String(MARELLEO_VERSION);
  _urlImage     = image;
  _md5          = m["md5"] | "";
  _tailleAnnoncee = m["taille"] | 0;
  _versionCible = version;
  return true;
}

// --- gravure ---------------------------------------------------------------

bool Maj::installer(String& erreur) {
  if (_active) { erreur = "une mise a jour est deja en cours"; return false; }
  JsonDocument info;
  if (!consulter(erreur, info)) return false;
  if (!_urlImage.length()) { erreur = "le manifeste ne donne pas d'image"; return false; }

  _erreur = "";
  _recu = 0;
  _total = 0;
  _active = true;
  // 12 Ko de pile, non 8 : la poignee de main chiffree en consomme largement,
  // et le tampon de transfert, lui, va sur le tas. Une tache qui deborde sa pile
  // ne laisse aucune trace : la puce redemarre, et la boite revient comme si de
  // rien n'etait, a l'ancienne version. C'est exactement ce qu'on a observe.
  if (xTaskCreatePinnedToCore(tache, "maj", 12288, this, 2, nullptr, 0) != pdPASS) {
    _active = false;
    erreur = "impossible de demarrer la tache de mise a jour";
    return false;
  }
  return true;
}

void Maj::tache(void* arg) {
  static_cast<Maj*>(arg)->graver();
  vTaskDelete(nullptr);
}

void Maj::graver() {
  WiFiClientSecure tls;
  WiFiClient clair;
  HTTPClient http;
  String erreur;

  Serial.printf("maj : %s -> %s\n", MARELLEO_VERSION, _versionCible.c_str());

  if (!ouvrirDistant(http, tls, clair, _urlImage, erreur)) {
    _erreur = erreur;
  } else {
    // Tous les hebergeurs n'annoncent pas la longueur : servi en morceaux, le
    // transfert n'a pas d'en-tete Content-Length. Le manifeste, lui, la donne
    // toujours, et c'est de toute facon elle qui fait foi puisque le MD5 porte
    // dessus. On prend celle de l'hebergeur quand elle existe, sinon la sienne.
    int taille = http.getSize();
    uint32_t attendue = taille > 0 ? (uint32_t)taille : _tailleAnnoncee;
    if (!attendue) {
      _erreur = "ni la source ni le manifeste ne disent la taille de l'image";
    } else {
      _total = attendue;
      // Update ecrit dans l'emplacement inactif : celui qui tourne reste entier
      // quoi qu'il arrive ensuite.
      if (!Update.begin((size_t)attendue)) {
        _erreur = "pas la place pour cette image";
      } else {
        if (_md5.length() == 32) Update.setMD5(_md5.c_str());
        WiFiClient* flux = http.getStreamPtr();
        // Sur le tas, jamais sur la pile : quatre kilo-octets de tampon dans une
        // tache en tiennent la moitie, et il n'en reste plus pour le chiffrement.
        uint8_t* tampon = (uint8_t*)malloc(BLOC);
        if (!tampon) _erreur = "pas assez de memoire pour le transfert";
        uint32_t dernier = millis();
        while (tampon && _recu < _total) {
          size_t dispo = flux->available();
          if (!dispo) {
            if (!http.connected()) { _erreur = "la source a ferme trop tot"; break; }
            if (millis() - dernier > 20000) { _erreur = "la source s'est tue"; break; }
            delay(5);
            continue;
          }
          size_t n = flux->readBytes(tampon, dispo > BLOC ? BLOC : dispo);
          if (!n) continue;
          if (Update.write(tampon, n) != n) { _erreur = "ecriture refusee"; break; }
          _recu = _recu + n;
          dernier = millis();
        }
        free(tampon);
        if (!_erreur.length() && !Update.end(true)) {
          // Une image tronquee ou un MD5 qui ne tombe pas juste finissent ici,
          // et rien n'a ete bascule : la boite redemarrera sur l'ancienne.
          _erreur = "image refusee : " + String(Update.errorString());
        }
        if (_erreur.length()) Update.abort();
      }
    }
  }
  http.end();

  if (_erreur.length()) {
    Serial.printf("maj : echec, %s\n", _erreur.c_str());
    _active = false;
    return;
  }

  // La nouvelle version devra prouver qu'elle sait se connecter.
  nvs.begin("maj", false);
  nvs.putUChar("essais", 1);
  nvs.end();
  Serial.println("maj : gravee, redemarrage");
  _active = false;
  delay(500);
  ESP.restart();
}

void Maj::ajouterEtat(JsonDocument& d) const {
  if (!_active && !_erreur.length() && !_enEssai) return;
  JsonObject m = d["maj"].to<JsonObject>();
  m["actif"] = _active.load();
  m["recu"]  = _recu.load();
  m["total"] = _total.load();
  if (_enEssai) m["essai"] = true;
  if (!_active && _erreur.length()) m["erreur"] = _erreur;
}
