.section .rodata

.global G_FONT_GLYPHS08
.global G_FONT_GLYPHS14
.global G_FONT_GLYPHS16
.global G_FONT_GLYPHS20
.global G_FONT_GLYPHS
.align 4
G_FONT_GLYPHS08: .incbin "fonts/font8x8.bin"
G_FONT_GLYPHS14: .incbin "fonts/font8x14.bin"
G_FONT_GLYPHS16: .incbin "fonts/font8x16.bin"
G_FONT_GLYPHS20: .incbin "fonts/font10x20.bin"

;@ if using a FONT_HEIGHT other than 8/14/16 in pigfx_config.h then
;@ must declare G_FONT_GLYPHS with the corresponding font data here:
;@ G_FONT_GLYPHS: .incbin "fonts/font8xXX.bin"
