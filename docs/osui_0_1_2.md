# OS-UI-0 / OS-UI-1 / OS-UI-2 - shell graphique de l'instance

**Date :** 15 septembre 2026
**Statut :** premieres tranches livrees dans `osui/`. Backend temporaire = `agent/`
**Ponctuation :** ASCII usuel et accents francais uniquement

L'instance Docker presente un **shell graphique du SE Mohhdy**. Surface
primaire : **chat central** (prompts et slash). Les programmes
(Browser-OS, Shell OS, Admin, Support, Statut, FS) s'ouvrent depuis le
chat. Ce n'est **pas** US-031, **pas** un LLM de production,
**pas** un moteur Chromium de session, **pas** le guest i386 QEMU.
`agent/` n'est **pas** retire (OS-UI-3). `make integration-qemu` et
`make ci` (QEMU) ne sont pas allonges.

Plan maitre : [PLAN_SE_MOHHDY_COMPLET.md](PLAN_SE_MOHHDY_COMPLET.md).
Epiques : [../US/mohhdy_os_ui_migration.md](../US/mohhdy_os_ui_migration.md).
Interaction : [osui_chat_desktop.md](osui_chat_desktop.md).

## Ce que `docker run` ouvre

Image produit : `mohhdy-os`. Port **8080**. Entree : `GET /` (bureau OS).

```text
docker build -t mohhdy-os -f osui/Dockerfile .
docker run --rm -p 8080:8080 mohhdy-os
docker run --rm -p 8080:8080 -e ADMIN_TOKEN=change-me-at-runtime mohhdy-os
make osui-docker
cd osui && docker compose up --build
python3 osui/server.py
make osui-smoke
```

Ouvrir `http://127.0.0.1:8080/` : chat central (surface de commande),
puis panes ouverts par `/browser`, `/shell`, `/admin`, `/support`,
`/status`, `/fs`. Sante : `GET /health` (JSON, pas de secret). Identite
shell : `GET /api/os` (`commands`, `interaction.primary=center_chat`).

Compose depuis `osui/` (contexte = racine du depot). Jeton uniquement a
l'execution. Pas de `.env` dans le build. Utilisateur non-root uid 10001.
Le noyau Multiboot, l'initrd et les modeles GPT-2 ne sont pas dans l'image.

## Architecture (un produit, deux couches temporaires)

```text
docker run mohhdy-os
        |
        +-- osui/          chrome OS (HTML/CSS/JS desktop)
        |     GET /        bureau
        |     GET /health  service=mohhdy-os shell=osui llm=stub_echo
        |     GET /api/os  panes + commands slash + drapeaux honnetes
        |
        +-- agent/         backend temporaire (APIs ASSIST)
              /api/sessions  chat, origine, escalade
              /api/admin     grant/revoke, file, takeover
              /api/sessions/{id}/tools   gestes + MCP
              /api/browser   vue + FS sandbox (simulateur)
```

Les APIs existantes ne sont pas reecrites. Le shell les appelle. OS-UI-3
retirera la facade Python une fois la parite mesuree.

## OS-UI-0 - chrome

- Chat central par defaut, barre haute, dock, icones, panes programmes
- Slash `/help` `/browser` `/shell` `/admin` `/support` `/status` `/fs`
- Chat flottant draggable des qu'un programme s'ouvre (position
  `sessionStorage`)
- Shell OS = UI d'instance, pas un root Linux
- Boot Docker = cette surface, pas une page marketing
- `phase3_complete=false`, `us031_complete=false`, `llm=stub_echo`
- `chromium_session_engine=false`

Detail : [osui_chat_desktop.md](osui_chat_desktop.md).

## OS-UI-1 - Support + Admin (natives)

Parite comportementale avec le scaffold (preuves `make osui-smoke`) :

- Sessions visiteur isolees, chat stub / KB (ASSIST-010/011/012)
- Origine etrangere : 403 `origin_denied` + `request_id` (ASSIST-013)
- Droits grant/revoke visibles dans Admin (ASSIST-030)
- Escalade, file humain, takeover meme `session_id` (ASSIST-031/040/041)

Le widget `embed.js` reste disponible sur le backend (`/embed.js`) pour
les sites tiers. Le visiteur parle dans le **chat central**. Le pane
Support reste la console detaillee (session, escalade). Admin = grant /
revoke / takeover.

## OS-UI-2 - Browser-OS (premiere tranche)

Dans le pane Browser-OS :

- Vue `iframe` de `/demo-app` (simulateur DOM, etiquete)
- Gestes allowlistes `dom.click` et facture `mcp.invoice.create`
  (ASSIST-020/021/022), meme `session_id`, journal `request_id`
- Explorateur FS sandbox lecture (ASSIST-060/061)
- `POST /api/browser/navigate` : 501 `optional_not_installed` sans
  Playwright. Ce n'est pas un navigateur-OS Chromium.

Outil non declare ou droit revoque : 403, pas d'execution. Traversal FS
refuse. Pas d'acte irreversible (la facture est un mock).

## Drapeaux honnetes (ne pas "completer")

| Champ | Valeur |
|---|---|
| `llm` | `stub_echo` |
| `phase3_complete` | `false` |
| `us031_complete` | `false` |
| `chromium_session_engine` | `false` |
| `harness` | `dom_simulator` |
| `billing` | `none` |

Aucun secret dans l'image, l'HTML, le JS ou `/health`. `ADMIN_TOKEN` au
run seulement.

## Tests

Hors `make ci` QEMU, hors `make integration-qemu` :

```text
make osui-smoke
make agent-smoke
```

Job GitHub parallele `agent-http-smoke` : `make agent-smoke` puis
`make osui-smoke`. Pas d'OpenAI. Pas de Docker dans ce job. Timeout 5 min.

Fumee contre une origine deja lancee :

```text
python3 osui/server.py
BASE_URL=http://127.0.0.1:8080 osui/scripts/smoke.sh
```

## Non-livré

- US-031 navigateur-OS Chromium / WebKit
- LLM de production
- Retrait de `agent/` (OS-UI-3)
- Facturation SaaS
- Guest i386 dans le conteneur
- Auth par comptes / par site

## Relation au guest

[ETAT_REEL.md](ETAT_REEL.md) reste borne au guest i386. Cette page ne s'y
ajoute pas. Gardes 0-4 : [PLAN_SUITE_IMPLEMENTATION.md](PLAN_SUITE_IMPLEMENTATION.md).
