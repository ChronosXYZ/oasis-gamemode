# Oasis

<p align="center">
  <a aria-label="Oasis Freeroam" href="https://oasisfreeroam.xyz">
    <img src="https://cdn.discordapp.com/attachments/1089521368699240528/1092068205071188098/5.png" width="420" />
  </a>
</p>

<p align="center">
  <em>A freeroam multi-mode server for SA-MP.</em>
</p>
												
<p align="center">
  <a href="https://discord.gg/T7UZYMEqSk">
  <img alt="Discord" src="https://img.shields.io/discord/1081202778245976124?logo=discord&logoColor=fff&label=%5Bopen.mp%5D%20Oasis%20Freeroam&labelColor=f00&color=5865F2">
  </a>

## Building with Docker

```bash
cd docker
docker compose build
docker compose run --rm builder
```

You may need to set up some directories first:

```bash
mkdir build
mkdir conan
sudo chown 1000 build
sudo chown 1000 conan
```

The output is in `docker/build/`

