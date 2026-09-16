# OS-UI : scene IA VGA structuree

**Date :** 16 septembre 2026
**Statut :** scene portee dans le guest Ring 3 (VGA 8x48 + canvas desktop 22x78). Pas de HTML `#ai-stage`
**Ponctuation :** ASCII usuel et accents francais uniquement

Le **produit final** est un seul SE : le **Multiboot Mohhdy OS**. La
scene IA est un **devoir de cet OS**. Elle se rend dans le framebuffer
texte VGA, pas dans un DOM HTML hote.

[ETAT_REEL.md](ETAT_REEL.md) mesure le guest. Le guest **n'heberge pas**
`#ai-stage`. `MOHHDY_OSUI_GUEST_HTML_STAGE 0`, `MOHHDY_OSUI_STAGE_VGA 1`,
`MOHHDY_OSUI_VGA_DESKTOP 1`.

## Modele

Le contrat serie conserve une zone 8 lignes x 48 colonnes. Le bureau
`gui` compose un canvas 22x78 (cercles, boites, graphe, arbre, horloge,
simulation stub). Les prompts (`chat`, `prompt`, `stage-prompt`, `/plan`,
`/draw`) mettent a jour les deux couches. Ce n'est **pas** un moteur
HTML ni SVG.

| Mode | Role |
|---|---|
| `reflecting` | Le stub lit le prompt et expose un plan texte |
| `acting` | Simulation courte etiquetee stub |
| `presenting` | Resultat structure (boites `[A] [B] [C]`, graphe, etc.) |

Mini-plan : `/plan` enchaine acting. `llm=stub_echo`. Les balises
`<script>` sont strippees (`scripts_stripped=1`). Ce n'est pas un
agent de production.

## Commandes

```text
stage
stage-prompt dessine trois boites
/plan
chat bonjour
os-status
```

Sortie : `mode=`, `kind=`, `canvas=vga_desktop`, `guest_html_stage=false`,
`llm=stub_echo`. Instantane nographic : `gui-status`.

## Hygiene

Pas d'execution de script, pas d'HTML. Allowlist texte. L'ancien
sanitizer `osui/stage.py` n'existe plus.

Guides : [osui_0_1_2.md](osui_0_1_2.md), [osui_chat_desktop.md](osui_chat_desktop.md).
