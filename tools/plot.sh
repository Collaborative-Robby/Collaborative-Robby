#!/bin/bash


#X=$(sed -n "/g$I\$/,/g$(($I+1))\$/p" fitvalues)
#X=$(echo "$X" | tail -n +2 | head -n -1)

if [ $# -lt 1 ]; then
    FILENAME="../build/fitvalues"
else
    FILENAME=$1
fi

GENCOUNT=$(tail -4 $FILENAME | head -1 | cut -d " " -f 3)

(echo "set term png size 600,600; set output \"graph.png\"; set xlabel \"Numero Robby\" rotate parallel offset 3,0; set ylabel \"Generazioni\" rotate parallel offset 0,-1; set zlabel \"Fitness\" rotate parallel;\
set ticslevel 0;set view 60, 120, 1, 1 ;splot for [i=0:$GENCOUNT] \"$FILENAME\" index i  using 1:3:2 with pm3d notitle") | gnuplot;
