#!/bin/sh
sed -n '/.*execute.*{/,/^}/ p' | grep case | cut -b 7- | tr -d ':'
