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

# compile drogon and drogon_ctl separately for caching
COPY vendored/ /app/vendored
COPY CMakeLists.txt /app/
RUN echo "int main() { return 0; }" >> main.cc
RUN mkdir build && \
    cd build && \
    cmake .. && \
    make drogon_ctl && \
    make drogon && \
    find . -name drogon_ctl -type f -exec cp {} /usr/bin \;

# compile normally
COPY . .
RUN cd build && cmake .. && make

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    libjsoncpp-dev \
    libsqlite3-dev \
    libchromaprint-tools
WORKDIR /app
COPY --from=builder /app/build/bbxxserver .

EXPOSE 80
CMD ["./bbxxserver"]