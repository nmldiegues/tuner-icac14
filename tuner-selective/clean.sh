#!/bin/sh

FOLDERS="genome intruder kmeans labyrinth ssca2 vacation yada redblacktree"

rm lib/*.o || true

for F in $FOLDERS
do
    cd $F
    rm *.o || true
    rm $F
    cd ..
done
