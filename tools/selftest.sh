#!/bin/zsh
# Runs the in-app self-test against a fresh store in /tmp. Exit code 0 when every check passed.
set -u
rm -rf ~/Library/Saved\ Application\ State/com.atoma.modo.savedState
store=/tmp/Modo-self.store
rm -f $store $store-shm $store-wal
APP_STORE=$store ~/Applications/Modo.app/Contents/MacOS/Modo --selftest
