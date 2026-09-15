# OS-UI : chat central, raccourcis slash, chat flottant

**Date :** 15 septembre 2026
**Statut :** modele d'interaction du shell `osui/` (tranches OS-UI-0/1/2)
**Ponctuation :** ASCII usuel et accents francais uniquement

Mohhdy est un **SE dirige par prompts**. Le bureau n'est pas un Support
lateral comme identite produit. La surface primaire est le **chat
central**. Les programmes s'ouvrent par raccourci (`/browser`, `/shell`,
...) ou par un prompt equivalent. Quand un programme s'ouvre, le chat
**quitte le centre** et devient un **panneau flottant, deplacable**.

Chrome et APIs : [osui_0_1_2.md](osui_0_1_2.md). Plan :
[PLAN_SE_MOHHDY_COMPLET.md](PLAN_SE_MOHHDY_COMPLET.md). Ce n'est **pas**
US-031, **pas** un LLM de production, **pas** un root Linux.

## Etat par defaut

`GET /` ouvre le bureau. Le chat occupe le centre (`#os-chat`,
`data-mode="center"`). Accueil honnete : `llm=stub_echo`,
`phase3_complete=false`, `us031_complete=false`. Les fenetres programmes
sont fermees.

Un prompt hors slash parle au stub (echo / KB locale) via
`POST /api/sessions/{id}/messages`. Session creee a la demande
(`site_id` defaut `osui_demo`). Origine binding inchangee
(ASSIST-013).

## Raccourcis (`/help`)

Registre HTML : `#os-slash-registry`. JSON : `GET /api/os` champ
`commands`. JS : `window.MohhdyOS.commands` et `parseLine`.

| Raccourci | Effet |
|---|---|
| `/help` | Liste les raccourcis |
| `/browser` | Ouvre Browser-OS (simulateur DOM) |
| `/shell` | Ouvre le shell UI de l'instance |
| `/admin` | Ouvre Admin |
| `/support` | Ouvre Support (sessions, escalade) |
| `/status` | Ouvre Statut |
| `/fs` | Ouvre le FS sandbox lecture |
| `/center` | Ferme les programmes, chat au centre |
| `/close` | Ferme les programmes, chat au centre |

Equivalents de prompt : "ouvre le navigateur", "open shell", etc.
Inconnu : message systeme, pas d'execution.

## Chat flottant

Des qu'un programme s'ouvre (`openPane` hors `chat`) :

- `#os-chat` passe `data-mode="float"`
- Coin bas-droit par defaut (au-dessus du dock)
- Z-index au-dessus des fenetres (`--os-chat-z: 1100`)
- Drag par l'en-tete (`data-drag="chat"`, cursor grab)
- Position persistee dans `sessionStorage` (`mohhdy.os.chat.pos`)
- Les prompts et slash restent disponibles

Fermer **toutes** les fenetres, `/center`, `/close`, ou le bouton
Centrer : le chat revient au centre.

API de test DOM : `window.MohhdyOS.getChatMode()`,
`openPane("browser")`, `setChatMode`, `closeAllPrograms`.

## Shell OS (`/shell`)

Pane etiquete **Shell OS**. Commandes locales : `help`, `status`,
`open`, `panes`, `llm`, `whoami`, `clear`, plus les memes slash que le
chat. Ce n'est **pas** un TTY root Linux, **pas** le guest i386.

## Preuves

```text
make osui-smoke
```

Hors `make ci` QEMU, hors `make integration-qemu`. Pas d'OpenAI. Pas de
secret dans l'UI. `agent/` n'est pas retire (OS-UI-3).
