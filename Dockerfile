FROM debian:8.6 as builder

MAINTAINER Guy John <patchwerk@rumblesan.com>

RUN apt-get update
RUN apt-get install -y clang cmake make git

RUN apt-get install -y libshout3-dev libconfig-dev libvorbis-dev libsndfile-dev libck-dev
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


FROM debian:jessie-slim

RUN apt-get update
RUN apt-get install -y libshout3 libconfig9 libvorbis-dev libsndfile1 libck-dev
RUN mkdir -p /opt/patchwerk

WORKDIR /opt/patchwerk

COPY --from=builder /opt/patchwerk/build/main/patchwerk /usr/local/bin/
COPY patches /opt/patchwerk/patches

# this feels sketchy...
COPY --from=builder /usr/local/lib/libpd.so /usr/local/lib/
COPY radio.cfg /opt/patchwerk/

CMD ["patchwerk", "/opt/patchwerk/radio.cfg"]
