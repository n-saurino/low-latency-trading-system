# Use Ubuntu 22.04 base image for ARM64
FROM arm64v8/ubuntu:22.04

# Set the working directory
WORKDIR /workspace

# Set environment variables to make the build non-interactive
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Chicago

# Install essential tools, GCC 11.3.0, CMake 3.23.2, and Ninja 1.10.2
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    software-properties-common \
    build-essential \
    wget \
    curl \
    git \
    vim \
    unzip \
    gdb && \
    apt-get install -y \
    gcc-11 g++-11 libstdc++-11-dev && \
    rm -rf /var/lib/apt/lists/*

# Set GCC and G++ alternatives to GCC 11
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100

RUN wget https://github.com/Kitware/CMake/releases/download/v3.23.2/cmake-3.23.2-linux-aarch64.tar.gz && \
    tar -xzf cmake-3.23.2-linux-aarch64.tar.gz && \
    rm -rf /usr/local/man && \
    cp -r cmake-3.23.2-linux-aarch64/* /usr/local/ && \
    rm -rf cmake-3.23.2-linux-aarch64 cmake-3.23.2-linux-aarch64.tar.gz

# Install Ninja 1.10.2
RUN wget https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-linux.zip && \
    unzip ninja-linux.zip -d /usr/local/bin/ && \
    chmod +x /usr/local/bin/ninja && \
    rm ninja-linux.zip

# Copy project files into the container
COPY . /workspace

# Add explicit compiler flags to enable exceptions
RUN export CXXFLAGS="-fexceptions" && \
    export CFLAGS="-fexceptions"

# Default command to keep the container running
CMD ["/bin/bash"]
