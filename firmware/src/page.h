// La page de configuration.
// Tout est dans ce fichier : aucune ressource externe, puisque la boite peut
// servir la page depuis son propre reseau, sans acces a internet. D'ou les
// polices du systeme plutot qu'une police chargee ailleurs.
// Ouvrir la page ne change rien au comportement de la boite.
//
// Elle sert aussi d'app : toutes les figurines sont modifiables, leurs pistes
// se reordonnent au doigt, et les episodes de podcast sont cherches ici puis
// telecharges par la boite elle-meme (voir /api/relais et /api/recuperer).
#pragma once

static const char PAGE[] PROGMEM = R"PAGE(<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#F0E9DE" media="(prefers-color-scheme: light)">
<meta name="theme-color" content="#171210" media="(prefers-color-scheme: dark)">
<title>Marelleo</title>
<style>
/* Les couleurs viennent de l'objet : corps beige poudre, bouton chocolat.
   Le vert ne sert qu'a une chose, dire ce qui joue. */
:root{
  --boitier-haut:#EFE7D8; --boitier-face:#E4D9C6; --boitier-cote:#D5C8B2;
  --papier:#F0E9DE; --carte:#FCF8F2; --creux:#E9E0D2;
  --encre:#2B1D14; --doux:#857463; --trait:#E3D8C8;
  --joue:#2F6B4B; --pause:#A9741B; --alerte:#9C3A2B;
  --ombre:0 1px 2px rgba(43,29,20,.05), 0 8px 20px -14px rgba(43,29,20,.35);
}
@media (prefers-color-scheme:dark){:root{
  --boitier-haut:#3C352C; --boitier-face:#312B23; --boitier-cote:#26211A;
  --papier:#171210; --carte:#221A15; --creux:#1C1512;
  --encre:#F2E7D9; --doux:#A5917D; --trait:#352A22;
  --joue:#7CC79B; --pause:#E0AC55; --alerte:#E08A7B;
  --ombre:0 1px 2px rgba(0,0,0,.4), 0 10px 24px -16px rgba(0,0,0,.8);
}}
*{box-sizing:border-box}
html{-webkit-text-size-adjust:100%}
body{
  margin:0;background:var(--papier);color:var(--encre);
  font:400 16px/1.5 system-ui,-apple-system,"Segoe UI",sans-serif;
  -webkit-font-smoothing:antialiased;
}
.rond{font-family:ui-rounded,"SF Pro Rounded","Hiragino Maru Gothic ProN",system-ui,sans-serif}

header{max-width:32rem;margin:0 auto;padding:1.5rem 1.15rem .3rem}
h1{margin:0;font-size:1.45rem;font-weight:700;letter-spacing:-.015em}
#statut{margin:.15rem 0 0;color:var(--doux);font-size:.85rem;display:flex;align-items:center;gap:.4rem}

main{max-width:32rem;margin:0 auto;padding:.9rem 1.15rem 6rem;display:grid;gap:.8rem}
.entete{margin:.2rem .15rem .1rem}
.entete h2{margin:0;font-size:1.12rem;font-weight:650;letter-spacing:-.01em}
.entete p{margin:.15rem 0 0;color:var(--doux);font-size:.85rem;max-width:34ch}

.carte{background:var(--carte);border:1px solid var(--trait);border-radius:14px;padding:.95rem 1.05rem;box-shadow:var(--ombre)}
.creuse{background:var(--creux);box-shadow:none}

/* une ligne : titre, sous-titre, et une action a droite */
.ligne{display:flex;align-items:center;gap:.8rem}
.ligne + .ligne{margin-top:.1rem}
.corps{flex:1;min-width:0}
.titre{font-size:.97rem;font-weight:600;line-height:1.3;overflow-wrap:anywhere}
.sous{color:var(--doux);font-size:.82rem;line-height:1.35;margin-top:.05rem}
.bascule{display:flex;align-items:flex-start;gap:.6rem;margin-top:.9rem;font-size:.92rem;cursor:pointer}
.bascule input{width:1.15rem;height:1.15rem;flex:none;accent-color:var(--joue);margin:.1rem 0 0}
.bascule>span{display:flex;flex-direction:column;min-width:0}
.bascule .sous{margin:0}
/* Le boitier dessine. Trois faces, trois teintes, et des jetons separes pour
   que le dessin suive le theme sans qu'aucune couleur ne soit ecrite en dur. */
.appareil{display:flex;justify-content:center;padding:.4rem 0 .2rem}
.appareil svg{width:min(300px,78vw);height:auto;display:block}
.jauge{display:inline-flex;align-items:center;gap:1.5px;vertical-align:-2px}
.jauge .enveloppe{display:inline-flex;align-items:center;gap:1.5px;height:12px;padding:1.5px;border:1.5px solid currentColor;border-radius:3px;box-sizing:border-box}
.jauge .bloc{width:4px;align-self:stretch;background:currentColor;border-radius:.5px;opacity:.22}
.jauge .bloc.rempli{opacity:1}
.jauge .pointe{width:1.5px;height:5px;background:currentColor;border-radius:0 1px 1px 0;opacity:.7}
.jauge.faible{color:var(--alerte)}
.meta{color:var(--doux);font-size:.78rem;font-variant-numeric:tabular-nums;white-space:nowrap}

button{font:inherit;cursor:pointer;border-radius:10px;border:1px solid transparent;background:none;color:inherit}
.plein{background:var(--encre);color:var(--carte);font-weight:600;padding:.72rem 1rem;width:100%}
.discret{border-color:var(--trait);color:var(--encre);padding:.55rem .8rem;font-size:.88rem;background:var(--carte)}
.discret:disabled{opacity:.4;cursor:default}
.rouge{border-color:var(--trait);color:var(--alerte);padding:.55rem .8rem;font-size:.88rem;background:transparent}
.rouge:disabled{opacity:.4;cursor:default}
.icone{color:var(--doux);width:2.1rem;height:2.1rem;display:grid;place-items:center;font-size:.95rem;flex:none;border-radius:8px}
.icone:hover{background:var(--creux)}
.icone:disabled{opacity:.25;cursor:default;background:none}
.ouvrir{display:flex;align-items:center;gap:.8rem;width:100%;text-align:left}
.ouvrir > .ligne{flex:1;min-width:0}

.pastille{display:inline-flex;align-items:center;gap:.35rem;font-size:.75rem;font-weight:600;color:var(--joue);flex:none}
.pastille::before{content:"";width:.45rem;height:.45rem;border-radius:50%;background:currentColor}
.pastille.pause{color:var(--pause)}

.playlist{list-style:none;margin:.75rem 0 0;padding:.15rem .5rem;counter-reset:piste;
  background:var(--creux);border-radius:12px}
.piste{display:flex;align-items:center;gap:.5rem;padding:.5rem 0;border-top:1px solid var(--trait)}
.piste:first-child{border-top:0;padding-top:.15rem}
.piste .poignee{flex:none;width:1.6rem;height:2rem;display:grid;place-items:center;color:var(--doux);
  font-size:1.05rem;cursor:grab;touch-action:none;user-select:none;-webkit-user-select:none}
.piste .nom{flex:1;min-width:0;font-size:.9rem;overflow-wrap:anywhere}
.piste .nom::before{counter-increment:piste;content:counter(piste) ". ";color:var(--doux);font-variant-numeric:tabular-nums}
.piste.attrapee{background:var(--creux);border-radius:10px;border-top-color:transparent}
.rangee{display:flex;gap:.35rem;flex:none}

:focus-visible{outline:2px solid var(--encre);outline-offset:2px}
[hidden]{display:none!important}
@media (prefers-reduced-motion:reduce){*{transition:none!important;animation:none!important}}

/* --- Mise en page a deux niveaux : une pile de cartes, une vue a la fois --- */
header{display:flex;align-items:flex-start;gap:.6rem}
.entete-corps{flex:1;min-width:0}
#retour{font-size:1.25rem}
#reglages-ouvrir{color:var(--encre);width:2.4rem;height:2.4rem;border-radius:50%;
                 border:1px solid var(--trait);background:var(--carte);box-shadow:var(--ombre)}
#reglages-ouvrir svg{display:block}
.pile{display:grid;gap:.55rem}
.groupe{display:grid;gap:.5rem}
.groupe .entete{margin:.25rem .15rem 0}
.vide{color:var(--doux);font-size:.9rem;text-align:center;padding:1.6rem .5rem}

/* L'etat de la boite, en cases de meme poids */
main section[data-vue]{display:grid;gap:.8rem;align-content:start}
.etat{display:grid;grid-template-columns:1fr 1fr;gap:1px;
      background:var(--trait);border:1px solid var(--trait);border-radius:14px;overflow:hidden}
.etat>div{background:var(--carte);padding:.55rem .7rem;min-width:0}
.etat dt{font-size:.66rem;letter-spacing:.09em;text-transform:uppercase;color:var(--doux);margin:0 0 .2rem}
.etat dd{margin:0;font-size:.88rem;font-weight:600;display:flex;align-items:center;gap:.35rem;
         white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.point{width:.45rem;height:.45rem;border-radius:50%;background:var(--joue);flex:none}
.point.eteint{background:var(--doux)}

/* Le lecteur */
.lecteur{display:flex;align-items:center;gap:.9rem}
.lecteur .corps{flex:1;min-width:0}
.rond-bouton{width:2.9rem;height:2.9rem;border-radius:50%;display:grid;place-items:center;flex:none;
             background:var(--encre);color:var(--carte);font-size:1.05rem}
.rond-bouton.clair{background:var(--creux);color:var(--encre);border:1px solid var(--trait)}
.rond-bouton:disabled{opacity:.35;cursor:default}

/* Le choix, dans le parcours d'ajout */
.choix{display:grid;gap:.55rem}
.choix button{display:flex;align-items:center;gap:.8rem;text-align:left;width:100%;
               background:var(--carte);border:1px solid var(--trait);border-radius:14px;padding:.9rem 1rem;box-shadow:var(--ombre)}
.choix .ligne{flex:1;min-width:0}
.choix .titre{font-size:1rem}
.choix .fleche-droite{color:var(--doux);flex:none}

/* Le signal, en barres plutot qu'en chiffre. Pas de couleur : le vert ne sert
   qu'a dire ce qui joue, et le mot dit deja si c'est bon. */
.barres{display:inline-flex;align-items:flex-end;gap:2.5px;height:15px;flex:none}
.barres span{width:3.5px;background:currentColor;border-radius:1.5px;opacity:.2}
.barres span.rempli{opacity:1}
.barres span:nth-child(1){height:5px}
.barres span:nth-child(2){height:8.5px}
.barres span:nth-child(3){height:11.5px}
.barres span:nth-child(4){height:15px}

/* Ce qu'on ne lit qu'une fois, pour reserver un bail sur le routeur. */
.plomberie{display:grid;gap:.4rem;margin:.85rem 0 0;padding:.6rem .75rem;
           background:var(--creux);border-radius:12px}
.plomberie>div{display:flex;align-items:baseline;gap:1rem}
.plomberie dt{flex:none;color:var(--doux);font-size:.78rem}
.plomberie dd{flex:1;margin:0;text-align:right;font-size:.8rem;font-weight:600;
              font-variant-numeric:tabular-nums;overflow-wrap:anywhere}

.champ{width:100%;font:inherit;padding:.65rem .75rem;border:1px solid var(--trait);
       border-radius:10px;background:var(--carte);color:inherit}
.mot{margin:.2rem .15rem;font-size:.85rem;color:var(--doux);min-height:1.1em}
.mot.ok{color:var(--joue)} .mot.ko{color:var(--alerte)}
.note{margin:.5rem 0 0;color:var(--doux);font-size:.8rem;line-height:1.45}
.ajouts{display:grid;gap:.5rem}
.vignette{width:2.6rem;height:2.6rem;border-radius:8px;flex:none;object-fit:cover;background:var(--creux)}
progress{width:100%;height:.45rem;margin-top:.6rem;accent-color:var(--joue)}
</style>
</head>
<body>

<header>
  <button type="button" id="retour" class="icone" hidden aria-label="Revenir">&#8592;</button>
  <div class="entete-corps">
    <h1 id="titre" class="rond">Marelleo</h1>
    <p id="statut">Connexion&#8230;</p>
  </div>
  <button type="button" id="reglages-ouvrir" class="icone" aria-label="Réglages">
    <!-- Dessinee plutot qu'ecrite : le caractere de la roue dentee porte ses
         propres marges, qui ne tombent au centre d'aucun rond, et passe en
         emoji couleur sur certains telephones. -->
    <svg viewBox="0 0 24 24" width="19" height="19" fill="none" stroke="currentColor"
         aria-hidden="true">
      <circle cx="12" cy="12" r="6.3" stroke-width="1.8"/>
      <circle cx="12" cy="12" r="2.3" stroke-width="1.8"/>
      <g stroke-width="3.2">
        <path d="M12 1.6v4.5"/>
        <path d="M12 1.6v4.5" transform="rotate(45 12 12)"/>
        <path d="M12 1.6v4.5" transform="rotate(90 12 12)"/>
        <path d="M12 1.6v4.5" transform="rotate(135 12 12)"/>
        <path d="M12 1.6v4.5" transform="rotate(180 12 12)"/>
        <path d="M12 1.6v4.5" transform="rotate(225 12 12)"/>
        <path d="M12 1.6v4.5" transform="rotate(270 12 12)"/>
        <path d="M12 1.6v4.5" transform="rotate(315 12 12)"/>
      </g>
    </svg>
  </button>
</header>

<main>

  <section data-vue="accueil">
    <div class="appareil">
      <!-- Le boîtier, construit depuis ses cotes et non depuis son maillage :
           120 x 120 x 90 mm, arêtes verticales arrondies de 16,7, zone de pose
           de 80, bouton de 17, molette de 21 qui dépasse de 12,5. Isométrie, où
           un cercle horizontal se projette en ellipse d'axes parallèles à ceux
           de l'écran, de demi-axes 1,2247 r et 0,7071 r : tout en découle. Le
           nom est posé dans le plan de la façade par la même matrice.
           La zone de pose est centrée en (400, 290). Le cadrage colle au
           dessin, mesuré par getBBox : 113,7 à 686,3 sur 124,7 à 725,3. -->
      <svg id="dessin" viewBox="100 111 601 629" role="img" aria-label="La boîte Marelleo">
        <path d="M686.3 290.0 L686.3 288.1 L686.0 286.3 L685.6 284.5 L685.0 282.6 L684.3 280.8 L683.3 279.1 L682.3 277.3 L681.0 275.6 L679.7 273.9 L678.1 272.3 L676.4 270.7 L674.6 269.2 L672.7 267.7 L670.6 266.3 L668.4 264.9 L443.4 135.0 L441.1 133.8 L438.6 132.6 L436.1 131.4 L433.4 130.4 L430.7 129.4 L427.9 128.5 L425.0 127.7 L422.0 127.0 L419.0 126.4 L415.9 125.9 L412.8 125.4 L409.6 125.1 L406.4 124.9 L403.2 124.7 L400.0 124.7 L396.8 124.7 L393.6 124.9 L390.4 125.1 L387.2 125.4 L384.1 125.9 L381.0 126.4 L378.0 127.0 L375.0 127.7 L372.1 128.5 L369.3 129.4 L366.6 130.4 L363.9 131.4 L361.4 132.6 L358.9 133.8 L356.6 135.0 L131.6 264.9 L129.4 266.3 L127.3 267.7 L125.4 269.2 L123.6 270.7 L121.9 272.3 L120.3 273.9 L119.0 275.6 L117.7 277.3 L116.7 279.1 L115.7 280.8 L115.0 282.6 L114.4 284.5 L114.0 286.3 L113.7 288.1 L113.7 290.0 L113.7 560.0 113.7 560.0 L113.7 561.9 L114.0 563.7 L114.4 565.5 L115.0 567.4 L115.7 569.2 L116.7 570.9 L117.7 572.7 L119.0 574.4 L120.3 576.1 L121.9 577.7 L123.6 579.3 L125.4 580.8 L127.3 582.3 L129.4 583.7 L131.6 585.0 L356.6 715.0 L358.9 716.2 L361.4 717.4 L363.9 718.6 L366.6 719.6 L369.3 720.6 L372.1 721.5 L375.0 722.3 L378.0 723.0 L381.0 723.6 L384.1 724.1 L387.2 724.6 L390.4 724.9 L393.6 725.1 L396.8 725.3 L400.0 725.3 L403.2 725.3 L406.4 725.1 L409.6 724.9 L412.8 724.6 L415.9 724.1 L419.0 723.6 L422.0 723.0 L425.0 722.3 L427.9 721.5 L430.7 720.6 L433.4 719.6 L436.1 718.6 L438.6 717.4 L441.1 716.2 L443.4 715.0 L668.4 585.0 L670.6 583.7 L672.7 582.3 L674.6 580.8 L676.4 579.3 L678.1 577.7 L679.7 576.1 L681.0 574.4 L682.3 572.7 L683.3 570.9 L684.3 569.2 L685.0 567.4 L685.6 565.5 L686.0 563.7 L686.3 561.9 L686.3 560.0 L686.3 290.0 Z" fill="var(--creux)"/>
        <path d="M356.6 135.0 L358.9 133.8 L361.4 132.6 L363.9 131.4 L366.6 130.4 L369.3 129.4 L372.1 128.5 L375.0 127.7 L378.0 127.0 L381.0 126.4 L384.1 125.9 L387.2 125.4 L390.4 125.1 L393.6 124.9 L396.8 124.7 L400.0 124.7 L403.2 124.7 L406.4 124.9 L409.6 125.1 L412.8 125.4 L415.9 125.9 L419.0 126.4 L422.0 127.0 L425.0 127.7 L427.9 128.5 L430.7 129.4 L433.4 130.4 L436.1 131.4 L438.6 132.6 L441.1 133.8 L443.4 135.0 L668.4 264.9 L670.6 266.3 L672.7 267.7 L674.6 269.2 L676.4 270.7 L678.1 272.3 L679.7 273.9 L681.0 275.6 L682.3 277.3 L683.3 279.1 L684.3 280.8 L685.0 282.6 L685.6 284.5 L686.0 286.3 L686.3 288.1 L686.3 290.0 L686.3 291.9 L686.0 293.7 L685.6 295.5 L685.0 297.4 L684.3 299.2 L683.3 300.9 L682.3 302.7 L681.0 304.4 L679.7 306.1 L678.1 307.7 L676.4 309.3 L674.6 310.8 L672.7 312.3 L670.6 313.7 L668.4 315.0 L443.4 445.0 L441.1 446.2 L438.6 447.4 L436.1 448.6 L433.4 449.6 L430.7 450.6 L427.9 451.5 L425.0 452.3 L422.0 453.0 L419.0 453.6 L415.9 454.1 L412.8 454.6 L409.6 454.9 L406.4 455.1 L403.2 455.3 L400.0 455.3 L396.8 455.3 L393.6 455.1 L390.4 454.9 L387.2 454.6 L384.1 454.1 L381.0 453.6 L378.0 453.0 L375.0 452.3 L372.1 451.5 L369.3 450.6 L366.6 449.6 L363.9 448.6 L361.4 447.4 L358.9 446.2 L356.6 445.0 L131.6 315.0 L129.4 313.7 L127.3 312.3 L125.4 310.8 L123.6 309.3 L121.9 307.7 L120.3 306.1 L119.0 304.4 L117.7 302.7 L116.7 300.9 L115.7 299.2 L115.0 297.4 L114.4 295.5 L114.0 293.7 L113.7 291.9 L113.7 290.0 L113.7 288.1 L114.0 286.3 L114.4 284.5 L115.0 282.6 L115.7 280.8 L116.7 279.1 L117.7 277.3 L119.0 275.6 L120.3 273.9 L121.9 272.3 L123.6 270.7 L125.4 269.2 L127.3 267.7 L129.4 266.3 L131.6 264.9 Z" fill="var(--carte)"/>
        <g fill="none" stroke="var(--encre)" stroke-width="3" stroke-linejoin="round"
           stroke-linecap="round" opacity=".9">
          <path d="M686.3 290.0 L686.3 288.1 L686.0 286.3 L685.6 284.5 L685.0 282.6 L684.3 280.8 L683.3 279.1 L682.3 277.3 L681.0 275.6 L679.7 273.9 L678.1 272.3 L676.4 270.7 L674.6 269.2 L672.7 267.7 L670.6 266.3 L668.4 264.9 L443.4 135.0 L441.1 133.8 L438.6 132.6 L436.1 131.4 L433.4 130.4 L430.7 129.4 L427.9 128.5 L425.0 127.7 L422.0 127.0 L419.0 126.4 L415.9 125.9 L412.8 125.4 L409.6 125.1 L406.4 124.9 L403.2 124.7 L400.0 124.7 L396.8 124.7 L393.6 124.9 L390.4 125.1 L387.2 125.4 L384.1 125.9 L381.0 126.4 L378.0 127.0 L375.0 127.7 L372.1 128.5 L369.3 129.4 L366.6 130.4 L363.9 131.4 L361.4 132.6 L358.9 133.8 L356.6 135.0 L131.6 264.9 L129.4 266.3 L127.3 267.7 L125.4 269.2 L123.6 270.7 L121.9 272.3 L120.3 273.9 L119.0 275.6 L117.7 277.3 L116.7 279.1 L115.7 280.8 L115.0 282.6 L114.4 284.5 L114.0 286.3 L113.7 288.1 L113.7 290.0 L113.7 560.0 113.7 560.0 L113.7 561.9 L114.0 563.7 L114.4 565.5 L115.0 567.4 L115.7 569.2 L116.7 570.9 L117.7 572.7 L119.0 574.4 L120.3 576.1 L121.9 577.7 L123.6 579.3 L125.4 580.8 L127.3 582.3 L129.4 583.7 L131.6 585.0 L356.6 715.0 L358.9 716.2 L361.4 717.4 L363.9 718.6 L366.6 719.6 L369.3 720.6 L372.1 721.5 L375.0 722.3 L378.0 723.0 L381.0 723.6 L384.1 724.1 L387.2 724.6 L390.4 724.9 L393.6 725.1 L396.8 725.3 L400.0 725.3 L403.2 725.3 L406.4 725.1 L409.6 724.9 L412.8 724.6 L415.9 724.1 L419.0 723.6 L422.0 723.0 L425.0 722.3 L427.9 721.5 L430.7 720.6 L433.4 719.6 L436.1 718.6 L438.6 717.4 L441.1 716.2 L443.4 715.0 L668.4 585.0 L670.6 583.7 L672.7 582.3 L674.6 580.8 L676.4 579.3 L678.1 577.7 L679.7 576.1 L681.0 574.4 L682.3 572.7 L683.3 570.9 L684.3 569.2 L685.0 567.4 L685.6 565.5 L686.0 563.7 L686.3 561.9 L686.3 560.0 L686.3 290.0 Z"/>
          <path d="M356.6 135.0 L358.9 133.8 L361.4 132.6 L363.9 131.4 L366.6 130.4 L369.3 129.4 L372.1 128.5 L375.0 127.7 L378.0 127.0 L381.0 126.4 L384.1 125.9 L387.2 125.4 L390.4 125.1 L393.6 124.9 L396.8 124.7 L400.0 124.7 L403.2 124.7 L406.4 124.9 L409.6 125.1 L412.8 125.4 L415.9 125.9 L419.0 126.4 L422.0 127.0 L425.0 127.7 L427.9 128.5 L430.7 129.4 L433.4 130.4 L436.1 131.4 L438.6 132.6 L441.1 133.8 L443.4 135.0 L668.4 264.9 L670.6 266.3 L672.7 267.7 L674.6 269.2 L676.4 270.7 L678.1 272.3 L679.7 273.9 L681.0 275.6 L682.3 277.3 L683.3 279.1 L684.3 280.8 L685.0 282.6 L685.6 284.5 L686.0 286.3 L686.3 288.1 L686.3 290.0 L686.3 291.9 L686.0 293.7 L685.6 295.5 L685.0 297.4 L684.3 299.2 L683.3 300.9 L682.3 302.7 L681.0 304.4 L679.7 306.1 L678.1 307.7 L676.4 309.3 L674.6 310.8 L672.7 312.3 L670.6 313.7 L668.4 315.0 L443.4 445.0 L441.1 446.2 L438.6 447.4 L436.1 448.6 L433.4 449.6 L430.7 450.6 L427.9 451.5 L425.0 452.3 L422.0 453.0 L419.0 453.6 L415.9 454.1 L412.8 454.6 L409.6 454.9 L406.4 455.1 L403.2 455.3 L400.0 455.3 L396.8 455.3 L393.6 455.1 L390.4 454.9 L387.2 454.6 L384.1 454.1 L381.0 453.6 L378.0 453.0 L375.0 452.3 L372.1 451.5 L369.3 450.6 L366.6 449.6 L363.9 448.6 L361.4 447.4 L358.9 446.2 L356.6 445.0 L131.6 315.0 L129.4 313.7 L127.3 312.3 L125.4 310.8 L123.6 309.3 L121.9 307.7 L120.3 306.1 L119.0 304.4 L117.7 302.7 L116.7 300.9 L115.7 299.2 L115.0 297.4 L114.4 295.5 L114.0 293.7 L113.7 291.9 L113.7 290.0 L113.7 288.1 L114.0 286.3 L114.4 284.5 L115.0 282.6 L115.7 280.8 L116.7 279.1 L117.7 277.3 L119.0 275.6 L120.3 273.9 L121.9 272.3 L123.6 270.7 L125.4 269.2 L127.3 267.7 L129.4 266.3 L131.6 264.9 Z"/>
        </g>
        <g fill="none" stroke="var(--encre)" stroke-width="2" opacity=".35"><path d="M356.6 445.0 L356.6 715.0"/><path d="M443.4 445.0 L443.4 715.0"/></g>
        <g fill="none" stroke="var(--joue)" stroke-width="2.5" opacity=".9"><ellipse cx="400.0" cy="290.0" rx="147.0" ry="84.9"/></g>
        <g fill="var(--carte)" stroke="var(--encre)" stroke-width="2.5" opacity=".9"><ellipse cx="400.0" cy="170.0" rx="31.2" ry="18.0"/></g>
        <g fill="var(--creux)" stroke="var(--encre)" stroke-width="2.5"
           stroke-linejoin="round" opacity=".9"><path d="M153.6 290.0 L153.6 252.5"/><path d="M230.7 290.0 L230.7 252.5"/><ellipse cx="192.2" cy="252.5" rx="38.6" ry="22.3"/></g>
        <g transform="matrix(2.598 1.500 0 3.000 88.2 290.0)">
          <text x="60" y="56" text-anchor="middle" font-size="11.5" font-weight="700"
                letter-spacing="1.1" fill="var(--doux)" opacity=".85">Marelleo</text>
        </g>

      </svg>
    </div>

    <dl class="etat" id="etat"></dl>

    <div class="carte" id="carte-lecture" hidden>
      <div class="lecteur">
        <div class="corps">
          <span class="pastille" id="pastille"></span>
          <div class="titre rond" id="lecture-nom" style="font-size:1.15rem"></div>
          <div class="sous" id="lecture-ou"></div>
        </div>
        <button type="button" id="bascule" class="rond-bouton" aria-label="Lecture ou pause"></button>
        <button type="button" id="suivante" class="rond-bouton clair" aria-label="Piste suivante">&#9197;</button>
      </div>
    </div>

    <div class="groupe">
      <div class="entete"><h2>Mes figurines</h2></div>
      <div class="pile" id="figurines"></div>
    </div>
  </section>

  <section data-vue="figurine" hidden>
    <div class="pile" id="detail"></div>
  </section>

  <section data-vue="ajout" hidden>
    <div class="pile" id="ajout"></div>
  </section>

  <section data-vue="reglages" hidden>
    <div class="carte">
      <div class="ligne">
        <div class="corps">
          <div class="titre">Mise à jour</div>
          <div class="sous">Version <span id="version-boite">&#8230;</span></div>
        </div>
        <button type="button" id="verifier" class="discret">Vérifier</button>
      </div>
      <div class="carte creuse" id="maj-proposee" hidden style="margin-top:.75rem"></div>
      <div id="maj-cours" hidden>
        <p class="sous" id="maj-etape" style="margin-top:.7rem"></p>
        <progress id="maj-barre" max="100" value="0" hidden></progress>
      </div>
    </div>

    <div class="carte">
      <label class="bascule" style="margin:0">
        <input type="checkbox" id="pose-requise">
        <span>Garder la figurine posée
          <span class="sous">La lecture s'arrête quand on la retire, et reprend quand on la repose.</span>
        </span>
      </label>
    </div>

    <div class="carte" id="reglages-reseau"></div>

    <div class="pile" id="orphelins"></div>
  </section>

  <div class="carte" id="carte-telechargement" hidden>
    <div class="ligne">
      <div class="corps">
        <div class="titre" id="telechargement-nom"></div>
        <div class="sous" id="telechargement-etape"></div>
      </div>
    </div>
    <progress id="telechargement-barre" max="100" value="0" hidden></progress>
  </div>

  <p class="mot" id="message"></p>
</main>

<script>
const $ = id => document.getElementById(id);
let etatCourant = null, bibli = {figurines: [], histoires: []};
let badgeAffiche, envoiEnCours = false, barreEnvoi = null;
let premierEtat = true;
let emissions = [], episodes = [], emissionOuverte = null, rechercheTexte = '';
let attenteFichier = null;     // {uid} : figurine qui recevra l'épisode en cours
let uidOuvert = null;          // figurine dont on regarde le détail

const taille = o => o < 1048576 ? Math.max(1, Math.round(o / 1024)) + ' Ko'
                 : o < 1073741824 ? (o / 1048576).toFixed(1) + ' Mo'
                 : (o / 1073741824).toFixed(1) + ' Go';
const court   = chemin => chemin.split('/').pop();
const pause   = ms => new Promise(r => setTimeout(r, ms));
const suffixe = () => Math.random().toString(36).slice(2, 6);
const estFlux = p => /^https?:\/\//i.test(p);
const figurine = uid => bibli.figurines.find(f => f.uid === uid);

// Un nom de fichier n'est pas fait pour être lu : on enlève l'extension et le
// suffixe anti-collision, et on rend les tirets à l'espace. Une adresse, elle,
// se résume à son hôte : c'est ce qu'on reconnaît d'une radio.
function joliNom(chemin) {
  if (estFlux(chemin)) {
    try { return new URL(chemin).hostname.replace(/^www\./, ''); } catch { return chemin; }
  }
  const brut = court(chemin).replace(/\.(mp3|m4a)$/i, '')
                            .replace(/-(?=[a-z0-9]{4}$)(?=[a-z0-9]*\d)[a-z0-9]{4}$/i, '');
  const mots = brut.replace(/[-_]+/g, ' ').replace(/\s+/g, ' ').trim();
  return mots ? mots.charAt(0).toUpperCase() + mots.slice(1) : court(chemin);
}

function message(texte, erreur) {
  const p = $('message');
  p.textContent = texte || '';
  p.className = 'mot ' + (texte ? (erreur ? 'ko' : 'ok') : '');
}

function bouton(texte, classe, action, titre) {
  const b = document.createElement('button');
  b.type = 'button'; b.textContent = texte; b.className = classe;
  if (titre) { b.title = titre; b.setAttribute('aria-label', titre); }
  b.onclick = action;
  return b;
}

function ligne(titre, sous, meta) {
  const l = document.createElement('div'); l.className = 'ligne';
  const corps = document.createElement('div'); corps.className = 'corps';
  const t = document.createElement('div'); t.className = 'titre'; t.textContent = titre;
  corps.append(t);
  if (sous) { const s = document.createElement('div'); s.className = 'sous'; s.textContent = sous; corps.append(s); }
  l.append(corps);
  if (meta) { const m = document.createElement('div'); m.className = 'meta'; m.textContent = meta; l.append(m); }
  return {ligne: l, corps};
}

function carte(...enfants) {
  const c = document.createElement('div'); c.className = 'carte'; c.append(...enfants); return c;
}

// --- une vue à la fois, et une flèche qui ramène d'où l'on vient

let vue = 'accueil';
const titres = {accueil: 'Marelleo', figurine: '', ajout: 'Ajouter', reglages: 'Réglages'};

function afficherVue(nom, titre) {
  vue = nom;
  document.querySelectorAll('main section[data-vue]').forEach(s => { s.hidden = s.dataset.vue !== nom; });
  $('titre').textContent = titre !== undefined ? titre : titres[nom];
  $('retour').hidden = nom === 'accueil';
  $('reglages-ouvrir').hidden = nom !== 'accueil';
  $('statut').hidden = nom !== 'accueil';
  message('');
  scrollTo({top: 0});
}

$('retour').onclick = () => {
  if (vue === 'ajout' && uidOuvert) reculer();
  else accueil();
};
$('reglages-ouvrir').onclick = () => ouvrirReglages();
const accueil = () => { uidOuvert = null; afficherVue('accueil'); };

// --- l'état, observé chaque seconde sans rien changer à la boîte

let echecs = 0;

async function suivre() {
  if (envoiEnCours) return;
  try {
    afficherEtat(await (await fetch('/api/etat')).json());
    echecs = 0;
  } catch (_) {
    // Un sondage raté arrive sur le WiFi d'une maison. On ne déclare la boîte
    // injoignable qu'au second, et on efface alors ce qu'elle disait jouer :
    // le garder affiché laisserait croire qu'on la commande encore.
    if (++echecs < 2) return;
    etatCourant = null;
    dessinerEtat(null);
    $('carte-lecture').hidden = true;
    if (vue === 'accueil') dessinerFigurines();
  }
}

function afficherEtat(e) {
  const avant = etatCourant;
  etatCourant = e;
  dessinerEtat(e);

  const l = e.lecture, charge = !!l.figurine;
  $('carte-lecture').hidden = !charge;
  if (charge) {
    const libelles = {joue: 'En lecture', pause: 'En pause', arret: 'Arrêtée'};
    const p = $('pastille');
    p.textContent = libelles[l.etat];
    p.className = 'pastille' + (l.etat === 'pause' ? ' pause' : '');
    p.style.color = l.etat === 'arret' ? 'var(--doux)' : '';
    $('lecture-nom').textContent = l.figurine.nom || 'Figurine à nommer';
    const total = (l.figurine.pistes || []).length;
    $('lecture-ou').textContent = l.fichier
      ? joliNom(l.fichier) + ' · ' + (l.piste + 1) + ' sur ' + total
      : total + ' piste' + (total > 1 ? 's' : '');
    $('bascule').textContent = l.etat === 'joue' ? '❚❚' : '▶';
    $('suivante').disabled = total < 2;
  }

  afficherTelechargement(e.telechargement, avant && avant.telechargement);
  afficherMaj(e.maj);
  if (e.poseRequise !== undefined) $('pose-requise').checked = !!e.poseRequise;

  const uid = e.badge ? e.badge.uid : null;
  if (uid !== badgeAffiche) {
    const geste = !premierEtat;
    badgeAffiche = uid;
    // Badger ouvre la figurine, mais seulement depuis l'accueil : interrompre
    // quelqu'un en train d'ajouter une piste serait pire que de ne rien faire.
    // Une figurine déjà posée quand la page s'ouvre n'est pas un geste : on la
    // montre dans la liste, sans détourner la vue.
    if (uid) chargerBibliotheque().then(() => {
      if (vue !== 'accueil') return;
      if (geste) ouvrirFigurine(uid); else dessinerFigurines();
    });
  } else if (avant && avant.lecture.etat !== e.lecture.etat && vue === 'accueil') {
    dessinerFigurines();
  }
  premierEtat = false;
}

function dessinerEtat(e) {
  // L'en-tete dit en une ligne si la boite repond et a quelle adresse la
  // retrouver ; les cases disent le reste.
  const statut = $('statut');
  const point = document.createElement('span');
  point.className = 'point' + (e ? '' : ' eteint');
  statut.replaceChildren(point, document.createTextNode(
    e ? (adresse(e.reseau) || 'Allumée') : 'Boîte injoignable'));

  const cases = [];
  const ajouter = (dt, dd) => {
    const d = document.createElement('div');
    const t = document.createElement('dt'); t.textContent = dt;
    const v = document.createElement('dd');
    if (typeof dd === 'string') v.textContent = dd; else v.append(...dd);
    d.append(t, v); cases.push(d);
  };
  ajouter('Réseau', e ? (ssidBoite || reseauCourt(e.reseau)) : '—');
  const pile = e && e.batterie ? [dessinerJauge(e.batterie)] : [];
  if (e && e.batterie && e.batterie.secteur) {
    const m = document.createElement('span'); m.className = 'sous'; m.textContent = 'en charge';
    pile.push(m);
  }
  ajouter('Batterie', pile.length ? pile : '—');
  ajouter('Carte SD', e ? (e.stockage === 'carte SD' ? taille(e.libre * 1024) + ' libres' : 'absente') : '—');
  ajouter('Version', e ? (versionBoite || '—') : '—');
  $('etat').replaceChildren(...cases);
}

// « maison, http://marelleo.local (192.168.1.185) » porte deux choses : le nom
// du reseau, qui tient dans une case, et l'adresse, qu'on tape ailleurs.
function reseauCourt(texte) {
  if (!texte) return '—';
  const virgule = texte.indexOf(',');
  return virgule > 0 ? texte.slice(0, virgule) : texte;
}

function adresse(texte) {
  const trouve = (texte || '').match(/https?:\/\/[^\s)]+/);
  return trouve ? trouve[0] : '';
}

function dessinerJauge(b) {
  const hote = document.createElement('span'); hote.className = 'jauge';
  const enveloppe = document.createElement('span'); enveloppe.className = 'enveloppe';
  const charge = b.pourcent === null || b.pourcent === undefined ? null : b.pourcent;
  const base = charge !== null ? charge : (b.dernier === undefined ? null : b.dernier);
  const blocs = base === null || base <= 0 ? 0 : Math.min(4, Math.ceil(base / 25));
  for (let i = 0; i < 4; i++) {
    const bloc = document.createElement('span');
    bloc.className = 'bloc' + (i < blocs ? ' rempli' : '');
    enveloppe.append(bloc);
  }
  const pointe = document.createElement('span'); pointe.className = 'pointe';
  hote.append(enveloppe, pointe);
  if (!b.secteur && blocs <= 1) hote.classList.add('faible');
  hote.title = b.secteur
    ? (base === null ? 'Sur secteur' : 'Sur secteur, ' + base + ' % à la dernière mesure')
    : 'Batterie ' + base + ' %';
  hote.setAttribute('aria-label', hote.title);
  return hote;
}

async function commande(action, uid) {
  const corps = new URLSearchParams({action});
  if (uid) corps.append('uid', uid);
  const reponse = await fetch('/api/commande', {method: 'POST', body: corps});
  const donnees = await reponse.json();
  if (reponse.ok) afficherEtat(donnees);
  else message(donnees.erreur || 'Commande refusée', true);
}
$('bascule').onclick  = () => commande(etatCourant && etatCourant.lecture.etat === 'joue' ? 'pause' : 'lecture');
$('suivante').onclick = () => commande('suivante');

// --- la bibliothèque

async function chargerBibliotheque() {
  bibli = await (await fetch('/api/bibliotheque')).json();
  if (vue === 'accueil') dessinerFigurines();
  if (vue === 'figurine' && uidOuvert) dessinerDetail();
  if (vue === 'reglages') dessinerOrphelins();
}

async function enregistrer(uid, nom, pistes, texte, aleatoire) {
  // Non précisé : on reconduit le réglage de la figurine. Sans ça, renommer
  // une figurine ou déplacer une piste désactiverait son mode aléatoire.
  const connue = figurine(uid);
  const melange = aleatoire === undefined ? !!(connue && connue.aleatoire) : !!aleatoire;
  const corps = new URLSearchParams({uid, nom: nom || '', aleatoire: melange ? '1' : '0'});
  pistes.forEach(p => corps.append('piste', p));
  const reponse = await fetch('/api/associer', {method: 'POST', body: corps});
  const donnees = await reponse.json();
  if (!reponse.ok) { message(donnees.erreur || 'Enregistrement refusé', true); return false; }
  await chargerBibliotheque();
  if (texte) message(texte);
  return true;
}

const poids = chemin => (bibli.histoires.find(h => h.chemin === chemin) || {}).taille || 0;

// Ce qu'une figurine porte, en une ligne : une radio, ou des histoires et ce
// qu'elles pèsent. La durée demanderait de décoder chaque fichier, ce que la
// boîte ne sait faire qu'en le jouant.
function resume(f) {
  if (!f.pistes.length) return 'Vide';
  if (f.pistes.every(estFlux)) return f.pistes.length > 1 ? f.pistes.length + ' radios' : 'Radio';
  const n = f.pistes.length;
  const o = f.pistes.reduce((t, p) => t + poids(p), 0);
  return n + (n > 1 ? ' histoires' : ' histoire') + (o ? ' · ' + taille(o) : '');
}

function dessinerFigurines() {
  const liste = bibli.figurines.slice();
  if (badgeAffiche && !figurine(badgeAffiche)) {
    liste.unshift({uid: badgeAffiche, nom: '', pistes: [], nouvelle: true});
  }
  const hote = $('figurines');
  if (!liste.length) {
    const vide = document.createElement('p');
    vide.className = 'vide';
    vide.textContent = 'Pose une figurine sur la boîte pour commencer.';
    hote.replaceChildren(vide);
    return;
  }
  const l = etatCourant && etatCourant.lecture;
  hote.replaceChildren(...liste.map(f => {
    const b = document.createElement('button');
    b.type = 'button'; b.className = 'carte ouvrir';
    const {ligne: rangee} = ligne(f.nom || 'Figurine à nommer',
                                  f.nouvelle ? 'Badgée à l\'instant' : resume(f));
    if (l && l.figurine && l.figurine.uid === f.uid && l.etat !== 'arret') {
      const p = document.createElement('span');
      p.className = 'pastille' + (l.etat === 'pause' ? ' pause' : '');
      p.textContent = l.etat === 'pause' ? 'Pause' : 'Joue';
      rangee.append(p);
    }
    const fleche = document.createElement('span');
    fleche.className = 'meta'; fleche.textContent = '›';
    rangee.append(fleche);
    b.append(rangee);
    b.onclick = () => ouvrirFigurine(f.uid, f.nouvelle ? f : null);
    return b;
  }));
}

// --- une figurine

let nouvelleEnCours = null;    // figurine badgée mais pas encore enregistrée

function ouvrirFigurine(uid, nouvelle) {
  uidOuvert = uid;
  nouvelleEnCours = nouvelle || null;
  const f = figurine(uid) || nouvelleEnCours;
  afficherVue('figurine', f ? (f.nom || 'Figurine') : 'Figurine');
  dessinerDetail();
}

function courante() {
  return figurine(uidOuvert) || nouvelleEnCours;
}

function dessinerDetail() {
  const f = courante();
  if (!f) { accueil(); return; }
  $('titre').textContent = f.nom || 'Figurine';

  const nom = document.createElement('input');
  nom.type = 'text'; nom.className = 'champ'; nom.maxLength = 40;
  nom.placeholder = 'Renard, Petit ours…';
  nom.value = f.nom || '';
  nom.setAttribute('aria-label', 'Nom de la figurine');
  nom.onchange = () => enregistrer(f.uid, nom.value.trim(), f.pistes, 'Nom enregistré.');

  const blocs = [carte(nom)];

  if (f.pistes.length > 1) {
    const label = document.createElement('label');
    label.className = 'bascule'; label.style.margin = '0';
    const coche = document.createElement('input');
    coche.type = 'checkbox'; coche.checked = !!f.aleatoire;
    const texte = document.createElement('span');
    texte.append(document.createTextNode('Lire dans le désordre'));
    const aide = document.createElement('span');
    aide.className = 'sous';
    aide.textContent = coche.checked ? 'L\'ordre ne se règle pas en aléatoire.'
                                     : 'Glisse les pistes pour changer l\'ordre.';
    texte.append(aide);
    coche.onchange = () => enregistrer(f.uid, nom.value.trim(), f.pistes, '', coche.checked);
    label.append(coche, texte);
    blocs.push(carte(label));
  }

  if (f.pistes.length) {
    // Nommer le genre ici est le seul endroit où il se voit vraiment : une
    // adresse de flux et un nom de fichier se ressemblent trop dans une liste.
    const genre = document.createElement('div');
    genre.className = 'titre';
    genre.textContent = radiophonique(f) ? 'Radio en direct' : 'Histoires sur la carte';
    const ou = document.createElement('div');
    ou.className = 'sous';
    ou.textContent = radiophonique(f) ? 'Jouée depuis internet, rien n\'est gardé sur la carte'
                                      : 'Des fichiers posés sur la boîte, joués l\'un après l\'autre';
    const ul = document.createElement('ul');
    ul.className = 'playlist';
    f.pistes.forEach((piste, i) => ul.append(dessinerPiste(f, piste, i, nom)));
    // Réordonner n'a pas de sens quand la lecture tire un ordre au hasard.
    if (!f.aleatoire) reordonnable(ul, f, nom);
    blocs.push(carte(genre, ou, ul));
  }

  const actions = document.createElement('div');
  actions.className = 'ajouts';
  actions.append(bouton('Ajouter une piste', 'plein', () => ouvrirAjout()));
  if (!f.nouvelle) actions.append(bouton('Retirer la figurine', 'rouge', () => dissocier(f)));
  blocs.push(actions);

  $('detail').replaceChildren(...blocs);
}

function dessinerPiste(f, piste, i, champ) {
  const li = document.createElement('li');
  li.className = 'piste'; li.dataset.piste = piste;

  if (!f.aleatoire && f.pistes.length > 1) {
    const poignee = document.createElement('span');
    poignee.className = 'poignee'; poignee.textContent = '⠿';
    poignee.title = 'Glisser pour changer l\'ordre';
    li.append(poignee);
  }

  const nom = document.createElement('span');
  nom.className = 'nom'; nom.textContent = joliNom(piste);
  nom.title = estFlux(piste) ? piste : court(piste);
  li.append(nom);

  if (estFlux(piste)) {
    const direct = document.createElement('span');
    direct.className = 'meta'; direct.textContent = 'direct';
    li.append(direct);
  }

  const actions = document.createElement('span');
  actions.className = 'rangee';
  actions.append(bouton('✕', 'icone', () => {
    const pistes = f.pistes.filter((_, j) => j !== i);
    enregistrer(f.uid, champ.value.trim(), pistes, 'Piste retirée.');
  }, 'Retirer de la liste'));
  li.append(actions);
  return li;
}

// Glisser-déposer au doigt comme à la souris : les événements Pointer couvrent
// les deux. La poignée porte touch-action:none, sans quoi le navigateur
// interpréterait le geste comme un défilement de la page.
function reordonnable(ul, f, champ) {
  let attrapee = null;
  ul.addEventListener('pointerdown', ev => {
    const poignee = ev.target.closest('.poignee');
    if (!poignee) return;
    attrapee = poignee.closest('.piste');
    attrapee.classList.add('attrapee');
    poignee.setPointerCapture(ev.pointerId);
    ev.preventDefault();
  });
  ul.addEventListener('pointermove', ev => {
    if (!attrapee) return;
    const sous = document.elementFromPoint(ev.clientX, ev.clientY);
    const cible = sous && sous.closest('.piste');
    if (!cible || cible === attrapee || cible.parentElement !== ul) return;
    const rect = cible.getBoundingClientRect();
    ul.insertBefore(attrapee, ev.clientY > rect.top + rect.height / 2 ? cible.nextSibling : cible);
  });
  const lacher = () => {
    if (!attrapee) return;
    attrapee.classList.remove('attrapee');
    attrapee = null;
    const ordre = [...ul.children].map(li => li.dataset.piste);
    if (ordre.join('|') !== f.pistes.join('|')) enregistrer(f.uid, champ.value.trim(), ordre, 'Ordre enregistré.');
  };
  ul.addEventListener('pointerup', lacher);
  ul.addEventListener('pointercancel', lacher);
}

async function dissocier(f) {
  if (!confirm('Retirer « ' + (f.nom || 'cette figurine') + ' » ?\nLes fichiers restent sur la boîte.')) return;
  const reponse = await fetch('/api/dissocier', {method: 'POST', body: new URLSearchParams({uid: f.uid})});
  if (!reponse.ok) { message('Impossible de retirer cette figurine.', true); return; }
  await chargerBibliotheque();
  accueil();
  dessinerFigurines();
  suivre();
}

// --- ajouter une piste : un choix après l'autre, jamais tout à la fois

function choix(...options) {
  const d = document.createElement('div');
  d.className = 'choix';
  options.forEach(([titre, sous, action]) => {
    const b = document.createElement('button');
    b.type = 'button';
    const {ligne: l} = ligne(titre, sous);
    const fleche = document.createElement('span');
    fleche.className = 'meta fleche-droite'; fleche.textContent = '›';
    l.append(fleche);
    b.append(l); b.onclick = action;
    d.append(b);
  });
  return d;
}

// Chaque etape s'empile avec son titre : la fleche du haut recule d'un pas, et
// seule la premiere ramene a la figurine.
let pileAjout = [];

function etape(fn, titre) {
  pileAjout.push({fn, titre: titre || 'Ajouter'});
  afficherVue('ajout', titre || 'Ajouter');
  fn();
}

function reculer() {
  pileAjout.pop();
  const dessous = pileAjout[pileAjout.length - 1];
  if (!dessous) { ouvrirFigurine(uidOuvert); return; }
  afficherVue('ajout', dessous.titre);
  dessous.fn();
}

// Une figurine porte des histoires ou une radio, jamais les deux : la boite
// enchaine des fichiers poses sur sa carte, elle ne quitte pas un direct. La
// question est donc reposee a chaque ajout, et celui des deux qui ferait
// changer de genre dit ce qu'il remplace avant qu'on y touche.
let remplacement = false;

function ouvrirAjout() {
  pileAjout = [];
  remplacement = false;
  etape(etapeType);
}

const radiophonique = f => !!f.pistes.length && f.pistes.every(estFlux);

const quantite = (n, mot) => n + ' ' + mot + (n > 1 ? 's' : '');

function etapeType() {
  const f = courante();
  const radio = radiophonique(f), vide = !f.pistes.length;

  const histoire = ['Des histoires',
    vide || !radio ? 'Un fichier envoyé, ou un épisode de podcast téléchargé sur la carte'
                   : 'Remplace ' + quantite(f.pistes.length, 'radio') + ' de cette figurine',
    () => { remplacement = radio; etape(etapeSource, 'Des histoires'); }];

  const direct = ['Une radio',
    vide || radio ? 'Un direct joué depuis internet, rien n\'est gardé sur la carte'
                  : 'Remplace ' + quantite(f.pistes.length, 'histoire') + ' de cette figurine',
    () => { remplacement = !vide && !radio; etape(etapeRadio, 'Une radio'); }];

  $('ajout').replaceChildren(choix(...(radio ? [direct, histoire] : [histoire, direct])));
}

// Ce que portera la figurine : on complète, ou on remplace quand on change de
// genre. Dans ce cas on le dit avant, en nommant ce qui quitte la liste.
const futuresPistes = (f, ...ajouts) => remplacement ? ajouts : [...f.pistes, ...ajouts];

function remplacementAccepte(f) {
  if (!remplacement || !f || !f.pistes.length) return true;
  const radio = radiophonique(f);
  return confirm('« ' + (f.nom || 'Cette figurine') + ' » joue ' +
    quantite(f.pistes.length, radio ? 'radio' : 'histoire') + '.\n\n' +
    'Une figurine joue des histoires ou une radio, pas les deux : sa liste sera remplacée.' +
    (radio ? '' : '\nLes fichiers, eux, restent sur la boîte.'));
}

function etapeSource() {
  const f = courante();
  const libres = bibli.histoires.filter(h => !f.pistes.includes(h.chemin));
  const options = [
    ['Envoyer un fichier', 'Depuis ce téléphone ou cet ordinateur',
     () => etape(etapeEnvoi, 'Envoyer un fichier')],
    ['Un podcast', 'L\'épisode est téléchargé sur la carte, pas joué en direct',
     () => etape(etapePodcast, 'Un podcast')]
  ];
  // Un fichier déjà sur la carte qu'aucune figurine n'emploie n'aurait sinon
  // aucun moyen de resservir.
  if (libres.length) options.push(['Déjà sur la boîte', libres.length + ' fichier' + (libres.length > 1 ? 's' : ''),
                                   () => etape(() => etapeExistantes(libres), 'Déjà sur la boîte')]);
  $('ajout').replaceChildren(choix(...options));
}

function etapeRadio() {
  const f = courante();
  const url = document.createElement('input');
  url.type = 'url'; url.className = 'champ'; url.placeholder = 'https://…';
  url.autocomplete = 'off'; url.inputMode = 'url';
  url.setAttribute('aria-label', 'Adresse du flux');

  const valider = bouton('Ajouter', 'plein', async () => {
    const adresse = url.value.trim();
    if (!/^https?:\/\/\S+$/i.test(adresse)) { message('Il faut une adresse commençant par http.', true); return; }
    if (!remplacement && f.pistes.includes(adresse)) { message('Cette radio est déjà dans la liste.', true); return; }
    if (!remplacementAccepte(f)) return;
    if (await enregistrer(f.uid, f.nom, futuresPistes(f, adresse))) ouvrirFigurine(f.uid);
  });
  valider.style.marginTop = '.6rem';

  const aide = document.createElement('p');
  aide.className = 'note';
  aide.textContent = "L'adresse du flux audio, pas celle d'une page web.";

  $('ajout').replaceChildren(carte(url, valider, aide));
  url.focus();
}

function etapeExistantes(libres) {
  const f = courante();
  const select = document.createElement('select');
  select.className = 'champ';
  select.setAttribute('aria-label', 'Histoire à ajouter');
  select.replaceChildren(...libres.map(h => new Option(joliNom(h.chemin) + ' · ' + taille(h.taille), h.chemin)));
  const valider = bouton('Ajouter', 'plein', async () => {
    if (!remplacementAccepte(f)) return;
    if (await enregistrer(f.uid, f.nom, futuresPistes(f, select.value))) ouvrirFigurine(f.uid);
  });
  valider.style.marginTop = '.6rem';
  $('ajout').replaceChildren(carte(select, valider));
}

function etapeEnvoi() {
  const f = courante();
  const fichier = document.createElement('input');
  fichier.type = 'file';
  fichier.accept = '.mp3,.m4a,audio/mpeg,audio/mp4';
  fichier.multiple = true;
  fichier.hidden = true;
  fichier.onchange = () => envoyerFichiers(f, [...fichier.files]).finally(() => { fichier.value = ''; });

  const barre = document.createElement('progress');
  barre.max = 100; barre.value = 0; barre.hidden = true;
  barreEnvoi = barre;

  const aide = document.createElement('p');
  aide.className = 'note';
  aide.textContent = 'MP3 et M4A. Plusieurs fichiers à la fois, dans l\'ordre choisi.';

  $('ajout').replaceChildren(carte(fichier,
    bouton('Choisir des fichiers', 'plein', () => fichier.click()), barre, aide));
}

// --- envoi de fichiers

// XMLHttpRequest plutôt que fetch : c'est le seul à donner la progression d'un
// envoi. Le fichier part en corps brut : la boîte lit un formulaire multipart
// octet par octet, et un corps brut par blocs.
function envoyerUneFois(fichier, barre) {
  return new Promise((reussi, echoue) => {
    const requete = new XMLHttpRequest();
    barre.value = 0; barre.hidden = false;
    requete.upload.onprogress = ev => { if (ev.lengthComputable) barre.value = 100 * ev.loaded / ev.total; };
    requete.onload = () => {
      let reponse = {};
      try { reponse = JSON.parse(requete.responseText); } catch (_) {}
      if (requete.status === 200) { reussi(reponse.chemin); return; }
      const erreur = new Error(reponse.erreur || 'envoi refusé');
      erreur.code = requete.status;
      echoue(erreur);
    };
    requete.onerror = () => echoue(new Error('connexion perdue pendant l\'envoi'));
    requete.open('POST', '/api/televerser');
    requete.setRequestHeader('Content-Type', 'application/octet-stream');
    // Un en-tête n'accepte que de l'ASCII : le nom, accents compris, est encodé.
    requete.setRequestHeader('X-Nom-Fichier', encodeURIComponent(fichier.name));
    requete.send(fichier);
  });
}

// La boîte répond 503 quand la carte SD refuse une écriture sans être pleine :
// une erreur passagère, qu'elle répare en remontant la carte. On renvoie alors
// le fichier entier, jusqu'à trois fois.
async function envoyer(fichier, barre) {
  for (let essai = 1; ; essai++) {
    try { return await envoyerUneFois(fichier, barre); }
    catch (erreur) {
      if (erreur.code !== 503 || essai >= 3) throw erreur;
      await pause(2000);
    }
  }
}

async function envoyerFichiers(f, fichiers) {
  if (!fichiers.length) return;
  if (!remplacementAccepte(f)) return;
  const besoin = fichiers.reduce((t, x) => t + x.size, 0);
  if (etatCourant && besoin > etatCourant.libre * 1024) {
    message('Pas assez de place : ' + taille(besoin) + ' à envoyer, ' + taille(etatCourant.libre * 1024) + ' libres.', true);
    return;
  }
  const barre = barreEnvoi;
  envoiEnCours = true;
  const nouvelles = [], echecs = [];
  try {
    // Un par un : la boîte ne traite qu'une connexion à la fois. Un fichier qui
    // échoue n'annule pas les autres, déjà sur la boîte.
    for (const [i, fichier] of fichiers.entries()) {
      message('Envoi ' + (i + 1) + ' sur ' + fichiers.length + ' · ' + fichier.name);
      try { nouvelles.push(await envoyer(fichier, barre)); }
      catch (erreur) { echecs.push(fichier.name + ' (' + erreur.message + ')'); }
    }
  } finally {
    envoiEnCours = false;
    barre.hidden = true;
  }
  if (!nouvelles.length) { message('Aucun fichier envoyé : ' + echecs.join(', '), true); return; }
  await enregistrer(f.uid, f.nom, futuresPistes(f, ...nouvelles));
  ouvrirFigurine(f.uid);
  if (echecs.length) message('Échec : ' + echecs.join(', '), true);
  suivre();
}

// --- podcasts
//
// La recherche d'Apple autorise les autres sites, le navigateur peut donc
// l'interroger directement. Les flux RSS, eux, le refusent souvent : c'est la
// boîte qui va les chercher, par /api/relais, n'étant pas un navigateur.

function etapePodcast() {
  const recherche = document.createElement('input');
  recherche.type = 'search'; recherche.className = 'champ';
  recherche.placeholder = 'Les Odyssées, ou l\'adresse d\'un RSS';
  recherche.autocomplete = 'off'; recherche.enterKeyHint = 'search';
  recherche.value = rechercheTexte;
  recherche.setAttribute('aria-label', 'Chercher une émission');
  recherche.onkeydown = ev => { if (ev.key === 'Enter') chercher(recherche.value); };

  const lancer = bouton('Chercher', 'plein', () => chercher(recherche.value));
  lancer.style.marginTop = '.6rem';

  const liste = document.createElement('div');
  liste.className = 'pile'; liste.id = 'emissions';

  $('ajout').replaceChildren(carte(recherche, lancer), liste);
  if (emissions.length) dessinerEmissions(); else recherche.focus();
}

async function chercher(termes) {
  termes = (termes || '').trim();
  if (!termes) return;
  rechercheTexte = termes;
  message('Recherche…');
  try {
    if (/^https?:\/\//i.test(termes)) {
      emissions = [{nom: termes.replace(/^https?:\/\//i, '').split('/')[0], auteur: 'RSS collé', flux: termes}];
    } else {
      const url = 'https://itunes.apple.com/search?media=podcast&entity=podcast&country=FR&limit=25&term=' +
                  encodeURIComponent(termes);
      const donnees = await (await fetch(url)).json();
      emissions = donnees.results.map(r => ({nom: r.collectionName, auteur: r.artistName,
                                             flux: r.feedUrl, image: r.artworkUrl100}));
    }
    message(emissions.length ? '' : 'Aucune émission à ce nom.');
    dessinerEmissions();
  } catch (_) {
    message('Recherche impossible. Cet appareil a-t-il accès à internet ?', true);
  }
}

function dessinerEmissions() {
  const hote = $('emissions');
  if (!hote) return;
  hote.replaceChildren(...emissions.map(e => {
    const b = document.createElement('button');
    b.type = 'button'; b.className = 'carte ouvrir';
    const {ligne: l, corps} = ligne(e.nom, e.flux ? (e.auteur || '') : 'Pas de RSS public');
    if (e.image) {
      const img = document.createElement('img');
      img.className = 'vignette'; img.src = e.image; img.alt = ''; img.loading = 'lazy';
      l.insertBefore(img, corps);
    }
    const fleche = document.createElement('span');
    fleche.className = 'meta'; fleche.textContent = '›';
    l.append(fleche);
    b.append(l);
    b.disabled = !e.flux;
    b.onclick = () => ouvrirEmission(e);
    return b;
  }));
}

async function ouvrirEmission(e) {
  emissionOuverte = e;
  message('Lecture du flux…');
  try {
    const texte = await (await fetch('/api/relais?url=' + encodeURIComponent(e.flux))).text();
    // La boîte ne rapatrie que le début du flux : on recoupe au dernier épisode
    // entier avant de lire le XML, sinon l'analyseur refuse le document.
    const fin = texte.lastIndexOf('</item>');
    const complet = fin < 0 ? texte : texte.slice(0, fin + 7) + '</channel></rss>';
    const doc = new DOMParser().parseFromString(complet, 'application/xml');
    episodes = [...doc.querySelectorAll('item')].map(item => {
      const enclosure = item.querySelector('enclosure');
      const titre = item.querySelector('title');
      const date  = item.querySelector('pubDate');
      return {
        titre: titre ? titre.textContent : 'Sans titre',
        url: enclosure ? enclosure.getAttribute('url') : null,
        taille: enclosure ? Number(enclosure.getAttribute('length')) || 0 : 0,
        date: date ? date.textContent : ''
      };
    }).filter(x => x.url);
    etape(dessinerEpisodes, e.nom);   // ouvre la vue, qui efface le message
    if (!episodes.length) message('Ce flux n\'annonce aucun épisode téléchargeable.', true);
  } catch (_) {
    message('Flux illisible.', true);
  }
}

function dessinerEpisodes() {
  const liste = document.createElement('div');
  liste.className = 'pile';
  liste.replaceChildren(...episodes.slice(0, 40).map(ep => {
    const jour = ep.date ? new Date(ep.date) : null;
    const {ligne: l} = ligne(ep.titre,
      jour && !isNaN(jour) ? jour.toLocaleDateString('fr-FR', {day: 'numeric', month: 'long', year: 'numeric'}) : '',
      ep.taille ? taille(ep.taille) : '');
    l.append(bouton('Télécharger', 'discret', () => recuperer(ep)));
    return carte(l);
  }));
  $('ajout').replaceChildren(liste);
}

async function recuperer(ep) {
  const f = courante();
  // Le nom du fichier vient du titre, sans accent ni ponctuation. Coupe a
  // trente caracteres, il repart au tiret precedent pour ne pas finir au
  // milieu d'un mot.
  const propre = ep.titre.toLowerCase().normalize('NFD').replace(/[\u0300-\u036f]/g, '')
                   .replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '');
  const coupe = propre.length > 30 ? propre.slice(0, 30).replace(/-[^-]*$/, '') : propre;
  const base = coupe || 'episode';
  const extension = /\.m4a(\?|$)/i.test(ep.url) ? '.m4a' : '.mp3';
  const nom = base + '-' + suffixe() + extension;
  if (!remplacementAccepte(f)) return;
  attenteFichier = f ? {uid: f.uid, remplace: remplacement} : null;
  const reponse = await fetch('/api/recuperer', {method: 'POST', body: new URLSearchParams({url: ep.url, nom})});
  const donnees = await reponse.json();
  if (!reponse.ok) { message(donnees.erreur || 'Téléchargement refusé', true); return; }
  if (f) ouvrirFigurine(f.uid);
  message('La boîte télécharge l\'épisode. Il rejoindra la liste tout seul.');
  suivre();
}

// Un épisode arrivé n'a plus rien à dire : il est dans la liste de la figurine.
// Seul un échec reste affiché, puisque rien d'autre ne le montrerait.
function afficherTelechargement(t, avant) {
  if (t && avant && avant.actif && !t.actif) terminerRecuperation(t);
  const montrer = !!(t && (t.actif || t.erreur));
  $('carte-telechargement').hidden = !montrer;
  if (!montrer) return;
  $('telechargement-nom').textContent = joliNom(t.nom);
  const barre = $('telechargement-barre');
  barre.hidden = !t.actif;
  barre.value = t.total ? 100 * t.recu / t.total : 0;
  $('telechargement-etape').textContent = t.actif
    ? taille(t.recu) + (t.total ? ' sur ' + taille(t.total) : '')
    : 'Échec · ' + t.erreur;
}

async function terminerRecuperation(t) {
  await chargerBibliotheque();
  const attente = attenteFichier;
  attenteFichier = null;
  if (t.erreur || !attente) return;
  const chemin = '/histoires/' + t.nom;
  const f = figurine(attente.uid);
  if (!f || !bibli.histoires.some(h => h.chemin === chemin)) return;
  const pistes = attente.remplace ? [chemin] : [...f.pistes, chemin];
  await enregistrer(f.uid, f.nom, pistes, 'Épisode ajouté.');
}

// --- réglages

let versionBoite = '', majEssai = false, ssidBoite = '';

const ecrireVersion = () =>
  $('version-boite').textContent = (versionBoite || '—') + (majEssai ? ' · à l\'essai' : '');

function ouvrirReglages() {
  afficherVue('reglages');
  chargerInfos();            // l'adresse et le signal changent, le reste non
  dessinerOrphelins();
}

// Un chiffre en dBm ne dit rien à personne. Le mot le traduit, les barres le
// montrent, et la mesure reste derrière, pour celui qui déplace la boîte et
// veut la voir bouger.
const MOTS_SIGNAL = [[-55, 'Excellent', 4], [-67, 'Bon', 3], [-75, 'Moyen', 2], [-999, 'Faible', 1]];

function dessinerSignal(dbm) {
  const [, mot, pleines] = MOTS_SIGNAL.find(([seuil]) => dbm >= seuil);
  const hote = document.createElement('span');
  hote.className = 'barres';
  for (let i = 1; i <= 4; i++) {
    const b = document.createElement('span');
    if (i <= pleines) b.className = 'rempli';
    hote.append(b);
  }
  hote.title = dbm + ' dBm';
  return {barres: hote, mot};
}

// Le nom du réseau et l'adresse à taper d'abord, le reste rangé dessous : on
// lit le premier tous les jours, le second une fois, le jour où on règle le
// routeur.
function dessinerReseau(r) {
  const hote = $('reglages-reseau');
  if (!r) {
    hote.replaceChildren(ligne('Réseau', 'Inconnu').ligne);
    return;
  }
  if (r.portail) {
    hote.replaceChildren(ligne('Portail ouvert',
      'La boîte attend un réseau. Son WiFi s\'appelle ' + r.portail + '.').ligne);
    return;
  }

  const {ligne: entete, corps} = ligne(r.ssid || 'Réseau', r.hote);
  if (r.rssi) {
    const {barres, mot} = dessinerSignal(r.rssi);
    entete.insertBefore(barres, corps);
    const m = document.createElement('span');
    m.className = 'meta'; m.textContent = mot;
    entete.append(m);
  }

  const plomberie = document.createElement('dl');
  plomberie.className = 'plomberie';
  [['IP', r.ip], ['Box', r.passerelle], ['MAC', r.mac]].forEach(([etiquette, valeur]) => {
    if (!valeur) return;
    const d = document.createElement('div');
    const t = document.createElement('dt'); t.textContent = etiquette;
    const v = document.createElement('dd'); v.textContent = valeur;
    d.append(t, v); plomberie.append(d);
  });
  hote.replaceChildren(entete, plomberie);
}

// La gravure fait redémarrer la boîte : la page perd le contact quelques
// secondes, ce qui est normal. On le dit, plutôt que de laisser croire à une panne.
// La période d'essai, elle, dure des jours : elle tient dans la ligne de version
// des réglages, pas dans une carte qui suivrait partout.
function afficherMaj(m) {
  majEssai = !!(m && m.essai);
  ecrireVersion();
  const montrer = !!(m && (m.actif || m.erreur));
  $('maj-cours').hidden = !montrer;
  if (!montrer) return;
  const barre = $('maj-barre');
  barre.hidden = !m.actif;
  barre.value = m.total ? 100 * m.recu / m.total : 0;
  $('maj-etape').textContent = m.actif
    ? taille(m.recu) + (m.total ? ' sur ' + taille(m.total) : '') + ' · elle redémarrera seule'
    : 'Échec · ' + m.erreur;
}

async function chargerInfos() {
  try {
    const d = await (await fetch('/api/infos')).json();
    versionBoite = d.version || '';
    ecrireVersion();
    dessinerReseau(d.reseau);
    if (d.reseau && d.reseau.ssid) ssidBoite = d.reseau.ssid;
    if (etatCourant) dessinerEtat(etatCourant);
  } catch (_) {}
}

async function verifierMaj() {
  const b = $('verifier');
  b.disabled = true; b.textContent = 'Recherche…';
  $('maj-proposee').hidden = true;
  message('');
  try {
    const reponse = await fetch('/api/maj');
    const d = await reponse.json();
    if (d.actuelle) { versionBoite = d.actuelle; ecrireVersion(); }
    if (!reponse.ok) { message(d.erreur || 'Source injoignable.', true); return; }
    if (!d.nouvelle) { message('La boîte est à jour.'); return; }
    proposerMaj(d);
  } catch {
    message('La boîte n\'a pas répondu.', true);
  } finally {
    b.disabled = false; b.textContent = 'Vérifier';
  }
}

function proposerMaj(d) {
  const bloc = $('maj-proposee');
  bloc.hidden = false;
  const titre = document.createElement('div');
  titre.className = 'titre';
  titre.textContent = 'Version ' + d.proposee + (d.taille ? ' · ' + taille(d.taille) : '');
  const enfants = [titre];
  if (d.notes) {
    const sous = document.createElement('div');
    sous.className = 'sous'; sous.textContent = d.notes;
    enfants.push(sous);
  }
  const installer = bouton('Installer', 'plein', installerMaj);
  installer.style.marginTop = '.7rem';
  enfants.push(installer);
  bloc.replaceChildren(...enfants);
}

async function installerMaj() {
  if (!confirm('Installer cette version ? La boîte s\'arrête de jouer et redémarre toute seule.')) return;
  try {
    const reponse = await fetch('/api/maj', {method: 'POST'});
    const d = await reponse.json();
    if (!reponse.ok) { message(d.erreur || 'Refusé.', true); return; }
    $('maj-proposee').hidden = true;
    message('La boîte télécharge sa nouvelle version.');
    suivre();
  } catch {
    message('La boîte n\'a pas répondu.', true);
  }
}
$('verifier').onclick = verifierMaj;

$('pose-requise').onchange = async ev => {
  const reponse = await fetch('/api/reglages',
    {method: 'POST', body: new URLSearchParams({pose_requise: ev.target.checked ? '1' : '0'})});
  if (!reponse.ok) { message('Réglage refusé.', true); ev.target.checked = !ev.target.checked; }
};

// Un fichier qu'aucune figurine n'emploie occupe la carte sans que rien ne
// permette de le retrouver : c'est le seul endroit d'où on peut le reprendre.
function dessinerOrphelins() {
  const pris = new Set(bibli.figurines.flatMap(f => f.pistes));
  const libres = bibli.histoires.filter(h => !pris.has(h.chemin));
  const hote = $('orphelins');
  if (!libres.length) { hote.replaceChildren(); return; }
  const entete = document.createElement('div');
  entete.className = 'entete';
  const h2 = document.createElement('h2');
  h2.textContent = 'Fichiers inutilisés';
  const p = document.createElement('p');
  p.textContent = libres.length + ' fichier' + (libres.length > 1 ? 's' : '') + ' qu\'aucune figurine ne joue.';
  entete.append(h2, p);
  hote.replaceChildren(entete, ...libres.map(h => {
    const {ligne: l} = ligne(joliNom(h.chemin), '', taille(h.taille));
    l.append(bouton('Supprimer', 'rouge', () => supprimer(h)));
    return carte(l);
  }));
}

async function supprimer(h) {
  if (!confirm('Supprimer ' + joliNom(h.chemin) + ' de la carte ?')) return;
  const reponse = await fetch('/api/supprimer', {method: 'POST', body: new URLSearchParams({chemin: h.chemin})});
  if (!reponse.ok) { message('Suppression refusée.', true); return; }
  await chargerBibliotheque();
  suivre();
}

afficherVue('accueil');
chargerInfos();
chargerBibliotheque().catch(() => dessinerEtat(null));
suivre();
setInterval(suivre, 1000);
</script>
</body>
</html>
)PAGE";
