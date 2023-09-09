# sft
Simple FreeType attempted font rendering library in XCB, based on Freetype and Fontconfig.
Sft uses the same dependencies as Xft, but an XCB implementation for fonts that's perhaps based on that.

This repository was forked from joshuakraemer/sft.

There was an error in one of the includes.

When working on porting it, I was informed that this may be incomplete, at least the Makefile is.
Makefile has no real targets: its targets are for testing and cleaning.
There's a "test.o" in the Makefile, but no test.c.

joshuakraemer/sft lacks a README, so, you'll have to understand C and Makefiles to see what so far has been done, and what's needed.
If you want to make changes, fork the project.

Alternatively, https://github.com/venam/fonts-for-xcb or its forks may be one of the ports to use for XCB fonts.
https://github.com/ahauser31/fonts-for-xcb - A lot of code removed and rewritten for the intent of being simpler and closer to Xft. Code consists of 1 header file.
https://github.com/ss7m/fonts-for-xcb - conformed to c90, fixed freetype include, removed xcb-xrm dependency, and a few other minor changes.
