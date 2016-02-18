## Burst ARM tools for Android and other Embedded Systems

Burst mining tools compiled for armhf processors for GNU/Linux and Android 

Howto: Clone the repository to your ARM system, run the binaries or  make your own.

> How to use dcct tools:

> ./plot -k YOURKEY -d DESTINATION -s STARTNONCE -n NONCES -m RAM -t THREADS

for example: ./plot -k 12345678901234 -s 0 -n 3888000 -m 4096 -t 4

means: Plotting around 1000 GB using 4 CPU-Threads, 1024 MB Ram. 

Solomining:
> ./mine localhost [plot_directory]

for example ./mine localhost /media/bananapi/externaldrive/plots/

Poolmining:
> ./mine_pool_all pooladdress:poolport [plot_directory]

for example: ./mine_pool_all burst.ninja:8124 /media/bananapi/externaldrive/plots/


NOTE: dcct tools are not writted by me, this is just a port to ARM, all rights for these developers.
