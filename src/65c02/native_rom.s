
.segment "CODE"

UART_READ    = $DFFA; read from uart input
UART_READ_Q  = $DFFB; can be $00-$7f in normal operation
UART_WRITE   = $DFFC; write to uart output
UART_WRITE_Q = $DFFD; can be $00-$7f in normal operation

TIMER_IRQ_VALUE_REPEAT = $DF00
TIMER_IRQ_VALUE_SINGLE = $DF02
TIMER_NMI_VALUE_REPEAT = $DF04
TIMER_NMI_VALUE_SINGLE = $DF06
WATCHDOG_STOP          = $DF08
WATCHDOG_LOW           = $DF09
WATCHDOG_MID           = $DF0A
WATCHDOG_HIGH_START    = $DF0B

; set to 65c02 code
; ...best not make use of opcode that are not supported by 65816 CPUs
.PC02

IRQ:
NMI:
   jmp   *
RESET:
   ldx   #$ff
   txs
@mainloop:
   lda   UART_READ_Q
   beq   @mainloop
   lda   UART_READ
@waitwq:   
   bit   UART_WRITE_Q
   bvs   @waitwq        ; simple hack: wait
   sta   UART_WRITE
   jmp   @mainloop

.segment "VECTORS"
   .word NMI
   .word RESET
   .word IRQ
