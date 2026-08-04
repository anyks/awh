#!/usr/bin/env bash
grep -iq 'co-authored-by' "$1" && sed -i '/[Cc][Oo]-[Aa][Uu][Tt][Hh][Oo][Rr][Ee][Dd]-[Bb][Yy]:/d' "$1"

