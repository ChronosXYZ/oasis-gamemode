
FROM arm64v8/ubuntu:23.10 as base_builder

ENV DEBIAN_FRONTEND=noninteractive

RUN sed 's/^deb http/deb [arch=arm64] http/' -i '/etc/apt/sources.list'

RUN echo "deb [arch=i386] http://security.ubuntu.com/ubuntu/ mantic-security main restricted universe multiverse" >> /etc/apt/sources.list.d/i386.list \
    && echo "deb [arch=i386] http://archive.ubuntu.com/ubuntu/ mantic main restricted universe multiverse" >> /etc/apt/sources.list.d/i386.list \
    && echo "deb [arch=i386] http://archive.ubuntu.com/ubuntu/ mantic-updates main restricted universe multiverse" >> /etc/apt/sources.list.d/i386.list \
    && echo "deb [arch=i386] http://archive.ubuntu.com/ubuntu/ mantic-backports main restricted universe multiverse" >> /etc/apt/sources.list.d/i386.list

RUN dpkg --add-architecture i386 && \
    apt update && \
    apt install -y \
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


# vscode devcontainers image
FROM base_builder AS devenv

USER root

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y --no-install-recommends git clangd wget gdb bash make ssh psmisc

# install sampctl
RUN curl -s https://api.github.com/repos/Southclaws/sampctl/releases/latest | \
    grep "browser_download_url.*arm64\.deb" | \
    cut -d : -f 2,3 | \
    tr -d \" | \
    wget -qi - -O tmp.deb && \
    dpkg -i tmp.deb && \
    rm tmp.deb

# install migrate
RUN wget -q https://github.com/golang-migrate/migrate/releases/download/v4.17.0/migrate.linux-arm64.deb -O tmp.deb && \
    dpkg -i tmp.deb && \
    rm tmp.deb

USER ubuntu

ENTRYPOINT ["/bin/bash", "-l", "-c"]
