FROM debian:stable as builder

MAINTAINER Guy John <patchwerk@rumblesan.com>

RUN apt-get update
RUN apt-get install -y clang cmake make git

RUN apt-get install -y libshout3-dev libconfig-dev libvorbis-dev libsndfile-dev libck-dev liblua5.4-dev
ENV CC /usr/bin/clang

RUN git clone https://github.com/rumblesan/bclib.git /opt/bclib
RUN cd /opt/bclib/build && cmake .. && make && make install

RUN git clone --recurse-submodules https://github.com/libpd/libpd.git /opt/libpd
RUN cd /opt/libpd && make && make install

RUN mkdir -p /opt/patchwerk/build
COPY CMakeLists.txt /opt/patchwerk
COPY libpatchwerk /opt/patchwerk/libpatchwerk
COPY main /opt/patchwerk/main
COPY tests /opt/patchwerk/tests

WORKDIR /opt/patchwerk
RUN cd build; cmake ..; make


FROM debian:stable

RUN apt-get update
RUN apt-get install -y git make
RUN apt-get install -y libshout3 libconfig9 libvorbis-dev libsndfile1 libck-dev libssl-dev
RUN apt-get install -y liblua5.4-dev luarocks

RUN mkdir -p /opt/patchwerk
RUN mkdir -p /opt/patchwerk/cfg
RUN mkdir -p /opt/patchwerk/patches
RUN mkdir -p /opt/patchwerk/lua

RUN luarocks --lua-version=5.4 install milua CRYPTO_DIR=/usr
RUN luarocks --lua-version=5.4 install luafilesystem

WORKDIR /opt/patchwerk

COPY --from=builder /opt/patchwerk/build/main/patchwerk /usr/local/bin/
COPY patches /opt/patchwerk/patches

# this feels sketchy...
COPY --from=builder /usr/local/lib/libpd.so /usr/local/lib/
COPY cfg /opt/patchwerk/cfg
COPY lua /opt/patchwerk/lua

CMD ["patchwerk", "/opt/patchwerk/cfg/radio.cfg"]
