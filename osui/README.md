# OS-UI : surface HTML hote + cerveau guest C

Le chrome produit est `osui/static` (HTML/CSS/JS glassmorphic). Le
**cerveau** reste `userspace/osui_runtime.c`. `osui/display_host.py` est
un helper mince : fichiers statiques + proxy serie vers QEMU.

Ce n'est **pas** le sidecar `agent/` (retire, OS-UI-3). Pas de sessions
HTTP, pas de MCP Python, pas de Prompt OS. `python_facade=false`.
`display_host=true`. `guest_html_stage=false` (le guest n'execute pas
HTML). `llm=stub_echo`. `us031_complete=false`.

```text
make all
make run-gui          # http://127.0.0.1:18080  (envoie gui tout seul)
make qemu-osui-gui    # contrat nographic, hors integration-qemu
make osui-smoke       # registre + facade absente + fumee HTML
```

Dans le guest, apres `MOHHDY>` : `gui` / `graphics` / `desktop`.
Quitter : `console` / ESC. Docker reste nographic par defaut.

Guides : [../docs/osui_0_1_2.md](../docs/osui_0_1_2.md),
[../docs/osui_chat_desktop.md](../docs/osui_chat_desktop.md).
