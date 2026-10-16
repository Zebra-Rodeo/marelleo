// La page que sert la boite quand elle ne connait aucun reseau.
//
// Volontairement minuscule et sans rien d'exterieur : elle doit s'afficher sur
// un telephone qui n'a, par definition, aucun acces a Internet a ce moment-la.
// Le telephone l'ouvre tout seul en se connectant au reseau de la boite, parce
// que le serveur DNS repond l'adresse de la boite a toutes les questions et
// que le systeme, croyant trouver un portail d'hotel, va voir.
#pragma once
#include <Arduino.h>

static const char PORTAIL[] PROGMEM = R"PAGE(<!doctype html>
<html lang="fr"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<title>Marelleo</title>
<style>
:root{--papier:#F0E9DE;--carte:#FBF7F1;--encre:#2B1D14;--doux:#8A7866;--trait:#E2D7C6;--ok:#3F7D52;--ko:#A4442F}
*{box-sizing:border-box}
body{margin:0;padding:1.1rem;background:var(--papier);color:var(--encre);
  font:16px/1.45 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;
  max-width:30rem;margin-inline:auto}
h1{font-size:1.5rem;margin:.2rem 0 .1rem}
p.intro{color:var(--doux);margin:.1rem 0 1.1rem}
.carte{background:var(--carte);border:1px solid var(--trait);border-radius:14px;padding:.9rem 1rem;margin-bottom:.7rem}
label{display:block;font-size:.85rem;color:var(--doux);margin:.1rem 0 .3rem}
input,select{width:100%;padding:.7rem .8rem;font-size:1rem;color:var(--encre);background:var(--carte);
  border:1px solid var(--trait);border-radius:10px;font-family:inherit}
button{width:100%;margin-top:.9rem;padding:.8rem 1rem;font-size:1rem;font-weight:600;
  background:var(--encre);color:var(--carte);border:0;border-radius:10px;font-family:inherit}
button:disabled{opacity:.5}
.mot{margin:.7rem 0 0;font-size:.9rem;min-height:1.2em}
.ok{color:var(--ok)} .ko{color:var(--ko)}
.note{color:var(--doux);font-size:.82rem;line-height:1.4;margin:1rem .2rem 0}
</style></head><body>
<h1>Marelleo</h1>
<p class="intro">Choisis le réseau WiFi de la maison.</p>

<div class="carte">
  <label for="reseau">Réseau</label>
  <select id="reseau"><option>Recherche…</option></select>
  <label for="mdp" style="margin-top:.8rem">Mot de passe</label>
  <input type="password" id="mdp" autocomplete="current-password" placeholder="laisser vide si le réseau est ouvert">
  <button id="valider" type="button" disabled>Connecter la boîte</button>
  <p class="mot" id="message"></p>
</div>

<p class="note">La boîte essaie le réseau avant de le retenir. Si ça échoue, elle rouvre ce réseau de réglage et tu peux recommencer.<br><br>
Une fois connectée, elle disparaît d'ici et se retrouve sur <b>http://marelleo.local</b>.</p>

<script>
const $ = i => document.getElementById(i);
const dire = (t, bon) => { const m = $('message'); m.textContent = t; m.className = 'mot ' + (t ? (bon ? 'ok' : 'ko') : ''); };

async function chercher() {
  try {
    const d = await (await fetch('/api/reseaux')).json();
    const s = $('reseau');
    if (!d.reseaux || !d.reseaux.length) { s.innerHTML = '<option>Aucun réseau trouvé</option>'; return; }
    s.innerHTML = '';
    d.reseaux.forEach(r => {
      const o = document.createElement('option');
      o.value = r.ssid;
      o.textContent = r.ssid + '  ' + (r.rssi > -60 ? '●●●' : r.rssi > -75 ? '●●' : '●');
      s.append(o);
    });
    $('valider').disabled = false;
  } catch { $('reseau').innerHTML = '<option>Recherche impossible</option>'; }
}

async function connecter() {
  const ssid = $('reseau').value, mdp = $('mdp').value;
  if (!ssid) return;
  $('valider').disabled = true;
  dire('La boîte essaie…', true);
  try {
    const r = await fetch('/api/wifi', {method: 'POST', body: new URLSearchParams({ssid, motdepasse: mdp})});
    if (!r.ok) { dire((await r.json()).erreur || 'Refusé', false); $('valider').disabled = false; return; }
  } catch { }
  // La boîte quitte son propre réseau pour essayer le tien : le téléphone perd
  // le contact, c'est normal et c'est bon signe. On laisse passer l'essai.
  setTimeout(() => dire('Si la boîte a réussi, ce réseau a disparu. Rejoins ton WiFi et ouvre marelleo.local', true), 12000);
}

$('valider').onclick = connecter;
chercher();
</script>
</body></html>
)PAGE";
