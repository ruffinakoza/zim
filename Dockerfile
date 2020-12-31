FROM ubuntu:bionic as zim

# Install build and dev tools.
RUN apt-get update 
RUN apt-get install -y gcc make gcc-multilib g++-multilib git bc

CMD /bin/bash

