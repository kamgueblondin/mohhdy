# ASSIST-012 / 030 / 031 / 041 - KB, droits, escalade, handoff

**Date :** 15 septembre 2026
**Statut :** livré dans `agent/`, avec limites honnêtes (stub local, pas de LLM de production)
**Ponctuation :** ASCII usuel et accents français uniquement

Ce document décrit la base de connaissance locale, le masque de droits
par site et par session, l'escalade humaine et la reprise de la **même**
conversation. L'image Docker reste celle d'ASSIST-050 :
[assist050_docker_runtime.md](assist050_docker_runtime.md).
Sessions / embed / admin de base : [assist010_sessions_admin.md](assist010_sessions_admin.md).
Spec produit : [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md).

Ce n'est **pas** un LLM de production. Ce n'est **pas** le noyau i386.
Aucun appel OpenAI public. Aucun secret dans l'image ni dans `embed.js`.
Les actes navigateur de cette tranche sont un **simulateur DOM**
(ASSIST-020), pas Chromium. Détail : [assist020_gestes_mcp.md](assist020_gestes_mcp.md).

## Configuration (KB + allowlist)

Deux variables d'environnement, lues **au démarrage** :

| Variable | Rôle |
|---|---|
| `MOHHDY_AGENT_CONFIG` | JSON : `default_capabilities`, `sites.{id}.capabilities`, `sites.{id}.kb` et/ou `kb_file` |
| `MOHHDY_AGENT_KB` | Fichier texte, Markdown (`##` titres) ou JSON d'extraits, fusionné comme KB par défaut |

Exemple (fichier du dépôt, sans secret) :

```text
MOHHDY_AGENT_CONFIG=agent/config.example.json python3 agent/server.py
```

Dans Docker :

```text
docker run --rm -p 8080:8080 \
  -e ADMIN_TOKEN=change-me-at-runtime \
  -e MOHHDY_AGENT_CONFIG=/app/config.example.json \
  mohhdy-agent
```

Sans fichier : la KB est vide. Une question du type "comment ça marche ?"
reçoit un **refus honnête**, pas une invention. Un message générique
reste un echo stub (`llm=stub_echo`).

Avec KB : les réponses sont ancrées dans les extraits (`llm=stub_kb`).
Le stub ne recopie pas la question comme si c'était un savoir.

`/health` expose `kb_loaded` (booléen). Jamais le chemin du fichier,
jamais un secret.

Allowlist par défaut :

```text
chat.reply
site.explain
session.escalate
admin.observe
admin.takeover
```

Placeholders restants : un `mcp.*` déclaré mais sans runner.

## Droits (ASSIST-030)

Grant / limit / revoke via l'API admin (jeton `ADMIN_TOKEN` si défini) :

| Route | Rôle |
|---|---|
| `GET /api/admin/capabilities` | Catalogue (session / admin / gesture / mcp) |
| `GET /api/admin/sites/{site_id}/capabilities` | Allowlist du site (nouvelles sessions) |
| `POST /api/admin/sites/{site_id}/capabilities` | `{grant, revoke, set}` |
| `POST /api/admin/sessions/{id}/capabilities` | Même contrat, **cette** session |
| `POST /api/sessions/{id}/tools` | Tentative d'outil visiteur |

Le widget (`GET /api/sessions/{id}`) reçoit un diagnostic public :
`chat.reply`, `site.explain`, `session.escalate`, et les gestes / MCP
accordés. Il **ne** reçoit **pas** `admin.observe`, `admin.takeover`,
ni de préfixe interne `acl.` / `internal.`. Accorder `acl.*` est rejeté
(400).

Preuves négatives :

- Outil absent ou révoqué : `403 capability_denied`, acte non exécuté.
- Origine étrangère : `403 origin_denied` + `request_id` (simulateur).
- MCP non déclaré : `403 tool_undeclared`.
- `chat.reply` révoqué : l'agent refuse de répondre, sans inventer un droit.

Une tentative hors allowlist place la session en `waiting_human`
(politique, sans exécution).

## Escalade (ASSIST-031)

Le visiteur demande un humain :

```text
POST /api/sessions/{id}/escalate
{"reason": "optionnel"}
```

Le widget expose le bouton **Parler a un humain**.

États :

| `status` | Signification |
|---|---|
| `open` | L'agent peut répondre (si `chat.reply`) |
| `waiting_human` | File admin, l'agent ne répond plus tout seul |
| `human_active` | Un humain a pris la main |
| `closed` | Réservé, pas de clôture UI dans cette tranche |

Si `session.escalate` est révoqué, la demande visiteur est `403`.
La politique (outil hors allowlist) peut quand même escalader : l'agent
n'exécute pas et n'invente pas le droit manquant.

File admin : `GET /api/admin/sessions?status=waiting_human`.

## Handoff (ASSIST-041)

Même `session_id`. L'humain écrit dans le fil visiteur.

| Route | Rôle |
|---|---|
| `POST /api/admin/sessions/{id}/takeover` | Passe en `human_active`, journal système |
| `POST /api/admin/sessions/{id}/messages` | Message `role=human` / `speaker=human` |

Après takeover :

- `POST /api/sessions/{id}/messages` enregistre le visiteur, `auto_reply=false`,
  `agent_message=null`.
- Le widget affiche les messages humains (sondage HTTP) et n'attend plus
  une réponse agent.
- `/admin` : file, bouton **Prendre la main**, zone de réponse.

Le journal de chaque message porte `role` et `speaker`
(`visitor` / `agent` / `human` / `system`).

Sans `admin.takeover` sur la session : takeover `403`.

## Vérifier

```text
make agent-smoke
```

Couvre isolation de deux sessions, refus KB vide, ancrage KB, révocation
d'outil, gestes simulateur, origine étrangère, facture, escalade, handoff
sur le même `session_id`, absence de secret et de préfixe ACL dans
`embed.js`. Hors `make ci` AOS QEMU : job parallèle `agent-http-smoke`.

Contre une origine déjà lancée :

```text
MOHHDY_AGENT_CONFIG=agent/config.example.json python3 agent/server.py
BASE_URL=http://127.0.0.1:8080 agent/scripts/smoke.sh
```

Ouvrir `http://127.0.0.1:8080/demo` : bulle, question, bouton humain.
Ouvrir `http://127.0.0.1:8080/admin` : file, takeover, réponse.

## Non livré

- LLM de production, GGUF, fournisseur public
- Playwright / Chromium (les gestes sont un simulateur : [assist020_gestes_mcp.md](assist020_gestes_mcp.md))
- Auth par site, comptes opérateurs, HTTPS terminé dans l'image
- Refus automatique d'origine document vs site déclaré (reste ouvert)
- Noyau Multiboot dans le conteneur
