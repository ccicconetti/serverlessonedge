set grid
set xlabel "Number of clients"
set ylabel "90th percentile of delay (ms)"
set key top left Left
set pointsize 2
set yrange [0:250]

plot \
  'data/out-090.s=8.e=static' u 1:($2*1000-50):($3*1000-50):($4*1000-50) w ye lt 1 notitle, \
  'data/out-090.s=8.e=static' u 1:($2*1000-50) w lp lt 1 pt 4 title "static", \
  'data/out-090.s=8.e=distributed' u 1:($2*1000-50):($3*1000-50):($4*1000-50) w ye lt 1 notitle, \
  'data/out-090.s=8.e=distributed' u 1:($2*1000-50) w lp lt 1 pt 6 title "distributed", \
  'data/out-090.s=8.e=centralized' u 1:($2*1000-50):($3*1000-50):($4*1000-50) w ye lt 1 notitle, \
  'data/out-090.s=8.e=centralized' u 1:($2*1000-50) w lp lt 1 pt 12 title "centralized", \
  'data/out-090.s=8.e=uncoordinated-2' u 1:($2*1000-50):($3*1000-50):($4*1000-50) w ye lt 1 notitle, \
  'data/out-090.s=8.e=uncoordinated-2' u 1:($2*1000-50) w lp lt 1 pt 8 title "uncoordinated-2", \
  'data/out-090.s=8.e=uncoordinated-3' u 1:($2*1000-50):($3*1000-50):($4*1000-50) w ye lt 1 notitle, \
  'data/out-090.s=8.e=uncoordinated-3' u 1:($2*1000-50) w lp lt 1 pt 10 title "uncoordinated-3"
