#!/bin/bash

echo "Enter unit type identifier (empty to cancel):"
read x

if [ -e $x ]; then
  exit; 
fi

xl="${x,,}"
xu="${x^^}"

for f in *.h; do mv -- "$f" "${f//tpl/$xl}"; done
for f in *.c; do mv -- "$f" "${f//tpl/$xl}"; done

sed "s/tpl/$xl/" -i *.h
sed "s/TPL/$xu/" -i *.h
sed "s/tpl/$xl/" -i *.c
sed "s/TPL/$xu/" -i *.c

echo "Unit $xu set up completed. Removing installer.."
rm '!README.TXT'
rm $0
