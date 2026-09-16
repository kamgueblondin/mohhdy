# OS-UI (retire)

Le chrome Python `osui/` (HTTP, HTML `#ai-stage`) et le scaffold `agent/`
sont **retires** (OS-UI-3).

La surface vit dans le guest Ring 3 Multiboot :

- `userspace/osui_runtime.c` : chat, scene VGA, sessions, droits, MCP, FS
- `userspace/osui_gui.c` : bureau VGA 80x25 (commande canonique `gui`)
- `userspace/shell.c` : vocabulaire `MOHHDY>`
- `shared/multiboot_shell_commands.json` : registre genere
- `userspace/mohhdy_osui_bridge.h` : contrat C (`PYTHON_FACADE 0`, `VGA_DESKTOP 1`)

Extracteur hote (pas un runtime produit) :

```text
python3 scripts/extract_guest_commands.py
python3 scripts/extract_guest_commands.py --check
```

Boot instance :

```text
make all && make run-gui
make qemu-osui-runtime
make qemu-osui-gui
docker build -t mohhdy-os .
docker run --rm -it mohhdy-os
```

Au prompt `MOHHDY>` : `gui`. Nographic : `gui-status`. Ce n'est **pas**
US-031, **pas** un LLM de production, **pas** un bash Linux.
Le VGA n'heberge pas `#ai-stage` HTML : scene structuree + canvas desktop.

Guides : [../docs/osui_0_1_2.md](../docs/osui_0_1_2.md),
[../docs/osui_convergence.md](../docs/osui_convergence.md).
