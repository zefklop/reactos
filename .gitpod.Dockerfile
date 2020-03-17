FROM gitpod/workspace-full:latest

USER gitpod

# Install custom tools, runtime, etc. using apt-get
# For example, the command below would install "bastet" - a command line tetris clone:
#
# RUN sudo apt-get -q update && \
#     sudo apt-get install -yq bastet && \
#     sudo rm -rf /var/lib/apt/lists/*
#
# More information: https://www.gitpod.io/docs/config-docker/
RUN sudo apt-get -q update && \
    sudo apt-get -yq install wget && \
    sudo rm -rf /var/lib/apt/lists/*

RUN wget http://downloads.sourceforge.net/reactos/RosBE-Unix-2.1.2.tar.bz2 && \
    tar -xjf RosBE-Unix-2.1.2.tar.bz2 && \
    cd RosBE-Unix-2.1.2 && \
    sudo ./RosBE-Builder.sh /usr/local/RosBE && \
    cd .. && \
    rm -rf RosBE-Unix-2.1.2
