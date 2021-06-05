# Unfolding Space - python edition

This branch contains a prototype of the functionality of the
`develop` branch. The main differences are:

- not full functionality
- fewer locks
- implemented in Python not C++

The intention of this branch is to assess whether a pure python solution
provides sufficient performance to render the depth data in perceived
real time.

## Installation

Currently WIP, see RANTS for all the fun stuff that needs to be
installed and whatever other hoops are necessary to get things to run.

## Testing

The Python code allows usage with a virtual camera that is fed using
`rrf` files. In case you have no camera at your disposal, a sample rrf
file containing 100 frames (60+MB!) of my stupid face and hands is
available for download here:

	https://github.com/jakobkilian/unfolding-space-private/releases/download/pseudo_rrf/test.rrf

## LAZY
	$ . royaleenv/bin/activate
	$ PYTHONPATH= python unfolding.py
