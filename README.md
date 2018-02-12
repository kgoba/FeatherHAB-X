# FeatherHAB-X

Firmware of [FeatherHAB project](http://protofusion.org/wordpress/2015/10/featherhab-2-gram-high-altitude-balloon-tracker/) by Ethan Zonca with the original code [here](http://hg.protofusion.org/featherhab-firmware/). 

This can be considered a fork and a derivative work. I will keep modifying the firmware and adding features.

## License

The original licensing terms hold (GNU Affero Public License).

## Building

Prerequisites:
- GNU ARM compiler (https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads)

After cloning the repository, update the submodules with `git submodule init` and `git submodule update` commands.

Then change to the `firmware` directory and run `make`.
