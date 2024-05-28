FROM ubuntu:18.04

RUN useradd -u 1000 -U -m -c Jenkins jenkins

RUN apt update && apt -y upgrade \
  && apt install -y \
    software-properties-common \
    clang \
    clang++-9 \
    gcc \
    g++ \
    git \
    imagemagick \
    make \
    python3 \
    python3-pip \
    python3-venv \
    python3-numpy \
    python3-pil \
    ca-certificates \
    gnupg \
    wget \
  && rm -rf /var/lib/apt/lists/*

# Install python modules
RUN pip3 install requests

# Install Coverity static analysis tools
COPY coverity_* /tmp/
RUN chmod 555 /tmp/coverity_install.sh && \
  /tmp/coverity_install.sh -q --license.region=6 --license.agreement=agree --license.cov.path=/tmp/coverity_license.dat -dir /usr/local/cov-analysis && \
  rm /tmp/coverity_*
ENV PATH="/usr/local/cov-analysis/bin:$PATH"

# Install up-to-date CMake, as standard Ubuntu 18.04 package is too old
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null \
    | gpg --dearmor - > /etc/apt/trusted.gpg.d/kitware.gpg
RUN apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
RUN apt-get update
RUN apt-get install -y cmake