#!/usr/bin/env python3
"""Every branch of execute_builtin_command() in userspace/shell.c must
return after running its command. A branch that falls through reaches the
end of the chain, returns 0 ("command not found") and the shell then also
tries to exec the name, printing a spurious error after valid output
(seen with lspci, devices, pci-rescan, vfs-grant, vfs-mkdir, vfs-list and
ai-peer-accept)."""
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SHELL = os.path.join(ROOT, "userspace", "shell.c")


def missing_returns(text):
    lines = text.split("\n")
    start = next(i for i, l in enumerate(lines)
                 if re.match(r"^int execute_builtin_command\(", l))
    end = next(i for i in range(start, len(lines)) if lines[i] == "}")
    branch = None
    body = []
    missing = []
    for line in lines[start:end]:
        match = re.search(r'(?:if|else if) \(strcmp\(command, "([^"]+)"\)', line)
        if match or line.startswith("    }") and not line.startswith("    } else"):
            if branch is not None and not any("return" in b for b in body):
                missing.append(branch)
            branch = match.group(1) if match else None
            body = []
        else:
            body.append(line)
    return missing


def main():
    with open(SHELL, "r", encoding="utf-8", errors="replace") as handle:
        missing = missing_returns(handle.read())
    if missing:
        print("FAIL shell dispatch branches without return: " + ", ".join(missing))
        return 1
    print("OK shell dispatch: every builtin branch returns")
    return 0


if __name__ == "__main__":
    sys.exit(main())
