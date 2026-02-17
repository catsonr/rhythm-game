FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    # libraries
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    libsqlite3-dev
WORKDIR /app
COPY . .
RUN mkdir build && cd build && cmake .. && make

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    libjsoncpp-dev \
    libsqlite3-dev \
    libchromaprint-tools
WORKDIR /app
COPY --from=builder /app/build/bbxxserver .

EXPOSE 80
CMD ["./bbxxserver"]