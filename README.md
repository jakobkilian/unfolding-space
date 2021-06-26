

This branch reimagines some of the functionality in `develop` with less locking 
and more clearly defined interface boundaries. The main 'release' code for user 
testing lives in `develop`.

We may decide to move some functionality from here to `develop` or try to
achieve feature parity in this branch or just keep it as a playground to sketch out ideas.

# Building

- create a symlink to the libroyale stuff you want to compile against:

      $ ln -s ../path/to/unpacked/unpacked/libroyale-PLATTFORM-x86/ libroyale

- `make`

# Running

      $ LD_LIBRARY_PATH=./libroyale/bin ./unfolding-app


# TODO

- Motorcontrol (most likely based on PIGIO)
- Unit Testing
  - tools to "draw" frames in bitmap, import them and run the algorithms against the frames
- plugable mechanism to switch out mapping mechanism from frame to motor values
- UDP stuff
