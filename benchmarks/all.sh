#!/bin/bash

set -e

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
TESTS_DIR="$SCRIPT_DIR"

"$TESTS_DIR/channels.sh"
"$TESTS_DIR/pipelines.sh"