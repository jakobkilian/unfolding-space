

# Building

- create a symlink to the libroyale stuff you want to compile against:

	$ ln -s ../path/to/unpacked/unpacked/libroyale-PLATTFORM-x86/ libroyale

- `make`

# Running

	$ LD_LIBRARY_PATH=./libroyale/bin ./unfolding-app

# DEPENDENCIES

apt install libboost-program-options-dev
