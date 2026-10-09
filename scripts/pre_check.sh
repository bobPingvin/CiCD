#!/bin/bash
echo "[SHELL] Running local environment pre-checks on ALT Linux..."
uname -a
echo "[SHELL] Free disk space:"
df -h . | tail -n 1
echo "[SHELL] Environment OK!"
exit 0
