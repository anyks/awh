#!/usr/bin/env bash

# Исправляем баг в git, когда при коммите в сообщении появляется лишняя строка Co-Authored-By:
sed -i '' '/Co-Authored-By:/d' "$1"
