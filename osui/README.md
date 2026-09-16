# Scaffold du shell graphique Mohhdy (OS-UI-0/1/2)

`osui/` est le **bootstrap graphique** du **meme** SE Multiboot Mohhdy :
chat central (prompts / slash), **scene IA** plein ecran (`#ai-stage`),
programmes (Browser-OS, Shell Multiboot, Admin, Support, Statut, FS).
`/shell` reprend le vocabulaire guest Ring 3 (`userspace/shell.c`) ;
bootstrap par defaut, hook live optionnel
([osui_shell_live.md](../docs/osui_shell_live.md)). Le backend HTTP reste le
scaffold temporaire `agent/` (parite ASSIST). Ce n'est **pas** US-031,
**pas** un LLM de production, **pas** un moteur Chromium de session.

Guide : [../docs/osui_0_1_2.md](../docs/osui_0_1_2.md).
Interaction : [../docs/osui_chat_desktop.md](../docs/osui_chat_desktop.md).
Scene IA : [../docs/osui_ai_stage.md](../docs/osui_ai_stage.md).
Live : [../docs/osui_shell_live.md](../docs/osui_shell_live.md).
Convergence : [../docs/osui_convergence.md](../docs/osui_convergence.md).
Plan : [../docs/PLAN_SE_MOHHDY_COMPLET.md](../docs/PLAN_SE_MOHHDY_COMPLET.md).

```text
python3 osui/server.py
make osui-smoke
make osui-shell-live-smoke
make osui-docker
docker build -t mohhdy-os -f osui/Dockerfile .
docker run --rm -p 8080:8080 mohhdy-os
docker run --rm -p 8080:8080 -e ADMIN_TOKEN=change-me-at-runtime mohhdy-os
```

Ouvrir `http://127.0.0.1:8080/` : bureau du SE. Sante : `GET /health`.
`agent/` n'est pas retire (OS-UI-3 plus tard).
