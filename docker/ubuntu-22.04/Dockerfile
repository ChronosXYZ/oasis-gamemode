FROM ubuntu:22.04
RUN \
    dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y \
    cmake \
    ninja-build \
    clang-11 \
    python3-pip \
    gcc-multilib \
    g++-multilib \
    libstdc++-11-dev:i386 \
    && \
    useradd -m user && \
    su user -c 'pip3 install --user conan==1.62.0'

USER user

ENV PATH=~/.local/bin:${PATH}

COPY docker-entrypoint.sh /
CMD /docker-entrypoint.sh
