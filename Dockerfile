FROM arm64v8/ubuntu:latest


RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y locales && rm -rf /var/lib/apt/lists/* \
    && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8
ENV LANG en_US.utf8

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    wget \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    make \
    git \
    curl

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential checkinstall zlib1g-dev
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -y install libcurl4-openssl-dev gettext libtool-bin libogg-dev doxygen libxml2-utils w3c-sgml-lib;

WORKDIR /usr/deps

RUN wget --no-check-certificate https://cmake.org/files/v3.17/cmake-3.17.0.tar.gz
RUN tar -xvf cmake-3.17.0.tar.gz

WORKDIR /usr/deps/cmake-3.17.0
RUN pwd
RUN ./bootstrap -- -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_USE_OPENSSL=OFF
RUN make
RUN make install

RUN useradd --user-group --system --create-home --shell /bin/bash userflac
USER userflac

WORKDIR /home/userflac