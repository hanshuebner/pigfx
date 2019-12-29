# TODO List

## Fix double width/height cursor save & restore

This will propbably require tracking of the double width/height
status for each line so that the right cursor save size can be
saved and restored.  Reuse Glyph?

## Implement 132 columns mode

This will require reinitialization of the framebuffer to 1400x1050
pixels but should otherwise be straightforward.

## Keyboard application mode

## Keyboard autorepeat

## Settings

## Device reports

libvterm has hard-coded device reports in state.c (case 0x63:).  It
may make sense to replace those using a patch or a fork of libvterm so
that the terminal reports to be a VT220 instead of a VT100.

xterm can report its window size
(https://www.xfree86.org/current/ctlseqs.html, "Window
manipulation").  This could be a useful extension to allow setting of
the terminal size when it is not 80x24.

## Fix DMA scrolling

## Flow control

We want hardware and optional XON/XOFF flow control

