# Profil Playwright / Chromium optionnel

**Date :** 15 septembre 2026
**Statut :** premiere tranche operateur dans `agent/` (optionnel, pas US-031)
**Ponctuation :** ASCII usuel et accents français uniquement

Ce document décrit comment **activer** un moteur Playwright / Chromium
sur le runtime agent. Le chemin par defaut reste le **simulateur DOM**
stdlib. Spec : [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md).
Gestes de session : [assist020_gestes_mcp.md](assist020_gestes_mcp.md).
Vue `/browser` : [assist060_061_browser.md](assist060_061_browser.md).
Plan : [PLAN_SUITE_IMPLEMENTATION.md](PLAN_SUITE_IMPLEMENTATION.md).

Ce n'est **pas** US-031. `phase3_complete` et `us031_complete` restent
**false**. Ce n'est **pas** un navigateur-OS. Les outils de session
(`POST /api/sessions/{id}/tools`) restent le simulateur DOM, même si
Playwright est installe. Aucun secret. Aucun OpenAI. `make agent-smoke`
et l'image Docker slim **n'installent pas** Chromium. `make ci` et
`make integration-qemu` ne sont pas allonges.

## Honnêteté

| Chemin | `browser_engine` | Gestes session | Controle `/browser` |
|---|---|---|---|
| Defaut (`Dockerfile`, `make agent-smoke`) | `optional_not_installed` | simulateur DOM | `POST /api/browser/navigate` = **501** |
| Env demandee, paquet absent | `optional_not_installed` | simulateur DOM | 501, pas de crash à l'import |
| `MOHHDY_AGENT_BROWSER_ENGINE=playwright` + paquet | `playwright` | simulateur DOM | navigate / titre / URL / capture |
| `...=chromium` + paquet | `chromium` | simulateur DOM | identique (Chromium via Playwright) |

`GET /health` et `GET /api/browser` publient le label **reel**. Ils ne
pretendent pas qu'un Chromium tourne si le paquet manque.

## Activer (hors CI AOS)

Python hôte, extra pip, **pas** dans l'image slim :

```text
pip install -r agent/requirements-playwright.txt
python3 -m playwright install chromium
MOHHDY_AGENT_BROWSER_ENGINE=playwright \
  MOHHDY_AGENT_CONFIG=agent/config.example.json \
  python3 agent/server.py
```

Dans un conteneur Docker (optionnel, **gros**) :

```text
docker build -f agent/Dockerfile.playwright --target playwright \
  -t mohhdy-agent-playwright ./agent
docker run --rm -p 8080:8080 \
  -e ADMIN_TOKEN=change-me-at-runtime \
  -e MOHHDY_AGENT_CONFIG=/app/config.example.json \
  mohhdy-agent-playwright
```

Compose :

```text
cd agent
docker compose --profile playwright up --build
```

Le profil compose `browser` reste **slim** (`runtime=browser`, pas
Chromium). Ne pas le confondre avec `--profile playwright`.

Variables :

- `MOHHDY_AGENT_BROWSER_ENGINE` : `off` (defaut), `playwright` ou `chromium`
- `MOHHDY_AGENT_PLAYWRIGHT_NO_SANDBOX=1` : utile dans Docker
- `ADMIN_TOKEN` : inchangé. Navigate et capture sont gates comme le FS
  sandbox si le jeton est defini

## Surface de controle (operateur)

```text
GET  /api/browser
POST /api/browser/navigate   {"url":"/demo-app"}
GET  /api/browser/screenshot
```

`url` : chemin local (`/demo-app`) ou URL `http`/`https` dont l'origine
est dans l'allowlist d'instance (`allowed_origins` et origines des
`sites` de `MOHHDY_AGENT_CONFIG`). `file:`, `javascript:` et une origine
étrangère : `403 origin_denied` + `request_id`. ASSIST-013 (liaison
d'origine des sessions visiteur) reste intacte.

UI : `GET /browser` affiche le miroir simulateur **et**, si le moteur
est actif, le titre / l'URL / une capture.

Sans moteur : `501` et `error=optional_not_installed`. L'import de
`agent/server.py` ne charge pas Playwright.

## Verifier

Chemin defaut, sans telecharger Chromium :

```text
make agent-smoke
```

Fumee optionnelle si Playwright **et** Chromium sont deja presents :

```text
MOHHDY_AGENT_BROWSER_ENGINE=playwright python3 agent/tests/test_http.py AgentPlaywrightOptional
```

Cette cible n'est **pas** dans `make ci` ni dans `make integration-qemu`.

## Hors perimetre (cette tranche)

- Declarer US-031 / phase 3 complete
- Routage des gestes de session (`dom.click`, etc.) via Playwright
- Internet public hors allowlist, OpenAI, secret dans l'image
- Chromium dans `Dockerfile` slim / `make agent-docker`
- Allonger `make integration-qemu`
