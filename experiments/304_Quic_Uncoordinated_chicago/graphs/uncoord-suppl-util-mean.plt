set grid
set xlabel "Number of clients"
set ylabel "Average executor load"
set key bottom right Left
set pointsize 2
set yrange [0:1]
set ytics 0.1

plot \
  'data/util-mean.s=8.e=uncoordinated-2' u 1:($2/1) w lp lc 1 pt 8 title "uncoordinated-2", \
  'data/util-mean.s=8.e=uncoordinated-3' u 1:($2/1) w lp lc 2 pt 10 title "uncoordinated-3", \
  'data/util-mean.s=8.e=uncoordinated-4' u 1:($2/1) w lp lc 3 pt 4 title "uncoordinated-4", \
  'data/util-mean.s=8.e=uncoordinated-6' u 1:($2/1) w lp lc 4 pt 6 title "uncoordinated-6"
