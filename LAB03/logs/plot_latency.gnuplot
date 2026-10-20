set terminal pngcairo size 1200,700
set output '/home/vboxuser/embedded_lab3/logs/latency_histogram.png'

set title 'LAB03 - Cyclictest Latency Histogram'
set xlabel 'Latency (microseconds)'
set ylabel 'Number of samples'
set grid
set key top right

plot '/home/vboxuser/embedded_lab3/logs/hist_standard.txt' \
     using 1:2 with lines title 'Thread 0', \
     '' using 1:3 with lines title 'Thread 1'

