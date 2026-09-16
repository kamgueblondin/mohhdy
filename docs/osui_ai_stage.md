# OS-UI : scene IA du bureau (`#ai-stage`)

**Date :** 16 septembre 2026
**Statut :** bootstrap graphique du SE Multiboot Mohhdy (`osui/`)
**Ponctuation :** ASCII usuel et accents francais uniquement

Le **produit final** est un seul SE : le **Multiboot Mohhdy OS**. Chat
dirige par prompts, programmes slash, navigateur-OS, shell Ring 3 et
scene IA sont des **devoirs de cet OS**. `osui/` / Docker sont le
**bootstrap graphique actuel** de ce meme OS, pas un second systeme
durable.

Cette page decrit la **scene IA** : le fond du bureau, derriere les
fenetres. Ce n'est **pas** le VGA du guest i386. [ETAT_REEL.md](ETAT_REEL.md)
reste borne au guest ; le guest **n'heberge pas** `#ai-stage`.

## Modele

Le bureau est une **zone ecran** : reflexion, action autonome, presentation
des resultats. Le LLM "pense en HTML" : constructions, simulations et
dessins se rendent dans `#ai-stage`. Les prompts du chat central (hors
slash) mettent a jour cette couche.

Modes visibles (etiquette `#ai-stage-mode`) :

| Mode | Role |
|---|---|
| `reflecting` | Le stub lit le prompt et expose un plan (liste + boites) |
| `acting` | Simulation courte (CSS, pas de JS modele) |
| `presenting` | Resultat structure (HTML / SVG : boites, cercle, graphe, arbre, horloge) |

Mini-plan autonome **etiquete stub** : un prompt du type `mini-plan autonome`,
`etape par etape`, `/plan`, ou `POST` avec `"autonomous": true` enchaine
reflecting -> acting -> presenting. Le client appelle
`POST /api/os/stage/tick` (timer). Ce n'est pas un agent de production.

`llm=stub_echo` tant qu'aucun modele reel n'est attache. Ne pas presenter
le stub comme un LLM de production.

## API

```text
POST /api/os/stage   {"prompt":"..."}
POST /api/os/stage   {"prompt":"...","autonomous":true}
POST /api/os/stage/tick
GET  /api/os/stage
```

JSON : `html`, `mode`, `llm=stub_echo`, `sanitizer=allowlist`,
`scripts_stripped`, `kind` (`scene` / `plan`), `autonomous`, `label=stub`,
`step_index`, `done`. Identite : `GET /api/os` champ `stage`
(`guest_html_stage=false`, `autonomous_plans=true`).

Le client (`os.js`) repeint `#ai-stage-content` apres une **seconde**
allowlist DOM (pas d'execution de script, pas d'attribut `on*`).

## Hygiene HTML

Allowlist serveur (`osui/stage.py`) et cliente : `div`, `span`, `p`,
titres, listes, `svg`/`rect`/`circle`/`path`/`text`/`g`, etc. Interdit :
`script`, `iframe`, `style`, gestionnaires, `javascript:`.

Le stub n'emet que des scenes simples (boites, SVG, simulation CSS, arbre
VFS, graphe, horloge). Un prompt `dessine un cercle` rend un `<circle>`.

## Preuves

```text
make osui-smoke
```

Un prompt `dessine trois boites` remplit la scene (`presenting` + SVG).
Un mini-plan avance via `/api/os/stage/tick`. Hors `make ci` QEMU, hors
`make integration-qemu`. Pas d'OpenAI.
