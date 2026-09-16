# OS-UI (retire)

Le chrome Python `osui/` (HTTP, HTML `#ai-stage`) et le scaffold `agent/`
sont **retires** (OS-UI-3).

La surface vit dans le guest Ring 3 Multiboot :

- `userspace/osui_runtime.c` : chat, scene VGA, sessions, droits, MCP, FS
- `userspace/shell.c` : vocabulaire `MOHHDY>`
- `shared/multiboot_shell_commands.json` : registre genere
- `userspace/mohhdy_osui_bridge.h` : contrat C (`PYTHON_FACADE 0`, `STAGE_VGA 1`)

Extracteur hote (pas un runtime produit) :

```text
python3 scripts/extract_guest_commands.py
python3 scripts/extract_guest_commands.py --check
```

Boot instance :

```text
make all && make run
make qemu-osui-runtime
docker build -t mohhdy-os .
docker run --rm -it mohhdy-os
```

Ce n'est **pas** US-031, **pas** un LLM de production, **pas** un bash Linux.
Le VGA n'heberge pas `#ai-stage` HTML : scene 8x48 structuree.

Guides : [../docs/osui_0_1_2.md](../docs/osui_0_1_2.md),
[../docs/osui_convergence.md](../docs/osui_convergence.md).
