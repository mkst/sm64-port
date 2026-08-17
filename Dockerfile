FROM devkitpro/devkitarm:latest AS build
#FROM ubuntu:18.04 AS build

RUN apt-get update && \
    apt-get install -y \
      binutils-mips-linux-gnu \
      bsdmainutils \
      build-essential \
      libaudiofile-dev \
      pkg-config \
      python3 \
      wget \
      unzip \
      zlib1g-dev

#RUN wget https://github.com/devkitPro/pacman/releases/download/v1.0.2/devkitpro-pacman.amd64.deb \
#  -O devkitpro.deb && \
#  echo ebc9f199da9a685e5264c87578efe29309d5d90f44f99f3dad9dcd96323fece3 devkitpro.deb | sha256sum --check && \
#  apt install -y ./devkitpro.deb && \
#  rm devkitpro.deb
#RUN dkp-pacman -Syu 3ds-dev --noconfirm

RUN wget https://github.com/3DSGuy/Project_CTR/releases/download/makerom-v0.17/makerom-v0.17-ubuntu_x86_64.zip \
  -O makerom.zip && \
  echo 976c17a78617e157083a8e342836d35c47a45940f9d0209ee8fd210a81ba7bc0  makerom.zip | sha256sum --check && \
  unzip -d /opt/devkitpro/tools/bin/ makerom.zip && \
  chmod +x /opt/devkitpro/tools/bin/makerom && \
  rm makerom.zip

RUN mkdir /sm64
WORKDIR /sm64

ENV PATH="/opt/devkitpro/tools/bin/:/sm64/tools:${PATH}"
ENV DEVKITPRO=/opt/devkitpro
ENV DEVKITARM=/opt/devkitpro/devkitARM
ENV DEVKITPPC=/opt/devkitpro/devkitPPC

#CMD echo 'usage: docker run --rm --mount type=bind,source="$(pwd)",destination=/sm64 sm64 make VERSION=${VERSION:-us} -j4'
CMD ["sh", "-c", "echo 'usage: docker run --rm --mount type=bind,source=\"$(pwd)\",destination=/sm64 sm64 make VERSION=${VERSION:-us} -j4'"]
