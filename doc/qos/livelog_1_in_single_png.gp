#!/usr/bin/gnuplot

set terminal png

set output "livelog_1_single.png"

set multiplot layout 3,2

plot "livelog_1.dat" u 1:2 w lp pt 5 title "cachedAudioSize"

#set output "livelog_1_durationMs.png"
plot "livelog_1.dat" u 1:3 w lp pt 7 title "durationMs"

#set output "livelog_1_bandwidth.png"
plot "livelog_1.dat" u 1:($4 / 1024) w lp pt 7 title "bandwidth(kB/s)"

#set output "livelog_1_parseDuration.png"
plot "livelog_1.dat" u 1:5 w lp pt 7 title "parseDuration"

#set output "livelog_1_auCounts.png"
plot "livelog_1.dat" u 1:6 w lp pt 7 title "Audio_AU_Counts"

#set output "livelog_1_auSize.png"
plot "livelog_1.dat" u 1:($7 / 1024) w lp pt 7 title "Audio_AU_Total_Size(KB)"

unset multiplot
