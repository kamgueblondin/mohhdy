# ASSIST-010 / 011 / 040 - sessions, embed et admin

**Date :** 15 septembre 2026
**Statut :** livré dans `agent/` (echo stub de base) ; KB / droits / escalade / handoff = [assist012_droits_handoff.md](assist012_droits_handoff.md)
**Ponctuation :** ASCII usuel et accents français uniquement

Ce document décrit le contrat HTTP des sessions visiteur, du widget
`embed.js`, et de la console `/admin`. L'image Docker reste celle
d'ASSIST-050 : [assist050_docker_runtime.md](assist050_docker_runtime.md).
Spec produit : [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md).

Ce n'est **pas** un LLM de production. Ce n'est **pas** le noyau i386.
Aucun appel OpenAI public. Aucun secret dans l'image ni dans `embed.js`.

## Ce qui est livré

| Route | Rôle |
|---|---|
| `POST /api/sessions` | Ouvre une session visiteur, rend `session_id` (UUID) |
| `GET /api/sessions/{id}` | Relit les messages de **cette** session |
| `POST /api/sessions/{id}/messages` | Dépose un message visiteur ; l'agent répond par un echo stub |
| `GET /api/admin/status` | `{auth: token}` ou `{auth: open_stub}` ; pas de jeton en clair |
| `GET /api/admin/sessions` | Liste (filtre optionnel `?site_id=`) ; protégé si `ADMIN_TOKEN` |
| `GET /api/admin/sessions/{id}` | Détail et fil ; même protection |
| `GET /embed.js` | Widget public : bulle, session, sondage HTTP |
| `GET /embed.css` | Styles du widget (pour une CSP sans `unsafe-inline` de style) |
| `GET /demo` | Page hôte qui charge l'embed |
| `GET /admin` | Console : liste et détail (jeton saisi dans la page) |

Stockage : mémoire processus, optionnellement fichier JSON si
`MOHHDY_AGENT_DATA` pointe vers un répertoire inscriptible.

Réponses agent : `llm=stub_echo`. Le texte rappelle que ce n'est pas un
LLM de production. Pas de modèle local GGUF branché ici, pas d'hôte public.

## Isoler deux visiteurs (ASSIST-011)

Chaque `POST /api/sessions` alloue un `session_id` distinct. Les messages
sont indexés par cet identifiant. Deux sondages (deux onglets, deux
navigateurs, deux `curl`) ne voient pas le fil de l'autre.

Le visiteur **ne liste pas** les sessions (`GET /api/sessions` = 405).
Connaître l'UUID vaut capability à ce stade (pas encore de cookie httpOnly).

Filtre admin `site_id` : l'admin d'instance peut restreindre l'affichage.
Il n'y a **pas** encore d'authentification par site (un seul jeton
d'instance). Le masque de droits par site/session est ASSIST-030 :
[assist012_droits_handoff.md](assist012_droits_handoff.md).

## Jeton admin (ASSIST-040)

`ADMIN_TOKEN` se lit **uniquement** à l'exécution :

```text
docker run --rm -p 8080:8080 -e ADMIN_TOKEN=change-me-at-runtime mohhdy-agent
```

Sans variable : `/api/admin/*` est ouvert (`admin_auth=open_stub`). La
console affiche un bandeau d'avertissement. Ne pas exposer ce mode sur
Internet.

Avec variable : `Authorization: Bearer ...` ou `X-Admin-Token`. Jeton
absent ou faux = 401. Le jeton n'est pas dans l'image, pas dans
`embed.js`, pas dans `/health`.

Ce n'est pas une auth par compte, pas de MFA. La reprise humaine dans
la même session est ASSIST-041 : [assist012_droits_handoff.md](assist012_droits_handoff.md).

## Snippet d'attache (ASSIST-013)

Sur un site que **vous** contrôlez, coller :

```html
<script
  src="https://INSTANCE/embed.js"
  data-mohhdy-site="site_public_demo"
  async>
</script>
```

Remplacer `INSTANCE` par l'origine HTTPS du runtime (self-host Docker
ou, plus tard, cloud). `data-mohhdy-site` est un identifiant public, pas
un secret.

Le widget déduit l'origine API depuis `script.src`, ouvre une session
au premier clic sur la bulle, envoie `POST /api/sessions/{id}/messages`
et sonde `GET /api/sessions/{id}` toutes les 2 s tant que le panneau
est ouvert.

### CSP recommandée (hôte)

Autoriser script, style et XHR / `fetch` vers l'instance, sans mettre
de secret dans la politique :

```text
script-src 'self' https://INSTANCE
style-src 'self' https://INSTANCE
connect-src 'self' https://INSTANCE
```

Le widget charge `/embed.css` depuis l'instance : pas besoin de
`unsafe-inline` pour les styles du chat. `'unsafe-inline'` côté script
de l'hôte n'est **pas** requis pour `embed.js` (fichier externe).

Si l'hôte a déjà une CSP stricte, ajouter seulement l'origine instance
aux trois directives. Ne pas élargir `default-src` à `*`.

### Origine du document

Le runtime accepte aujourd'hui n'importe quelle page qui charge
`embed.js` (CORS visiteur `Access-Control-Allow-Origin: *` sur
`/api/sessions*`). Un refus automatique si l'origine du document ne
matche pas le site déclaré **n'est pas** encore en place. En attendant :
ne coller le snippet que sur des sites que vous contrôlez, et ne pas
publier une instance ouverte sans `ADMIN_TOKEN`.

## Vérifier

```text
make agent-smoke
```

Couvre santé, isolation de deux sessions, liste admin, jeton admin, KB,
révocation, escalade, handoff, et l'absence de marqueurs de secret dans
`embed.js`. Hors `make ci` AOS QEMU : job GitHub parallèle `agent-http-smoke`.

Contre une origine déjà lancée :

```text
python3 agent/server.py
BASE_URL=http://127.0.0.1:8080 agent/scripts/smoke.sh
```

Avec jeton :

```text
ADMIN_TOKEN=change-me-at-runtime python3 agent/server.py
ADMIN_TOKEN=change-me-at-runtime BASE_URL=http://127.0.0.1:8080 agent/scripts/smoke.sh
```

Ouvrir `http://127.0.0.1:8080/demo` : bulle, message, echo stub.
Ouvrir `http://127.0.0.1:8080/admin` : la session apparaît.

## Non livré

- LLM de production, GGUF, fournisseur public
- Gestes navigateur et outils MCP exécutés (ASSIST-020 / 021 / 022)
- Auth par site, comptes opérateurs, HTTPS terminé dans l'image
- Noyau Multiboot dans le conteneur
