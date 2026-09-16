# ASSIST-020 / 021 / 022 - gestes, MCP, facture demo

> **Historique.** Runtime Python `agent/` / `osui/` retire (OS-UI-3). Surface actuelle : `userspace/osui_runtime.c` sous QEMU Multiboot. Ce fichier conserve le contrat comportemental d'origine ; il ne decrit plus un serveur HTTP produit.

**Date :** 15 septembre 2026
**Statut :** livré dans `agent/`, avec limites honnêtes (simulateur DOM, pas Chromium)
**Ponctuation :** ASCII usuel et accents français uniquement

Ce document décrit les gestes allowlistés, les outils MCP déclarés et
l'acte métier "créer une facture" dans une appli hôte mock **dans le
même conteneur**. Image Docker : [assist050_docker_runtime.md](assist050_docker_runtime.md).
Droits / escalade : [assist012_droits_handoff.md](assist012_droits_handoff.md).
Spec : [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md).

Ce n'est **pas** un LLM de production. Ce n'est **pas** Playwright.
Ce n'est **pas** un vrai moteur de rendu navigateur. Aucun appel OpenAI.
Aucun secret dans `embed.js`. `make integration-qemu` n'est pas allongé.

## Honnêteté : simulateur vs navigateur

| Chemin | Ce qui tourne | Ce qui ne tourne pas |
|---|---|---|
| Défaut (`Dockerfile`, `make agent-smoke`) | Simulateur DOM in-process. Les gestes mutent un état serveur. `/demo-app` l'affiche par sondage HTTP | Chromium, Playwright, souris OS, JS du site tiers |
| Profil compose `browser` (`Dockerfile.playwright` slim) | **Le même** runtime slim, `runtime=browser` | Playwright n'est pas installé |
| Profil compose `playwright` (extra) | Chromium pour le **controle operateur** `/browser` | Les gestes de session restent le simulateur. Pas US-031 |

`/health` publie `"harness":"dom_simulator"`. Chaque acte réussi le répète.
Le champ `browser_engine` vaut `optional_not_installed` sur le slim, ou
`playwright` / `chromium` si le profil optionnel est réellement chargé.
Guide : [assist_playwright_optional.md](assist_playwright_optional.md).

La page `/demo-app` est une **appli hôte mock locale** (menu + formulaire
facture). Les tests observent `GET /api/demo-app/state` et
`GET /api/demo-app/invoices`. Pas d'Internet public. L'opérateur peut
voir le meme etat sur `/browser` (ASSIST-060, pas US-031) :
[assist060_061_browser.md](assist060_061_browser.md).

## Configuration (origines + outils)

Fichier JSON (`MOHHDY_AGENT_CONFIG`), lu au démarrage. Exemple :
`agent/config.example.json`.

```text
allowed_origins : ["self"]     # origine du runtime, ou URL explicite
tools : { "mcp.invoice.create": { "kind": "mcp", "description": "..." } }
sites.{id}.capabilities        # grant initial des nouvelles sessions
sites.{id}.allowed_origins     # surcharge par site
```

`self` désigne l'origine HTTP de l'instance (Host de la requête).
Une entrée `http://127.0.0.1` sans port accepte n'importe quel port de
ce hôte. Une origine étrangère (`https://evil.example`) est refusée.
La même allowlist lie aussi le document embed (ASSIST-013) :
[assist013_origine_embed.md](assist013_origine_embed.md).

Outil MCP **absent** de `tools` : refusé même s'il a été accordé à la
session. `mcp.invoice.create` est le démonstrateur enregistré par défaut
(et dans l'exemple de config).

Les gestes (`dom.click`, `dom.type`, `pointer.move`) et
`mcp.invoice.create` ne sont **pas** dans l'allowlist par défaut : moindre
privilège. Le site `site_public_demo` de l'exemple les accorde.

## API visiteur (même session)

```text
POST /api/sessions/{id}/tools
{
  "tool": "dom.click",
  "origin": "http://127.0.0.1:8080",
  "args": { "selector": "#menu-toggle" }
}
```

Succès : `200`, `ok=true`, `request_id`, `harness=dom_simulator`,
`session_id` inchangé.

Refus :

| Cas | Code | `error` |
|---|---|---|
| Droit absent ou révoqué | 403 | `capability_denied` (escalade, acte non exécuté) |
| Origine hors allowlist | 403 | `origin_denied` + `request_id` journalisé |
| MCP non déclaré dans la config | 403 | `tool_undeclared` |
| Arguments invalides (sélecteur, champs) | 400 | `bad_request` |

Les 403 d'allowlist placent la session en `waiting_human` et proposent
l'escalade. Le widget public ne reçoit ni `admin.takeover` ni préfixe `acl.`.

### Gestes (ASSIST-020)

Sélecteurs allowlistés uniquement :

| Sélecteur | Effet simulateur |
|---|---|
| `#menu-toggle` | Ouvre / ferme le menu |
| `#menu-invoices` | Ouvre le menu, focus formulaire |
| `#invoice-customer` | Focus (click) ou saisie (`dom.type`) |
| `#invoice-amount` | Focus ou saisie |
| `#invoice-submit` | Marque le formulaire soumis (UI seulement) |

`pointer.move` : `{ "x": 0..1000, "y": 0..1000 }`.

Observer : `GET /api/demo-app/state`.

### Outils MCP (ASSIST-021)

Allowlist nominative dans la config. Exemple déclaré :
`mcp.invoice.create`. Un `mcp.not_registered` est `tool_undeclared`.
Révoquer le droit bloque l'appel suivant.

### Facture (ASSIST-022)

```text
POST /api/sessions/{id}/tools
{
  "tool": "mcp.invoice.create",
  "origin": "http://127.0.0.1:8080",
  "args": { "customer": "Ada", "amount": "42.00" }
}
```

Si `customer` / `amount` manquent, le runner reprend les champs du
formulaire simulateur. La facture porte le **même** `session_id`.
Liste : `GET /api/demo-app/invoices?session_id=...`.

Persistance optionnelle : répertoire `MOHHDY_AGENT_DATA`
(`invoices.json`, `journal.json`, `sessions.json`). Sinon : mémoire.

Sans le droit : 403, aucune facture, offre d'escalade.

## Journal d'audit

`GET /api/admin/journal` (jeton `ADMIN_TOKEN` si défini). Chaque ligne :
`request_id`, `session_id`, `tool`, `origin`, `outcome`
(`ok`, `capability_denied`, `origin_denied`, `undeclared`, `bad_args`).
La console `/admin` affiche les dernières lignes.

## Vérifier

```text
make agent-smoke
```

Couvre : geste sur origine allowlistee, refus d'origine étrangère avec
`request_id`, facture avec grant / refuse + revoke, outil MCP absent,
pas de Playwright **dans le slim**, isolation et handoff déjà livrés. Hors `make ci` AOS
QEMU : job parallèle `agent-http-smoke` (stdlib, sans Docker, sans
téléchargement Chromium).

Contre une origine déjà lancée :

```text
MOHHDY_AGENT_CONFIG=agent/config.example.json python3 agent/server.py
BASE_URL=http://127.0.0.1:8080 agent/scripts/smoke.sh
```

Ouvrir `http://127.0.0.1:8080/demo-app` : menu, formulaire, état JSON.
Ouvrir `http://127.0.0.1:8080/demo` : bulle de chat.

Image optionnelle slim (toujours sans Chromium) :

```text
docker compose --profile browser up --build
```

Profil extra Playwright (gros, hors CI) :
[assist_playwright_optional.md](assist_playwright_optional.md).

## Non livré

- Routage des gestes de session via Playwright (reste le simulateur)
- Gestes sur un site tiers public hors allowlist
- LLM de production, GGUF, fournisseur public
- Auth par site, comptes opérateurs, HTTPS terminé dans l'image
- Noyau Multiboot dans le conteneur
