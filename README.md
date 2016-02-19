## Burst ARM tools for Android and other Embedded Systems

## Guide: How to mine on your Android Phone:
Plotting:

    You need a Terminal for your phone – I recommend Termux for Android 5.x → https://termux.com/
    After starting Termux type: apt upgrade
    type apt update
    type apt install apt install git
    type git clone https://github.com/dawallet/burst_arm_tools
    type cd burst_arm_tools
    type cd dcct_tools_for_android
    type termux-setup-storage
    type cd bin
    type chmod 777 plot32
    type ./plot32 -k 1232353462354235 -d /storage/sdcard1/Android/data/com.termux/ -s 0 -n 10000 -m 1024 -t 4


Mining:

    type chmod 777 mine_pool_all32 (or mine32, mine_pool_share32 – depends which file you want to use)
    type ./mine_pool_all32 burst.ninja /storage/sdcard1/

The port is alpha, there are many bugs. Please share your experience.

The directory on SD Card have to be writeable and due to Androids permission policy it has to be the /com.termux/ directory!

Voilá.

____________________________________
Burst mining tools compiled for armhf processors for GNU/Linux and Android 

Howto: Clone the repository to your ARM system, run the binaries or  make your own.

> How to use dcct tools:

> ./plot -k YOURKEY -d DESTINATION -s STARTNONCE -n NONCES -m RAM -t THREADS

for example: ./plot32 -k 12345678901234 -s 0 -n 3888000 -m 4096 -t 4

means: Plotting around 1000 GB using 4 CPU-Threads, 1024 MB Ram. 

Solomining:
> ./mine localhost [plot_directory]

for example ./mine32 localhost /media/bananapi/externaldrive/plots/

Poolmining:
> ./mine_pool_all32 pooladdress:poolport [plot_directory]

for example: ./mine_pool_all burst.ninja:8124 /media/bananapi/externaldrive/plots/


NOTE: dcct tools are not writted by me, this is a port to ARM, all rights for these developers.
