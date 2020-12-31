# ZiM 2.0 BETA codebase by Zsuzsu et al

This source code was found among [MudBytes archives](http://www.mudbytes.net/files/1635/) and published on GitHub for everyone to enjoy.

Original MUD this codebase was used for was called [Legends & Lore](http://www.topmudsites.com/forums/muddisplay.php?mudid=cozmo).

See [ZiM README](README.zim) for more details.

## Compile and run

I tweaked it slightly to compile with gcc 7 and run as a docker container. Quick steps:

* Clone this repo somewhere, e.g. /home/zim
* Build the docker image:
```bash
cd /home/zim
docker build . -t zim
```
* Run the container, mapping your `/home/zim` folder as a volume and exposing MUD ports 7777 (game), 7778 (info service):
```bash
docker run -v /home/margo/cpp/muds/zim:/home/zim -p 7777:7777 -p 7778:7778 --name zim -it zim
cd /home/zim/src
make
make run
```

ZiM MUD is now running on your localhost port 7777. Enjoy!


