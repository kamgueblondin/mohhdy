#!/usr/bin/env python3
"""Contrat MOHHDY Foundation : lecture VFS par médiateur Ring 3."""
import os
import re
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "vfs-service.log")
ERR = os.path.join(LOG_DIR, "vfs-service.err")
MON = os.path.join(LOG_DIR, "vfs-service-monitor.sock")
KERNEL = os.path.join(ROOT, "build", "mohhdy.bin")
INITRD = os.path.join(ROOT, "my_initrd.tar")
DISK = os.path.join(LOG_DIR, "vfs-service-overlay.img")
FAT32_DISK = os.path.join(LOG_DIR, "vfs-service-fat32.img")
# Chaque caractère reste confirmé par le shell avant la suite de la ligne. Le
# poll d’écho de 50 ms ne ralentit plus deux fois chaque caractère ordinaire :
# la commande suivante n’est envoyée qu’après réception confirmée de la
# précédente, même sous charge de deux QEMU.
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.05"))
KEY_HOLD_MS = int(os.environ.get("KEY_HOLD_MS", "10"))
KEY_ECHO_TIMEOUT = float(os.environ.get("KEY_ECHO_TIMEOUT", "3"))
# La ligne entière est réconciliée avant `ret`. Cette courte fenêtre absorbe
# les scan-codes tardifs sans imposer une attente longue à chaque caractère.
KEY_LINE_SETTLE_DELAY = float(os.environ.get("KEY_LINE_SETTLE_DELAY", "0.50"))
KEY_CHAR_RETRIES = int(os.environ.get("KEY_CHAR_RETRIES", "3"))
KEY_PRE_RET_RETRIES = int(os.environ.get("KEY_PRE_RET_RETRIES", "3"))


def log_text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def normalized_log(output):
    """Retire les diagnostics noyau asynchrones sans recoller les réponses."""
    # Un timer peut couper une réponse au milieu d’un mot ou juste avant une
    # ponctuation. Retirer toute sa ligne recolle ces fragments sans toucher
    # aux sauts de ligne qui appartiennent aux messages applicatifs.
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?\r?\n?", "", output)
    # L’ordonnanceur peut couper deux champs. S’il est joint à deux mots,
    # garder un séparateur ; s’il suit déjà un espace applicatif, le retirer
    # sans en ajouter un second qui casserait une assertion textuelle.
    scheduler = r"\[SCHED\] switching to task \d+\s*"
    output = re.sub(r"(?<=\w)" + scheduler + r"(?=\w)", " ", output)
    output = re.sub(scheduler, "", output)
    return re.sub(r"[ \t]{2,}", " ", output)

def wait_for(needle, proc, offset=0, timeout=25):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU s'est arrêté prématurément")
        # Les diagnostics timer asynchrones peuvent couper une ligne applicative
        # sans modifier le protocole VFS ; ils ne font pas partie du contrat testé.
        output = normalized_log(log_text()[offset:])
        if needle in output:
            return
        time.sleep(0.1)
    raise RuntimeError("sortie manquante : %s" % needle)


def connect_monitor():
    deadline = time.time() + 5
    while time.time() < deadline:
        if os.path.exists(MON):
            client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                client.connect(MON)
                client.settimeout(0.1)
                try:
                    client.recv(4096)
                except socket.timeout:
                    pass
                return client
            except OSError:
                client.close()
        time.sleep(0.1)
    raise RuntimeError("moniteur QEMU indisponible")


class CommandEchoMismatch(RuntimeError):
    """La ligne reçue par le shell diffère de la commande injectée."""


def send_key(client, key):
    client.sendall(("sendkey %s %d\n" % (key, KEY_HOLD_MS)).encode("ascii"))


def key_echoes(output):
    """Retourne la séquence exacte reçue par le buffer de la ligne courante."""
    pattern = r"SYS_GETS: caractère ajouté:\s*'(.)'"
    return re.findall(pattern, normalized_log(output))


def prepared_line_matches(output, command):
    return key_echoes(output) == [char.lower() for char in command]


def verify_pre_ret_reconciliation_parser():
    """Vérifie que la réconciliation refuse tout écho non exactement préparé."""
    command = "vfs-read overlay/note.txt"
    exact = "\n".join(
        "SYS_GETS: caractère ajouté: '%s'" % char
        for char in command
    )
    exact = exact.replace(
        "SYS_GETS: caractère ajouté: 'o'\nSYS_GETS: caractère ajouté: 'v'",
        "SYS_GETS: caractère ajouté: 'o'\nTIMER_ALIVE: tick=99+\n"
        "[SCHED] switching to task 3\nSYS_GETS: caractère ajouté: 'v'",
        1,
    )
    if not prepared_line_matches(exact, command):
        raise RuntimeError("sonde pre-ret: echos exacts refuses")
    if prepared_line_matches(exact + "\nSYS_GETS: caractère ajouté: 't'", command):
        raise RuntimeError("sonde pre-ret: doublon accepte")
    if prepared_line_matches(exact[:-32], command):
        raise RuntimeError("sonde pre-ret: ligne partielle acceptee")


def send_command_once(client, command, proc=None, key_delay=KEY_DELAY):
    """Prépare une ligne, sans jamais la soumettre au shell."""
    special = {" ": "spc", "-": "minus", ".": "dot", "/": "slash"}
    for char in command:
        received = False
        for _ in range(KEY_CHAR_RETRIES):
            start = len(log_text())
            send_key(client, special.get(char, char.lower()))
            deadline = time.time() + KEY_ECHO_TIMEOUT
            while time.time() < deadline:
                if proc is not None and proc.poll() is not None:
                    raise RuntimeError("QEMU s'est arrêté prématurément")
                time.sleep(key_delay)
                if char.lower() in key_echoes(log_text()[start:]):
                    received = True
                    break
            if received:
                break
        if not received:
            raise RuntimeError("caractère non reçu : %s" % char)


def clear_prepared_line(client, output, key_delay=KEY_DELAY):
    """Efface localement une ligne non soumise après un écho divergent."""
    for _ in key_echoes(output):
        send_key(client, "backspace")
        time.sleep(key_delay)
    time.sleep(KEY_LINE_SETTLE_DELAY)


def command_echoed(output, command):
    """Accepte les espaces redondants, mais aucune autre altération."""
    expected = " ".join(command.split())
    for received in re.findall(r"SYS_GETS: ligne lue: ([^\r\n]+)", normalized_log(output)):
        if " ".join(received.split()) == expected:
            return True
    return False


def send_command(client, command, proc=None, key_delay=KEY_DELAY):
    """Réconcilie la ligne avant `ret`, puis n’exécute jamais de rejeu."""
    if proc is None:
        send_command_once(client, command, None, key_delay)
        return
    for attempt in range(KEY_PRE_RET_RETRIES):
        start = len(log_text())
        send_command_once(client, command, proc, key_delay)
        time.sleep(KEY_LINE_SETTLE_DELAY)
        prepared = normalized_log(log_text()[start:])
        if prepared_line_matches(prepared, command):
            send_key(client, "ret")
            deadline = time.time() + 25
            while time.time() < deadline:
                if proc.poll() is not None:
                    raise RuntimeError("QEMU s'est arrêté prématurément")
                output = normalized_log(log_text()[start:])
                if command_echoed(output, command):
                    return
                if "SYS_GETS: ligne lue: " in output:
                    actuals = re.findall(r"SYS_GETS: ligne lue: ([^\r\n]+)", output)
                    actual_str = actuals[-1] if actuals else "inconnu"
                    if attempt + 1 < KEY_PRE_RET_RETRIES:
                        try:
                            wait_for("(-.-)", proc, start, timeout=10)
                        except RuntimeError:
                            pass
                        break
                    raise CommandEchoMismatch("echo commande altéré : %s (reçu: %r)" % (command, actual_str))
                time.sleep(0.1)
            else:
                raise RuntimeError("echo commande absent : %s" % command)
        else:
            clear_prepared_line(client, prepared, key_delay)
    raise CommandEchoMismatch("echo pre-ret instable : %s" % command)


def wait_for_mount_add_worker(proc, alias, source, offset=0, timeout=15, terminal=None):
    """Prouve l’exécution worker malgré les traces asynchrones intercalées.

    Le worker, le médiateur et le shell écrivent sur la même sortie série et
    leur ordre de trace est coopérativement variable. La fenêtre commence à
    la requête unique et exige donc les trois preuves : worker, délégation et
    résultat corrélé avec l’alias et la source attendus.
    """
    if terminal is None:
        terminal = r"vfsserver mount added " + re.escape(alias) + r" " + re.escape(source)
    pattern = (r"(?=[\s\S]{0,800}?vfsvirtual mount add)"
               r"(?=[\s\S]{0,800}?vfsserver delegated mount add)"
               r"(?=[\s\S]{0,800}?" + terminal + r")")
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU s'est arrêté prématurément")
        if re.search(pattern, normalized_log(log_text()[offset:])):
            return
        time.sleep(0.1)
    raise RuntimeError("montage worker manquant : %s" % alias)


def wait_for_listed_name(client, proc, directory, name, first_page_timeout=15,
                         list_needle="vfsserver list request",
                         page_needle="vfsserver list page request"):
    """Parcourt les pages VFS de 4 noms jusqu'a trouver `name`."""
    start = len(log_text())
    send_command_until(client, "vfs-list %s" % directory, list_needle, proc)
    wait_for("vfs-list ", proc, start, timeout=first_page_timeout)
    if name in normalized_log(log_text()[start:]):
        return
    page = 0
    for _ in range(8):
        page += 4
        before = len(log_text())
        send_command_until(client, "vfs-list-page %s %d" % (directory, page),
                           page_needle, proc)
        wait_for("vfs-list-page ", proc, before)
        chunk = normalized_log(log_text()[before:])
        if name in chunk:
            return
        if "next end" in chunk:
            break
    raise RuntimeError("nom absent de %s : %s" % (directory, name))


def send_command_until(client, command, needle, proc, attempts=1, key_delay=KEY_DELAY):
    """Exécute une commande une fois, sauf réessai explicitement demandé."""
    error = None
    for _ in range(attempts):
        start = len(log_text())
        try:
            send_command(client, command, proc, key_delay)
            wait_for(needle, proc, start)
            # La réponse VFS peut précéder le retour du shell pendant une
            # rafale IRQ0. Synchroniser le prompt évite de concaténer la
            # commande suivante, sans jamais réinjecter une mutation VFS.
            wait_for("(-.-)", proc, start)
            return
        except CommandEchoMismatch:
            raise
        except RuntimeError as caught:
            error = caught
            time.sleep(0.2)
    raise error


def wait_for_cooperative_client(client, proc, needle, start, rounds=4):
    """Accorde au client nouvellement créé des tours bornés avant son I/O.

    Seuls des `yield` shell sont injectés ici et le client n’a encore reçu
    aucun grant ; aucune mutation ni I/O VFS ne peut donc être rejouée.
    """
    error = None
    for _ in range(rounds):
        try:
            wait_for(needle, proc, start, timeout=1)
            return
        except RuntimeError as caught:
            error = caught
        send_command_until(client, "yield", "yield ok", proc)
    try:
        wait_for(needle, proc, start, timeout=2)
        return
    except RuntimeError as caught:
        raise error or caught


def wait_for_single_io_delivery(client, proc, needle, start, rounds=4):
    """Laisse progresser une unique I/O déjà envoyée sans jamais la rejouer."""
    error = None
    for _ in range(rounds):
        try:
            wait_for(needle, proc, start, timeout=1)
            return
        except RuntimeError as caught:
            error = caught
        send_command_until(client, "yield", "yield ok", proc)
    try:
        wait_for(needle, proc, start, timeout=2)
        return
    except RuntimeError as caught:
        raise error or caught


def assert_lfn_lifecycle(client, proc, mount, name, renamed, payload):
    """Valide les mutations LFN racine via le VFS, sans écrasement implicite."""
    path = "%s/%s" % (mount, name)
    renamed_path = "%s/%s" % (mount, renamed)
    before_write = len(log_text())
    send_command_until(client, "vfs-write %s %s" % (path, payload),
                       "vfsserver write request", proc)
    wait_for("vfs-write ok request", proc, before_write)
    wait_for("vfsserver delegated write", proc, before_write)
    wait_for("vfsvirtual write %s" % path, proc, before_write)
    before_collision = len(log_text())
    send_command_until(client, "vfs-write %s collision" % path,
                       "vfsserver write request", proc)
    wait_for("vfs-write: ecriture refusee", proc, before_collision)
    before_read = len(log_text())
    send_command_until(client, "vfs-read %s" % path, "vfs-read ok", proc)
    wait_for(payload, proc, before_read)
    before_rename = len(log_text())
    send_command_until(client, "vfs-rename %s %s" % (path, renamed_path),
                       "vfsserver rename request", proc)
    wait_for("vfs-rename ok request", proc, before_rename)
    wait_for("vfsserver delegated rename", proc, before_rename)
    wait_for("vfsvirtual rename %s -> %s" % (path, renamed_path), proc, before_rename)
    before_old_read = len(log_text())
    send_command_until(client, "vfs-read %s" % path, "vfsserver read request", proc)
    wait_for("vfs-read: lecture refusee ou fichier absent", proc, before_old_read)
    before_renamed_read = len(log_text())
    send_command_until(client, "vfs-read %s" % renamed_path, "vfs-read ok", proc)
    wait_for(payload, proc, before_renamed_read)
    before_remove = len(log_text())
    send_command_until(client, "vfs-remove %s" % renamed_path,
                       "vfsserver remove request", proc)
    wait_for("vfs-remove ok request", proc, before_remove)
    wait_for("vfsserver delegated remove", proc, before_remove)
    wait_for("vfsvirtual remove %s" % renamed_path, proc, before_remove)
    before_removed_read = len(log_text())
    send_command_until(client, "vfs-read %s" % renamed_path,
                       "vfsserver read request", proc)
    wait_for("vfs-read: lecture refusee ou fichier absent", proc, before_removed_read)


def assert_fat_subdirectory_lifecycle(client, proc, mount, payload):
    """Valide un niveau FAT 8.3 sans écrasement ni rejeu local ambigu."""
    directory = "%s/qdir" % mount
    path = "%s/child.txt" % directory
    renamed = "%s/renamed.txt" % directory
    before_mkdir = len(log_text())
    send_command_until(client, "vfs-mkdir %s" % directory, "vfsserver mkdir request", proc)
    wait_for("vfs-mkdir ok request", proc, before_mkdir)
    wait_for("vfsserver delegated mkdir", proc, before_mkdir)
    wait_for("vfsvirtual mkdir %s" % directory, proc, before_mkdir)
    before_duplicate = len(log_text())
    send_command_until(client, "vfs-mkdir %s" % directory, "vfsserver mkdir request", proc)
    wait_for("vfs-mkdir: creation refusee", proc, before_duplicate)
    before_write = len(log_text())
    send_command_until(client, "vfs-write %s %s" % (path, payload),
                       "vfsserver write request", proc)
    wait_for("vfs-write ok request", proc, before_write)
    wait_for("vfsvirtual write %s" % path, proc, before_write)
    before_stat = len(log_text())
    send_command_until(client, "vfs-stat %s" % path,
                       "vfs-stat ok size %d flags file" % len(payload), proc)
    wait_for("vfs-stat ok size %d flags file" % len(payload), proc, before_stat)
    before_collision = len(log_text())
    send_command_until(client, "vfs-write %s collision" % path,
                       "vfsserver write request", proc)
    wait_for("vfs-write: ecriture refusee", proc, before_collision)
    before_list = len(log_text())
    send_command_until(client, "vfs-list %s/" % directory, "vfsserver list request", proc)
    wait_for("vfs-list ok count 1", proc, before_list)
    wait_for("CHILD.TXT", proc, before_list)
    before_read = len(log_text())
    send_command_until(client, "vfs-read %s" % path, "vfs-read ok", proc)
    wait_for(payload, proc, before_read)
    before_rename = len(log_text())
    send_command_until(client, "vfs-rename %s %s" % (path, renamed),
                       "vfsserver rename request", proc)
    wait_for("vfs-rename ok request", proc, before_rename)
    wait_for("vfsvirtual rename %s -> %s" % (path, renamed), proc, before_rename)
    escaped = "%s/escaped.txt" % mount
    before_escape = len(log_text())
    send_command_until(client, "vfs-rename %s %s" % (renamed, escaped),
                       "vfsserver rename request", proc)
    wait_for("vfsvirtual rename %s -> %s" % (renamed, escaped), proc, before_escape)
    wait_for("vfsserver backend prefix denied", proc, before_escape)
    wait_for("vfs-rename: chemins hors montage ecriture", proc, before_escape)
    before_source_intact = len(log_text())
    send_command_until(client, "vfs-read %s" % renamed, "vfs-read ok", proc)
    wait_for(payload, proc, before_source_intact)
    before_escape_absent = len(log_text())
    send_command_until(client, "vfs-read %s" % escaped, "vfsserver read request", proc)
    wait_for("vfs-read: lecture refusee ou fichier absent", proc, before_escape_absent)
    before_rmdir_refused = len(log_text())
    send_command_until(client, "vfs-rmdir %s" % directory, "vfsserver rmdir request", proc)
    wait_for("vfs-rmdir: suppression refusee", proc, before_rmdir_refused)
    before_remove = len(log_text())
    send_command_until(client, "vfs-remove %s" % renamed, "vfsserver remove request", proc)
    wait_for("vfs-remove ok request", proc, before_remove)
    wait_for("vfsvirtual remove %s" % renamed, proc, before_remove)
    before_rmdir = len(log_text())
    send_command_until(client, "vfs-rmdir %s" % directory, "vfsserver rmdir request", proc)
    wait_for("vfs-rmdir ok request", proc, before_rmdir)
    wait_for("vfsserver delegated rmdir", proc, before_rmdir)
    wait_for("vfsvirtual rmdir %s" % directory, proc, before_rmdir)


def main():
    verify_pre_ret_reconciliation_parser()
    if os.environ.get("VFS_INPUT_PROBE_ONLY") == "1":
        return 0
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, ERR, MON, DISK, FAT32_DISK):
        try:
            os.remove(path)
        except OSError:
            pass
    subprocess.check_call([
        sys.executable,
        os.path.join(ROOT, "tests", "scripts", "make_fat16_image.py"),
        "--image", DISK,
    ])
    subprocess.check_call([
        sys.executable,
        os.path.join(ROOT, "scripts", "make_fat32_secondary_image.py"),
        "--image", FAT32_DISK,
    ])
    command = [
        "qemu-system-i386", "-cpu", "pentium3", "-kernel", KERNEL, "-initrd", INITRD,
        "-m", "1024M", "-display", "none", "-vga", "none",
        "-serial", "file:" + LOG,
        "-monitor", "unix:%s,server,nowait" % MON,
        "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
        "-drive", "file=%s,format=raw,if=ide,index=0,cache=writethrough" % DISK,
        "-drive", "file=%s,format=raw,if=ide,index=1,cache=writethrough" % FAT32_DISK,
    ]
    with open(ERR, "wb") as err_handle:
        proc = subprocess.Popen(command, stdout=err_handle, stderr=err_handle)
        monitor = None
        try:
            wait_for("(-.-)", proc)
            monitor = connect_monitor()
            time.sleep(0.5)
            before_probe = len(log_text())
            send_command_until(monitor, "vfs-backend-probe hello.txt",
                               "vfs-backend-probe denied", proc)
            before_worker_spawn = len(log_text())
            send_command_until(monitor, "spawn vfsvirtual", "spawn ok pid", proc)
            worker_spawned = re.search(r"spawn ok pid[\s\S]*?(\d+) vfsvirtual", log_text()[before_worker_spawn:])
            if not worker_spawned:
                raise RuntimeError("worker virtuel VFS non lance")
            worker_pid = worker_spawned.group(1)
            send_command(monitor, "yield", proc)
            wait_for("vfsvirtual ready", proc, before_worker_spawn)
            send_command_until(monitor, "service-find vfs-virtual",
                               "service-find ok vfs-virtual %s" % worker_pid, proc)
            before_spawn = len(log_text())
            send_command_until(monitor, "spawn vfsserver", "spawn ok pid", proc)
            spawned = re.search(r"spawn ok pid[\s\S]*?(\d+) vfsserver", log_text()[before_spawn:])
            if not spawned:
                raise RuntimeError("serveur VFS non lance")
            server_pid = spawned.group(1)
            send_command(monitor, "yield", proc)
            wait_for("vfsserver ready vfs", proc, before_spawn)
            wait_for("vfsserver mount initrd/", proc, before_spawn)
            wait_for("vfsserver mount overlay/ rw", proc, before_spawn)
            before_cap_claim = len(log_text())
            send_command_until(monitor, "spawn vfscapclaim", "spawn ok pid", proc)
            cap_claimed = re.search(r"spawn ok pid[\s\S]*?(\d+) vfscapclaim", normalized_log(log_text()[before_cap_claim:]))
            if not cap_claimed:
                raise RuntimeError("beneficiaire de capacite VFS non lance")
            cap_claim_pid = cap_claimed.group(1)
            wait_for_cooperative_client(monitor, proc, "vfscapclaim waiting backend", before_cap_claim)
            before_cap_grant = len(log_text())
            send_command_until(monitor, "vfs-backend-grant %s" % cap_claim_pid,
                               "vfsserver backend grant request", proc)
            wait_for("vfs-backend-grant ok request", proc, before_cap_grant)
            wait_for("vfscapclaim backend granted", proc, before_cap_grant)
            before_full_status = len(log_text())
            send_command_until(monitor, "vfs-backend-status %s" % cap_claim_pid,
                               "vfsserver backend status request", proc)
            wait_for("vfs-backend-status ok rights full", proc, before_full_status)
            before_full_list = len(log_text())
            send_command_until(monitor, "vfs-backend-list", "vfsserver backend list request", proc)
            wait_for("vfs-backend-list ok count 1", proc, before_full_list)
            wait_for("vfs-backend-list pid %s rights full" % cap_claim_pid, proc, before_full_list)
            before_observe_current = len(log_text())
            send_command_until(monitor, "vfs-backend-observe 0", "vfsserver backend observe request", proc)
            wait_for("vfs-backend-observe ok generation 2 count 1", proc, before_observe_current)
            before_owner_after_cap = len(log_text())
            send_command_until(monitor, "service-find vfs", "service-find ok vfs %s" % server_pid, proc)
            before_cap_revoke = len(log_text())
            send_command_until(monitor, "vfs-backend-revoke %s" % cap_claim_pid,
                               "vfsserver backend revoke request", proc)
            wait_for("vfs-backend-revoke ok request", proc, before_cap_revoke)
            wait_for("vfscapclaim backend revoked", proc, before_cap_revoke)
            before_absent_status = len(log_text())
            send_command_until(monitor, "vfs-backend-status %s" % cap_claim_pid,
                               "vfsserver backend status request", proc)
            wait_for("vfs-backend-status: capacite absente ou refusee", proc, before_absent_status)
            before_empty_list = len(log_text())
            send_command_until(monitor, "vfs-backend-list", "vfsserver backend list request", proc)
            wait_for("vfs-backend-list ok count 0", proc, before_empty_list)
            before_observe_stale = len(log_text())
            send_command_until(monitor, "vfs-backend-observe 2", "vfsserver backend observe request", proc)
            wait_for("vfs-backend-observe stale generation 3", proc, before_observe_stale)
            send_command_until(monitor, "kill %s" % cap_claim_pid,
                               "Processus %s termine" % cap_claim_pid, proc)
            send_command_until(monitor, "service-find vfs", "service-find ok vfs %s" % server_pid, proc)
            before_release_claim = len(log_text())
            send_command_until(monitor, "spawn vfsreleaseclaim", "spawn ok pid", proc)
            release_claimed = re.search(r"spawn ok pid[\s\S]*?(\d+) vfsreleaseclaim", normalized_log(log_text()[before_release_claim:]))
            if not release_claimed:
                raise RuntimeError("beneficiaire de liberation autonome non lance")
            release_claim_pid = release_claimed.group(1)
            wait_for_cooperative_client(monitor, proc, "vfsreleaseclaim waiting backend", before_release_claim)
            before_release_grant = len(log_text())
            send_command_until(monitor, "vfs-backend-grant %s" % release_claim_pid,
                               "vfsserver backend grant request", proc)
            wait_for("vfs-backend-grant ok request", proc, before_release_grant)
            wait_for("vfsreleaseclaim backend granted", proc, before_release_grant)
            wait_for("vfsreleaseclaim backend self-released", proc, before_release_grant)
            before_release_status = len(log_text())
            send_command_until(monitor, "vfs-backend-status %s" % release_claim_pid,
                               "vfsserver backend status request", proc)
            wait_for("vfs-backend-status: capacite absente ou refusee", proc, before_release_status)
            # Le bénéficiaire reste coopératif après sa libération. Le terminer ici
            # ajouterait un événement enfant au endpoint shell déjà sollicité par le
            # scénario IPC différé qui suit et masquerait la réponse VFS concernée.
            before_read_claim = len(log_text())
            send_command_until(monitor, "spawn vfsreadclaim", "spawn ok pid", proc)
            read_claimed = re.search(r"spawn ok pid[\s\S]*?(\d+) vfsreadclaim", normalized_log(log_text()[before_read_claim:]))
            if not read_claimed:
                raise RuntimeError("beneficiaire lecture seule VFS non lance")
            read_claim_pid = read_claimed.group(1)
            wait_for_cooperative_client(monitor, proc, "vfsreadclaim waiting read", before_read_claim)
            before_read_grant = len(log_text())
            send_command_until(monitor, "vfs-backend-grant-read %s" % read_claim_pid,
                               "vfsserver backend scoped grant request", proc)
            wait_for("vfs-backend-grant-read ok request", proc, before_read_grant)
            wait_for("vfsreadclaim read-only enforced", proc, before_read_grant)
            before_read_status = len(log_text())
            send_command_until(monitor, "vfs-backend-status %s" % read_claim_pid,
                               "vfsserver backend status request", proc)
            wait_for("vfs-backend-status ok rights read", proc, before_read_status)
            before_read_list = len(log_text())
            send_command_until(monitor, "vfs-backend-list", "vfsserver backend list request", proc)
            wait_for("vfs-backend-list ok count 1", proc, before_read_list)
            wait_for("vfs-backend-list pid %s rights read" % read_claim_pid, proc, before_read_list)
            send_command_until(monitor, "kill %s" % read_claim_pid,
                               "Processus %s termine" % read_claim_pid, proc)
            send_command_until(monitor, "service-find vfs", "service-find ok vfs %s" % server_pid, proc)
            before_mutate_claim = len(log_text())
            send_command_until(monitor, "spawn vfsmutateclaim", "spawn ok pid", proc)
            mutate_claimed = re.search(r"spawn ok pid[\s\S]*?(\d+) vfsmutateclaim", normalized_log(log_text()[before_mutate_claim:]))
            if not mutate_claimed:
                raise RuntimeError("beneficiaire mutation seule VFS non lance")
            mutate_claim_pid = mutate_claimed.group(1)
            wait_for_cooperative_client(monitor, proc, "vfsmutateclaim waiting mutate", before_mutate_claim)
            before_mutate_grant = len(log_text())
            send_command_until(monitor, "vfs-backend-grant-mutate %s" % mutate_claim_pid,
                               "vfsserver backend scoped grant request", proc)
            wait_for("vfs-backend-grant-mutate ok request", proc, before_mutate_grant)
            wait_for("vfsmutateclaim mutate-only enforced", proc, before_mutate_grant)
            before_mutate_status = len(log_text())
            send_command_until(monitor, "vfs-backend-status %s" % mutate_claim_pid,
                               "vfsserver backend status request", proc)
            wait_for("vfs-backend-status ok rights mutate", proc, before_mutate_status)
            before_mutate_list = len(log_text())
            send_command_until(monitor, "vfs-backend-list", "vfsserver backend list request", proc)
            wait_for("vfs-backend-list ok count 1", proc, before_mutate_list)
            wait_for("vfs-backend-list pid %s rights mutate" % mutate_claim_pid, proc, before_mutate_list)
            send_command_until(monitor, "kill %s" % mutate_claim_pid,
                               "Processus %s termine" % mutate_claim_pid, proc)
            send_command_until(monitor, "service-find vfs", "service-find ok vfs %s" % server_pid, proc)
            before_initrd_list = len(log_text())
            send_command_until(monitor, "vfs-list initrd/", "vfsserver list request", proc)
            wait_for("vfs-list partiel count 4", proc, before_initrd_list)
            wait_for_listed_name(monitor, proc, "initrd/", "hello.txt")
            before_page_zero = len(log_text())
            send_command_until(monitor, "vfs-list-page initrd/ 0", "vfsserver list page request", proc)
            wait_for("vfs-list-page partiel count 4 next 4", proc, before_page_zero)
            before_page_last = len(log_text())
            send_command_until(monitor, "vfs-list-page initrd/ 4", "vfsserver list page request", proc)
            wait_for("vfs-list-page ok count 4 next end", proc, before_page_last)
            before_observe = len(log_text())
            send_command_until(monitor, "vfs-list-observe initrd/ 0 0", "vfsserver list observe request", proc)
            wait_for("vfs-list-observe partiel count 4 next 4 generation 1", proc, before_observe)
            wait_for_listed_name(monitor, proc, "initrd/bin/", "shell")
            before_overlay_empty_list = len(log_text())
            send_command_until(monitor, "vfs-list overlay/", "vfsserver list request", proc)
            wait_for("vfs-list ok count 0", proc, before_overlay_empty_list)
            before_mkdir = len(log_text())
            send_command_until(monitor, "vfs-mkdir overlay/newdir", "vfsserver mkdir request", proc)
            wait_for("vfs-mkdir ok request", proc, before_mkdir)
            before_overlay_dir_list = len(log_text())
            send_command_until(monitor, "vfs-list overlay/", "vfsserver list request", proc)
            wait_for("newdir", proc, before_overlay_dir_list)
            before_rmdir = len(log_text())
            send_command_until(monitor, "vfs-rmdir overlay/newdir", "vfs-rmdir ok request", proc)
            before_overlay_after_rmdir = len(log_text())
            send_command_until(monitor, "vfs-list overlay/", "vfsserver list request", proc)
            wait_for("vfs-list ok count 0", proc, before_overlay_after_rmdir)
            before_missing_list = len(log_text())
            send_command_until(monitor, "vfs-list missing/", "vfsserver list outside mounts", proc)
            wait_for("vfs-list: repertoire hors montage", proc, before_missing_list)
            before_initial_stats = len(log_text())
            send_command_until(monitor, "vfs-stats", "vfsserver delegated vfs-stats", proc)
            wait_for("vfsvirtual format stats", proc, before_initial_stats)
            wait_for("reads=1", proc, before_initial_stats)
            wait_for("writes=0", proc, before_initial_stats)
            before_worker_initial_health = len(log_text())
            send_command_until(monitor, "vfs-read vfs-worker", "vfsserver virtual vfs-worker local", proc)
            wait_for("vfsvirtual ready pid=%s recoveries=0" % worker_pid, proc,
                     before_worker_initial_health)
            wait_for("removes=0", proc, before_initial_stats)
            wait_for("renames=0", proc, before_initial_stats)
            before_direct_write = len(log_text())
            send_command_until(monitor, "vfs-backend-write-probe note.txt denied",
                               "vfs-backend-write-probe denied", proc)
            send_command_until(monitor, "vfs-backend-remove-probe note.txt",
                               "vfs-backend-remove-probe denied", proc)
            before_direct_rename = len(log_text())
            send_command_until(monitor, "vfs-backend-rename-probe note.txt moved.txt",
                               "vfs-backend-rename-probe denied", proc)
            before_add_assets = len(log_text())
            send_command_until(monitor, "vfs-mount-add assets/ initrd",
                               "vfsserver mount added assets/ initrd", proc)
            wait_for("vfsserver delegated mount add", proc, before_add_assets)
            wait_for_mount_add_worker(proc, "assets/", "initrd", before_add_assets)
            wait_for("vfs-mount-add ok request", proc, before_add_assets)
            before_add_work = len(log_text())
            send_command_until(monitor, "vfs-mount-add work/ overlay",
                               "vfsserver mount added work/ overlay", proc)
            wait_for("vfsserver delegated mount add", proc, before_add_work)
            wait_for_mount_add_worker(proc, "work/", "overlay", before_add_work)
            wait_for("vfs-mount-add ok request", proc, before_add_work)
            before_add_fat32 = len(log_text())
            send_command_until(monitor, "vfs-mount-add media32/ fat32",
                               "vfsserver mount added media32/ fat32", proc)
            wait_for("vfs-mount-add ok request", proc, before_add_fat32)
            before_add_fat16 = len(log_text())
            send_command_until(monitor, "vfs-mount-add media/ fat16",
                               "vfsserver mount added media/ fat16", proc)
            wait_for("vfs-mount-add ok request", proc, before_add_fat16)
            before_full = len(log_text())
            send_command_until(monitor, "vfs-mount-add full/ overlay",
                               "vfsserver mount add rc -62", proc)
            wait_for("vfsserver delegated mount add", proc, before_full)
            wait_for_mount_add_worker(proc, "full/", "overlay", before_full,
                                       terminal=r"vfsserver mount add rc -62")
            wait_for("vfs-mount-add: table de montages pleine", proc, before_full)
            before_protected = len(log_text())
            send_command_until(monitor, "vfs-mount-remove initrd/",
                               "vfsserver mount remove rc -60", proc)
            wait_for("vfs-mount-remove: montage protege ou refuse", proc, before_protected)
            before_mounts = len(log_text())
            send_command_until(monitor, "vfs-read vfs-mounts", "vfsserver delegated vfs-mounts", proc)
            wait_for("vfs-read ok", proc, before_mounts)
            wait_for("initrd/", proc, before_mounts)
            wait_for("assets/ ro", proc, before_mounts)
            wait_for("work/ rw", proc, before_mounts)
            wait_for("media32/ ro", proc, before_mounts)
            before_mount_page_zero = len(log_text())
            send_command_until(monitor, "vfs-list-page vfs-mounts 0", "vfsserver list page request", proc)
            wait_for("vfsserver delegated mount page", proc, before_mount_page_zero)
            wait_for("vfs-list-page partiel count 4 next 4", proc, before_mount_page_zero)
            wait_for("initrd/ ro", proc, before_mount_page_zero)
            if log_text()[before_mount_page_zero:].count("vfsvirtual format mount") < 4:
                time.sleep(0.3)
            if log_text()[before_mount_page_zero:].count("vfsvirtual format mount") < 4:
                raise RuntimeError("formatage worker incomplet de la premiere page de montages")
            before_mount_page_last = len(log_text())
            send_command_until(monitor, "vfs-list-page vfs-mounts 4", "vfsserver list page request", proc)
            wait_for("vfsserver delegated mount page", proc, before_mount_page_last)
            wait_for("vfs-list-page ok count 4 next end", proc, before_mount_page_last)
            if log_text()[before_mount_page_last:].count("vfsvirtual format mount") < 4:
                time.sleep(0.3)
            if log_text()[before_mount_page_last:].count("vfsvirtual format mount") < 4:
                raise RuntimeError("formatage worker incomplet de la derniere page de montages")
            wait_for("media32/ ro", proc, before_mount_page_last)
            wait_for("media/ ro", proc, before_mount_page_last)
            before_mount_observe = len(log_text())
            send_command_until(monitor, "vfs-list-observe vfs-mounts 0 0", "vfsserver list observe request", proc)
            wait_for("vfsserver delegated mount observe", proc, before_mount_observe)
            wait_for("vfs-list-observe partiel count 4 next 4 generation", proc, before_mount_observe)
            wait_for("initrd/ ro", proc, before_mount_observe)
            if log_text()[before_mount_observe:].count("vfsvirtual format mount") < 4:
                time.sleep(0.3)
            if log_text()[before_mount_observe:].count("vfsvirtual format mount") < 4:
                raise RuntimeError("formatage worker incomplet de la page observee de montages")
            before_mount_stale = len(log_text())
            send_command_until(monitor, "vfs-list-observe vfs-mounts 0 1", "vfsserver list observe request", proc)
            wait_for("vfs-list-observe obsolete generation", proc, before_mount_stale)
            before_alias_read = len(log_text())
            send_command_until(monitor, "vfs-read assets/hello.txt", "vfs-read ok", proc)
            wait_for("Un autre fichier de demonstration.", proc, before_alias_read)
            wait_for("vfsserver delegated alias read", proc, before_alias_read)
            wait_for("vfsvirtual alias read assets/hello.txt", proc, before_alias_read)
            before_alias_stat = len(log_text())
            send_command_until(monitor, "vfs-stat assets/hello.txt",
                               "vfsserver delegated alias stat", proc)
            wait_for("vfsvirtual alias stat assets/hello.txt", proc, before_alias_stat)
            wait_for("vfs-stat ok size 35 flags file", proc, before_alias_stat)
            before_alias_list = len(log_text())
            send_command_until(monitor, "vfs-list assets/", "vfsserver delegated alias list", proc)
            wait_for("vfsvirtual alias list assets/", proc, before_alias_list)
            wait_for("vfs-list partiel count 4", proc, before_alias_list)
            wait_for_listed_name(
                monitor, proc, "assets/", "hello.txt",
                list_needle="vfsserver delegated alias list",
                page_needle="vfsserver delegated alias list page",
            )
            before_alias_page = len(log_text())
            send_command_until(monitor, "vfs-list-page assets/ 0",
                               "vfsserver delegated alias list page", proc)
            wait_for("vfsvirtual alias list page assets/", proc, before_alias_page)
            wait_for("vfs-list-page partiel count 4 next 4", proc, before_alias_page)
            before_alias_page_end = len(log_text())
            send_command_until(monitor, "vfs-list-page assets/ 4",
                               "vfsserver delegated alias list page", proc)
            wait_for("vfsvirtual alias list page assets/", proc, before_alias_page_end)
            wait_for("vfs-list-page ok count 4 next end", proc, before_alias_page_end)
            before_alias_observe = len(log_text())
            send_command_until(monitor, "vfs-list-observe assets/ 0 0",
                               "vfsserver delegated alias list observe", proc)
            wait_for("vfsvirtual alias list page assets/", proc, before_alias_observe)
            wait_for("vfs-list-observe partiel count 4 next 4 generation", proc, before_alias_observe)
            before_alias_observe_end = len(log_text())
            send_command_until(monitor, "vfs-list-observe assets/ 4 0",
                               "vfsserver delegated alias list observe", proc)
            wait_for("vfsvirtual alias list page assets/", proc, before_alias_observe_end)
            wait_for("vfs-list-observe ok count 4 next end generation", proc, before_alias_observe_end)
            before_alias_read_revoked = len(log_text())
            send_command_until(monitor, "vfs-backend-status %s" % worker_pid,
                               "vfsserver backend status request", proc)
            wait_for("vfs-backend-status: capacite absente ou refusee", proc, before_alias_read_revoked)
            before_alias_write = len(log_text())
            send_command_until(monitor, "vfs-write work/alias.txt workok",
                               "vfs-write ok request", proc)
            before_alias_list = len(log_text())
            send_command_until(monitor, "vfs-list work/", "vfsserver list request", proc)
            wait_for("vfs-list ok count 1", proc, before_alias_list)
            wait_for("alias.txt", proc, before_alias_list)
            before_alias_written_read = len(log_text())
            send_command_until(monitor, "vfs-read work/alias.txt", "vfs-read ok", proc)
            wait_for("workok", proc, before_alias_written_read)
            before_alias_rename = len(log_text())
            send_command_until(monitor, "vfs-rename work/alias.txt work/renamed.txt",
                               "vfs-rename ok request", proc)
            before_alias_renamed_read = len(log_text())
            send_command_until(monitor, "vfs-read work/renamed.txt", "vfs-read ok", proc)
            wait_for("workok", proc, before_alias_renamed_read)
            before_alias_file_remove = len(log_text())
            send_command_until(monitor, "vfs-remove work/renamed.txt", "vfs-remove ok request", proc)
            before_alias_empty_list = len(log_text())
            send_command_until(monitor, "vfs-list work/", "vfsserver list request", proc)
            wait_for("vfs-list ok count 0", proc, before_alias_empty_list)
            before_alias_remove = len(log_text())
            send_command_until(monitor, "vfs-mount-remove work/",
                               "vfsserver mount removed work/", proc)
            wait_for("vfsserver delegated mount remove", proc, before_alias_remove)
            wait_for("vfsvirtual mount remove work/", proc, before_alias_remove)
            wait_for("vfs-mount-remove ok request", proc, before_alias_remove)
            before_alias_revoked = len(log_text())
            send_command_until(monitor, "vfs-read work/alias.txt",
                               "vfs-read: chemin hors montage", proc)
            before_alias_missing = len(log_text())
            send_command_until(monitor, "vfs-mount-remove work/",
                               "vfs-mount-remove: montage absent", proc)
            before_fat16_list = len(log_text())
            send_command_until(monitor, "vfs-list media/", "vfsserver list request", proc)
            wait_for("vfs-list ok count 2", proc, before_fat16_list)
            wait_for("FATOK.TXT", proc, before_fat16_list)
            before_fat16_read = len(log_text())
            send_command_until(monitor, "vfs-read media/fatok.txt", "vfs-read ok", proc)
            wait_for("FAT16 fixture OK", proc, before_fat16_read)
            before_fat16_stat = len(log_text())
            send_command_until(monitor, "vfs-stat media/fatok.txt",
                               "vfs-stat ok size 17 flags file", proc)
            wait_for("vfs-stat ok size 17 flags file", proc, before_fat16_stat)
            before_fat16_write = len(log_text())
            send_command_until(monitor, "vfs-write fat16/new.txt qemu-fat16",
                               "vfsserver write request", proc)
            wait_for("vfs-write ok request", proc, before_fat16_write)
            before_fat16_new_read = len(log_text())
            send_command_until(monitor, "vfs-read fat16/new.txt", "vfs-read ok", proc)
            wait_for("qemu-fat16", proc, before_fat16_new_read)
            before_fat16_rename = len(log_text())
            send_command_until(monitor, "vfs-rename fat16/new.txt fat16/renamed.txt",
                               "vfsserver rename request", proc)
            wait_for("vfs-rename ok request", proc, before_fat16_rename)
            before_fat16_old_read = len(log_text())
            send_command_until(monitor, "vfs-read fat16/new.txt", "vfsserver read request", proc)
            wait_for("vfs-read: lecture refusee ou fichier absent", proc, before_fat16_old_read)
            before_fat16_renamed_read = len(log_text())
            send_command_until(monitor, "vfs-read fat16/renamed.txt", "vfs-read ok", proc)
            wait_for("qemu-fat16", proc, before_fat16_renamed_read)
            before_fat16_new_list = len(log_text())
            send_command_until(monitor, "vfs-list fat16/", "vfsserver list request", proc)
            wait_for("vfs-list ok count 3", proc, before_fat16_new_list)
            wait_for("RENAMED.TXT", proc, before_fat16_new_list)
            before_fat16_remove = len(log_text())
            send_command_until(monitor, "vfs-remove fat16/renamed.txt", "vfsserver remove request", proc)
            wait_for("vfs-remove ok request", proc, before_fat16_remove)
            before_fat16_removed_read = len(log_text())
            send_command_until(monitor, "vfs-read fat16/renamed.txt", "vfsserver read request", proc)
            wait_for("vfs-read: lecture refusee ou fichier absent", proc, before_fat16_removed_read)
            before_fat16_removed_list = len(log_text())
            send_command_until(monitor, "vfs-list fat16/", "vfsserver list request", proc)
            wait_for("vfs-list ok count 2", proc, before_fat16_removed_list)
            if "RENAMED.TXT" in log_text()[before_fat16_removed_list:]:
                raise RuntimeError("RENAMED.TXT reste visible apres vfs-remove FAT16")
            assert_lfn_lifecycle(monitor, proc, "fat16", "long-fichier-fat16.txt",
                                 "renomme-lfn-fat16.txt", "qemu-lfn16")
            assert_fat_subdirectory_lifecycle(monitor, proc, "fat16", "qemu-sub16")
            before_fat32_list = len(log_text())
            send_command_until(monitor, "vfs-list media32/", "vfsserver list request", proc)
            wait_for("vfs-list ok count 1", proc, before_fat32_list)
            wait_for("FAT32OK.TXT", proc, before_fat32_list)
            before_fat32_read = len(log_text())
            send_command_until(monitor, "vfs-read media32/fat32ok.txt", "vfs-read ok", proc)
            wait_for("FAT32 secondary fixture OK", proc, before_fat32_read)
            before_fat32_stat = len(log_text())
            send_command_until(monitor, "vfs-stat media32/fat32ok.txt",
                               "vfs-stat ok size 27 flags file", proc)
            wait_for("vfs-stat ok size 27 flags file", proc, before_fat32_stat)
            before_fat32_write = len(log_text())
            send_command_until(monitor, "vfs-write fat32/new.txt qemu-fat32",
                               "vfsserver write request", proc)
            wait_for("vfs-write ok request", proc, before_fat32_write)
            before_fat32_new_read = len(log_text())
            send_command_until(monitor, "vfs-read fat32/new.txt", "vfs-read ok", proc)
            wait_for("qemu-fat32", proc, before_fat32_new_read)
            before_fat32_rename = len(log_text())
            send_command_until(monitor, "vfs-rename fat32/new.txt fat32/renamed.txt",
                               "vfsserver rename request", proc)
            wait_for("vfs-rename ok request", proc, before_fat32_rename)
            before_fat32_old_read = len(log_text())
            send_command_until(monitor, "vfs-read fat32/new.txt", "vfsserver read request", proc)
            wait_for("vfs-read: lecture refusee ou fichier absent", proc, before_fat32_old_read)
            before_fat32_renamed_read = len(log_text())
            send_command_until(monitor, "vfs-read fat32/renamed.txt", "vfs-read ok", proc)
            wait_for("qemu-fat32", proc, before_fat32_renamed_read)
            before_fat32_new_list = len(log_text())
            send_command_until(monitor, "vfs-list fat32/", "vfsserver list request", proc)
            wait_for("vfs-list ok count 2", proc, before_fat32_new_list)
            wait_for("RENAMED.TXT", proc, before_fat32_new_list)
            before_fat32_remove = len(log_text())
            send_command_until(monitor, "vfs-remove fat32/renamed.txt", "vfsserver remove request", proc)
            wait_for("vfs-remove ok request", proc, before_fat32_remove)
            before_fat32_removed_read = len(log_text())
            send_command_until(monitor, "vfs-read fat32/renamed.txt", "vfsserver read request", proc)
            wait_for("vfs-read: lecture refusee ou fichier absent", proc, before_fat32_removed_read)
            before_fat32_removed_list = len(log_text())
            send_command_until(monitor, "vfs-list fat32/", "vfsserver list request", proc)
            wait_for("vfs-list ok count 1", proc, before_fat32_removed_list)
            if "RENAMED.TXT" in log_text()[before_fat32_removed_list:]:
                raise RuntimeError("RENAMED.TXT reste visible apres vfs-remove FAT32")
            assert_lfn_lifecycle(monitor, proc, "fat32", "long-fichier-fat32.txt",
                                 "renomme-lfn-fat32.txt", "qemu-lfn32")
            assert_fat_subdirectory_lifecycle(monitor, proc, "fat32", "qemu-sub32")
            before_worker_mutate_only = len(log_text())
            send_command_until(monitor, "vfs-backend-status %s" % worker_pid,
                               "vfsserver backend status request", proc)
            wait_for("vfs-backend-status: capacite absente ou refusee", proc, before_worker_mutate_only)
            before_outside = len(log_text())
            send_command_until(monitor, "vfs-read hello.txt",
                               "vfsserver path outside mounts", proc)
            wait_for("vfs-read: chemin hors montage", proc, before_outside)
            before_pending = len(log_text())
            send_command_until(monitor, "ipc-send 1 deferred", "ipc-send ok", proc,
                               key_delay=0.55)
            before_read = len(log_text())
            send_command_until(monitor, "vfs-read initrd/hello.txt", "vfs-read ok", proc)
            wait_for("vfs-read ok 35 request", proc, before_read)
            wait_for("Un autre fichier de demonstration.", proc, before_read)
            before_deferred_list = len(log_text())
            send_command_until(monitor, "vfs-list initrd/", "vfs-list partiel count", proc)
            wait_for("vfs-list partiel count 4", proc, before_deferred_list)
            wait_for_listed_name(monitor, proc, "initrd/", "hello.txt")
            before_receive = len(log_text())
            send_command_until(monitor, "ipc-recv", "ipc-recv from 1", proc, attempts=6)
            wait_for("type 0", proc, before_receive)
            wait_for("data deferred", proc, before_receive)
            before_virtual = len(log_text())
            send_command_until(monitor, "vfs-read vfs-info", "vfsserver delegated vfs-info", proc)
            wait_for("vfsvirtual read vfs-info", proc, before_virtual)
            wait_for("vfsserver ring3 policy", proc, before_virtual)
            before_virtual_mounts = len(log_text())
            send_command_until(monitor, "vfs-read vfs-mounts", "vfsserver delegated vfs-mounts", proc)
            wait_for("initrd/ ro", proc, before_virtual_mounts)
            wait_for("overlay/ rw", proc, before_virtual_mounts)
            wait_for("fat16/ ro", proc, before_virtual_mounts)
            wait_for("fat32/ ro", proc, before_virtual_mounts)
            if log_text()[before_virtual_mounts:].count("vfsvirtual format mount") < 4:
                time.sleep(0.3)
            if log_text()[before_virtual_mounts:].count("vfsvirtual format mount") < 4:
                raise RuntimeError("formatage sequentiel des montages incomplet")
            before_stale_alias_add = len(log_text())
            send_command_until(monitor, "vfs-mount-add stale/ overlay",
                               "vfsserver mount added stale/ overlay", proc)
            wait_for("vfsserver delegated mount add", proc, before_stale_alias_add)
            wait_for_mount_add_worker(proc, "stale/", "overlay", before_stale_alias_add)
            before_worker_stop = len(log_text())
            send_command_until(monitor, "kill %s" % worker_pid, "Processus %s termine" % worker_pid, proc)
            before_worker_missing = len(log_text())
            send_command_until(monitor, "service-find vfs-virtual",
                               "service-find: service indisponible", proc)
            before_worker_missing_health = len(log_text())
            send_command_until(monitor, "vfs-read vfs-worker", "vfsserver virtual vfs-worker local", proc)
            wait_for("vfsvirtual missing recoveries=0", proc, before_worker_missing_health)
            before_worker_fallback = len(log_text())
            send_command_until(monitor, "vfs-read vfs-info", "vfsserver virtual vfs-info local", proc)
            wait_for("vfsserver ring3 policy", proc, before_worker_fallback)
            before_worker_restart = len(log_text())
            send_command_until(monitor, "spawn vfsvirtual", "spawn ok pid", proc)
            worker_restarted = re.search(r"spawn ok pid[\s\S]*?(\d+) vfsvirtual",
                                         log_text()[before_worker_restart:])
            if not worker_restarted:
                raise RuntimeError("worker virtuel VFS non relance")
            worker_pid = worker_restarted.group(1)
            send_command(monitor, "yield", proc)
            wait_for("vfsvirtual ready", proc, before_worker_restart)
            send_command_until(monitor, "service-find vfs-virtual",
                               "service-find ok vfs-virtual %s" % worker_pid, proc)
            before_worker_restart_health = len(log_text())
            send_command_until(monitor, "vfs-read vfs-worker", "vfsserver virtual vfs-worker local", proc)
            wait_for("vfsvirtual ready pid=%s recoveries=0" % worker_pid, proc,
                     before_worker_restart_health)
            before_stale_alias_purged = len(log_text())
            send_command_until(monitor, "vfs-read stale/alias.txt",
                               "vfs-read: chemin hors montage", proc)
            before_worker_recovered = len(log_text())
            send_command_until(monitor, "vfs-read vfs-info", "vfsserver delegated vfs-info", proc)
            wait_for("vfsvirtual read vfs-info", proc, before_worker_recovered)
            wait_for("vfsserver ring3 policy", proc, before_worker_recovered)
            before_worker_suspend = len(log_text())
            send_command_until(monitor, "task-suspend %s" % worker_pid,
                               "task-suspend ok %s" % worker_pid, proc)
            before_flight_spawn = len(log_text())
            send_command_until(monitor, "spawn vfsflight", "spawn ok pid", proc)
            flight_spawned = re.search(r"spawn ok pid[\s\S]*?(\d+) vfsflight",
                                       log_text()[before_flight_spawn:])
            if not flight_spawned:
                raise RuntimeError("client VFS en vol non lance")
            flight_pid = flight_spawned.group(1)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            wait_for("vfsflight waiting vfs-info", proc, before_flight_spawn)
            wait_for("vfsserver delegated vfs-info", proc, before_flight_spawn)
            before_worker_timeout = len(log_text())
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            wait_for("vfsserver virtual worker timeout local", proc, before_worker_timeout, timeout=35)
            wait_for("vfsflight local reply ok", proc, before_worker_timeout)
            before_worker_timeout_health = len(log_text())
            send_command_until(monitor, "vfs-read vfs-worker", "vfsserver virtual vfs-worker local", proc)
            wait_for("vfsvirtual ready pid=%s recoveries=0 timeouts=1" % worker_pid,
                     proc, before_worker_timeout_health)
            before_flight_stop = len(log_text())
            send_command_until(monitor, "kill %s" % flight_pid,
                               "Processus %s termine" % flight_pid, proc)
            before_worker_resume = len(log_text())
            send_command_until(monitor, "task-resume %s" % worker_pid,
                               "task-resume ok %s" % worker_pid, proc)
            before_worker_timeout_recovered = len(log_text())
            send_command_until(monitor, "vfs-read vfs-info", "vfsserver delegated vfs-info", proc)
            wait_for("vfsvirtual read vfs-info", proc, before_worker_timeout_recovered)
            wait_for("vfsserver ring3 policy", proc, before_worker_timeout_recovered)
            before_alias_timeout_mount = len(log_text())
            send_command_until(monitor, "vfs-mount-add assets/ initrd",
                               "vfsserver mount added assets/ initrd", proc)
            wait_for("vfsserver delegated mount add", proc, before_alias_timeout_mount)
            wait_for_mount_add_worker(proc, "assets/", "initrd", before_alias_timeout_mount)
            before_alias_worker_suspend = len(log_text())
            send_command_until(monitor, "task-suspend %s" % worker_pid,
                               "task-suspend ok %s" % worker_pid, proc)
            before_alias_flight_spawn = len(log_text())
            send_command_until(monitor, "spawn vfsaliasflight", "spawn ok pid", proc)
            alias_flight_spawned = re.search(r"spawn ok pid[\s\S]*?(\d+) vfsaliasflight",
                                             normalized_log(log_text()[before_alias_flight_spawn:]))
            if not alias_flight_spawned:
                raise RuntimeError("client alias VFS en vol non lance")
            alias_flight_pid = alias_flight_spawned.group(1)
            # Une seule bascule suffit à exécuter le client. Son traceur est
            # postérieur au relais IPC qui peut être suspendu ; le vrai jalon
            # corrélé est donc l’arrivée de son unique lecture au médiateur.
            # Aucune requête applicative n’est renvoyée.
            send_command(monitor, "yield", proc)
            wait_for_single_io_delivery(monitor, proc, "vfsserver read request",
                                        before_alias_flight_spawn)
            # Le client a envoyé une seule lecture d’alias et le médiateur est
            # engagé. Le grant source-scopé cède lui-même le CPU avant l’envoi
            # IPC : une seconde bascule confirmée achève ce grant, puis une
            # troisième reprend le médiateur. Aucune requête VFS ni mutation
            # n’est rejouée.
            send_command_until(monitor, "yield", "yield ok", proc)
            send_command_until(monitor, "yield", "vfsserver delegated alias read", proc)
            before_alias_scope = len(log_text())
            send_command_until(monitor, "vfs-backend-scope %s" % worker_pid,
                               "vfsserver backend scope request", proc)
            wait_for("vfs-backend-scope ok rights read sources initrd", proc, before_alias_scope)
            scope_line = re.search(r"(?m)^vfs-backend-scope ok rights read sources initrd request \d+\r?$",
                                   normalized_log(log_text()[before_alias_scope:]))
            if not scope_line:
                raise RuntimeError("diagnostic de scope divulgue un prefixe interne")
            before_alias_worker_timeout = len(log_text())
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            wait_for("vfsaliasflight bounded invalid", proc, before_alias_worker_timeout, timeout=35)
            before_alias_timeout_health = len(log_text())
            send_command_until(monitor, "vfs-read vfs-worker",
                               "vfsserver virtual vfs-worker local", proc)
            wait_for("timeouts=2", proc, before_alias_timeout_health)
            before_alias_right_revoked = len(log_text())
            send_command_until(monitor, "vfs-backend-status %s" % worker_pid,
                               "vfsserver backend status request", proc)
            wait_for("vfs-backend-status: capacite absente ou refusee", proc,
                     before_alias_right_revoked)
            before_alias_worker_resume = len(log_text())
            send_command_until(monitor, "task-resume %s" % worker_pid,
                               "task-resume ok %s" % worker_pid, proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            send_command(monitor, "yield", proc)
            if "vfsvirtual alias read assets/bin/shell" in log_text()[before_alias_worker_resume:]:
                raise RuntimeError("lecture alias rejouee apres expiration du worker")
            send_command_until(monitor, "kill %s" % alias_flight_pid,
                               "Processus %s termine" % alias_flight_pid, proc)
            before_initrd_stat = len(log_text())
            send_command_until(monitor, "vfs-stat initrd/hello.txt", "vfs-stat ok size 35 flags file", proc)
            wait_for("vfs-stat ok size 35 flags file", proc, before_initrd_stat)
            before_readonly_write = len(log_text())
            send_command_until(monitor, "vfs-write initrd/no.txt denied",
                               "vfsserver write outside mounts", proc)
            wait_for("vfs-write: chemin hors montage ecriture", proc, before_readonly_write)
            before_readonly_remove = len(log_text())
            send_command_until(monitor, "vfs-remove initrd/no.txt",
                               "vfsserver remove outside mounts", proc)
            wait_for("vfs-remove: chemin hors montage ecriture", proc, before_readonly_remove)
            before_readonly_rename = len(log_text())
            send_command_until(monitor, "vfs-rename initrd/no.txt overlay/moved.txt",
                               "vfsserver rename outside mounts", proc)
            wait_for("vfs-rename: chemins hors montage ecriture", proc, before_readonly_rename)
            before_write = len(log_text())
            send_command_until(monitor, "vfs-write overlay/note.txt vfsok",
                               "vfsserver write request", proc)
            wait_for("vfs-write ok request", proc, before_write)
            before_overlay_list = len(log_text())
            send_command_until(monitor, "vfs-list overlay/", "vfsserver list request", proc)
            wait_for("note.txt", proc, before_overlay_list)
            before_overlay_stat = len(log_text())
            send_command_until(monitor, "vfs-stat overlay/note.txt", "vfsserver stat request", proc)
            wait_for("vfs-stat ok size 5 flags file", proc, before_overlay_stat)
            before_written_read = len(log_text())
            send_command_until(monitor, "vfs-read overlay/note.txt", "vfs-read ok", proc)
            wait_for("vfsok", proc, before_written_read)
            before_rename = len(log_text())
            send_command_until(monitor, "vfs-rename overlay/note.txt overlay/moved.txt",
                               "vfs-rename ok request", proc)
            before_old_read = len(log_text())
            send_command_until(monitor, "vfs-read overlay/note.txt",
                               "vfs-read: lecture refusee ou fichier absent", proc)
            before_renamed_read = len(log_text())
            send_command_until(monitor, "vfs-read overlay/moved.txt", "vfs-read ok", proc)
            wait_for("vfsok", proc, before_renamed_read)
            before_remove = len(log_text())
            send_command_until(monitor, "vfs-remove overlay/moved.txt",
                               "vfs-remove ok request", proc)
            before_removed_read = len(log_text())
            send_command_until(monitor, "vfs-read overlay/moved.txt",
                               "vfs-read: lecture refusee ou fichier absent", proc)
            before_stat_outside = len(log_text())
            send_command_until(monitor, "vfs-stat hello.txt", "vfsserver stat outside mounts", proc)
            wait_for("vfs-stat: chemin hors montage", proc, before_stat_outside)
            before_final_stats = len(log_text())
            send_command_until(monitor, "vfs-stats", "vfsserver delegated vfs-stats", proc)
            wait_for("vfsvirtual format stats", proc, before_final_stats)
            wait_for("reads=", proc, before_final_stats)
            wait_for("writes=", proc, before_final_stats)
            wait_for("removes=", proc, before_final_stats)
            wait_for("renames=", proc, before_final_stats)
            final_stats = re.search(
                r"reads=(\d+)\s+writes=(\d+)\s+removes=(\d+)\s+renames=(\d+)",
                normalized_log(log_text()[before_final_stats:]),
            )
            if not final_stats:
                raise RuntimeError("compteurs vfs-stats finaux illisibles")
            if int(final_stats.group(2)) < 2 or int(final_stats.group(3)) < 2 or int(final_stats.group(4)) < 2:
                raise RuntimeError("mutations FAT/overlay absentes des compteurs vfs-stats")
            before_claim = len(log_text())
            send_command_until(monitor, "spawn vfsclaim", "spawn ok pid", proc)
            claimed = re.search(r"spawn ok pid[\s\S]*?(\d+) vfsclaim", log_text()[before_claim:])
            if not claimed:
                raise RuntimeError("beneficiaire VFS non lance")
            claim_pid = claimed.group(1)
            send_command(monitor, "yield", proc)
            wait_for("vfsclaim waiting vfs", proc, before_claim)
            before_grant = len(log_text())
            send_command_until(monitor, "vfs-grant %s" % claim_pid,
                               "vfs-grant sent %s" % claim_pid, proc)
            before_handoff = len(log_text())
            send_command_until(monitor, "yield", "yield ok", proc)
            wait_for("vfsserver grant vfs %s" % claim_pid, proc, before_grant)
            wait_for("vfsclaim backend granted", proc, before_grant)
            before_old_kill = len(log_text())
            send_command_until(monitor, "kill %s" % server_pid,
                               "Processus %s termine" % server_pid, proc)
            before_lookup = len(log_text())
            send_command_until(monitor, "service-find vfs", "service-find ok vfs %s" % claim_pid, proc)
            before_revoked_read = len(log_text())
            send_command_until(monitor, "vfs-read initrd/hello.txt",
                               "vfs-read: reponse VFS absente ou invalide", proc)
            before_new_kill = len(log_text())
            send_command_until(monitor, "kill %s" % claim_pid,
                               "Processus %s termine" % claim_pid, proc)
            before_missing = len(log_text())
            send_command_until(monitor, "vfs-read initrd/hello.txt",
                               "vfs-read: service vfs indisponible", proc)
            before_return = len(log_text())
            send_command_until(monitor, "rc", "rc ok 0", proc)
            print("MOHHDY Foundation VFS service contract passed")
            return 0
        finally:
            if monitor is not None:
                monitor.close()
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    proc.kill()
            for path in (MON, DISK):
                try:
                    os.remove(path)
                except OSError:
                    pass


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("MOHHDY Foundation VFS service contract failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
