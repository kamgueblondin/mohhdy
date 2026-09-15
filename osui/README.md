# Scaffold du shell graphique Mohhdy (OS-UI-0/1/2)

`osui/` est l'**entree produit** de l'instance Docker : un chrome OS dont
la surface primaire est le **chat central** (prompts / slash). Les
programmes (Browser-OS, Shell OS, Admin, Support, Statut, FS) s'ouvrent
depuis le chat ; celui-ci passe alors en panneau flottant draggable.
Le backend HTTP reste le scaffold temporaire `agent/` (parite
ASSIST). Ce n'est **pas** US-031, **pas** un LLM de production, **pas**
un moteur Chromium de session.

Guide : [../docs/osui_0_1_2.md](../docs/osui_0_1_2.md).
Interaction : [../docs/osui_chat_desktop.md](../docs/osui_chat_desktop.md).
Plan : [../docs/PLAN_SE_MOHHDY_COMPLET.md](../docs/PLAN_SE_MOHHDY_COMPLET.md).

```text
python3 osui/server.py
make osui-smoke
make osui-docker
docker build -t mohhdy-os -f osui/Dockerfile .
docker run --rm -p 8080:8080 mohhdy-os
docker run --rm -p 8080:8080 -e ADMIN_TOKEN=change-me-at-runtime mohhdy-os
```

Ouvrir `http://127.0.0.1:8080/` : bureau du SE. Sante : `GET /health`.
`agent/` n'est pas retire (OS-UI-3 plus tard).
