set size 1,1
set terminal postscript enhanced color dashed lw 1 "Helvetica" 14
set output "cache_size.eps"
set title "Comparison:Measured Cache Size"
set ylabel "Measured Cache Size (KB)"
set logscale y 2
set yrange [1:262144]
#set key horizontal
set style histogram cluster gap 1
set style data histograms
set style fill solid border -1
set boxwidth 0.9
set xtic rotate by -46 scale 0

plot \
 'cs.dat' u 2:xtic(1) ti "L1 Data Cache", \
       '' u 3         ti "L2 Cache", \
       '' u 4         ti "L3 Cache"
