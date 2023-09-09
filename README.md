# sft
Simple FreeType font rendering library in XCB, based on Freetype and Fontconfig.
Sft uses the same dependencies as Xft, but an XCB implementation for fonts that's perhaps based on that.

This repository was forked from joshuakraemer/sft.

There was an error in one of the includes.

When working on porting it, I was informed that this may be incomplete, at least the Makefile is.
Makefile has no real targets: its targets are for testing and cleaning.
There's a "test.o" in the Makefile, but no test.c.

joshuakraemer/sft lacks a README, so, you'll have to understand C and Makefiles to see what so far has been done, and what's needed.
If you want to make changes, fork the project.

Alternatively, https://github.com/venam/fonts-for-xcb may be the port to use for fonts.
