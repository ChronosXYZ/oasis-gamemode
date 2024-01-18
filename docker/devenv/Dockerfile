FROM open.mp/builder

USER root

RUN DEBIAN_FRONTEND=noninteractive apt update && apt install -y --no-install-recommends git clangd wget gdb bash make
RUN curl -s https://api.github.com/repos/Southclaws/sampctl/releases/latest | \
    grep "browser_download_url.*amd64\.deb" | \
    cut -d : -f 2,3 | \
    tr -d \" | \
    wget -qi - -O tmp.deb && \
    dpkg -i tmp.deb && \
    rm tmp.deb

USER user

ENTRYPOINT ["/bin/bash", "-l", "-c"]