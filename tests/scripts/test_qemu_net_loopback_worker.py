#!/usr/bin/env python3
"""Build reseau strict sans NE2000 : networker sert seul les sockets en
boucle locale (registre Ring 3), sans carte et sans repli Ring 0.

Contrat (noyau par defaut build/mohhdy.bin, strict, aucune carte reseau) :
1. networker lance au boot meme sans carte : pile Ring 3 loopback-only ;
2. `netrelay` : boucle TCP locale (open/listen/poignee de main/send/feed/
   receive/close), les 10 ops socket executees par le worker ;
3. `ai-acquire` et `ai-peer-listen` relayes au worker, qui repond
   "NE2000 absent" depuis le Ring 3 (ops 91 et 128) ;
4. worker tue : socket et LLM -59, aucun repli Ring 0.
"""
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_net_tls_worker as tw  # noqa: E402
import test_qemu_net_no_ring0 as strict  # noqa: E402

tw.LOG = os.path.join(tw.LOG_DIR, "net-loopback-worker%s.log" % tw.RUN_SUFFIX)
tw.ERR = os.path.join(tw.LOG_DIR, "net-loopback-worker%s.err" % tw.RUN_SUFFIX)
tw.MON = os.path.join(tw.LOG_DIR, "net-loopback-worker%s-monitor.sock" % tw.RUN_SUFFIX)


def boot():
    os.makedirs(tw.LOG_DIR, exist_ok=True)
    for path in (tw.LOG, tw.ERR, tw.MON):
        try:
            os.remove(path)
        except OSError:
            pass
    command = [
        "qemu-system-i386", "-kernel", os.path.join(tw.ROOT, "build", "mohhdy.bin"),
        "-initrd", os.path.join(tw.ROOT, "my_initrd.tar"), "-cpu", "max", "-m", "1024M",
        "-display", "none", "-vga", "none", "-serial", "file:" + tw.LOG,
        "-monitor", "unix:%s,server,nowait" % tw.MON, "-machine", "type=pc,accel=tcg",
        "-net", "none", "-no-reboot", "-no-shutdown",
    ]
    with open(tw.ERR, "wb") as err:
        return subprocess.Popen(command, cwd=tw.ROOT, stdout=err, stderr=err)


def relay_ops(chunk):
    return set(int(op) for op in re.findall(r"net-driver relay op (\d+) rc -?\d+ reply 0", chunk))


def main():
    proc = client = None
    try:
        proc = boot()
        tw.wait_for(proc, "(-.-)")
        client = tw.monitor()
        log = tw.normalized_log(tw.text())
        if strict.BANNER not in log:
            raise RuntimeError("strict build banner absent (wrong kernel?)")
        if "[NET] boot networker spawned" not in log:
            raise RuntimeError("networker not spawned at boot without a NE2000")
        strict.yield_until(proc, client, "net-driver stack ring3 ready loopback-only (no NE2000)", 0)
        if "net-driver kernel socket stack absent, ring3 only" not in tw.normalized_log(tw.text()):
            raise RuntimeError("worker still reaches the kernel socket stack")
        worker = str(strict.relay_status(proc, client)[0])

        pid, start = strict.spawn(proc, client, "netrelay")
        strict.yield_until(proc, client, "netrelay forged reply refused", start)
        chunk = tw.normalized_log(tw.text()[start:])
        match = re.search(r"netrelay tcp loopback ok ping pong mode relay forwarded (\d+) "
                          r"completed (\d+)", chunk)
        if not match or match.group(1) != match.group(2) or int(match.group(1)) < 10:
            raise RuntimeError("loopback sockets not served by networker: %s" % chunk[-1500:])
        sockets = int(match.group(1))
        missing = set(range(99, 109)) - relay_ops(chunk)
        if missing:
            raise RuntimeError("socket ops not run by the worker: %r" % sorted(missing))
        strict.command(proc, client, "kill %s" % pid, "Processus %s termine" % pid)

        start = strict.command(proc, client, "ai-acquire example.com", "ai-acquire:")
        start2 = strict.command(proc, client, "ai-peer-listen", "ai-peer-listen:")
        chunk = tw.normalized_log(tw.text()[start:])
        if "ai-acquire: NE2000 absent" not in chunk:
            raise RuntimeError("ai-acquire not answered UNAVAILABLE: %s" % chunk[-800:])
        if "ai-peer-listen: NE2000 absent" not in chunk:
            raise RuntimeError("ai-peer-listen not answered UNAVAILABLE: %s" % chunk[-800:])
        missing = {91, 128} - relay_ops(chunk)
        if missing:
            raise RuntimeError("LLM/peer ops not relayed to the worker: %r" % sorted(missing))
        del start2
        log = tw.normalized_log(tw.text())
        if "[NET] ring0 fallback absent" in log[log.find("net-driver stack ring3 ready loopback-only"):]:
            raise RuntimeError("a network syscall fell to the strict gate while the worker was live")

        live = strict.relay_status(proc, client)
        if str(live[0]) != worker:
            raise RuntimeError("net-relay-status worker %r != spawned %s" % (live[0], worker))
        strict.command(proc, client, "kill %s" % worker, "Processus %s termine" % worker)
        mark = len(tw.text())
        pid, start = strict.spawn(proc, client, "netrelay")
        strict.yield_until(proc, client, "netrelay forged reply refused", start)
        after = tw.normalized_log(tw.text()[mark:])
        if "netrelay tcp loopback failed at open rc -59" not in after:
            raise RuntimeError("socket did not fail closed without worker: %s" % after[-1200:])
        if "net-driver relay op" in after:
            raise RuntimeError("relay op after the worker was killed")
        dead = strict.relay_status(proc, client)
        if dead[0] != 0 or dead[5] <= live[5]:
            raise RuntimeError("unexpected counters after kill: %r -> %r" % (live, dead))
        print("net loopback worker: %d socket ops + LLM/peer (91, 128) served by networker "
              "without NE2000; after kill denied %d -> %d" % (sockets, live[5], dead[5]))
        print("QEMU strict network loopback-only worker contract passed.")
        return 0
    finally:
        if client is not None:
            client.close()
        if proc is not None and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=4)
            except subprocess.TimeoutExpired:
                proc.kill()
        try:
            os.remove(tw.MON)
        except OSError:
            pass


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU strict network loopback-only worker contract failed: %s" % error)
        raise SystemExit(1)
