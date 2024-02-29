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

## Building

### 1. Building C++ part with Docker

```bash
cd docker
docker compose build
make
```

You may need to set up some directories first:

```bash
mkdir build
mkdir conan
sudo chown 1000 build
sudo chown 1000 conan
```

The output is in `docker/build/`

#### VSCode Dev Container feature

This repository supports Dev Container feature. Use the `Dev Containers: Reopen in Container` command from the Command Palette (`F1`, `Ctrl+Shift+P`).

To build gamemode inside Dev Container environment, use `make` in the workspace root folder. Further use `SKIP_CMAKE` environment variable to prevent repeated CMake configuration process.

### 2. Building the server

You should have [sampctl](https://github.com/Southclaws/sampctl) installed.

Build the server strictly after building C++ part.

```bash
cd server
sampctl ensure
sampctl build
```

### 3. Configuring env vars

Copy `.env.example` in `server` folder and set specified env vars to your settings.

### 4. Run the server!

Go to `server` folder and type:

```bash
./samp03svr
```

If you are in VSCode Dev Container environment, you can use `make run` in the workspace root folder to run the server.

