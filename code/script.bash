#!/bin/bash
#proc.sh

nb=$1 #nombre de sites attendus
ip=$2 #port du processus de départ que l'on donne en arg aux sites
port=$3 #ip du processus de départ que l'on donne en arg aux sites

xterm -geometry 95x40+400+200 -fa 'Monospace' -fs 12 -e bash -c './proc '$nb' '$port'; exec bash'  & 
sleep 1
xterm -geometry 60x15+1+1 -fa 'Monospace' -fs 12 -e bash -c './site '$ip' '$port'; exec bash' &
sleep 1
xterm -geometry 60x15+650+1 -fa 'Monospace' -fs 12 -e bash -c './site '$ip' '$port'; exec bash' &
sleep 1
xterm -geometry 60x15+1300+1 -fa 'Monospace' -fs 12 -e bash -c './site '$ip' '$port'; exec bash' &
sleep 1

xterm -geometry 60x15+1+320 -fa 'Monospace' -fs 12 -e bash -c './site '$ip' '$port'; exec bash' &
sleep 1
xterm -geometry 60x15+650+320 -fa 'Monospace' -fs 12 -e bash -c './site '$ip' '$port'; exec bash' &
sleep 1
xterm -geometry 60x15+1300+320 -fa 'Monospace' -fs 12 -e bash -c './site '$ip' '$port'; exec bash' &
sleep 1

xterm -geometry 60x15+1+650 -fa 'Monospace' -fs 12 -e bash -c './site '$ip' '$port'; exec bash' &
sleep 1
xterm -geometry 60x15+650+650 -fa 'Monospace' -fs 12 -e bash -c './site '$ip' '$port'; exec bash' &
sleep 1
xterm -geometry 60x15+1300+650 -fa 'Monospace' -fs 12 -e bash -c './site '$ip' '$port'; exec bash' &