Howto use goattracker on Windows with sorbus

- install TeraTerm 
- on goattracker save your music to $1000 as raw-binary ( no load adress ) to this directory
- link the player with a copy-command on a command-prompt :
  'copy /y /b play.prg +my_music_file.bin player.prg'
- use serial-connection with 115200 baud 8n1 , no Flowcontrol
  ( You should see output like : "Sorbus JAM V0.5: 1-4)Bootsector, 0)Exec RAM @ $E000," )
- press '^' twice , you are in meta-mode now .
  ( You should see output like : "B)acktrace, D)isassemble, E)vent queue, H)eap, I)nternal drive, M)emory dump,
    S)peeds, U)pload")
- press 'U' for upload and '0' + return for the starting address ( taken from file )
  ( "Start XModem Transfer now:" is shown)
- goto 'File' / 'Transfer' / 'XMODEM' / 'send' and choose "player.prg" , which you just created 
  (you should see the transfer-bar shown )
- press return 
  ( "Reception successful. 0x0F80 - 0x1B00" , last address may vary due to your music)
- press "m" for the systemmonitor ( maybe a "CTRL+c" is needed to stop a running player) 
  ( "Sorbus System Monitor via menu
   PC  BK AC XR YR SP NV-BDIZC" )
- start the player with 'g0f80'
  ( "play song
     ctrl-C to stop "  )
- listen to the music :) 
    