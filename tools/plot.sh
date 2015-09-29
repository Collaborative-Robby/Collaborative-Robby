#!/bin/bash

GENCOUNT=$(tail -4 ../build/fitvalues | head -1 | cut -d " " -f 3)

#X=$(sed -n "/g$I\$/,/g$(($I+1))\$/p" fitvalues)
#X=$(echo "$X" | tail -n +2 | head -n -1)

(echo "set ticslevel 0;set view 60, 120, 1, 1 ;splot for [i=0:$GENCOUNT] \"../build/fitvalues\" index i  using 1:3:2 with pm3d notitle";cat) | gnuplot;
