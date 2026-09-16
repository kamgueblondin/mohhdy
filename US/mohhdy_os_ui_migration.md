# Migration OS-UI : capacites ASSIST dans le SE Multiboot

**Date :** 16 septembre 2026
**Statut :** OS-UI-000 a OS-UI-3 livres dans le guest C. Facade Python retiree
**IDs :** `OS-UI-xxx` (ordonnancement). Les tickets `ASSIST-xxx` restent la spec fonctionnelle
**Ponctuation :** ASCII usuel et accents francais uniquement

Mohhdy est un seul SE. Les tickets `ASSIST-xxx` de [mohhdy_agent_support_web.md](mohhdy_agent_support_web.md) sont des **devoirs du SE**, portes dans `userspace/osui_runtime.c` derriere le vocabulaire `MOHHDY>`. Le sidecar `agent/` et le serveur Python `osui/` sont **retires**.

Plan maitre : [../docs/PLAN_SE_MOHHDY_COMPLET.md](../docs/PLAN_SE_MOHHDY_COMPLET.md). Gardes guest : [../docs/PLAN_SUITE_IMPLEMENTATION.md](../docs/PLAN_SUITE_IMPLEMENTATION.md). Guest mesure : [mohhdy_us.md](mohhdy_us.md) et [../docs/ETAT_REEL.md](../docs/ETAT_REEL.md). Guide runtime : [../docs/osui_0_1_2.md](../docs/osui_0_1_2.md). Interaction : [../docs/osui_chat_desktop.md](../docs/osui_chat_desktop.md). Scene IA : [../docs/osui_ai_stage.md](../docs/osui_ai_stage.md). Live : [../docs/osui_shell_live.md](../docs/osui_shell_live.md). Convergence : [../docs/osui_convergence.md](../docs/osui_convergence.md).

**Prochain build :** gardes AOS 0-4 et US-031 honnete. Ne pas reintroduire Python comme produit. Ne pas marquer US-031 ni un LLM de production comme livres.

## Convention

**En tant que** / **je veux** / **afin de**. Critere = observable.

Gates : moindre privilege, grant/revoke, `request_id`, pas de secret image, origine binding, pas d'acte irreversible sans politique, pas d'allongement `make integration-qemu` sans compensation, ponctuation ASCII, honnetete ETAT_REEL.

## OS-UI-000 - Spec de migration

**Statut.** Docs livrees.

## OS-UI-0 - Instance Docker = boot Multiboot

**Statut.** Livre : `docker run -it mohhdy-os` lance QEMU. Slash, scene VGA, vocabulaire Ring 3. Pas US-031. Pas `#ai-stage` HTML.

## OS-UI-1 - Sessions, chat, admin, droits

**Statut.** Livre dans le guest C (`session-*`, `chat`, `grant`/`revoke`, `escalate`/`takeover`, `origin-check`). `llm=stub_echo`.

## OS-UI-2 - Actes navigateur-OS allowlistes

**Statut.** Livre : `browser-*` simulateur, `mcp-invoice`, `fs-list`/`fs-read`. `phase3_complete=false`. Pas US-031.

## OS-UI-C - Convergence Multiboot

**Statut.** Livre : registre `shared/multiboot_shell_commands.json`, header `mohhdy_osui_bridge.h`, scene VGA, runtime C unique.

## OS-UI-3 - Retrait de la facade Python

**En tant que** mainteneur, **je veux** qu'il n'y ait plus deux recits d'instance, **afin que** Docker boot le SE.

**Statut.** Livre. `agent/` et `osui/` Python supprimes. Docker/QEMU. Smokes `make osui-smoke` + `make qemu-osui-runtime`. Docs `assist*` historiques.

**Hors perimetre reste.** Chromium, billing ASSIST-053, embed HTTP, UUID, LLM de production.

## Table recap

| Rang | Epic | Build ? | ASSIST |
|---:|---|---|---|
| 0 | OS-UI-000 | docs livres | 000 |
| 1 | OS-UI-0 | **livre** boot QEMU | 050 (boot) |
| 2 | OS-UI-1 | **livre** guest C | 010-013, 030, 031, 040, 041 |
| 3 | OS-UI-2 | **livre** simulateur | 020-022, 060, 061 |
| 3b | OS-UI-C | **livre** registre + VGA | pont Multiboot |
| 4 | OS-UI-3 | **livre** Python retire | fin 050 |
| - | Gardes guest 0-4 | parallele | (AOS, pas ASSIST) |
