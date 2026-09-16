# OS-UI : chat, scene VGA, shell Multiboot

**Date :** 16 septembre 2026
**Statut :** modele d'interaction du guest Ring 3 (`osui_runtime.c`)
**Ponctuation :** ASCII usuel et accents francais uniquement

Mohhdy est **un seul SE Multiboot**. La surface primaire est le **prompt
`MOHHDY>`** avec commandes chat-like. Le fond est la **scene IA VGA**.
Les programmes s'ouvrent par slash (`/browser`, `/shell`, `/admin`) ou
par `prompt ouvre le shell`. Quand un programme s'ouvre, le chat passe
en `chat_mode=float`. `/center` ramene `chat_mode=center`.

Chrome : [osui_0_1_2.md](osui_0_1_2.md). Scene :
[osui_ai_stage.md](osui_ai_stage.md). Attache :
[osui_shell_live.md](osui_shell_live.md). Convergence :
[osui_convergence.md](osui_convergence.md). Plan :
[PLAN_SE_MOHHDY_COMPLET.md](PLAN_SE_MOHHDY_COMPLET.md). Ce n'est **pas**
US-031, **pas** un LLM de production, **pas** un bash Linux.

## Etat par defaut

Le shell boot. Session `s0001`, caps visiteur (`chat.reply`). Scene
`reflecting`. Accueil honnete : `llm=stub_echo`, `phase3_complete=false`,
`us031_complete=false`, `python_facade=false`.

Un prompt hors slash (`chat ...` ou texte non builtin) :

1. repond par stub echo / KB locale
2. met a jour la scene VGA

`session-new` cree `s0002+` isole. Origine binding : `origin-check`
(ASSIST-013). Grant/revoke avant geste ou MCP.

## Slash (vocabulaire conserve)

`/help` `/browser` `/shell` `/admin` `/support` `/status` `/fs` `/plan`
`/center` `/close`. Pieges Linux (`apt`, `sudo`, `bash`) refuses **avant**
le mode question IA.

Verification : `make qemu-osui-runtime`, tests Unity `test_osui_runtime.c`.
