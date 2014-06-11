#!/bin/sh

FOLDERS="genome-itm intruder-itm kmeans-itm labyrinth-itm ssca2-itm vacation-itm yada-itm redblacktree-itm"

rm lib-itm/*.o || true

for F in $FOLDERS
do
    cd $F
    rm *.o || true
    rm $F
    cd ..
done
