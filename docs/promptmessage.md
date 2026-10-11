# PromptMessage (phase 4, US-046 to US-060)

Status on 10 October 2026: first lot delivered in the guest shell (C,
Ring 3, QEMU). This is a small deterministic language with a compiler, a
bytecode image and a bounded VM. It is not natural language understanding:
the words are a fixed grammar, and no model interprets intentions.

## Language (US-046)

One statement per line. `#` and `//` start a comment, `##` starts a
documentation comment (collected by `pm-doc`). Keywords are case-insensitive.

    create file "document.txt" with content "Hello MOHHDY"
    append "line" to file "log.txt"
    show file "log.txt"
    print "text" + $name          (alias: say)
    set name to "MOHHDY"           (read it back as $name)
    if system.memory < 50% then print "ok"
    if $n > 3 and $name contains "HH" then
      ...
    else
      ...
    end
    repeat 3 times print "x"       (or a block closed by end)
    when user says "bonjour" then print "salut"   (alias: when user dit)
    expect file "document.txt" exists
    use "/pm/lib.pm"               (library, top level, one level deep)
    stop

Values are text or integers. `+` adds integers and concatenates otherwise.
Comparisons: `== != < > <= >=` (order only on integers), `contains`, `is`.
Conditions combine with `and`, `or`, `not`. `file X exists` tests a path.
`system.memory` is the percentage of used physical pages (SYS_MEMINFO).
`50%` is the integer 50.

## Compiler, image and VM (US-047, US-048)

`userspace/promptmessage.c` compiles the source to a stack bytecode
(21 opcodes). The PMC1 image is: magic, sizes, triggers, variable names,
string pool, code and an FNV-1a 32 checksum. A truncated or modified image is
rejected before execution. The VM is bounded: 20000 steps, 12 stack slots,
16 variables, 8 triggers, 4 KiB of source, 4 KiB of code, 2 KiB of strings.
File writes refuse `..`, `/bin/` and `/models/`.

## Tooling in the guest shell

| Command | Story | Output |
|---|---|---|
| `pm-check FILE` | US-049 | `pm-check ok ...` or `pm-check error line L col C: message` |
| `pm-run FILE` | US-048 | runs main body, keeps the program for `pm-say` |
| `pm-say PHRASE` | US-046 | runs the matching `when user says` trigger |
| `pm-test FILE` | US-057 | `pm-test ok FILE expects N failed 0` |
| `pm-doc FILE` | US-051 | `##` comments, triggers, variables, summary |
| `pm-debug FILE [LINE]` | US-052 | trace of lines, or break before LINE with variables |
| `pm-disasm FILE` | US-052 | bytecode listing |
| `pm-compile SRC OUT` / `pm-exec IMG` | US-047 | PMC1 image with checksum |
| `use "lib.pm"` | US-053 | library inclusion at compile time |
| `pm-version FILE` / `pm-versions FILE` | US-054 | snapshots FILE.v1..v9 with checksums |
| `pm-edit FILE` | US-050 | line editor (`.` save and check, `.l` list, `.q` quit) |
| `pm STATEMENT` | US-048 | runs one statement typed on the shell line |
| `pm-catalog` / `pm-install NAME` | US-059 (local) | lists /pm/*.pm with their `##` line, installs a validated copy |
| `pm-certify FILE` / `pm-verify FILE` | US-060 (local) | FILE.cert record (FNV-1a sum, expects passed); verify detects a changed source; with a collab key (`collab-join`) also FILE.sig, a Schnorr signature over SHA-256 of the source, checked by `pm-verify` (author = key fingerprint) |

The optimizer (US-056) folds integer constants and constant conditions at
compile time (`folded N` in `pm-check` / `pm-compile`).

Samples: `promptmessage/examples/*.pm`, shipped in the initrd under `/pm/`.

## Proofs

- Unit: `tests/unit/userspace/test_promptmessage.c` (9 tests: spec examples,
  blocks and loops, syntax errors with line and column, folding, image round
  trip and tamper, expect, sandbox and budget, libraries, doc, debugger,
  limits).
- QEMU: `make qemu-promptmessage` (`tests/integration/test_qemu_promptmessage.py`,
  about 1 min, CI job "QEMU integration contracts (vfs-service)").

## Not delivered

US-055 cross compiler (one target: this VM), US-058 AI assistance (no
instruction-tuned model offline). US-059 is only a local catalog (no network
marketplace, no publishing). US-060 is local: FILE.sig proves which node key signed
the source (Schnorr, same key as the collab ledger), but there is no
certificate authority or revocation, so trusting an author fingerprint is up
to the user. The editor is
a line editor, not a full screen IDE.
