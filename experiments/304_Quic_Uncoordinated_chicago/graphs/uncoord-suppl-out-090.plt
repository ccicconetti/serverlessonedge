set grid
set xlabel "Number of clients"
set ylabel "90th percentile of delay (ms)"
set key top left Left
set pointsize 2
set yrange [0:150]

plot \
  'data/out-090.s=8.e=uncoordinated-2' u 1:($2*1000-50):($3*1000-50):($4*1000-50) w yerrorlines lc 1 pt 8 title "uncoordinated-2", \
  'data/out-090.s=8.e=uncoordinated-3' u 1:($2*1000-50):($3*1000-50):($4*1000-50) w yerrorlines lc 2 pt 10 title "uncoordinated-3", \
  'data/out-090.s=8.e=uncoordinated-4' u 1:($2*1000-50):($3*1000-50):($4*1000-50) w yerrorlines lc 3 pt 4 title "uncoordinated-4", \
  'data/out-090.s=8.e=uncoordinated-6' u 1:($2*1000-50):($3*1000-50):($4*1000-50) w yerrorlines lc 4 pt 6 title "uncoordinated-6"
