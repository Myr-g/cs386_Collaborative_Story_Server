# Base image: Ubuntu with build tools
FROM ubuntu:22.04

# Install basic development tools
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    gdb \
    make \
    netcat \
    vim \
    bash \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code into container
COPY ./src ./src
COPY ./scripts ./scripts

# Compile the server (optional for now)
RUN gcc -pthread -o ./server ./src/server.c || true

# Default command
CMD ["bash"]
