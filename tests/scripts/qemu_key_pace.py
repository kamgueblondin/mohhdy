#!/usr/bin/env python3
"""Pace QEMU PS/2 sendkey on the guest serial echo.

SYS_GETS and SYS_GETC each print one serial line per accepted character.
The line contains "SYS_GETS" or "SYS_GETC" and ends with ": '<char>'".
Waiting for that echo, and not less than the sendkey hold, keeps the next
scancode from overlapping the key release. Callers keep their own assertions
(the submitted line, command output). This does not drop or skip checks.

MOHHDY_KEY_GAP is the minimum seconds between scancodes (default 0.03).
"""
from __future__ import print_function

import os
import re
import time

MIN_GAP = float(os.environ.get("MOHHDY_KEY_GAP", "0.03"))
POLL = float(os.environ.get("MOHHDY_KEY_POLL", "0.005"))
ECHO_TIMEOUT = float(os.environ.get("MOHHDY_KEY_ECHO_TIMEOUT", "3"))


def visible(text):
    """Drop scheduler and timer lines that can split a key-echo line."""
    text = re.sub(r"TIMER_ALIVE: tick=\d+\+?\r?\n?", "", text)
    text = re.sub(r"\[SCHED\] switching to task \d+\r?\n?", "", text)
    return text


def echo_count(text, char):
    """How many SYS_GETS / SYS_GETC echoes of char are in text."""
    suffix = ": '%s'" % char
    count = 0
    start = 0
    text = visible(text)
    while True:
        index = text.find(suffix, start)
        if index < 0:
            return count
        line = text[text.rfind("\n", 0, index) + 1:index]
        if "SYS_GETS" in line or "SYS_GETC" in line:
            count += 1
        start = index + len(suffix)


def _self_check():
    sample = "SYS_GETS: marker: 'a'\nSYS_GETC: marker: 'b'\nplain: 'a'\n"
    if echo_count(sample, "a") != 1 or echo_count(sample, "b") != 1 or echo_count(sample, "c") != 0:
        raise AssertionError("echo_count self-check failed: %s %s" % (
            echo_count(sample, "a"), echo_count(sample, "b")))
    split = "SYS_GETS: marker: '[SCHED] switching to task 4\nz'\n"
    if echo_count(split, "z") != 1:
        raise AssertionError("echo_count did not rejoin a scheduler split")


_self_check()


def send_paced(client, proc, log_text, command, hold_ms, aliases):
    """Type command and Enter. Raise before Enter if a character is not echoed."""
    for char in command:
        if proc is not None and proc.poll() is not None:
            raise RuntimeError("QEMU stopped while typing %r" % command)
        before = len(log_text())
        key = aliases.get(char, char.lower())
        started = time.monotonic()
        client.sendall(("sendkey %s %d\n" % (key, hold_ms)).encode("ascii"))
        deadline = started + ECHO_TIMEOUT
        seen = 0
        while time.monotonic() < deadline:
            if proc is not None and proc.poll() is not None:
                raise RuntimeError("QEMU stopped while typing %r" % command)
            seen = echo_count(log_text()[before:], char)
            elapsed = time.monotonic() - started
            if seen and elapsed >= MIN_GAP:
                break
            if seen:
                time.sleep(MIN_GAP - elapsed)
                seen = echo_count(log_text()[before:], char)
                break
            time.sleep(POLL)
        else:
            seen = echo_count(log_text()[before:], char)
        if seen == 0:
            raise RuntimeError("key %r not echoed while typing %r" % (char, command))
        # A doubled scancode is erased before Enter so a mutated command
        # (prod-inject 8 instead of 80) is not submitted.
        while seen > 1:
            client.sendall(("sendkey backspace %d\n" % hold_ms).encode("ascii"))
            time.sleep(MIN_GAP)
            seen -= 1
        elapsed = time.monotonic() - started
        if elapsed < MIN_GAP:
            time.sleep(MIN_GAP - elapsed)
    client.sendall(("sendkey ret %d\n" % hold_ms).encode("ascii"))
