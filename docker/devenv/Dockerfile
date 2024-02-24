FROM open.mp/builder

USER root

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y --no-install-recommends git clangd wget gdb bash make ssh psmisc

# install sampctl
RUN curl -s https://api.github.com/repos/Southclaws/sampctl/releases/latest | \
    grep "browser_download_url.*amd64\.deb" | \
    cut -d : -f 2,3 | \
    tr -d \" | \
    wget -qi - -O tmp.deb && \
    dpkg -i tmp.deb && \
    rm tmp.deb

# install migrate
RUN wget -q https://github.com/golang-migrate/migrate/releases/download/v4.17.0/migrate.linux-amd64.deb -O tmp.deb && \
    dpkg -i tmp.deb && \
    rm tmp.deb

USER ubuntu

ENTRYPOINT ["/bin/bash", "-l", "-c"]