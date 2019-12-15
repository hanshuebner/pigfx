# PiGFX 
## Raspberry Pi graphics card / VT220 emulator

This is a fork of https://github.com/scott5/pigfx with changes to make
it emulate a VT220 more faithfully.

The bug fixes from scott5's version have been carried over, but
functionality that is not compatible with the VT220 have been
removed.  Also, the low-level optimizations that required fonts to be
always 8 pixels wide have been replaced by straightforward and
possibly slower code that allows "arbitrary" font dimensions.  This is
to support faithful rendering of the VT220 font which requires 10 by
20 pixels for each character.  See https://vt100.net/dec/vt220/glyphs
for a description of how that works.

# License

The MIT License (MIT)

Copyright (c) 2016 Filippo Bergamasco.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
