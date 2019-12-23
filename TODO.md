# TODO List

## Fix double width/height cursor save & restore

This will propbably require tracking of the double width/height
status for each line so that the right cursor save size can be
saved and restored.  Reuse Glyph?

## Fix UART IRQ handling

It seems that currently, UART interrupts received during screen
processing are not correctly processed.  Maybe interrupts are not
processed at all?  Removing `uart_fill_queue` seems to be required,
but why?

## Implement 132 columns mode

This will require reinitialization of the framebuffer to 1400x1050
pixels but should otherwise be straightforward.

## Keyboard application mode

## Settings

## Attributes

Implement underline and frieds

## Character sets

It seems that graphics characters are indicated by another character
set in the cell attributes.
