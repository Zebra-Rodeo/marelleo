// Deux notes au demarrage, et les memes a l'envers a l'extinction, pour que la
// boite dise qu'elle est la, puis qu'elle s'en va.
//
// Le son se fabrique ici, echantillon par echantillon, au lieu de venir d'un
// fichier : une boite dont la carte SD est absente ou vide doit quand meme
// pouvoir repondre, et c'est precisement dans ce cas qu'on a besoin de savoir
// si elle a demarre.
//
// Le carillon d'accueil s'appelle AVANT Audio::setPinout() : il installe son
// propre canal I2S et le rend avant que la bibliotheque audio ne reclame le
// sien. Celui d'adieu, lui, arrive forcement apres : l'ESP32-S3 a deux
// controleurs I2S, la bibliotheque prend le premier, le carillon prend l'autre
// et lui vole le routage des broches en partant. C'est sans consequence parce
// que la boite s'eteint ou redemarre dans la seconde qui suit, et jamais ne
// rejoue quoi que ce soit entre les deux. Si aucun controleur n'est libre,
// rien ne sonne, et rien ne casse.
#pragma once
#include <Arduino.h>

void carillon(uint8_t bclk, uint8_t lrc, uint8_t dout, uint8_t volume, uint8_t volumeMax);
void carillonAdieu(uint8_t bclk, uint8_t lrc, uint8_t dout, uint8_t volume, uint8_t volumeMax);
