# this dockerfile is to help m1 mac users compile to linux

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y build-essential python3 scons git && rm -rf /var/lib/apt/lists/*

WORKDIR /project

CMD ["scons", "platform=linux"]