set size 1,1
set terminal postscript enhanced color dashed lw 1 "Helvetica" 14
set output "l2cache.eps"
set title "Comparison: L2 Cache Size and Latency"
set ylabel "Measured Cache Size (KB)"
#set logscale y 2
set yrange [0:500]
set y2range [0:10]
set y2label "Measured Read Latency (ns)"
set y2tics
set key top left 
set style histogram
set style data histograms
set style fill solid border -1
set boxwidth 0.9
set xtic rotate by -46 scale 0
set label 1 "Dell Optiplex 760:6144KB" at 5.8,475

plot \
 'cs.dat' u 3:xtic(1) ti "Size" lc rgb "#0000FF", \
       '' u 7 axes x1y2 ti "Latency" with linespoints lw 3 lc rgb "#FF0000"
