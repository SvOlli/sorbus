
.segment "CODE"

UART_READ    = $DF00; read from uart input
UART_READ_Q  = $DF01; can be $00-$7f in normal operation 
UART_WRITE   = $DF02; write to uart output
UART_WRITE_Q = $DF03; can be $00-$7f in normal operation


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
