#!/usr/bin/gnuplot -persist
#
#    
#    	G N U P L O T
#    	Version 5.2 patchlevel 2    last modified 2017-11-01 
#    
#    	Copyright (C) 1986-1993, 1998, 2004, 2007-2017
#    	Thomas Williams, Colin Kelley and many others
#    
#    	gnuplot home:     http://www.gnuplot.info
#    	faq, bugs, etc:   type "help FAQ"
#    	immediate help:   type "help"  (plot window: hit 'h')
# set terminal x11  nopersist enhanced size 960,400  position 0,0 
# set output
unset clip points
set clip one
unset clip two
set errorbars front 1.000000 
set border 31 front lt black linewidth 1.000 dashtype solid
set zdata 
set ydata 
set xdata 
set y2data 
set x2data 
set boxwidth
set style fill  empty border
set style rectangle back fc  bgnd fillstyle   solid 1.00 border lt -1
set style circle radius graph 0.02 
set style ellipse size graph 0.05, 0.03 angle 0 units xy
set dummy x, y
set format x "% h" 
set format y "% h" 
set format x2 "% h" 
set format y2 "% h" 
set format z "% h" 
set format cb "% h" 
set format r "% h" 
set ttics format "% h"
set timefmt "%d/%m/%y,%H:%M"
set angles radians
set tics back
set grid nopolar
set grid xtics nomxtics ytics nomytics noztics nomztics nortics nomrtics \
 nox2tics nomx2tics noy2tics nomy2tics nocbtics nomcbtics
set grid layerdefault   lt 0 linecolor 0 linewidth 0.500,  lt 0 linecolor 0 linewidth 0.500
unset raxis
set theta counterclockwise right
set style parallel front  lt black linewidth 2.000 dashtype solid
set key title "" center
set key fixed bottom right vertical Right noreverse enhanced autotitle nobox
set key noinvert samplen 4 spacing 1 width 0 height 0 
set key maxcolumns 0 maxrows 0
set key noopaque
unset label
unset arrow
set style increment default
unset style line
unset style arrow
set style histogram clustered gap 2 title textcolor lt -1
unset object
set style textbox transparent margins  1.0,  1.0 border  lt -1 linewidth  1.0
set offsets 0, 0, 0, 0
set pointsize 1
set pointintervalbox 1
set encoding default
unset polar
unset parametric
unset decimalsign
unset micro
unset minussign
set view 60, 30, 1, 1
set view azimuth 0
set rgbmax 255
set samples 100, 100
set isosamples 10, 10
set surface 
unset contour
set cntrlabel  format '%8.3g' font '' start 5 interval 20
set mapping cartesian
set datafile separator ","
unset hidden3d
set cntrparam order 4
set cntrparam linear
set cntrparam levels auto 5
set cntrparam points 5
set size ratio 0 1,1
set origin 0,0
set style data points
set style function lines
unset xzeroaxis
unset yzeroaxis
unset zzeroaxis
unset x2zeroaxis
unset y2zeroaxis
set xyplane relative 0.5
set tics scale  1, 0.5, 1, 1, 1
set mxtics default
set mytics default
set mztics default
set mx2tics default
set my2tics default
set mcbtics default
set mrtics default
set nomttics
set xtics border in scale 1,0.5 mirror norotate  autojustify
set xtics  norangelimit autofreq 
set ytics border in scale 1,0.5 mirror norotate  autojustify
set ytics  norangelimit autofreq 
set ztics border in scale 1,0.5 nomirror norotate  autojustify
set ztics  norangelimit autofreq 
unset x2tics
unset y2tics
set cbtics border in scale 1,0.5 mirror norotate  autojustify
set cbtics  norangelimit autofreq 
set rtics axis in scale 1,0.5 nomirror norotate  autojustify
set rtics  norangelimit autofreq 
unset ttics
set title "" 
set title  font "" norotate
set timestamp bottom 
set timestamp "" 
set timestamp  font "" norotate
set trange [ * : * ] noreverse nowriteback
set urange [ * : * ] noreverse nowriteback
set vrange [ * : * ] noreverse nowriteback
set xlabel "Fraction of {/Symbol m}-apps in the cloud"
set xlabel  font "" textcolor lt -1 norotate
set x2label "" 
set x2label  font "" textcolor lt -1 norotate
set xrange [ 0 : 3 ] noreverse nowriteback
set x2range [ * : * ] noreverse nowriteback
set ylabel "CDF" 
set ylabel  font "" textcolor lt -1 rotate
set y2label "" 
set y2label  font "" textcolor lt -1 rotate
set yrange [ 0 : 1 ] noreverse nowriteback
set y2range [ * : * ] noreverse nowriteback
set zlabel "" 
set zlabel  font "" textcolor lt -1 norotate
set zrange [ * : * ] noreverse nowriteback
set cblabel "" 
set cblabel  font "" textcolor lt -1 rotate
set cbrange [ * : * ] noreverse nowriteback
set rlabel "" 
set rlabel  font "" textcolor lt -1 norotate
set rrange [ * : * ] noreverse nowriteback
unset logscale
unset jitter
set zero 1e-08
set lmargin  -1
set bmargin  -1
set rmargin  -1
set tmargin  -1
set locale "en_US.UTF-8"
set pm3d explicit at s
set pm3d scansautomatic
set pm3d interpolate 1,1 flush begin noftriangles noborder corners2color mean
set pm3d nolighting
set palette positive nops_allcF maxcolors 0 gamma 1.5 color model RGB 
set palette rgbformulae 7, 5, 15
set colorbox default
set colorbox vertical origin screen 0.9, 0.2 size screen 0.05, 0.6 front  noinvert bdefault
set style boxplot candles range  1.50 outliers pt 7 separation 1 labels auto unsorted
set loadpath 
set fontpath 
set psdir
set fit brief errorvariables nocovariancevariables errorscaling prescale nowrap v5
GNUTERM = "wxt"
STATS_records = 6400
STATS_invalid = 0
STATS_headers = 0
STATS_blank = 0
STATS_blocks = 1
STATS_outofrange = 0
STATS_columns = 19
STATS_mean = 84.635424269219
STATS_stddev = 14.7288673240804
STATS_ssd = 14.7300181517044
STATS_skewness = 0.536542586052812
STATS_kurtosis = 3.41354596633994
STATS_adev = 11.6336408846384
STATS_mean_err = 0.184110841551005
STATS_stddev_err = 0.130186024550677
STATS_skewness_err = 0.0306186217847897
STATS_kurtosis_err = 0.0612372435695794
STATS_sum = 541666.715323002
STATS_sumsq = 47232605.2728385
STATS_min = 42.136601
STATS_max = 169.26589
STATS_median = 83.264159
STATS_lo_quartile = 74.271161
STATS_up_quartile = 93.744859
STATS_index_min = 3902
STATS_index_max = 6061
## Last datafile plotted: "< grep "2.000000,60000.000000,50,200,10,10,0.500000,0.500000,1" ../data/011-urban0.csv"
plot \
'< grep "2.000000,60000.000000,50,100,10,10,0.500000,0.500000,1" ../data/011-urban0.csv' u ($17):(1) smooth cnorm title "E[|A|] = 100",\
'< grep "2.000000,60000.000000,50,150,10,10,0.500000,0.500000,1" ../data/011-urban0.csv' u ($17):(1) smooth cnorm title "E[|A|] = 150",\
'< grep "2.000000,60000.000000,50,200,10,10,0.500000,0.500000,1" ../data/011-urban0.csv' u ($17):(1) smooth cnorm title "E[|A|] = 200",\
'< grep "2.000000,60000.000000,50,250,10,10,0.500000,0.500000,1" ../data/011-urban0.csv' u ($17):(1) smooth cnorm title "E[|A|] = 250"
#    EOF
