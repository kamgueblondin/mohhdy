# Migration OS-UI : capacites ASSIST dans le SE graphique

**Date :** 15 septembre 2026
**Statut :** OS-UI-000 docs. Premieres tranches OS-UI-0/1/2 livrees dans `osui/` (backend `agent/` temporaire). OS-UI-3 ouvert
**IDs :** `OS-UI-xxx` (ordonnancement). Les tickets `ASSIST-xxx` restent la spec fonctionnelle
**Ponctuation :** ASCII usuel et accents francais uniquement

Mohhdy est un seul SE. Les tickets `ASSIST-xxx` de [mohhdy_agent_support_web.md](mohhdy_agent_support_web.md) sont des **devoirs du SE**, aujourd'hui portes par le scaffold `agent/` derriere le chrome `osui/`. Ce fichier les range dans des epiques **OS-UI** pour les faire vivre dans le shell graphique et le navigateur-OS, puis retirer la facade Python.

Plan maitre : [../docs/PLAN_SE_MOHHDY_COMPLET.md](../docs/PLAN_SE_MOHHDY_COMPLET.md). Gardes guest : [../docs/PLAN_SUITE_IMPLEMENTATION.md](../docs/PLAN_SUITE_IMPLEMENTATION.md). Guest mesure : [mohhdy_us.md](mohhdy_us.md) et [../docs/ETAT_REEL.md](../docs/ETAT_REEL.md). Guide runtime : [../docs/osui_0_1_2.md](../docs/osui_0_1_2.md). Interaction : [../docs/osui_chat_desktop.md](../docs/osui_chat_desktop.md). Scene IA : [../docs/osui_ai_stage.md](../docs/osui_ai_stage.md).

**Prochain build :** OS-UI-3 (retrait facade Python) apres checklist de parite. Ne pas etendre `agent/` comme produit. Ne pas marquer US-031 ni un LLM de production comme livres.

## Convention

**En tant que** / **je veux** / **afin de**. Critere = observable. Statut implicite : **spec de portage**.

Les IDs `OS-UI-*` n'annulent pas `ASSIST-*`. Mapping : une epique OS-UI couvre plusieurs ASSIST.

Gates de chaque epique : moindre privilege, grant/revoke, `request_id`, pas de secret UI/image, origine binding, pas d'acte irreversible sans politique, pas d'allongement `make integration-qemu` sans compensation mesuree, ponctuation ASCII des docs, honnetete ETAT_REEL, smoke de tranche, PR rollback-friendly, pas d'OpenAI en CI. Detail : section 4 du plan maitre.

## OS-UI-000 - Spec de migration

**En tant que** proprietaire produit, **je veux** un plan unique de toutes les capacites visees et un ordre de portage vers l'OS+UI, **afin de** ne plus traiter `agent/` comme un sidecar durable.

**Critere.** [../docs/PLAN_SE_MOHHDY_COMPLET.md](../docs/PLAN_SE_MOHHDY_COMPLET.md) existe. README / docs / PLAN_SUITE pointent dessus. Le prochain code est OS-UI-0. Aucune phrase ne dit que US-031 ou un LLM de production sont livres. `agent/` n'est pas reecrit dans cette epique.

**ASSIST couverts.** ASSIST-000 (cadrage).

**Statut.** Docs livrees (OS-UI-000).

## OS-UI-0 - Shell graphique minimal (instance Docker)

**En tant qu'**operateur d'instance, **je veux** que `docker run` ouvre le chrome du SE (fenetre, vue operateur), **afin que** Docker boot l'OS comme une machine vierge, pas un serveur HTTP de support.

**Dependances.** OS-UI-000. Aucune garde AOS 0-4 bloquante. Interdit d'attendre US-001 complet. Interdit de booter Chromium dans QEMU i386.

**Critere.**

- L'entree Docker presente un shell graphique minimal du SE
- Sante instance, aucun secret dans l'image
- Documente : pas US-031, pas LLM de production, `make integration-qemu` inchange
- PC / hyperviseur : meme contrat d'instance, meme si le packaging reste scaffold

**ASSIST couverts.** Debut de ASSIST-050 / 051 / 052 (boot d'instance, pas encore parite ni retrait Python).

**Statut.** Premiere tranche livree : `osui/`, `docker run mohhdy-os` ouvre le chrome a **chat central** + **scene IA**. Slash `/help` `/browser` `/shell` etc. `/shell` = vocabulaire Multiboot Ring 3 (bootstrap). Chat flottant si un programme s'ouvre. Pas US-031. Guest sans `#ai-stage` (ETAT_REEL).

**Hors perimetre.** Retrait de `agent/`. Chromium de session.

## Modele d'interaction (shell courant)

**En tant qu'**utilisateur de l'instance, **je veux** commander le SE Multiboot par prompts dans un chat central, avec une scene IA derriere les fenetres, **afin que** l'OS pense et presente en HTML, pas seulement par icones.

**Critere.**

- Etat par defaut : chat large au centre, `llm=stub_echo` visible, `#ai-stage` en fond (mode `reflecting`)
- Un prompt hors slash met a jour la scene (HTML/SVG stub, allowlist, pas de script) ; modes `reflecting` / `acting` / `presenting`
- `/help` liste le registre ; `/browser` `/shell` `/admin` `/support` `/status` `/fs` ouvrent le programme
- `/shell` est le vocabulaire guest Ring 3 (`userspace/shell.c`, prompt `MOHHDY>`), surface bootstrap ; live QEMU/serial non branche
- A l'ouverture d'un programme, le chat quitte le centre et devient un panneau flottant draggable (coin, z-index au-dessus des fenetres, position `sessionStorage`)
- Fermer tous les programmes ou `/center` ramene le chat au centre
- APIs `agent/` inchangees (sessions, admin, tools, FS). Origine binding inchangee. `phase3_complete=false`, `us031_complete=false`
- ETAT_REEL guest inchange : pas de scene HTML dans le VGA i386

Guides : [../docs/osui_chat_desktop.md](../docs/osui_chat_desktop.md), [../docs/osui_ai_stage.md](../docs/osui_ai_stage.md).

## OS-UI-1 - Sessions, chat, admin, droits en UI native

**En tant que** visiteur, operateur de site et humain de support, **je veux** parler, etre isole par session, voir une KB honnete, accorder/revoquer, escalader et reprendre **dans l'UI du SE**, **afin que** le support ne soit plus une facade Python.

**Dependances.** OS-UI-0 (chrome). Vocabulaire Foundation (grant/revoke/scope). Pas US-001 termine.

**Critere.** Parite comportementale avec le scaffold : deux `session_id`, embed ou fenetre sans secret, `origin_denied` + `request_id`, masque de droits, revoke efficace, file d'escalade, takeover dans la meme session, stub LLM visible (`llm=stub` tant que pas de moteur reel). Auth par comptes / par site : pas exigee ici.

**ASSIST couverts.** 010, 011, 012, 013, 030, 031, 040, 041.

**Statut.** Premiere tranche livree dans les panes Support et Admin. APIs `agent/` conservees.

**Hors perimetre.** Gestes DOM reels Chromium, US-031, comptes multi-tenant, OpenAI.

## OS-UI-2 - Actes dans le navigateur-OS du SE

**En tant qu'**utilisateur de l'instance, **je veux** que le SE agisse dans **son** navigateur-OS (gestes allowlistes, outil MCP, facture demo, FS-as-web), **afin que** Playwright ne soit plus le recit produit.

**Dependances.** OS-UI-1 (droits de session). Voisinage phase 3 / US-031 **non livre**.

**Critere.** Un geste allowliste et `mcp.invoice.create` (ou equivalent) reussissent dans la surface OS, meme `session_id`, journal `request_id`. Origine etrangere et outil non declare : 403, pas d'execution. FS : list+read sandbox, traversal refuse, pas d'ecriture sauf politique. `phase3_complete=false`, `us031_complete=false` tant que le moteur n'est pas un navigateur-OS reel. ETAT_REEL guest inchange.

**ASSIST couverts.** 020, 021, 022, 060, 061. Playwright optionnel : a absorber, pas a etendre.

**Statut.** Premiere tranche livree dans le pane Browser-OS (simulateur etiquete, FS lecture, facture mock). `phase3_complete=false`.

**Hors perimetre.** Declarer US-031 livre. PWA / FS web unifie (US-032 / US-033 maitre).

## OS-UI-3 - Retrait de la facade Python

**En tant que** mainteneur, **je veux** retirer `agent/` comme facade une fois la parite tenue, **afin qu'il** n'y ait plus deux recits d'instance.

**Dependances.** OS-UI-1 et OS-UI-2 sortis. Checklist de parite du plan maitre (section 8.3).

**Critere.** Packaging Docker/PC/hyperviseur pointe vers l'OS. Smokes de regression verts. Docs `assist*` marquees historiques. Contrats comportementaux conserves. Guest C et `make ci` inchanges sauf besoin prouve.

**ASSIST couverts.** Fin de 050 / 051 / 052 (parite deploiement).

**Hors perimetre.** Big-bang sans parite. Billing ASSIST-053. Corps ASSIST-090.

## Hors epiques proches (rappel)

| ID | Motif |
|---|---|
| ASSIST-053 paiement | Scaffold `billing=false` seulement |
| ASSIST-090 | Corps physique inexistant |
| US-016 | Pas TFLite proche |
| US-031 phase 3 complet | Apres un vrai moteur ; pas la DoD d'OS-UI-2 |
| AOS 0-4 | Gardes paralleles, pas des epiques OS-UI |

## Table recap

| Rang | Epic | Build ? | ASSIST |
|---:|---|---|---|
| 0 | OS-UI-000 | docs livres | 000 |
| 1 | OS-UI-0 | **premiere tranche** `osui/` chat + scene IA + shell Multiboot | 050/051/052 (boot) |
| 2 | OS-UI-1 | **premiere tranche** chat + panes Support/Admin | 010-013, 030, 031, 040, 041 |
| 3 | OS-UI-2 | **premiere tranche** pane Browser-OS | 020-022, 060, 061 |
| 4 | OS-UI-3 | **prochain** apres parite 1+2 | fin 050-052 |
| - | Gardes guest 0-4 | parallele | (AOS, pas ASSIST) |

Nouveaux tickets de portage : les ajouter ici (`OS-UI-xxx`) **ou** comme sous-taches ASSIST, sans `AOS-` et sans renumeroter `US-xxx`.
