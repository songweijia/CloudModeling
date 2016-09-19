set size 0.5,0.5
set terminal postscript enhanced color dashed lw 1 "Helvetica" 10
set output "cache.eps"
set xlabel "Buffer size (KB)"
set ylabel "Write throughput(GB/s)"
set logscale x 2

plot \
  "data/amazon.dat" u 1:2 title "Amazon EC2(t2.micro)" with lines lt 1 lc rgb "red", \
  "data/google.dat" u 1:2 title "Google Compute(n1-standard-1)" with lines lt 2 lc rgb "blue", \
  "data/azure.dat" u 1:2 title "Microsoft Azure(standard A0)" with lines lt 3 lc rgb "green"

#  "data/fractus.dat" u 1:2 title "Fractus (m1.medium)" with lines lt 4 lc rgb "black"
