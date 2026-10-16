#include "reseau.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <atomic>

namespace {
  constexpr const char* NOM      = "marelleo";   // http://marelleo.local
  constexpr uint32_t    ESSAI_MS = 15000;        // delai laisse a un nouveau reseau
  constexpr uint32_t    ATTENTE_MS = 60000;      // sans reseau, le portail s'ouvre en secours

  enum class Etat : uint8_t { Demarrage, Portail, Connexion, Connecte, HorsLigne };

  // Ecrits par les evenements WiFi, qui arrivent sur une autre tache.
  std::atomic<Etat> etat{Etat::Demarrage};
  std::atomic<bool> ipObtenue{false};

  enum class Modif : uint8_t { Aucune, Programmee, Essai, Reussie, Echouee };
  Modif    modif = Modif::Aucune;
  uint32_t modifDepuis = 0;
  String   nouveauSsid, nouveauMdp, ancienSsid, ancienMdp;

  Etat etatAnnonce = Etat::Demarrage;
  bool mdnsDemarre = false;

  // Le serveur DNS du portail : il repond l'adresse de la boite a toutes les
  // questions, ce qui fait croire au telephone qu'il est derriere un portail
  // d'hotel et le pousse a ouvrir la page tout seul.
  DNSServer dns;
  bool      portailOuvert = false;
  bool      portailDeSecours = false;
  String    nomPortail;

  // Derniere fois que la boite a eu une adresse. C'est ce qui declenche le
  // portail de secours plus bas.
  uint32_t  dernierContact = 0;

  void surEvenement(arduino_event_t* e) {
    switch (e->event_id) {
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:  ipObtenue = true; etat = Etat::Connecte; break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        if (etat == Etat::Connecte) etat = Etat::HorsLigne;
        break;
      default: break;
    }
  }

  String texte(const uint8_t* octets, size_t max) {
    char tampon[65] = {0};
    memcpy(tampon, octets, min(max, sizeof tampon - 1));
    return String(tampon);
  }

  String ssidEnregistre() {
    wifi_config_t conf;
    if (esp_wifi_get_config(WIFI_IF_STA, &conf) != ESP_OK) return String();
    return texte(conf.sta.ssid, sizeof conf.sta.ssid);
  }

  // Le reseau que la boite ouvre porte les quatre derniers chiffres de son
  // adresse materielle : deux boites dans la meme maison restent distinctes.
  // Lue dans les fusibles de la puce et non par WiFi.macAddress(), qui rend
  // six zeros tant que le pilote n'a pas fini de demarrer : toutes les boites
  // se seraient appelees Marelleo-0000, ce que la carte a d'ailleurs fait.
  String nomDeLaBoite() {
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char suffixe[5];
    snprintf(suffixe, sizeof suffixe, "%02X%02X", mac[4], mac[5]);
    return String("Marelleo-") + suffixe;
  }

  void ouvrirPortail(bool secours = false) {
    // AP_STA et non AP seul : il faut pouvoir scruter les reseaux a portee
    // pendant que le portail tourne, sinon la page ne saurait rien proposer.
    // Le mode d'abord, le nom ensuite : avant que le pilote ne soit demarre,
    // l'adresse materielle se lit comme une suite de zeros et toutes les boites
    // s'appelleraient pareil.
    WiFi.mode(WIFI_AP_STA);
    nomPortail = nomDeLaBoite();
    WiFi.softAP(nomPortail.c_str());
    dns.setErrorReplyCode(DNSReplyCode::NoError);
    dns.start(53, "*", WiFi.softAPIP());
    portailOuvert = true;
    portailDeSecours = secours;
    etat = Etat::Portail;
    Serial.printf("reseau : portail%s ouvert, reseau %s, http://%s\n",
                  secours ? " de secours" : "",
                  nomPortail.c_str(), WiFi.softAPIP().toString().c_str());
  }

  void fermerPortail() {
    if (!portailOuvert) return;
    dns.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    portailOuvert = false;
    portailDeSecours = false;
    Serial.println("reseau : portail referme");
  }
}

void Reseau::begin(bool effacerIdentifiants) {
  WiFi.onEvent(surEvenement);
  WiFi.setHostname(NOM);

  // Le pilote d'abord, la lecture ensuite. esp_wifi_get_config() ne rend rien
  // tant que le pilote n'est pas demarre : la boite se croyait vierge et
  // rouvrait son portail alors qu'elle connaissait deja un reseau. Meme piege
  // que l'adresse materielle lue trop tot, qui donnait Marelleo-0000.
  WiFi.mode(WIFI_STA);

  if (effacerIdentifiants) {
    WiFi.disconnect(false, true);   // efface les identifiants de la NVS
    delay(100);
    Serial.println("reseau : identifiants effaces");
  }

  dernierContact = millis();        // point de depart du delai de secours

  if (ssidEnregistre().length()) {
    WiFi.begin();                   // reprend ce qui est enregistre
    etat = Etat::Connexion;
  } else {
    ouvrirPortail();
  }
}

void Reseau::loop() {
  if (portailOuvert) dns.processNextRequest();

  Etat e = etat;
  if (e != etatAnnonce) {
    etatAnnonce = e;
    Serial.printf("reseau : %s\n", description().c_str());
  }
  if (e == Etat::Connecte && portailOuvert && modif != Modif::Essai) fermerPortail();
  if (e == Etat::Connecte) dernierContact = millis();

  // Une boite scellee doit garder une porte d'entree. Sans acces a l'USB, une
  // boite dont le reseau a disparu, routeur remplace, demenagement, mot de
  // passe change, reessayait indefiniment un reseau qui n'existe plus : elle
  // jouait encore ses histoires mais devenait impossible a reconfigurer et a
  // mettre a jour, et il fallait ouvrir le boitier. Au bout d'une minute sans
  // adresse, elle ouvre donc son portail **sans cesser de chercher** : si le
  // routeur revient elle se raccroche et le referme toute seule, et sinon on
  // lui donne son nouveau reseau depuis un telephone, comme au premier jour.
  //
  // Le revers est assume : une boite qui a perdu son reseau expose un point
  // d'acces ouvert tant qu'elle ne retrouve rien. Quelqu'un a portee pourrait
  // jouer une histoire ou changer une association. Il ne pourrait pas lui faire
  // graver un micrologiciel, l'adresse de mise a jour etant compilee et non
  // modifiable depuis le reseau.
  if (!portailOuvert && e != Etat::Connecte &&
      modif != Modif::Programmee && modif != Modif::Essai &&
      millis() - dernierContact > ATTENTE_MS) {
    Serial.println("reseau : une minute sans reseau");
    ouvrirPortail(true);
    WiFi.begin();                   // le portail n'arrete pas la recherche
  }
  if (e == Etat::Connecte && !mdnsDemarre) {
    mdnsDemarre = true;
    MDNS.begin(NOM);
    MDNS.addService("http", "tcp", 80);
    // Service dedie : l'app iOS cherche _marelleo._tcp par Bonjour, sans avoir a
    // trier toutes les imprimantes et box qui annoncent _http._tcp.
    MDNS.addService("marelleo", "tcp", 80);
  }

  switch (modif) {
    case Modif::Programmee: {
      if (millis() - modifDepuis < 500) break;   // laisser partir la reponse HTTP
      ancienSsid = ssidEnregistre();
      wifi_config_t conf;
      esp_wifi_get_config(WIFI_IF_STA, &conf);
      ancienMdp = texte(conf.sta.password, sizeof conf.sta.password);
      Serial.printf("reseau : essai de %s, sans l'enregistrer\n", nouveauSsid.c_str());
      WiFi.persistent(false);   // en memoire vive seulement : l'ancien reste enregistre
      WiFi.disconnect();
      ipObtenue = false;
      etat = Etat::Connexion;
      WiFi.begin(nouveauSsid.c_str(), nouveauMdp.c_str());
      modif = Modif::Essai;
      modifDepuis = millis();
      break;
    }
    case Modif::Essai:
      if (ipObtenue) {
        // La connexion tient : on ecrit enfin ces identifiants dans la NVS.
        wifi_config_t conf;
        esp_wifi_get_config(WIFI_IF_STA, &conf);
        esp_wifi_set_storage(WIFI_STORAGE_FLASH);
        esp_wifi_set_config(WIFI_IF_STA, &conf);
        WiFi.persistent(true);
        modif = Modif::Reussie;
        Serial.printf("reseau : %s adopte et enregistre\n", nouveauSsid.c_str());
        fermerPortail();
      } else if (millis() - modifDepuis > ESSAI_MS) {
        WiFi.disconnect();
        WiFi.persistent(true);
        modif = Modif::Echouee;
        if (ancienSsid.length()) {
          Serial.printf("reseau : %s injoignable, retour a %s\n",
                        nouveauSsid.c_str(), ancienSsid.c_str());
          WiFi.begin(ancienSsid.c_str(), ancienMdp.c_str());
        } else {
          // Aucun reseau d'avant : on rouvre le portail plutot que de laisser
          // la boite muette et injoignable.
          Serial.printf("reseau : %s injoignable, retour au portail\n", nouveauSsid.c_str());
          if (!portailOuvert) ouvrirPortail(); else etat = Etat::Portail;
        }
      }
      break;
    default:
      break;
  }
}

void Reseau::reseauxAPortee(JsonDocument& d) {
  int n = WiFi.scanNetworks(false, false);
  JsonArray a = d["reseaux"].to<JsonArray>();
  for (int i = 0; i < n && i < 20; i++) {
    if (!WiFi.SSID(i).length()) continue;        // reseaux caches : rien a afficher
    JsonObject o = a.add<JsonObject>();
    o["ssid"]   = WiFi.SSID(i);
    o["rssi"]   = WiFi.RSSI(i);
    o["ouvert"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();
}

void Reseau::changer(const String& ssid, const String& motDePasse) {
  nouveauSsid = ssid;
  nouveauMdp  = motDePasse;
  modif       = Modif::Programmee;
  modifDepuis = millis();
}

void Reseau::forcer(const String& ssid, const String& motDePasse) {
  wifi_config_t conf = {};
  strlcpy((char*)conf.sta.ssid, ssid.c_str(), sizeof conf.sta.ssid);
  strlcpy((char*)conf.sta.password, motDePasse.c_str(), sizeof conf.sta.password);
  esp_wifi_set_storage(WIFI_STORAGE_FLASH);
  esp_wifi_set_config(WIFI_IF_STA, &conf);
  Serial.printf("reseau : %s enregistre, redemarrage\n", ssid.c_str());
  delay(300);
  ESP.restart();
}

const char* Reseau::modification() const {
  switch (modif) {
    case Modif::Programmee:
    case Modif::Essai:   return "en cours";
    case Modif::Reussie: return "reussie";
    case Modif::Echouee: return "echouee";
    default:             return "aucune";
  }
}

bool   Reseau::connecte() const     { return etat == Etat::Connecte; }
bool   Reseau::enPortail() const    { return portailOuvert; }
String Reseau::nomDuPortail() const { return nomPortail; }
String Reseau::ssid() const         { return WiFi.SSID(); }
int    Reseau::rssi() const         { return connecte() ? WiFi.RSSI() : 0; }
String Reseau::ip() const           { return connecte() ? WiFi.localIP().toString() : String(); }

String Reseau::hote() const         { return String(NOM) + ".local"; }

String Reseau::description() const {
  switch (etat.load()) {
    case Etat::Portail:
      return portailDeSecours
        ? String("reseau introuvable, portail de secours ") + nomPortail + ", recherche en cours"
        : String("portail ouvert, reseau ") + nomPortail;
    case Etat::Connexion: return "connexion en cours";
    case Etat::Connecte:  return String("maison, http://") + NOM + ".local (" + WiFi.localIP().toString() + ")";
    case Etat::HorsLigne: return "reseau perdu, nouvelle tentative";
    default:              return "demarrage";
  }
}
