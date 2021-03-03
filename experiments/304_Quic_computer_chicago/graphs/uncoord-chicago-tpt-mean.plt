set grid
set xlabel "Number of clients"
set ylabel "Network traffic (kb/s)"
set key top left Left
set yrange [0:*]
set pointsize 2

plot \
  'data/tpt-mean.s=8.e=static' u 1:($2/1000*8):($3/1000*8):($4/1000*8) w ye lt 1 notitle, \
  'data/tpt-mean.s=8.e=static' u 1:($2/1000*8) w lp lt 1 pt 4 title "static", \
  'data/tpt-mean.s=8.e=distributed' u 1:($2/1000*8):($3/1000*8):($4/1000*8) w ye lt 1 notitle, \
  'data/tpt-mean.s=8.e=distributed' u 1:($2/1000*8) w lp lt 1 pt 6 title "distributed", \
  'data/tpt-mean.s=8.e=centralized' u 1:($2/1000*8):($3/1000*8):($4/1000*8) w ye lt 1 notitle, \
  'data/tpt-mean.s=8.e=centralized' u 1:($2/1000*8) w lp lt 1 pt 12 title "centralized", \
  'data/tpt-mean.s=8.e=uncoordinated-2' u 1:($2/1000*8):($3/1000*8):($4/1000*8) w ye lt 1 notitle, \
  'data/tpt-mean.s=8.e=uncoordinated-2' u 1:($2/1000*8) w lp lt 1 pt 8 title "uncoordinated-2", \
  'data/tpt-mean.s=8.e=uncoordinated-3' u 1:($2/1000*8):($3/1000*8):($4/1000*8) w ye lt 1 notitle, \
  'data/tpt-mean.s=8.e=uncoordinated-3' u 1:($2/1000*8) w lp lt 1 pt 10 title "uncoordinated-3"
