// Aller chercher une ressource ailleurs sur le reseau.
//
// Le serveur s'en sert pour relayer un flux RSS qu'un navigateur refuserait,
// et pour rapatrier un episode. La mise a jour s'en sert pour son manifeste et
// pour l'image du micrologiciel. Meme porte pour les trois, donc un seul
// endroit ou corriger le jour ou l'une d'elles pose probleme.
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

bool ouvrirDistant(HTTPClient& http, WiFiClientSecure& tls, WiFiClient& clair,
                   const String& url, String& erreur, const String& plage = "",
                   int* codeRendu = nullptr);
