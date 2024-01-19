FROM ubuntu:23.10
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
    libstdc++-13-dev:i386 \
    && \
    su ubuntu -c 'pip3 install --break-system-packages --user conan==1.62.0'

USER ubuntu

ENV PATH=~/.local/bin:${PATH}

COPY docker-entrypoint.sh /
CMD /docker-entrypoint.sh
