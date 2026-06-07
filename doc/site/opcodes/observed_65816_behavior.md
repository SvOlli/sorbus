
# Observed 65816 Behavior

The 65816 can run in five different modes:

- Emulation
- Native: M=1, X=1
- Native: M=0, X=1
- Native: M=1, X=0
- Native: M=0, X=0

This can be a cause for a lot of cases of slightly different behavior.

Following are some random things that were observed.


## Interrupts

In native mode, interrupts also add an extra cycle during execution in which
the program bank register (PBR) is pushed on the stack (first stack access,
before return address). Obviously, vectors are fetched from $FFEx (bank 0)
instead of $FFFx. BRK is also separated from IRQ using a different vector.


## REP/SEP In Emulation Mode

```asm65816
SEC
XCE         ; set to emulation mode
SEP  #$FF
PHP         ; -> pushes $FF
REP  #$FF
PHP         ; -> pushes $30

            ; still in emulation mode
SEP  #$30
CLC
XCE         ; set to native mode
PHP         ; -> pushes $31
XCE         ; set to emulation mode
REP  #$30
PHP
XCE         ; set to native mode
PHP         ; -> pushes $31
```

This leads to the conclusion that MX can only be changed in native mode. Also,
in emulation mode, "B" and "-" flag cannot be changed using REP/SEP.


