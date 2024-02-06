FROM ubuntu:23.10 as builder
ARG BUILD_TYPE=Release
ENV DEBIAN_FRONTEND=noninteractive
RUN \
    dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y \
    cmake \
    ninja-build \
    g++-13 \
    python3-pip \
    gcc-multilib \
    g++-multilib \
    gettext \
    libstdc++-13-dev:i386
RUN pip3 install --break-system-packages --user conan==1.62.0
ENV PATH=~/.local/bin:${PATH}

COPY . /app
WORKDIR /app
RUN bash -c "conan profile new default --detect"
RUN mkdir build && cmake \
    -S . \
    -B build \
    -G Ninja \
    -DCMAKE_C_FLAGS=-m32 \
    -DCMAKE_CXX_FLAGS=-m32 \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE
RUN cmake \
    --build build \
    --config $BUILD_TYPE \
    --parallel $(nproc)
RUN make copy_output

FROM ubuntu:23.10
ENV DEBIAN_FRONTEND=noninteractive
ARG OPEN_MP_VERSION=1.2.0.2670
COPY --from=builder /app/server /app
COPY --from=builder /app/migrations /app/migrations
WORKDIR /app
RUN dpkg --add-architecture i386 && \
    apt update && \
    apt install -y git wget curl libc6:i386 libstdc++6:i386
RUN curl -s https://api.github.com/repos/Southclaws/sampctl/releases/latest | \
    grep "browser_download_url.*amd64\.deb" | \
    cut -d : -f 2,3 | \
    tr -d \" | \
    wget -qi - -O tmp.deb && \
    dpkg -i tmp.deb && \
    rm tmp.deb
RUN sampctl ensure && sampctl build
RUN wget -q https://github.com/openmultiplayer/open.mp/releases/download/v$OPEN_MP_VERSION/open.mp-linux-x86.tar.gz && \
    tar -xzf open.mp-linux-x86.tar.gz --strip-components=1 Server && \
    rm -f open.mp-linux-x86.tar.gz

# install migration tool
RUN wget -q https://github.com/golang-migrate/migrate/releases/download/v4.17.0/migrate.linux-amd64.deb -O tmp.deb && \
    dpkg -i tmp.deb && \
    rm tmp.deb

ENTRYPOINT [ "./omp-server" ]