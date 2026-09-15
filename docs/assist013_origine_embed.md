# ASSIST-013 - origine du document embed

**Date :** 15 septembre 2026
**Statut :** livré dans `agent/` (liaison d'origine, pas une auth multi-tenant)
**Ponctuation :** ASCII usuel et accents français uniquement

Ce document décrit le refus automatique quand l'origine du **document**
qui charge `embed.js` ne correspond pas au site déclaré. Snippet et CSP :
[assist010_sessions_admin.md](assist010_sessions_admin.md). Spec :
[../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md).

Ce n'est **pas** une authentification par comptes opérateurs. Ce n'est
**pas** Chromium. L'en-tête `Origin` d'un client non navigateur reste
spoofable. Le but est d'empêcher qu'une page tierce ouvre une session
ou dépose des messages / outils au nom d'un `site_id` qu'elle n'a pas
déclaré.

## Allowlist

Fichier JSON (`MOHHDY_AGENT_CONFIG`). Exemple : `agent/config.example.json`.

```text
allowed_origins              # repli instance, defaut ["self"]
sites.{id}.allowed_origins   # surcharge par site_id
```

`self` désigne l'origine HTTP de l'instance (en-tête `Host`). La page
`/demo` et les appels curl sans `Origin` (fumee) tombent sur `self`.

Pour attacher un site que vous contrôlez :

```json
"sites": {
  "site_shop_example": {
    "allowed_origins": ["https://shop.example"]
  }
}
```

Une entrée `http://127.0.0.1` sans port accepte n'importe quel port de
ce hôte. Une origine absente de la liste est refusée.

## Enforcement

Le runtime lit, dans cet ordre : `Origin`, sinon `Referer`, sinon le
champ JSON `origin` (widget), sinon `self`.

| Requete | Succes | Echec |
|---|---|---|
| `POST /api/sessions` | 201, session liée à l'origine | 403 `origin_denied`, **pas** de session |
| `GET /api/sessions/{id}` | fil de **cette** origine | 403, pas de messages dans le corps |
| `POST .../messages` | message enregistré | 403, contenu **non** stocké |
| `POST .../tools` | voir [assist020_gestes_mcp.md](assist020_gestes_mcp.md) | 403, aucun acte |
| `POST .../escalate` | file humain | 403, statut inchangé |

Le JSON d'erreur porte `error=origin_denied` et `request_id`. Le journal
admin (`GET /api/admin/journal`) et stderr (`mohhdy-agent: ... origin_denied
request_id=...`) reçoivent la même corrélation. Aucun secret.

Une session créée depuis `self` reste liée à l'instance (y compris après
redémarrage sur un autre port). Une session créée depuis
`https://shop.example` refuse `self` et toute autre origine.

`/api/admin/*` n'applique **pas** cette liaison. `ADMIN_TOKEN` inchangé.

Les gestes gardent en plus le champ JSON `origin` (ASSIST-020) : un
corps `https://evil.example` reste `origin_denied` même sans en-tête.

## Vérifier

```text
make agent-smoke
```

Couvre : create same-origin, create origine étrangère, message et outil
refusés sans fuite de secret. Hors `make ci` / hors QEMU.

## Non livré

- Comptes opérateurs, auth par site, cookies httpOnly
- Liaison cryptographique (l'en-tête Origin n'est pas une signature)
- Playwright / Chromium, LLM de production
- Noyau Multiboot dans le conteneur
