set size 1,1
set terminal postscript enhanced color dashed lw 1 "Helvetica" 14
set output "l3cache.eps"
set title "Comparison: L3 Cache Size and Latency"
set ylabel "Measured Cache Size (KB)"
#set logscale y 2
set yrange [0:50000]
set y2range [0:50]
set y2label "Measured Read Latency (ns)"
set y2tics
#set key horizontal
set style histogram
set style data histograms
set style fill solid border -1
set boxwidth 0.9
set xtic rotate by -46 scale 0

plot \
 'cs.dat' u 4:xtic(1) ti "Size" lc rgb "#0000FF", \
       '' u 8 axes x1y2 ti "Latency" with linespoints lw 3 lc rgb "#FF0000"
