# Super Mario 64 Wii/Gamecube Port

This repo does **not** include all assets necessary for compiling the game.
A prior copy of the game is required to extract the assets.

## Known Issues

This is a work-in-progress, while everything appears to be working fine and running at full speed, the game hasn't
been fully tested yet, although I do assume you could reasonably play through the entire game in the state that it's currently in.

## Building

Successful compilation will result in a `boot.dol` being created in `build/VERSION_GX/boot.dol` where `VERSION` is one of `us`, `eu`, `jp` or `sh`, and `GX` is either `wii` or `cube`.

Place the `boot.dol`, `meta.xml`, and `icon.png` from the above `build/VERSION_GX/boot.dol` in a directory called `/apps/sm64` on your SD card and run using the [Homebrew Channel](https://wiibrew.org/wiki/Homebrew_Channel).

**Supported Build Methods:**

- [Docker](#docker)
- [Linux / WSL (Ubuntu >= 18.04)](#linux--wsl-ubuntu)
- [Windows (MSYS2)](#windows-msys2)

### Docker
(Note this way of building did not work for me (PalindromicBreadLoaf), however both of the other two did. However, it is
possible that I just messed up.)
The following assumes a basic understanding of [Docker](https://www.docker.com/); if you do not belong to the `docker` group, prefix those commands with `sudo`.

**Clone Repository:**

```sh
git clone https://github.com/mkst/sm64-port.git --branch wii
```

**Navigate into freshly checked out repo:**

```sh
cd sm64-port
```

**Copy in baserom.XX.z64:**

```sh
cp /path/to/your/baserom.us.z64 ./ # change 'us' to 'eu', 'jp' or 'sh' as appropriate
```

**Build with pre-baked image:**

Change `VERSION=us` if applicable. If on Windows replace `$(pwd):/sm64` with the path to the current directory, e.g. `"C:\path\to\sm64-port:/sm64"`.
```sh
docker run --rm -v $(pwd):/sm64 markstreet/sm64:wii make VERSION=us --jobs 4 # Linux/OSX
```

### Linux / WSL (Ubuntu)

This works on **Ubuntu >= 18.04**\
This can be done on any Linux distro, just substitute the packages for your distro's equivalent.

```sh
sudo -i

apt-get update && \
    apt-get install -y \
        binutils-mips-linux-gnu \
        bsdmainutils \
        build-essential \
        libaudiofile-dev \
        pkg-config \
        python3 \
        wget \
        zlib1g-dev

wget https://github.com/devkitPro/pacman/releases/download/v1.0.2/devkitpro-pacman.amd64.deb \
  -O devkitpro.deb && \
  echo ebc9f199da9a685e5264c87578efe29309d5d90f44f99f3dad9dcd96323fece3 devkitpro.deb | sha256sum --check && \
  apt install -y ./devkitpro.deb && \
  rm devkitpro.deb

dkp-pacman -Syu wii-dev gamecube-dev --noconfirm
# if this ^^ fails with error about archive format, use a VPN to get yourself out of the USA and then try again.

exit

cd

git clone https://github.com/mkst/sm64-port.git --branch wii

cd sm64-port

# Go and copy the baserom to c:\temp (create that directory in Windows Explorer)
# If you are running Linux natively, just copy the baserom.us.z64 file to the current directory. No need to create anything.
cp /mnt/c/temp/baserom.us.z64 ./

sudo chmod 644 ./baserom.us.z64

export PATH="/opt/devkitpro/tools/bin/:~/sm64-port/tools:${PATH}"
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=/opt/devkitpro/devkitPPC

make -j4
```

### Windows (MSYS2)

WSL is the preferred route, but you can also use MSYS2 (MINGW64) to compile.

For each instruction copy and paste the contents into the **MINGW64** console.

**Get MSYS2:**

Navigate to https://www.msys2.org/ and download the installer.

**Install and Run MINGW64:**

```
Next, Next, Next, Finish (keep the box checked to "Run MSYS 64bit now").
```

**Add keyserver for package validation:**

```sh
pacman-key --recv BC26F752D25B92CE272E0F44F7FD5492264BB9D0 --keyserver keyserver.ubuntu.com
pacman-key --lsign BC26F752D25B92CE272E0F44F7FD5492264BB9D0
```

**Add DevKitPro keyring:**

```sh
pacman -U --noconfirm https://downloads.devkitpro.org/devkitpro-keyring.pkg.tar.xz
```

**Add DevKitPro package repositories:**

```sh
cat <<EOF >> /etc/pacman.conf
[dkp-libs]
Server = https://downloads.devkitpro.org/packages
[dkp-windows]
Server = https://downloads.devkitpro.org/packages/windows
EOF
```

**Update dependencies:**

```sh
pacman -Syu --noconfirm
```

MINGW64 may close itself when done, if it does, find `MSYS2 MinGW 64bit` in your Start Menu and open again.

**Install Dependencies:**

```sh
pacman -S wii-dev git make python3 mingw-w64-x86_64-gcc --noconfirm
```

**Setup Environment Variables:**

```sh
export PATH=$PATH:/opt/devkitpro/tools/bin && echo "OK!"
export DEVKITPRO=/opt/devkitpro && echo "OK!"
export DEVKITPPC=/opt/devkitpro/devkitPPC && echo "OK!"
```

**Clone Repository:**

```sh
git clone https://github.com/mkst/sm64-port.git --branch wii
```

**Navigate into freshly checked out repo:**

```sh
cd sm64-port && echo "OK!"
```

**Copy in baserom.XX.z64:**

This assumes that you have create the directory `c:\temp` via Windows Explorer and copied the Super Mario 64 `baserom.XX.z64` to it.
```sh
cp /c/temp/baserom.us.z64 ./ && echo "OK!" # change 'us' to 'eu', 'jp' or 'sh' as appropriate
```

**Compile:**

```sh
make VERSION=us --jobs 4 # change 'us' to 'eu', 'jp' or 'sh' as appropriate
```

### Other Operating Systems

TBD; feel free to submit a PR.

## Project Structure

    sm64
    ├── actors: object behaviors, geo layout, and display lists
    ├── asm: handwritten assembly code, rom header
    │   └── non_matchings: asm for non-matching sections
    ├── assets: animation and demo data
    │   ├── anims: animation data
    │   └── demos: demo data
    ├── bin: C files for ordering display lists and textures
    ├── build: output directory
    ├── data: behavior scripts, misc. data
    ├── doxygen: documentation infrastructure
    ├── enhancements: example source modifications
    ├── include: header files
    ├── levels: level scripts, geo layout, and display lists
    ├── lib: SDK library code
    ├── rsp: audio and Fast3D RSP assembly code
    ├── sound: sequences, sound samples, and sound banks
    ├── src: C source code for game
    │   ├── audio: audio code
    │   ├── buffers: stacks, heaps, and task buffers
    │   ├── engine: script processing engines and utils
    │   ├── game: behaviors and rest of game source
    │   ├── goddard: Mario intro screen
    │   ├── menu: title screen and file, act, and debug level selection menus
    │   └── pc: port code, audio and video renderer
    ├── text: dialog, level names, act names
    ├── textures: skybox and generic texture data
    └── tools: build tools

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.
