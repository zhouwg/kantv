FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive 
ENV TZ=Asia/Shanghai

# Install 32-bit architecture support
RUN dpkg --add-architecture i386

# Update package lists and install required packages
RUN apt-get update && apt-get install -y \
    sudo \
    build-essential \
    cmake \
    curl \
    wget \
    python \
    tcl \
    expect \
    nginx \
    git \
    vim \
    spawn-fcgi \
    u-boot-tools \
    ffmpeg \
    openssh-client \
    nasm \
    yasm \
    openjdk-17-jdk \
    lib32z1 \
    android-tools-adb \
    android-tools-fastboot \
    autoconf \
    automake \
    bc \
    bison \
    ccache \
    cscope \
    curl \
    device-tree-compiler \
    expect \
    flex \
    ftp-upload \
    gdisk \
    acpica-tools \
    libattr1-dev \
    libcap-dev \
    libfdt-dev \
    libftdi-dev \
    libglib2.0-dev \
    libhidapi-dev \
    libncurses5-dev \
    libpixman-1-dev \
    libssl-dev \
    libtool \
    make \
    mtools \
    netcat \
    python-crypto \
    python3-crypto \
    python-pyelftools \
    python3-pycryptodome \
    python3-pyelftools \
    python3-serial \
    rsync \
    unzip \
    uuid-dev \
    xdg-utils \
    xterm \
    xz-utils \
    zlib1g-dev \
    python3-pip \
    indent \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*


ARG USER_ID
ARG GROUP_ID
ARG USER_NAME

RUN addgroup --gid $GROUP_ID $USER_NAME

RUN useradd -ms /bin/bash -u $USER_ID -g $GROUP_ID $USER_NAME
RUN usermod -aG sudo $USER_NAME

USER $USER_NAME
WORKDIR /home/$USER_NAME

RUN mkdir -p /home/$USER_NAME/kantv

RUN pip3 install meson ninja
RUN echo "export PATH=/home/`whoami`/.local/bin:\$PATH" >> ~/.bashrc

CMD ["/bin/bash"]
