set size 1,1
set terminal postscript enhanced color dashed lw 1 "Helvetica" 14
set output "mem.eps"
set title "Comparison: Memory Size and Latency"
set ylabel "Measured Memory Size (GB)"
set logscale y 2
set yrange [1:256]
set y2range [0:200]
set y2label "Measured Read Latency (ns)"
set y2tics
#set key horizontal
set style histogram
set style data histograms
set style fill solid border -1
set boxwidth 0.9
set xtic rotate by -46 scale 0

plot \
 'cs.dat' u 5:xtic(1) ti "Size" lc rgb "#0000FF", \
       '' u 9 axes x1y2 ti "Latency" with linespoints lw 3 lc rgb "#FF0000"
