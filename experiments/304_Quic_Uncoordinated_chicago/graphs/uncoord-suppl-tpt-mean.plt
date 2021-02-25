set grid
set xlabel "Number of clients"
set ylabel "Network traffic (kb/s)"
set key top left Left
set yrange [0:*]
set pointsize 2

plot \
  'data/tpt-mean.s=8.e=uncoordinated-2' u 1:($2/1000*8):($3/1000*8):($4/1000*8) w yerrorlines lc 1 pt 8 title "uncoordinated-2", \
  'data/tpt-mean.s=8.e=uncoordinated-3' u 1:($2/1000*8):($3/1000*8):($4/1000*8) w yerrorlines lc 2 pt 10 title "uncoordinated-3", \
  'data/tpt-mean.s=8.e=uncoordinated-4' u 1:($2/1000*8):($3/1000*8):($4/1000*8) w yerrorlines lc 3 pt 4 title "uncoordinated-4", \
  'data/tpt-mean.s=8.e=uncoordinated-6' u 1:($2/1000*8):($3/1000*8):($4/1000*8) w yerrorlines lc 4 pt 6 title "uncoordinated-6"
