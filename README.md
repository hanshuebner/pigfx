# PiVT

## Raspberry Pi VT220 emulator

This project started off as a fork of https://github.com/scott5/pigfx
with the intent to carry over bug fixes from Filippo Bergamasco's
original version and make it emulate a Digital Equipment VT220
terminal.  In the process, all of the original terminal emulation code
has been removed and replaced by https://github.com/neovim/libvterm.
All new code has been written in C++.

This emulator tries to faithfully render the VT220 font which requires
10 by 20 pixels for each character.  See
https://vt100.net/dec/vt220/glyphs for a description of how that
works.

# License

The MIT License (MIT)

Copyright (c) 2016 Filippo Bergamasco.
Copyright (c) 2019, 2020 Hans Hübner

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
