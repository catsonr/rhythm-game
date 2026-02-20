# BEATBOXX

your free, open source, platform-agnostic rhythm game!
made with godot \<3

## about

BEATBOXX is written in c++ using godot's godot-cpp bindings
there is also bbxxserver which both acts as a backend for BEATBOXX, as well as hosts as public web website at [beatboxx.org](http://beatboxx.org/)!

**NOTE**: in its current state, bbxxserver will only be live if my (catsonr) laptop is open

### compiling BEATBOXX

simply run `scons`

#### running BEATBOXX

after compiling with scons, there will now be some `libbeatboxx` in `rhyhm-game/bin` (depending on your platform).

`rhythm-game` is a regular ol' godot project, so simply run with the godot editor!

### compiling bbxxserver

you can manually compile bbxxserver with the following commands:

`cd bbxxserver`
`mkdir build`
`cd build`
`cmake ..`
`make`

this creates a `bbxxserver` executable inside of `build/`.

however manually building is no longer supported (though still totally do-able), in favor of running with docker

#### running bbxxserver

simply run `docker-compose up --build`

**NOTE**: docker is expecting `bbxxserver/.env` to contain `TUNNEL_TOKEN`, which is used for tunnel any requests from beatboxx.org to the localhost instance of beatboxx thanks to `cloudflared`
