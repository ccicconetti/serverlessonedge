set grid
set xlabel "Number of clients"
set ylabel "Average executor load"
set key bottom right Left
set pointsize 2
set yrange [0:0.7]
set ytics 0.1

plot \
  'data/util-mean.s=8.e=static' u 1:($2/1):($3/1):($4/1) w ye lt 1 notitle, \
  'data/util-mean.s=8.e=static' u 1:($2/1) w lp lt 1 pt 4 title "static", \
  'data/util-mean.s=8.e=distributed' u 1:($2/1):($3/1):($4/1) w ye lt 1 notitle, \
  'data/util-mean.s=8.e=distributed' u 1:($2/1) w lp lt 1 pt 6 title "distributed", \
  'data/util-mean.s=8.e=centralized' u 1:($2/1):($3/1):($4/1) w ye lt 1 notitle, \
  'data/util-mean.s=8.e=centralized' u 1:($2/1) w lp lt 1 pt 12 title "centralized", \
  'data/util-mean.s=8.e=uncoordinated-2' u 1:($2/1):($3/1):($4/1) w ye lt 1 notitle, \
  'data/util-mean.s=8.e=uncoordinated-2' u 1:($2/1) w lp lt 1 pt 8 title "uncoordinated-2", \
  'data/util-mean.s=8.e=uncoordinated-3' u 1:($2/1):($3/1):($4/1) w ye lt 1 notitle, \
  'data/util-mean.s=8.e=uncoordinated-3' u 1:($2/1) w lp lt 1 pt 10 title "uncoordinated-3"
