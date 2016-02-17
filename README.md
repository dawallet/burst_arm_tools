## Burst ARM tools
Burst miner compiled for armhf processors for GNU/Linux.
Compiled with BananaPi. 

Howto: Clone the repository to your ARM system, extract it, and execute like:

> For dcct tools:

> ./plot -k YOURKEY -d DESTINATION -s STARTNONCE -n NONCES -m RAM -t THREADS

for example: ./plot -k 12345678901234 -s 0 -n 3888000 -m 2048 -t 4

Solomining:
> ./mine localhost [plot_directory]

for example ./mine localhost /media/bananapi/externaldrive/plots/

Poolmining:
> ./mine_pool_all pooladdress:poolport [plot_directory]

for example: ./mine_pool_all burst.ninja:8124 /media/bananapi/externaldrive/plots/


NOTE: dcct tools are not writted by me, this is just a port to ARM, all rights for these developers.
