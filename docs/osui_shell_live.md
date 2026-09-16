# OS-UI : attache live du shell Multiboot

**Date :** 16 septembre 2026
**Statut :** chemin best-effort, honnete. Defaut = bootstrap
**Ponctuation :** ASCII usuel et accents francais uniquement

`/shell` parle le **meme vocabulaire** que `userspace/shell.c` (prompt
`MOHHDY>`). Par defaut l'interpreteur est **bootstrap osui**
(`live_guest=false`). Cette page decrit le **hook live** vers un guest
QEMU deja lance. Ce n'est **pas** un bash Linux. Le guest **n'heberge
pas** `#ai-stage` (voir [ETAT_REEL.md](ETAT_REEL.md)).

## Drapeaux

| Variable | Defaut | Role |
|---|---|---|
| `MOHHDY_SHELL_ATTACH` | `bootstrap` | `bootstrap` ou `live` |
| `MOHHDY_GUEST_SERIAL` | (vide) | `unix:/chemin.sock`, `tcp:127.0.0.1:PORT`, ou PTY |
| `MOHHDY_GUEST_SERIAL_KIND` | `auto` | `unix` / `tcp` / `pty` |
| `MOHHDY_GUEST_MONITOR` | (vide) | socket HMP QEMU (`unix:/chemin-mon.sock`) |
| `MOHHDY_GUEST_TRANSPORT` | `auto` | `serial`, `hmp`, ou les deux |
| `MOHHDY_GUEST_TIMEOUT` | `2.0` | secondes |
| `MOHHDY_GUEST_PROMPT` | `MOHHDY>` | handshake |

Sans serie ni moniteur : `attach` refuse, **reste bootstrap**,
`live_guest=false`. Pas de pretention.

Le guest Ring 3 lit le **clavier PS/2**. Le harness existant
(`make qemu-smoke`) injecte des scancodes via **HMP sendkey**. La serie
est surtout un journal (parfois un TTY nographic). L'attache live
reproduit ce contrat : HMP pour taper, serie pour lire si elle porte le
prompt.

## Exemple operateur

```text
qemu-system-i386 -kernel build/mohhdy.bin -initrd my_initrd.tar \
  -display none \
  -serial unix:/tmp/mohhdy-serial.sock,server,nowait \
  -monitor unix:/tmp/mohhdy-mon.sock,server,nowait \
  -m 256M -cpu pentium3

MOHHDY_SHELL_ATTACH=live \
MOHHDY_GUEST_SERIAL=unix:/tmp/mohhdy-serial.sock \
MOHHDY_GUEST_MONITOR=unix:/tmp/mohhdy-mon.sock \
python3 osui/server.py
```

Dans `/shell` : `guest-status`, `attach`, `detach`. Les pieges Linux
(`apt`, `sudo`, `bash`) sont refuses **avant** tout envoi au guest.

`POST /api/os/attach` `{"action":"attach"}`. `GET /api/os/attach`.
`GET /api/os` champ `attach` + `multiboot_shell.live_guest`.

## Preuves

```text
make osui-smoke
make osui-shell-live-smoke
```

`osui-smoke` reste court (faux guest serie dans les tests, pas de boot
QEMU). `osui-shell-live-smoke` prouve le meme chemin, puis **SKIP** si
`qemu-system-i386` ou le kernel manquent. Ne pas allonger
`make integration-qemu` ni `make ci`. `MOHHDY_LIVE_QEMU=1` n'est pas le
defaut.

Pas d'OpenAI. Pas de secret.
