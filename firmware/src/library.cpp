#include "library.h"
#include <ArduinoJson.h>

static void lireFigurine(JsonObjectConst src, Figurine& dst) {
  dst.nom = src["nom"] | "sans nom";
  dst.aleatoire = src["aleatoire"] | false;
  dst.pistes.clear();
  for (JsonVariantConst p : src["pistes"].as<JsonArrayConst>())
    dst.pistes.push_back(String(p.as<const char*>()));
}

static void ecrireFigurine(JsonObject dst, const Figurine& src) {
  dst["nom"] = src.nom;
  if (src.aleatoire) dst["aleatoire"] = true;   // absent par defaut : fichier plus lisible
  JsonArray pistes = dst["pistes"].to<JsonArray>();
  for (const String& p : src.pistes) pistes.add(p);
}

bool Library::charger(fs::FS& fs, const char* chemin) {
  _figurines.clear();
  _aDefaut    = false;
  _modifiable = false;
  _erreur     = "";

  // Reprise apres coupure. sauver() ecrit un temporaire complet et le ferme,
  // puis seulement supprime l'ancien fichier et renomme. On en deduit :
  //  - l'ancien absent, le temporaire present : la coupure est tombee entre la
  //    suppression et le renommage, donc le temporaire est forcement complet ;
  //  - les deux presents : la coupure est tombee avant la suppression, le
  //    temporaire est peut-etre tronque, et l'ancien fait foi.
  String tmp = String(chemin) + ".tmp";
  if (fs.exists(tmp)) {
    if (!fs.exists(chemin)) fs.rename(tmp.c_str(), chemin);
    else                    fs.remove(tmp);
  }

  File f = fs.open(chemin, "r");
  if (!f) {
    _erreur = String("fichier absent : ") + chemin;
    _modifiable = !fs.exists(chemin);   // absent : on peut le creer sans rien perdre
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) { _erreur = String("JSON invalide : ") + err.c_str(); return false; }

  _volume = doc["volume"] | 13;
  _poseRequise = doc["pose_requise"] | false;
  if (_volume > 21) _volume = 21;

  for (JsonPairConst kv : doc["figurines"].as<JsonObjectConst>()) {
    Figurine fig;
    lireFigurine(kv.value().as<JsonObjectConst>(), fig);
    if (!fig.pistes.empty()) _figurines[String(kv.key().c_str())] = fig;
  }

  if (doc["defaut"].is<JsonObjectConst>()) {
    lireFigurine(doc["defaut"].as<JsonObjectConst>(), _defaut);
    _aDefaut = !_defaut.pistes.empty();
  }
  _modifiable = true;
  return true;
}

bool Library::sauver(fs::FS& fs, const char* chemin) {
  if (!_modifiable) {
    _erreur = "library.json etait illisible au demarrage : ecriture refusee pour ne pas l'ecraser";
    return false;
  }
  JsonDocument doc;
  doc["version"] = 1;
  doc["volume"]  = _volume;
  doc["pose_requise"] = _poseRequise;
  JsonObject figurines = doc["figurines"].to<JsonObject>();
  for (const auto& [uid, fig] : _figurines) ecrireFigurine(figurines[uid].to<JsonObject>(), fig);
  if (_aDefaut) ecrireFigurine(doc["defaut"].to<JsonObject>(), _defaut);

  String tmp = String(chemin) + ".tmp";
  File f = fs.open(tmp, "w");
  if (!f) { _erreur = String("ecriture impossible : ") + tmp; return false; }
  size_t ecrits = serializeJsonPretty(doc, f);
  f.close();
  if (ecrits == 0) { _erreur = "ecriture vide"; fs.remove(tmp); return false; }

  // FAT refuse de renommer par-dessus un fichier existant, d'ou la suppression
  // prealable. La fenetre entre ces deux lignes est couverte par charger().
  fs.remove(chemin);
  if (!fs.rename(tmp.c_str(), chemin)) { _erreur = "renommage impossible"; return false; }
  return true;
}

const Figurine* Library::pour(const String& uid) const {
  if (const Figurine* f = explicite(uid)) return f;
  return _aDefaut ? &_defaut : nullptr;
}

const Figurine* Library::explicite(const String& uid) const {
  auto it = _figurines.find(uid);
  return it != _figurines.end() ? &it->second : nullptr;
}

void Library::associer(const String& uid, const String& nom, const std::vector<String>& pistes,
                       bool aleatoire) {
  Figurine& f = _figurines[uid];
  f.nom       = nom.length() ? nom : String("sans nom");
  f.pistes    = pistes;
  f.aleatoire = aleatoire;
}

bool Library::dissocier(const String& uid) {
  return _figurines.erase(uid) > 0;
}

void Library::pourChaque(const std::function<void(const String&, const Figurine&)>& f) const {
  for (const auto& [uid, fig] : _figurines) f(uid, fig);
}
