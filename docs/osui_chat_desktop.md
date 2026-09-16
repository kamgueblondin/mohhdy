# OS-UI : chat central, scene IA, shell Multiboot

**Date :** 15 septembre 2026
**Statut :** modele d'interaction du bootstrap graphique `osui/`
**Ponctuation :** ASCII usuel et accents francais uniquement

Mohhdy est **un seul SE Multiboot**. Le bureau n'est pas un Support
lateral comme identite produit. La surface primaire est le **chat
central**. Le **fond du bureau** est la **scene IA** (`#ai-stage`) :
reflexion, action, resultats en HTML. Les programmes s'ouvrent par
raccourci (`/browser`, `/shell`, ...) ou par un prompt equivalent.
Quand un programme s'ouvre, le chat **quitte le centre** et devient un
**panneau flottant, deplacable**. `/shell` ouvre le **meme vocabulaire**
que le shell guest Ring 3 (`userspace/shell.c`), en surface bootstrap
tant que QEMU n'est pas attache.

Chrome et APIs : [osui_0_1_2.md](osui_0_1_2.md). Scene IA :
[osui_ai_stage.md](osui_ai_stage.md). Plan :
[PLAN_SE_MOHHDY_COMPLET.md](PLAN_SE_MOHHDY_COMPLET.md). Ce n'est **pas**
US-031, **pas** un LLM de production, **pas** un bash Linux. Le guest
mesure : [ETAT_REEL.md](ETAT_REEL.md) (pas de scene HTML guest).

## Etat par defaut

`GET /` ouvre le bureau. Le chat occupe le centre (`#os-chat`,
`data-mode="center"`). La scene IA occupe tout le fond
(`#ai-stage`, mode `reflecting`). Accueil honnete : `llm=stub_echo`,
`phase3_complete=false`, `us031_complete=false`. Les fenetres programmes
sont fermees.

Un prompt hors slash :

1. parle au stub (echo / KB locale) via `POST /api/sessions/{id}/messages`
2. met a jour `#ai-stage` via `POST /api/os/stage` (HTML / SVG stub)

Session creee a la demande (`site_id` defaut `osui_demo`). Origine
binding inchangee (ASSIST-013).

## Raccourcis (`/help`)

Registre HTML : `#os-slash-registry`. JSON : `GET /api/os` champ
`commands`. JS : `window.MohhdyOS.commands` et `parseLine`.

| Raccourci | Effet |
|---|---|
| `/help` | Liste les raccourcis |
| `/browser` | Ouvre Browser-OS (simulateur DOM) |
| `/shell` | Ouvre le shell Multiboot (vocabulaire guest Ring 3) |
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
- Les prompts et slash restent disponibles (un prompt continue de
  mettre a jour la scene IA)

Fermer **toutes** les fenetres, `/center`, `/close`, ou le bouton
Centrer : le chat revient au centre.

API de test DOM : `window.MohhdyOS.getChatMode()`,
`openPane("browser")`, `setChatMode`, `closeAllPrograms`,
`applyStage`, `sanitizeStageHtml`.

## Shell Multiboot (`/shell`)

`/shell` n'est **pas** un jouet bash. C'est la surface du **meme**
shell que le guest Multiboot (Ring 3, `userspace/shell.c`, prompt
`MOHHDY>`).

| Commande (extrait) | Mapping guest | Bootstrap osui |
|---|---|---|
| `help` | `cmd_help` | Liste alignee (ai, vfs, ls, ...) |
| `ai <q>` | `SYS_GPT2_GENERATE` / GGUF | stub `llm=stub_echo` + scene IA |
| `ai-help` / `ai-runtime` | builtins IA | honnete : pas de modele charge |
| `vfs-list` / `vfs-read` / `vfs-stat` | `vfsserver` | miroir lecture, mutations refusees |
| `ls` / `pwd` / `cat` | syscalls fichiers | miroir `initrd/` `overlay/` `fat16/` |
| `net-status` | `SYS_NET_STATUS` | `nic=absent` |
| `attach` | futur TTY QEMU/serial | **refuse** : non branche |

Etat visible : `attachment=bootstrap live_guest=false qemu_serial=false`.
`POST /api/os/shell` `{"line":"..."}`. `GET /api/os/shell` : registre.

Ne pas pretendre qu'un TTY QEMU tourne dans Docker. Ne pas inventer
`apt` / `sudo`. Future attache live : meme vocabulaire, autre transport.

## Preuves

```text
make osui-smoke
```

Hors `make ci` QEMU, hors `make integration-qemu`. Pas d'OpenAI. Pas de
secret dans l'UI. `agent/` n'est pas retire (OS-UI-3).
