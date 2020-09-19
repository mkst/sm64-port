# Super Mario 64 Nintendo 3DS Port

This repo does **not** include all assets necessary for compiling the game.
A prior copy of the game is required to extract the assets.

## Changes vs. Vanilla 3DS Port

 - Based off [Refresh 11](https://github.com/sm64-port/sm64-port/commit/9214dddabcce4723d9b6cda2ebccbac209f6447d)
 - Multi-threaded; audio thread runs on Core 1 on O3DS and Core 2 on N3DS; needs [Luma v10.1.1](https://github.com/LumaTeam/Luma3DS/releases) or higher
 - Naive frame-skip if frame takes longer than 33.3ms (1 / 30 FPS) to render
     - Disable by building with `DISABLE_N3DS_FRAMESKIP=1`
 - Configurable controls via `sm64config.txt`
     - Use [this](https://codepen.io/benoitcaron/full/abNZrbP) online editor from [BenoitCaron](https://github.com/BenoitCaron).
 - GFX_POOL_SIZE [fix](https://github.com/aboood40091/sm64-port/commit/6ae4f4687ed234291ac1e572b75d65191ca9f364) (support 60 FPS on 32bit platforms)
 - Mini-menu (tap touch-screen to trigger)
     - Enable/disable AA
     - Enable/disable 800px mode
     - Exit the game
 - Experimental Stereo 3D support; add build flag `ENABLE_N3DS_3D_MODE=1` to try it out
 - Support injection of [SMDH](https://www.3dbrew.org/wiki/SMDH) file into the .3dsx
     - Change the `icon.png` in the base of this repository before building.
 - Patches updated for 3DS:
     - [60 FPS](enhancements/60fps.patch)
     - [Puppycam](enhancements/puppycam.patch)
     - [Show FPS](enhancements/fps.patch)

## Building

After building, either install the `.cia` if you made one, or copy over the `sm64.us.f3dex2e.3dsx` into the `/3ds` directory on your SD card and load via [The Homebrew Launcher](https://smealum.github.io/3ds/).

  - [Docker](#docker)
  - [Linux / WSL (Ubuntu 18.04 or higher)](#linux--wsl-ubuntu)
  - [Windows (MSYS2)](#windows-msys2)

Visit the [wiki](https://github.com/mkst/sm64-port/wiki) for information on converting the `.3dsx` to `.cia` via WSL/MSYS2.

### Docker

The following assumes a basic understanding of [Docker](https://www.docker.com/); if you do not belong to the `docker` group, prefix those commands with `sudo`.

**Clone Repository:**

```sh
git clone https://github.com/mkst/sm64-port.git
```

**Navigate into freshly checked out repo:**

```sh
cd sm64-port
```

**Copy in baserom.XX.z64:**

```sh
cp /path/to/your/baserom.us.z64 ./ # change 'us' to 'eu', 'jp' or 'sh' as appropriate
```

**Checkout this branch:**

```sh
git checkout 3ds-port
```

**Build with pre-baked image:**

Change `VERSION=us` if applicable.
```sh
docker run --rm -v $(pwd):/sm64  \
  markstreet/sm64:3ds \
  make VERSION=us --jobs 4
```

**Create .cia from .3dsx (Optional):**

```sh
docker run --rm -v $(pwd):/data \
  markstreet/3dstools:0.1 \
  sh -c "cxitool build/us_3ds/sm64.us.f3dex2e.3dsx sm64.cxi && \
  makerom -f cia -o sm64.cia -target t -i sm64.cxi:0:0 -ignoresign"
```

### Linux / WSL (Ubuntu)

Tested successfully on **Ubuntu 18.04** and **20.04**. Does not work on **16.04**.

```sh
sudo su -

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

dkp-pacman -Syu 3ds-dev --noconfirm
# if this ^^ fails with error about archive format, use a VPN to get yourself out of the USA and then try again.

exit

cd

git clone https://github.com/mkst/sm64-port.git

cd sm64-port

git checkout 3ds-port

# go and copy the baserom to c:\temp (create that directory in Windows Explorer)
cp /mnt/c/temp/baserom.us.z64 ./

sudo chmod 644 ./baserom.us.z64

export PATH="/opt/devkitpro/tools/bin/:~/sm64-port/tools:${PATH}"
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
export DEVKITPPC=/opt/devkitpro/devkitPPC

make -j4
```

### Windows (MSYS2)

WSL is the preferred route, but you can also use MSYS2 to compile.

**Get MSYS2:**

Navigate to https://www.msys2.org/ and download the installer.

**Install and Run MSYS2:**

```
Next, Next, Next, Finish (keep the box checked to run now).
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

MSYS2 may close itself when done, if it does, find `MSYS2 MinGW 64bit` in your Start Menu and open again.

**Install Dependencies:**

```sh
pacman -S 3ds-dev git make python3 mingw-w64-x86_64-gcc --noconfirm
```

**Setup Environment Variables:**

```sh
export PATH=$PATH:/opt/devkitpro/tools/bin
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
export DEVKITPPC=/opt/devkitpro/devkitPPC
```

**Clone Repository:**

```sh
git clone https://github.com/mkst/sm64-port.git
```

**Navigate into freshly checked out repo:**

```sh
cd sm64-port
```

**Copy in baserom.XX.z64:**

This assumes that you have create the directory `c:\temp` via Windows Explorer and copied the `baserom.XX.z64` to it.
```sh
cp /c/temp/baserom.us.z64 ./ # change 'us' to 'eu', 'jp' or 'sh' as appropriate
```

**Checkout this branch:**

```sh
git checkout 3ds-port
```

**Compile:**

```sh
make VERSION=us --jobs 4 # change 'us' to 'eu', 'jp' or 'sh' as appropriate
```

### Other Operating Systems

TBD; feel free to submit a PR.

## Project Structure

```
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
```

## Credits

 - Credits go to [Gericom](https://github.com/Gericom) for the [sm64_3ds](https://github.com/sm64-port/sm64_3ds) port that this flavour is based off.

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.
