// Version du micrologiciel, publiee par /api/infos. L'app s'en sert pour savoir
// ce que la boite sait faire.
#pragma once
#define MARELLEO_VERSION "0.7.8"

// Ou la boite va chercher sa prochaine version. Une boite neuve sait donc se
// mettre a jour sans qu'on lui dise rien. L'adresse n'est pas modifiable depuis
// le reseau : une boite qui accepterait qu'on lui dise ou chercher accepterait
// qu'on lui fasse graver n'importe quoi.
// C'est un sous-domaine dedie, et pas marelleo.org tout court, parce que cette
// adresse est gravee dans chaque boite vendue : l'apex doit rester libre pour
// un site, et les mises a jour doivent pouvoir demenager d'hebergeur sans
// qu'aucune boite ne s'en apercoive.
// Deux materiels, deux micrologiciels, donc deux manifestes. Une carte qui
// avalerait l'image du prototype y gagnerait un brochage SD et audio permute,
// une PSRAM cherchee sur le mauvais bus et une image qui annonce 16 Mo de flash
// sur une puce qui en a 8. La periode d'essai ne la rattraperait meme pas,
// puisque le WiFi, lui, fonctionnerait.
#ifdef CARTE_IMPRIMEE
#define MAJ_SOURCE_DEFAUT "https://maj.marelleo.org/marelleo.json"
#else
#define MAJ_SOURCE_DEFAUT "https://maj.marelleo.org/marelleo-proto.json"
#endif
