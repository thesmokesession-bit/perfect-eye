# Build environment for RADV (Mesa) and the PS5 title: Ubuntu 26.04 with LLVM 18 and 21.
FROM mirror.gcr.io/library/ubuntu:26.04
ENV DEBIAN_FRONTEND=noninteractive
COPY packages.txt /tmp/packages.txt
RUN apt-get update && apt-get install -y --no-install-recommends $(cat /tmp/packages.txt) sudo \
    && rm -rf /var/lib/apt/lists/*
