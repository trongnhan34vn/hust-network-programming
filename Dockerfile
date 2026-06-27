FROM ubuntu:24.04

RUN apt update && apt install -y \
    build-essential \
    gdb \
    cmake \
    make \
    git \
    vim \
    net-tools \
    iproute2 \
    iputils-ping \
    tcpdump \
    curl \
    wget

WORKDIR /workspace

CMD ["tail", "-f", "/dev/null"]