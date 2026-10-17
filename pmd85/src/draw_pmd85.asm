;; from draw.c
EXTERN _xpos, _ypos, _video_ptr, _video_bit, _color_attr, _invert_mask

;; from font.asm
EXTERN _font_data

PUBLIC _draw_char
PUBLIC _draw_char_double
PUBLIC _draw_bitmap
PUBLIC _clear_part_asm
PUBLIC fputc_cons_none

;----------------------------------------------------------------------------

;; video ram
defc    DISPLAY     = 0xC000

;; bitmap RLE escape byte: RLE_ESC, count, value
defc    RLE_ESC     = 0xE1

;----------------------------------------------------------------------------

SECTION data_user

;----------------------------------------------------------------------------

; pointer to character data (next column)
char_ptr:
    dw  0
; column counter
char_cols:
    db  0

; bitmap column counter
bmp_cols:
    db  0
; bitmap height
bmp_height:
    db  0
; bitmap current column top
bmp_col_ptr:
    dw  0

;----------------------------------------------------------------------------

SECTION code_user

;----------------------------------------------------------------------------

;; extern void __FASTCALL__ draw_char(unsigned char c);
; draw 8x8 px character, character data = 8 columns, bit 7 = top pixel
; L = char
; uses: xpos, ypos, video_ptr, video_bit, color_attr, invert_mask
_draw_char:
    ; char_ptr = font_data + 8 * c
    ld      h, 0
    add     hl, hl
    add     hl, hl
    add     hl, hl
    ld      de, _font_data
    add     hl, de
    ld      (char_ptr), hl
    ld      a, 8                ; char_cols = 8
    ld      (char_cols), a
; column loop
dc1:
    ; B = mask = color_attr | video_bit
    ld      a, (_color_attr)
    ld      hl, _video_bit
    or      a, (hl)
    ld      b, a
    ; C = column data = *char_ptr++ ^ invert_mask
    ld      a, (_invert_mask)
    ld      hl, (char_ptr)
    xor     (hl)
    ld      c, a
    inc     hl
    ld      (char_ptr), hl
    ; HL = video_ptr, DE = 64 (bytes per line)
    ld      hl, (_video_ptr)
    ld      de, 64
; pixel loop, ends when no more pixels in column
dc2:
    ld      a, c                ; CY = pixel, Z = no more pixels
    add     a, a
    ld      c, a
    jp      nc, dc3
    ld      a, (hl)             ; *vp |= mask
    or      a, b
    ld      (hl), a
    ld      a, c                ; restore Z flag
    or      a
dc3:
    add     hl, de              ; next line, keeps Z flag
    jp      nz, dc2             ; --> loop

    ; next column
    ld      hl, (_xpos)         ; xpos++
    inc     hl
    ld      (_xpos), hl
    ld      a, (_video_bit)     ; video_bit <<= 1
    add     a, a
    cp      64
    jp      nz, dc6

    ; next byte
    ld      hl, (_video_ptr)    ; video_ptr++
    inc     hl
    ld      (_video_ptr), hl
    ld      hl, (_xpos)         ; if (xpos == 288) next text line
    ld      a, l
    cp      288 % 256
    jp      nz, dc5
    ld      a, h
    cp      288 / 256
    jp      nz, dc5
    ld      hl, 0               ; xpos = 0
    ld      (_xpos), hl
    ld      hl, (_ypos)         ; ypos += 8
    ld      de, 8
    add     hl, de
    ld      (_ypos), hl
    ld      a, h                ; if (ypos >= 256) back to top
    or      a
    jp      z, dc4
    ld      hl, 0               ; ypos = 0
    ld      (_ypos), hl
    ld      hl, DISPLAY         ; video_ptr = DISPLAY
    ld      (_video_ptr), hl
    jp      dc5
dc4:
    ld      hl, (_video_ptr)    ; skip notepad area + next 7 lines
    ld      de, 16 + 7*64
    add     hl, de
    ld      (_video_ptr), hl
dc5:
    ld      a, 1                ; video_bit = 1
dc6:
    ld      (_video_bit), a
    ld      hl, char_cols       ; if (--char_cols == 0) return
    dec     (hl)
    jp      nz, dc1             ; --> loop
    ret

;----------------------------------------------------------------------------

;; extern void __FASTCALL__ draw_char_double(unsigned char c);
; draw character scaled to 16x16 px, character data = 8 columns, bit 7 = top pixel
; L = char
; uses: xpos, ypos, video_ptr, video_bit, color_attr, invert_mask
_draw_char_double:
    ; char_ptr = font_data + 8 * c
    ld      h, 0
    add     hl, hl
    add     hl, hl
    add     hl, hl
    ld      de, _font_data
    add     hl, de
    ld      (char_ptr), hl
    ld      a, 8                ; char_cols = 8
    ld      (char_cols), a
    ; align position to pixel pair
    ld      hl, _xpos           ; xpos &= ~1
    ld      a, (hl)
    and     a, 0xFE
    ld      (hl), a
    ld      a, (_video_bit)     ; video_bit = 16, 4 or 1
    cp      16
    ld      b, 16
    jp      nc, dx0
    cp      4
    ld      b, 4
    jp      nc, dx0
    ld      b, 1
dx0:
    ld      a, b
    ld      (_video_bit), a
; column pair loop
dx1:
    ; B = mask = color_attr | video_bit | video_bit << 1
    ld      a, (_video_bit)
    ld      b, a
    add     a, a
    or      a, b
    ld      hl, _color_attr
    or      a, (hl)
    ld      b, a
    ; C = column data = *char_ptr++ ^ invert_mask
    ld      a, (_invert_mask)
    ld      hl, (char_ptr)
    xor     (hl)
    ld      c, a
    inc     hl
    ld      (char_ptr), hl
    ; HL = video_ptr, DE = 64 (bytes per line)
    ld      hl, (_video_ptr)
    ld      de, 64
; pixel loop, each pixel on two lines, ends when no more pixels in column
dx2:
    ld      a, c                ; CY = pixel, Z = no more pixels
    add     a, a
    ld      c, a
    jp      nc, dx3
    ld      a, (hl)             ; *vp |= mask
    or      a, b
    ld      (hl), a
    add     hl, de              ; next line
    ld      a, (hl)             ; *vp |= mask
    or      a, b
    ld      (hl), a
    add     hl, de              ; next line
    ld      a, c                ; restore Z flag
    or      a
    jp      nz, dx2             ; --> loop
    jp      dx4
dx3:
    add     hl, de              ; skip two lines, keeps Z flag
    add     hl, de
    jp      nz, dx2             ; --> loop

dx4:
    ; next column pair
    ld      hl, (_xpos)         ; xpos += 2
    inc     hl
    inc     hl
    ld      (_xpos), hl
    ld      a, (_video_bit)     ; video_bit <<= 2
    add     a, a
    add     a, a
    cp      64
    jp      nz, dx7

    ; next byte
    ld      hl, (_video_ptr)    ; video_ptr++
    inc     hl
    ld      (_video_ptr), hl
    ld      hl, (_xpos)         ; if (xpos == 288) next text line
    ld      a, l
    cp      288 % 256
    jp      nz, dx6
    ld      a, h
    cp      288 / 256
    jp      nz, dx6
    ld      hl, 0               ; xpos = 0
    ld      (_xpos), hl
    ld      hl, (_ypos)         ; ypos += 16
    ld      de, 16
    add     hl, de
    ld      (_ypos), hl
    ld      a, h                ; if (ypos >= 256) back to top
    or      a
    jp      z, dx5
    ld      hl, 0               ; ypos = 0
    ld      (_ypos), hl
    ld      hl, DISPLAY         ; video_ptr = DISPLAY
    ld      (_video_ptr), hl
    jp      dx6
dx5:
    ld      hl, (_video_ptr)    ; skip notepad area + next 15 lines
    ld      de, 16 + 15*64
    add     hl, de
    ld      (_video_ptr), hl
dx6:
    ld      a, 1                ; video_bit = 1
dx7:
    ld      (_video_bit), a
    ld      hl, char_cols       ; if (--char_cols == 0) return
    dec     (hl)
    jp      nz, dx1             ; --> loop
    ret

;----------------------------------------------------------------------------

;; extern void __FASTCALL__ draw_bitmap(const unsigned char *bp);
; draw bitmap: width in bytes, height in pixels, RLE data column by column
; RLE_ESC, count, value = run of count identical bytes, other bytes as they are
; HL = bitmap data
; uses: xpos, video_ptr
_draw_bitmap:
    ld      a, (hl)             ; bmp_cols = width
    ld      (bmp_cols), a
    inc     hl
    ld      a, (hl)             ; bmp_height = height
    ld      (bmp_height), a
    ld      c, a                ; C = rows left in column
    inc     hl
    push    hl                  ; source pointer kept on stack
    ld      hl, (_video_ptr)    ; HL = bmp_col_ptr = video_ptr
    ld      (bmp_col_ptr), hl
    ld      de, 64              ; DE = 64 (bytes per line)
; RLE loop, A = value, B = count
db1:
    ex      (sp), hl            ; HL = source pointer
    ld      a, (hl)             ; A = *bp++
    inc     hl
    ld      b, 1                ; single byte
    cp      RLE_ESC
    jp      nz, db2
    ld      b, (hl)             ; B = run count
    inc     hl
    ld      a, (hl)             ; A = run value
    inc     hl
db2:
    ex      (sp), hl            ; HL = video pointer
; run loop
db3:
    ld      (hl), a             ; *vp = value
    add     hl, de              ; next line
    dec     c
    jp      z, db5              ; --> next column
db4:
    dec     b
    jp      nz, db3             ; --> loop
    jp      db1                 ; --> next RLE item

    ; next column, runs may continue there
db5:
    push    af                  ; save value
    ld      a, (bmp_height)     ; reset rows counter
    ld      c, a
    ld      hl, (_xpos)         ; xpos += 6
    ld      de, 6
    add     hl, de
    ld      (_xpos), hl
    ld      de, 64
    ld      hl, bmp_cols        ; --bmp_cols
    dec     (hl)
    ld      hl, (bmp_col_ptr)   ; HL = ++bmp_col_ptr, keeps Z flag
    inc     hl
    ld      (bmp_col_ptr), hl
    jp      z, db6              ; all columns done
    pop     af                  ; restore value
    jp      db4                 ; --> continue run
db6:
    pop     af                  ; drop saved value
    pop     af                  ; drop source pointer
    ld      (_video_ptr), hl    ; continue right after bitmap, top aligned
    ret

;----------------------------------------------------------------------------

;; extern void __FASTCALL__ clear_part_asm(unsigned short wh);
; clear area at current position, pixels and color attributes
; H = width in bytes (6 px each), nothing is done if 0
; L = height in lines, 0 = 256 lines (whole screen)
; uses: video_ptr (position is not changed)
_clear_part_asm:
    ld      b, h                ; B = width
    ld      c, l                ; C = height
    ld      a, b                ; if (width == 0) return
    or      a
    ret     z
    ; DE = 64 - width (from end of cleared part to next line)
    ld      a, 64
    sub     b
    ld      e, a
    sbc     a, a                ; sign extend, in case of width > 64
    ld      d, a
    ld      hl, (_video_ptr)    ; HL = video_ptr
    xor     a                   ; A = 0, kept for both loops
; line loop
cp1:
    push    bc                  ; save width
; byte loop
cp2:
    ld      (hl), a             ; *vp++ = 0
    inc     hl
    dec     b
    jp      nz, cp2             ; --> loop
    pop     bc                  ; restore width
    add     hl, de              ; next line
    dec     c                   ; height 0 wraps to 255, so 256 lines
    jp      nz, cp1             ; --> loop
    ret

;----------------------------------------------------------------------------

;; Console output replacement, linked in place of z88dk's fputc_cons_generic
;; (see LDFLAGS_EXTRA_PMD85 in Makefile). The pmd85 startup code always links
;; the generic console together with its system font, about 1.6 KB. The program
;; draws everything itself, so any console output is just dropped.
fputc_cons_none:
    ret

;----------------------------------------------------------------------------
