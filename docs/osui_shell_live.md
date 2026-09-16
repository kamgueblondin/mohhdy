# OS-UI : instance = guest live

**Date :** 16 septembre 2026
**Statut :** plus d'attache hote Python. Docker boot QEMU. On est le guest
**Ponctuation :** ASCII usuel et accents francais uniquement

`/shell` et le prompt `MOHHDY>` **sont** `userspace/shell.c`. Il n'y a
plus d'interpreteur bootstrap Python (`live_guest=false` hote). Dans le
guest, `guest-status` publie `live_guest=true` et
`python_facade=false`. Ce n'est **pas** un bash Linux. Le guest
**n'heberge pas** `#ai-stage` (voir [ETAT_REEL.md](ETAT_REEL.md)).

## Boot

```text
make run
make run-nographic
docker run --rm -it mohhdy-os
```

Le harness de test (`make qemu-osui-runtime`, `make qemu-smoke`) injecte
des scancodes via **HMP sendkey**. La serie est le journal.

L'ancien hook `MOHHDY_SHELL_ATTACH=live` + `python3 osui/server.py` est
retire avec la facade.

## Pieges Linux

`apt`, `sudo`, `bash`, `docker`, `systemctl` sont refuses **avant** tout
chemin IA. Contrat QEMU : aiguille `apt` dans le serial log.

Guides : [osui_0_1_2.md](osui_0_1_2.md), [osui_convergence.md](osui_convergence.md).
