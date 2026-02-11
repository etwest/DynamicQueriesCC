# build
FROM ubuntu:24.04 AS build
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential \
      ca-certificates \
      cmake \
      git \
      sudo \
      gnupg \
    #   libboost-context-dev \
    #   libboost-filesystem-dev \
    #   libboost-program-options-dev \
    #   libboost-system-dev \
    #   libboost-thread-dev \
      libboost-all-dev \
      libdouble-conversion-dev \
      libfast-float-dev \
      libevent-dev \
      libfmt-dev \
      libgflags-dev \
      libgoogle-glog-dev \
      libjemalloc-dev \
      libmimalloc-dev \
      libssl-dev \
      libunwind-dev \
      libzstd-dev \
      ninja-build \
      openmpi-bin \
      libopenmpi-dev \
      python3 \
      python3-venv \
      wget \
      libtbb-dev \
 && rm -rf /var/lib/apt/lists/*
 

WORKDIR /opt/deps

# install the latest version of fast-float

# TODO - checkout a specific tag/release
RUN git clone https://github.com/fastfloat/fast_float.git \
 && cd fast_float \
 && cmake -B build -DFASTFLOAT_TEST=OFF \
 && sudo cmake --build build --target install
# build folly + install dependencies we may have missed
# TODO - would be good to enumerate them
# RUN git clone https://github.com/facebook/folly.git
# WORKDIR /opt/deps/folly
# RUN python3 ./build/fbcode_builder/getdeps.py install-system-deps --recursive
# ENV FOLLY_PREFIX=/opt/deps/folly/_build/opt/facebook
# ENV CMAKE_PREFIX_PATH=${FOLLY_PREFIX}:${CMAKE_PREFIX_PATH}


# build DynamicQueriesCC in Release
WORKDIR /opt/dynamiccc
COPY . .
# remove old build director
RUN rm -rf build
RUN cmake -S . -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=${FOLLY_PREFIX} \
      -DSKETCH_BUFFER_SIZE=5 \
      -DPARLAY_TBB=On \
      
 && cmake --build build --target \
      dynamicCC_tests \
      mpi_dynamicCC_tests \
      hybrid_mpi_dynamicCC_tests \
      hybrid_shmem_dynamicCC_tests \
      -j "$(nproc)"
      

# ------------------------------

# runtime
FROM ubuntu:24.04 AS runtime
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates \
      git \
      libboost-context-dev \
      libboost-filesystem-dev \
      libboost-program-options-dev \
      libboost-system-dev \
      libboost-thread-dev \
      libdouble-conversion-dev \
      libevent-dev \
      libfmt-dev \
      libgflags-dev \
      libgoogle-glog-dev \
      libjemalloc-dev \
      libmimalloc-dev \
      libssl-dev \
      libunwind-dev \
      libzstd-dev \
      openmpi-bin \
      libopenmpi-dev \
      python3 \
      wget \
      libtbb-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/dynamiccc

# project binaries and libs
COPY --from=build /opt/dynamiccc/build/*tests /opt/dynamiccc/bin/
COPY scripts ./scripts

ENV PATH="/opt/dynamiccc/bin:${PATH}"
ENV LD_LIBRARY_PATH="/opt/dynamiccc/lib:${LD_LIBRARY_PATH}"


ENTRYPOINT ["/bin/bash"]