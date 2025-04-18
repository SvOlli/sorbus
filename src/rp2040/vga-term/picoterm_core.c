/*
 * Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication with RC2014 via UART on GPIO20 &21
 * Shiela Dixon, https://peacockmedia.software
 *
 * main.c handles the ins and outs
 * picoterm.c handles the behaviour of the terminal and storing the text
 *
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */


#include "picoterm_core.h"
#include "main.h" // Build font
#include "hardware/uart.h"
#include "pmhid.h" // keyboard definitions
#include "picoterm_config.h"
#include "picoterm_dec.h" // DEC lines
#include "picoterm_stddef.h"
#include "picoterm_screen.h" // display_x screen function
#include "picoterm_conio.h" // basic input/output function for console


// escape sequence state
#define ESC_READY               0
#define ESC_ESC_RECEIVED        1
#define ESC_PARAMETER_READY     2

#define MAX_ESC_PARAMS          5
static int esc_state = ESC_READY;
static int esc_parameters[MAX_ESC_PARAMS+1];
static bool parameter_q;
static bool parameter_p;
static bool parameter_sp;
static int esc_parameter_count;
static unsigned char esc_c1;
static unsigned char esc_final_byte;


#define VT100   1
#define VT52    2
#define BOTH    3
int mode = VT100;


/* picoterm_config.c */
extern picoterm_config_t config; // Issue #13, awesome contribution of Spock64

/* picoterm_conio.c */
extern picoterm_conio_config_t conio_config;

char bell_state = 0;
bool insert_mode = false;

// early declaration
void reset_escape_sequence();
// command answers
void response_VT52Z();
void response_VT52ID();
void response_VT100OK();
void response_VT100ID();
void response_csr();

void terminal_init(){
    reset_escape_sequence();
    // initialize ConIO buffers and main parameters.
    conio_init( config.graph_id ); // // ansi graphical font_id
    cursor_visible(true);
    clear_cursor();  // so we have the character
    print_cursor();  // turns on
}

void clear_escape_parameters(){
    for(int i=0;i<MAX_ESC_PARAMS;i++){
        esc_parameters[i]=0;
    }
    esc_parameter_count = 0;
}

void reset_escape_sequence(){
    clear_escape_parameters();
    esc_state=ESC_READY;
    esc_c1=0;
    esc_final_byte=0;
    parameter_q=false;
    parameter_p=false;
    parameter_sp=false;
}

void terminal_reset(){
    move_cursor_home();
    reset_saved_cursor();

    mode = VT100;
    insert_mode = false;

    conio_reset( get_cursor_char( config.font_id, CURSOR_TYPE_DEFAULT ) - 0x20 );
    cursor_visible(true);
    clear_cursor();  // so we have the character
    print_cursor();  // turns on
}

char get_bell_state() { return bell_state; }
void set_bell_state(char state) { bell_state = state; }


// for debugging purposes only
void print_ascii_value(unsigned char asc){
    // takes value eg 65 ('A') and sends characters '6' and '5' (0x36 and 0x35)
    int hundreds = asc/100;
    unsigned char remainder = asc-(hundreds*100);
    int tens = remainder/10;
    remainder = remainder-(tens*10);
    if(hundreds>0){
        handle_new_character(0x30+hundreds);
    }
    if(tens>0 || hundreds>0){
        handle_new_character(0x30+tens);
    }
    handle_new_character(0x30+remainder);
    handle_new_character(' ');
    if(conio_config.cursor.pos.x>COLUMNS-5){
        handle_new_character(CR);
        handle_new_character(LF);
    }
}


void esc_sequence_received(){
/* Manage the execution of PARAMETRIZED Escape sequence.
   These should now be populated:
    static int esc_parameters[MAX_ESC_PARAMS];
    static int esc_parameter_count;
    static unsigned char esc_c1;
    static unsigned char esc_final_byte;
*/

  int n,m;
  if(mode==VT100){
      //ESC H           Set tab at current column
      //ESC [ g         Clear tab at current column
      //ESC [ 0g        Same
      //ESC [ 3g        Clear all tabs

      //ESC H  HTS  Horizontal Tab Set  Sets a tab stop in the current column the cursor is in.
      //ESC [ <n> I  CHT  Cursor Horizontal (Forward) Tab  Advance the cursor to the next column (in the same row) with a tab stop. If there are no more tab stops, move to the last column in the row. If the cursor is in the last column, move to the first column of the next row.
      //ESC [ <n> Z  CBT  Cursor Backwards Tab  Move the cursor to the previous column (in the same row) with a tab stop. If there are no more tab stops, moves the cursor to the first column. If the cursor is in the first column, doesn’t move the cursor.
      //ESC [ 0 g  TBC  Tab Clear (current column)  Clears the tab stop in the current column, if there is one. Otherwise does nothing.
      //ESC [ 3 g  TBC  Tab Clear (all columns)


      if(esc_c1=='['){
          // CSI
          switch(esc_final_byte){
          case 'H':
          case 'f':
              //Move cursor to upper left corner ESC [H
              //Move cursor to upper left corner ESC [;H
              //Move cursor to screen location v,h ESC [<v>;<h>H
              //Move cursor to upper left corner ESC [f
              //Move cursor to upper left corner ESC [;f
              //Move cursor to screen location v,h ESC [<v>;<h>f

              n = esc_parameters[0];
              m = esc_parameters[1];
              if(n == 0) n = 1;
              if(m == 0) m = 1;

              move_cursor_at(n,m);
              break;

          case 'E': // ESC[#E  moves cursor to beginning of next line, # lines down
              n = esc_parameters[0];
              if(n==0)n=1;
              // these are ONE based
              move_cursor_at( conio_config.cursor.pos.y+n+1, 1); // y, x
              break;

          case 'F':  // ESC[#F  moves cursor to beginning of previous line, # lines up
              n = esc_parameters[0];
              if(n==0)n=1;
              // these are ONE based
              move_cursor_at( conio_config.cursor.pos.y - n+1, 1);
              break;

          case 'd': // ESC[#d  moves cursor to an absolute # line
              n = esc_parameters[0];
              n--;
              // these are ONE based
              move_cursor_at( n+1, conio_config.cursor.pos.x+1 );
              break;

          case 'G': // ESC[#G  moves cursor to column #
              n = esc_parameters[0];
              n--;
              // there are ONE based
              move_cursor_at( conio_config.cursor.pos.y+1, n+1 );
              break;

          case 'h':
              //[ 2 h    Keyboard locked
              //[ 4 h    Insert mode selected
              //[ 20 h    Set new line mode

              //[ ? 1 h       Set cursor key to application
              //[ ? 2 h       Set ANSI (versus VT52)
              //[ ? 3 h    132 Characters on
              //[ ? 4 h    Smooth Scroll on
              //[ ? 5 h    Inverse video on
              //[ ? 7 h    Wraparound ON
              //[ ? 8 h    Autorepeat ON
              //[ ? 9 h       Set 24 lines per screen (default)
              //[ ? 12 h    Text Cursor Enable Blinking
              //[ ? 14 h    Immediate operation of ENTER key
              //[ ? 16 h    Edit selection immediate
              //[ ? 25 h    Cursor ON
              //[ ? 47 h      save screen
              //[ ? 50 h    Cursor ON
              //[ ? 75 h    Screen display ON
              //[ ? 1049 h  enables the alternative buffer
              if(parameter_q){
                  if(esc_parameters[0]==25 || esc_parameters[0]==50){
                      // show csr
                      cursor_visible(true);
                  }
                  else if(esc_parameters[0]==7){
                      //Auto-wrap mode on (default) ESC [?7h
                      conio_config.wrap_text = true;
                  }
                  else if(esc_parameters[0]==9){
                      //Set 24 lines per screen (default)
                      terminal_reset(); // reset to simulate change
                  }
                  else if(esc_parameters[0]==12){
                      //Text Cursor Enable Blinking
                      conio_config.cursor.state.blinking_mode = true;
                  }
                  else if(esc_parameters[0]==47 || esc_parameters[0]==1047){
                      //save screen
                      copy_main_to_secondary_screen();
                  }
                  else if(esc_parameters[0]==1048){
                      //save cursor
                      save_cursor_position();
                  }
                  else if(esc_parameters[0]==1049){
                      //save cursor and save screen
                      save_cursor_position();
                      copy_main_to_secondary_screen();
                  }
              }
              else{
                  if(esc_parameters[0]==4){
                      //Insert mode selected
                      insert_mode = true;
                  }
              }
              break;

          case 'l':
              //[ 2 l    Keyboard unlocked
              //[ 4 l    Replacement mode selected
              //[ 20 l    Set line feed mode

              //[ ? 1 l       Set cursor key to cursor
              //[ ? 2 l       Set VT52 (versus ANSI)
              //[ ? 3 l    80 Characters on
              //[ ? 4 l    Jump Scroll on
              //[ ? 5 l    Normal video off
              //[ ? 7 l    Wraparound OFF
              //[ ? 8 l    Autorepeat OFF
              //[ ? 9 l       Set 36 lines per screen
              //[ ? 12 l      Text Cursor Disable Blinking
              //[ ? 14 l      Deferred operation of ENTER key
              //[ ? 16 l      Edit selection deferred
              //[ ? 25 l      Cursor OFF
              //[ ? 47 l    restore screen
              //[ ? 50 l      Cursor OFF
              //[ ? 75 l      Screen display OFF

              //[ ? 1049 l  disables the alternative buffer

              if(parameter_q){
                  if(esc_parameters[0]==25 || esc_parameters[0]==50){
                      // hide csr
                      cursor_visible(false);
                  }
                  else if(esc_parameters[0]==2){
                      //Set VT52 (versus ANSI)
                      mode = VT52;
                  }
                  else if(esc_parameters[0]==7){
                      //Auto-wrap mode off ESC [?7l
                      conio_config.wrap_text = false;
                  }
                  else if(esc_parameters[0]==9){
                      //Set 36 lines per screen
                      terminal_reset(); // reset to simulate change
                  }
                  else if(esc_parameters[0]==12){
                      //Text Cursor Disable Blinking
                      conio_config.cursor.state.blinking_mode = false;
                  }
                  else if(esc_parameters[0]==47 || esc_parameters[0]==1047){
                      //restore screen
                      copy_secondary_to_main_screen();
                  }
                  else if(esc_parameters[0]==1048){
                      //restore cursor
                      copy_secondary_to_main_screen();
                  }
                  else if(esc_parameters[0]==1049){
                      //restore screen and restore cursor
                      copy_secondary_to_main_screen();
                      restore_cursor_position();
                      //conio_config.cursor.pos.x = saved_csr.x;
                      //conio_config.cursor.pos.y = saved_csr.y;
                  }
              }
              else{
                  if(esc_parameters[0]==4){
                      //Replacement mode selected
                      insert_mode = false;
                  }
              }
              break;

          case 'm':
              //SGR
              // Sets colors and style of the characters following this code

              //[ 0 m    Clear all character attributes
              //[ 1 m    (Bold) Alternate Intensity ON
              //[ 3 m     Select font #2 (large characters)
              //[ 4 m    Underline ON
              //[ 5 m    Blink ON
              //[ 6 m     Select font #2 (jumbo characters)
              //[ 7 m    Inverse video ON
              //[ 8 m     Turn invisible text mode on
              //[ 22 m    Alternate Intensity OFF
              //[ 24 m    Underline OFF
              //[ 25 m    Blink OFF
              //[ 27 m    Inverse Video OFF
              for(int param_idx = 0; param_idx <= esc_parameter_count && param_idx <= MAX_ESC_PARAMS; param_idx++){ //allows multiple parameters
                  int param = esc_parameters[param_idx];
                  if(param==0){
                      conio_config.rvs = false; // reset / normal
                      conio_config.blk = false;
                  }
                  else if(param==5){
                      conio_config.blk = true;
                  }
                  else if(param==7){
                      conio_config.rvs = true;
                  }
                  else if(param==25){
                      conio_config.blk = false;
                  }
                  else if(param==27){
                      conio_config.rvs = false;
                  }
                  else if(param>=30 && param<=39){ //Foreground
                  }
                  else if(param>=40 && param<=49){ //Background
                  }
              }
             break;

          case 's':
              // save cursor position
              save_cursor_position();
              //saved_csr.x = conio_config.cursor.pos.x;
              //saved_csr.y = conio_config.cursor.pos.y;
              break;

          case 'u':
              // move to saved cursor position
              restore_cursor_position();
              //conio_config.cursor.pos.x = saved_csr.x;
              //conio_config.cursor.pos.y = saved_csr.y;
              break;

          case 'J':
              // Clears part of the screen. If n is 0 (or missing), clear from cursor to end of screen.
              // If n is 1, clear from cursor to beginning of the screen. If n is 2, clear entire screen
              // (and moves cursor to upper left on DOS ANSI.SYS).
              // If n is 3, clear entire screen and delete all lines saved in the scrollback buffer
              // (this feature was added for xterm and is supported by other terminal applications).
              switch(esc_parameters[0]){
                case 0:  // clear from cursor to end of screen
                  clear_screen_from_cursor();
                  break;

                case 1: // clear from cursor to beginning of the screen
                  clear_screen_to_cursor();
                  break;

                case 2: // clear entire screen
                case 3:
                  clrscr();
                  conio_config.cursor.pos.x=0; conio_config.cursor.pos.y=0;
                  break;
              }
              break;

          case 'K':
              // Erases part of the line. If n is 0 (or missing), clear from cursor to the end of the line.
              // If n is 1, clear from cursor to beginning of the line. If n is 2, clear entire line.
              // Cursor position does not change.
              switch(esc_parameters[0]){
                case 0: // clear from cursor to the end of the line
                  clear_line_from_cursor();
                  break;
                case 1:  // clear from cursor to beginning of the line
                  clear_line_to_cursor();
                  break;
                case 2:  // clear entire line
                  clear_entire_line();
                  break;
              }
              break;

          case 'A': // Cursor Up - Moves the cursor n (default 1) cells
              n = esc_parameters[0];
              move_cursor_up(n);
              break;

          case 'B': // Cursor Down - Moves the cursor n (default 1) cells
              n = esc_parameters[0];
              move_cursor_down(n);
              break;

          case 'C': // Cursor Forward - Moves the cursor n (default 1) cells
              n = esc_parameters[0];
              move_cursor_forward(n);
              break;

          case 'D': // Cursor Backward - Moves the cursor n (default 1) cells
              n = esc_parameters[0];
              move_cursor_backward(n);
              break;

          case 'S': // Scroll whole page up by n (default 1) lines. New lines are added at the bottom. (not ANSI.SYS)
              n = esc_parameters[0];
              if(n==0)n=1;
              for(int i=0;i<n;i++)
                  shuffle_down();
             break;

          case 'T': // Scroll whole page down by n (default 1) lines. New lines are added at the top. (not ANSI.SYS)
              n = esc_parameters[0];
              if(n==0)n=1;
              for(int i=0;i<n;i++)
                  shuffle_up();
              break;

          // MORE

          case 'L': // 'INSERT LINE' - scroll rows down from and including cursor position. (blank the cursor's row??)
              n = esc_parameters[0];
              if(n==0)n=1;
              insert_lines(n);
              break;

          case 'M': // 'DELETE LINE' - delete row at cursor position, scrolling everything below, up to fill. Leaving blank line at bottom.
              n = esc_parameters[0];
              if(n==0)n=1;
              delete_lines(n);
              break;

          case 'P': // 'DELETE CHARS' - delete <n> characters at the current cursor position, shifting in space characters from the right edge of the screen.
              n = esc_parameters[0];
              if(n==0)n=1;
              delete_chars(n);
              break;

          case 'X': // 'ERASE CHARS' - erase <n> characters from the current cursor position by overwriting them with a space character.
              n = esc_parameters[0];
              if(n==0)n=1;
              erase_chars(n);
              break;

          case '@': // 'Insert Character' - insert <n> spaces at the current cursor position, shifting all existing text to the right. Text exiting the screen to the right is removed.
              n = esc_parameters[0];
              if(n==0)n=1;
              insert_chars(n);
              break;

          case 'q':
              if(parameter_sp){
                  parameter_sp = false;
                  //ESC [ 0 SP q  User Shape  Default cursor shape configured by the user
                  //ESC [ 1 SP q  Blinking Block  Blinking block cursor shape
                  //ESC [ 2 SP q  Steady Block  Steady block cursor shape
                  //ESC [ 3 SP q  Blinking Underline  Blinking underline cursor shape
                  //ESC [ 4 SP q  Steady Underline  Steady underline cursor shape
                  //ESC [ 5 SP q  Blinking Bar  Blinking bar cursor shape
                  //ESC [ 6 SP q  Steady Bar  Steady bar cursor shape
                  conio_config.cursor.symbol = get_cursor_char( config.font_id, esc_parameters[0] ) - 0x20; // parameter correspond to picoterm_cursor.h::CURSOR_TYPE_xxx
                  conio_config.cursor.state.blinking_mode = get_cursor_blinking( config.font_id, esc_parameters[0] );
              }
              break; // case q

          case 'c':
              response_VT100ID();
              break;

          case 'n':
              if (esc_parameters[0]==5){
                  response_VT100OK();
              }
              else if (esc_parameters[0]==6){
                  response_csr();
              }
              break;
          }

      } // if( esc_c1=='[' )
      else{
          // ignore everything else
      }
  }
  else if(mode==VT52){ // VT52
      // \033[^^  VT52
      // \033[Z   VT52
      if(esc_c1=='['){
          // CSI
          switch(esc_final_byte){
          case 'Z':
              response_VT52ID();
          break;
          }
      }
      else{
          // ignore everything else
      }
  }

  // Both VT52 & VT100
  if(esc_c1=='('){
      // CSI
      switch(esc_final_byte){
      case 'B':
          // display ascii chars (not letter) but stays in NupetScii font
          // to allow swtich back to DEC "single/double line" drawing.
          conio_config.dec_mode = DEC_MODE_NONE;
          break;
      case '0':
          conio_config.dec_mode = DEC_MODE_SINGLE_LINE;
          break;
      case '2':
          conio_config.dec_mode = DEC_MODE_DOUBLE_LINE;
          break;
      default:
          conio_config.dec_mode = DEC_MODE_NONE;
          break;
      }
  }

  // our work here is done
  reset_escape_sequence();
}

void handle_new_character(unsigned char asc){
	// Ask Terminal core to handle this new character
  if(esc_state != ESC_READY){
      // === ESC SEQUENCE ====================================================
      switch(esc_state){
          case ESC_ESC_RECEIVED:
              // --- waiting on c1 character ---
              // c1 is the first parameter after the ESC
              if( (asc=='(') || (asc=='[') ){
                  // 0x9B = CSI, that's the only one we're interested in atm
                  // the others are 'Fe Escape sequences'
                  // usually two bytes, ie we have them already.
                  if(asc=='['){    // ESC+[ =  0x9B){
                      esc_c1 = asc;
                      esc_state=ESC_PARAMETER_READY; // Lets wait for parameter
                      clear_escape_parameters();
                      // number of expected parameter depends on next caracter.
                  }
                  else if(asc=='('){    // ESC+(
                      esc_c1 = asc;
                      esc_state=ESC_PARAMETER_READY; // Lets wait for parameter
                      clear_escape_parameters();
                      parameter_p=true; // we just expecty a single parameter.
                  }
                  // other type Fe sequences go here
                  else
                      // for now, do nothing
                      reset_escape_sequence();
              }
              // --- SINGLE CHAR escape ----------------------------------------
              // --- VT100 / VT52 ----------------------------------------------
              else if(asc=='c'){ // mode==BOTH VT52 / VT100 Commands
                    terminal_reset();
                    reset_escape_sequence();
              }
              else if (asc=='F' ){
                    config.font_id=config.graph_id; // Enter graphic charset
                    build_font( config.font_id );
                    conio_config.dec_mode = DEC_MODE_NONE; // use approriate ESC to enter DEC Line Drawing mode
                    reset_escape_sequence();
              }
              else if (asc=='G'){
                    config.font_id=FONT_ASCII; // Enter ASCII charset
                    build_font( config.font_id );
                    conio_config.dec_mode = DEC_MODE_NONE;
                    reset_escape_sequence();
              }
              // --- SINGLE CHAR escape ----------------------------------------
              // --- VT100 -----------------------------------------------------
              else if(mode==VT100){  // VT100 Commands

                if (asc=='7' ){
                    // save cursor position
                    save_cursor_position();
                    reset_escape_sequence();
                }
                else if (asc=='8' ){
                    // move to saved cursor position
                    restore_cursor_position();
                    reset_escape_sequence();
                }
                else if (asc=='D' ){
                    //cmd_lf();
                    move_cursor_lf( false ); // normal move
                    reset_escape_sequence();
                }
                else if (asc=='M' ){
                    //cmd_rev_lf();
                    move_cursor_lf( true ); // reverse move
                    reset_escape_sequence();
                }
                else if (asc=='E' ){
                    //cmd_lf();
                    move_cursor_lf(true); // normal move
                    reset_escape_sequence();
                }

              }
              // --- SINGLE CHAR escape ----------------------------------------
              // --- VT52 ------------------------------------------------------
              else if(mode==VT52){ // VT52 Commands

                if (asc=='A' ){
                    move_cursor_up( 1 );
                    reset_escape_sequence();
                }
                else if (asc=='B' ){
                    move_cursor_down( 1 );
                    reset_escape_sequence();
                }
                else if (asc=='C' ){
                    move_cursor_forward( 1 );
                    reset_escape_sequence();
                }
                else if (asc=='D' ){
                    move_cursor_backward( 1 );
                    reset_escape_sequence();
                }
                else if (asc=='H' ){
                    move_cursor_home();
                    reset_escape_sequence();
                }
                else if (asc=='I' ){
                    move_cursor_lf( true ); // reverse move
                    reset_escape_sequence();
                }
                else if (asc=='J' ){
                    clear_screen_from_cursor();
                    reset_escape_sequence();
                }
                else if (asc=='K' ){
                    clear_line_from_cursor();
                    reset_escape_sequence();
                }
                else if (asc=='Z' ){
                    response_VT52Z();
                    reset_escape_sequence();
                }
                else if (asc=='<' ){
                    mode = VT100;
                    reset_escape_sequence();
                }
                else
                    // unrecognised character after escape.
                    reset_escape_sequence();
            }
            // ==============

              else
                  // unrecognised character after escape.
                  reset_escape_sequence();
              break;

          case ESC_PARAMETER_READY:
              // waiting on parameter character, semicolon or final byte
              if(asc>='0' && asc<='9'){

                  if(parameter_p){
                    // final byte. Log and handle
                    esc_final_byte = asc;
                    esc_sequence_received(); // execute esc sequence
                  }
                  else{
                    // parameter value
                    if(esc_parameter_count<MAX_ESC_PARAMS){
                        unsigned char digit_value = asc - 0x30; // '0'
                        esc_parameters[esc_parameter_count] *= 10;
                        esc_parameters[esc_parameter_count] += digit_value;
                    }
                  }

              }
              else if(asc==';'){
                  // move to next param
                  if(esc_parameter_count<MAX_ESC_PARAMS) esc_parameter_count++;
              }
              else if(asc=='?'){
                  parameter_q=true;
              }
              else if(asc==' '){
                  parameter_sp=true;
              }
              else if(asc>=0x40 && asc<0x7E){
                  // final byte. Log and handle
                  esc_final_byte = asc;
                  esc_sequence_received(); // execute esc sequence
              }
              else{
                  // unexpected value, undefined
              }
              break;
      }

  }
  else {
      // === regular characters ==============================================
      if(asc>=0x20 && asc<=0xFF){

          //if insert mode shift chars to the right
          if(insert_mode) insert_chars(1);

          // --- Strict ASCII <0x7f or Extended NuPetSCII <= 0xFF ---
          put_char(asc-32,conio_config.cursor.pos.x,conio_config.cursor.pos.y);
          conio_config.cursor.pos.x++;

          if(!conio_config.wrap_text){
            // this for disabling wrapping in terminal
            constrain_cursor_values();
          }
          else{
            // alternatively, use this code for enabling wrapping in terminal
            wrap_constrain_cursor_values();
          }
      }
      else if(asc==0x1B){
          // --- Begin of ESCAPE SEQUENCE ---
          esc_state=ESC_ESC_RECEIVED;
      }
      else {
          // --- return, backspace etc ---
          switch (asc){
              case BEL:
              bell_state = 1;
              break;

              case BSP:
                if( conio_config.cursor.pos.x>0 ){
                  conio_config.cursor.pos.x--;
                }
                break;

              case LF:
                //cmd_lf();
                move_cursor_lf( false ); // normal move
                break;

              case CR:
                conio_config.cursor.pos.x=0;
                break;

              case FF:
                clrscr();
                conio_config.cursor.pos.x=0; conio_config.cursor.pos.y=0;
                break;
          } // switch(asc)
      } // else
  } // eof Regular character

  if(conio_config.cursor.state.blink_state)
    conio_config.cursor.state.blink_state = false;
}


void __send_string(char str[]){
  /* send string back to host via UART */
  char c;
  for(int i=0;i<strlen(str);i++){
      c = str[i];
      //insert_key_into_buffer( c );
      //SVOLLI_TODO
      //uart_putc (UART_ID, c);
  }
}


void response_VT52Z() {
    __send_string("\033/Z");
}

void response_VT52ID() {
    __send_string("\033[/Z");
}

void response_VT100OK() {
    __send_string("\033[0n");
}

void response_VT100ID() {
    __send_string("\033[?1;0c");    // vt100 with no options
}

void response_csr() { // cursor position
    char s[20];
    sprintf(s, "\033[%d;%dR", conio_config.cursor.pos.y+1, conio_config.cursor.pos.x+1);
    __send_string(s);
}
