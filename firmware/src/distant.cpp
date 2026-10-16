#include "distant.h"
#include "version.h"

// Ouvre une connexion vers une adresse distante. Un client chiffre coute
// environ 45 Ko de tas : il vit sur la pile de l'appelant et disparait avec
// lui, plutot que d'immobiliser cette memoire en permanence.
// plage, par exemple "bytes=0-524287" pour n'avoir que le debut d'un flux, ou
// "bytes=7340032-" pour reprendre un telechargement interrompu. Un serveur qui
// ignore l'en-tete repond 200 et envoie tout depuis le debut : l'appelant le
// voit au code de reponse, rendu ci-dessous.
bool ouvrirDistant(HTTPClient& http, WiFiClientSecure& tls, WiFiClient& clair,
                   const String& url, String& erreur, const String& plage,
                   int* codeRendu) {
  if (url.startsWith("https://")) {
    // La boite n'a ni horloge a l'heure ni magasin de certificats : verifier
    // la chaine echouerait toujours. On chiffre sans authentifier, ce qui
    // suffit ici puisque l'adresse vient de l'utilisateur.
    tls.setInsecure();
    tls.setTimeout(15);
    if (!http.begin(tls, url))   { erreur = "adresse refusee"; return false; }
  } else if (url.startsWith("http://")) {
    if (!http.begin(clair, url)) { erreur = "adresse refusee"; return false; }
  } else {
    erreur = "seules les adresses http et https sont acceptees";
    return false;
  }
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000);
  http.setUserAgent("Marelleo/" MARELLEO_VERSION);
  static const char* enTetes[] = {"Content-Type"};
  http.collectHeaders(enTetes, 1);
  if (plage.length()) http.addHeader("Range", plage);
  int code = http.GET();
  if (codeRendu) *codeRendu = code;
  // 206 : le serveur a respecte la plage demandee.
  if (code != HTTP_CODE_OK && code != HTTP_CODE_PARTIAL_CONTENT) {
    erreur = code > 0 ? "la source repond " + String(code) : "connexion impossible";
    return false;
  }
  return true;
}
