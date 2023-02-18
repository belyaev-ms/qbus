#! /usr/bin/gnuplot -persist
#set terminal postscript eps enhanced color solid
#set output "ipc_test.ps"

#set terminal png size 1024, 768
#set output "ipc_test.png"
 
set datafile separator ","
set grid xtics ytics

set ylabel "Level"
set xlabel "Time"
set title "ipc-test"

plot "test_consumer.0.log" using 2:1 with lines title "consumer 0", \
     "test_consumer.1.log" using 2:1 with lines title "consumer 1", \
     "test_consumer.2.log" using 2:1 with lines title "consumer 2", \
     "test_consumer.3.log" using 2:1 with lines title "consumer 3", \
     "test_producer.log" using 2:1 with lines title "producer"

#plot "test_consumer.0.log" using 2:1 with lines title "consumer 0", \
#     "test_consumer.1.log" using 2:1 with lines title "consumer 1", \
#     "test_consumer.2.log" using 2:1 with lines title "consumer 2", \
#     "test_consumer.3.log" using 2:1 with lines title "consumer 3", \
#     "test_consumer.4.log" using 2:1 with lines title "consumer 4", \
#     "test_consumer.5.log" using 2:1 with lines title "consumer 5", \
#     "test_consumer.6.log" using 2:1 with lines title "consumer 6", \
#     "test_consumer.7.log" using 2:1 with lines title "consumer 7", \
#     "test_consumer.8.log" using 2:1 with lines title "consumer 8", \
#     "test_consumer.9.log" using 2:1 with lines title "consumer 9", \
#     "test_producer.log" using 2:1 with lines title "producer"
 
