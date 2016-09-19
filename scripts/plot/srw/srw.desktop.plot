set size 1,1
set terminal postscript enhanced color dashed lw 1 "Helvetica" 14
set output "srw.desktop.eps"
set title "Sequential Read/Write Throughput of my desktop(Intel E8500/DDR2 800MHz)"
set xlabel "Working Set Size (KB)"
set ylabel "Throughput (GB/s)"
set yrange [0:64]
set xrange [8:1048576]
set logscale x 2
set key horizontal
set style line 1 lt 2 lc rgb "#999999" lw 1 dashtype 2
set arrow from 32,0 to 32,64 nohead ls 1
set label 1 "L1 Data Cache 32KB" at 32,50 tc rgb "#999999"
set arrow from 6144,0 to 6144,64 nohead ls 1
set label 2 "L2 Smart Cache" at 6144,50 tc rgb "#999999"

plot \
 'data/desktop_raw.dat' u 1:2 with points title "read" ps .3 lc rgb "#FF0000", \
 'data/desktop_raw.dat' u 1:3 with points title "write" ps .3 lc rgb "#0000FF", \
 'data/desktop_avg.dat' u 1:2 with lines title "read avg" lw 2 lc rgb '#FF0000', \
 'data/desktop_avg.dat' u 1:3 with lines title "write avg" lw 2 lc rgb '#0000FF', \
 'data/desktop_raw.dat' u 1:4 with points title "stride read" ps .3 lt 3 lc rgb "dark-green", \
 'data/desktop_raw.dat' u 1:5 with points title "stride write" ps .3 lt 3 lc rgb "sienna4", \
 'data/desktop_avg.dat' u 1:4 with lines title "stride read avg" lw 2 lt 3 lc rgb "dark-green", \
 'data/desktop_avg.dat' u 1:5 with lines title "stride write avg" lw 2 lt 3 lc rgb "sienna4"
