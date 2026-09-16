# OS-UI : chat, scene IA, shell Multiboot

**Date :** 16 septembre 2026
**Statut :** modele d'interaction du guest Ring 3 (`osui_runtime.c`) + surface HTML hote
**Ponctuation :** ASCII usuel et accents francais uniquement

Mohhdy est **un seul SE Multiboot**. Le boot reste le **prompt
`MOHHDY>`**. La commande canonique **`gui`** (aliases `graphics`,
`desktop`) entre le mode graphique. `console` / `gui-exit` / ESC
revient au texte.

Le bureau **produit** est la surface HTML/CSS/JS (`osui/static`), rendue
par le helper mince `osui/display_host.py` (`make run-gui` ->
http://127.0.0.1:18080). L'etat (chat central/flottant, panes slash,
scene IA, sessions) vient du guest C via des instantanes `OSUI-SNAP`.
Le blit VGA n'est plus un bureau ASCII 80x25 ; c'est un pointeur
honnete vers la surface HTML.

Chrome : [osui_0_1_2.md](osui_0_1_2.md). Scene :
[osui_ai_stage.md](osui_ai_stage.md). Attache :
[osui_shell_live.md](osui_shell_live.md). Convergence :
[osui_convergence.md](osui_convergence.md). Plan :
[PLAN_SE_MOHHDY_COMPLET.md](PLAN_SE_MOHHDY_COMPLET.md). Ce n'est **pas**
US-031, **pas** un LLM de production, **pas** un bash Linux, **pas**
un sidecar `agent/`.

## Honnêteté

| Drapeau | Valeur |
|---|---|
| `llm` | `stub_echo` |
| `us031_complete` | `false` |
| `phase3_complete` | `false` |
| `python_facade` | `false` |
| `guest_html_stage` | `false` (le guest n'execute pas HTML) |
| `display_surface` | `html_host` |
| `display_host` | `true` (static + proxy serie, pas de logique metier) |

## Etat par defaut

Le shell boot. Session `s0001`, caps visiteur (`chat.reply`). Scene
`reflecting`. Un prompt hors slash (`chat ...` ou texte non builtin) :

1. repond par stub echo / KB locale
2. met a jour la scene (kind/mode dans l'instantane)

Quand un programme s'ouvre (`/browser`, `/shell`, ...), le chat passe
en `chat_mode=float`. `/center` ramene `chat_mode=center`.

## Slash (vocabulaire conserve)

`/help` `/browser` `/shell` `/admin` `/support` `/status` `/fs` `/plan`
`/center` `/close`. Pieges Linux (`apt`, `sudo`, `bash`) refuses **avant**
le mode question IA.

Verification : `make qemu-osui-runtime`, `make qemu-osui-gui`,
`make osui-smoke`, tests Unity `test_osui_runtime.c` et `test_osui_gui.c`.
Bureau live : `make run-gui`. Instantane nographic : `gui-status`.
