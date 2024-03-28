ARG ALPINE_VERSION=3.19

FROM alpine:$ALPINE_VERSION as build
WORKDIR /root
RUN apk upgrade -a -U && apk add g++ patch cmake samurai libvirt-dev qt6-qtbase-dev qt6-qtdeclarative-dev qt6-qttools-dev

ARG CUTELEE_VERSION=6.1.0
RUN mkdir src && cd src && \
    wget -O cutelee.tar.gz https://github.com/cutelyst/cutelee/archive/refs/tags/v$CUTELEE_VERSION.tar.gz && \
    tar xf cutelee.tar.gz --strip-components 1 && \
    cmake -GNinja -B build -DCMAKE_INSTALL_PREFIX=/usr/local . && \
    cmake --build build && cmake --install build && \
    rm -rf /root/src /root/build

ARG CUTELYST_VERSION=4.2.1
RUN mkdir src && cd src && \
    wget -O cutelyst.tar.gz https://github.com/cutelyst/cutelyst/archive/refs/tags/v$CUTELYST_VERSION.tar.gz && \
    tar xf cutelyst.tar.gz --strip-components 1 && \
    cmake -GNinja -B build -DCMAKE_INSTALL_PREFIX=/usr/local -DPLUGIN_VIEW_CUTELEE=ON . && \
    cmake --build build && cmake --install build && \
    rm -rf /root/src /root/build

COPY . /root/src
RUN cd src && \
    cmake -GNinja -B build -DCMAKE_INSTALL_PREFIX=/usr/local . && \
    cmake --build build

FROM alpine:$ALPINE_VERSION

RUN set -ex && \
    apk upgrade -a -U && \
    apk add openssh libvirt qt6-qtbase qt6-qtdeclarative qt6-qtbase-sqlite && \
    rm -rf /var/cache/apk

COPY --from=build /usr/local /usr/local
COPY --from=build /root/src/root /var/www/root
COPY --from=build /root/src/build/src/libVirtlyst.so /usr/local/lib

CMD ["cutelystd4-qt6", "--application", "/usr/local/lib/libVirtlyst.so", "--chdir2", "/var/www", "--static-map", "/static=root/static", "--http-socket", "80", "--master", "--using-frontend-proxy"]
