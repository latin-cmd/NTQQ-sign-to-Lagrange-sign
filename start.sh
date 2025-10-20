#!/bin/bash
SCRIPT_DIR=$(realpath $(dirname "${BASH_SOURCE[0]}"))
export ELECTRON_RUN_AS_NODE=1
LD_PRELOAD=./libSignerServer.so /opt/QQ/qq ${SCRIPT_DIR}/load.js $@
