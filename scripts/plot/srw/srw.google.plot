set size 1,1
set terminal postscript enhanced color dashed lw 1 "Helvetica" 14
set output "srw.google.eps"
set title "Sequential Read/Write Throughput of google virtual machine(unknown CPU/unknown memory)"
set xlabel "Working Set Size (KB)"
set ylabel "Throughput (GB/s)"
set yrange [0:64]
set xrange [8:1048576]
set logscale x 2
set key horizontal
set style line 1 lt 2 lc rgb "#999999" lw 1 dashtype 2
set style line 2 lt 2 lc rgb "#000000" lw 1 dashtype 2
set arrow from 32,0 to 32,64 nohead ls 1
set label 1 "L1 Cache 32KB" at 32,50 tc rgb "#999999"
set arrow from 256,0 to 256,64 nohead ls 1
set label 2 "L2 Cache 256KB" at 256,50 tc rgb "#999999"
set arrow from 46080,0 to 46080,64 nohead ls 1
set label 3 "L3 SmartCache 45MB" at 46080,50 tc rgb "#999999"
set arrow from 14000,0 to 14000,64 nohead ls 2
set label 4 "About 14MB Effective L3 Cache" at 12000,40 tc rgb "#000000"


plot \
 'data/google_raw.dat' u 1:2 with points title "read" ps .3 lc rgb "#FF0000", \
 'data/google_raw.dat' u 1:3 with points title "write" ps .3 lc rgb "#0000FF", \
 'data/google_avg.dat' u 1:2 with lines title "read-avg" lw 2 lc rgb '#880000', \
 'data/google_avg.dat' u 1:3 with lines title "write-avg" lw 2 lc rgb '#000088'
