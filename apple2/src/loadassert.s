; Link-time guard: Apple II main segment must load at or above $4000 so code
; does not overwrite Hi-Res page ($2000-$3FFF). Requires --start-addr 0x4000

.import __MAIN_START__
.assert __MAIN_START__ >= $4000, error, "Apple II load address < $4000 (need --start-addr 0x4000)"
