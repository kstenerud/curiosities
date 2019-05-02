#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <conio.h>
#include <string.h>
#include <fcntl.h>
#include <values.h>
#include <unistd.h>

#define IS16BIT

#define MAX_X 80
#define MAX_Y 24

#define ANSI_BLACK    0x00
#define ANSI_BLUE     0x01
#define ANSI_GREEN    0x02
#define ANSI_CYAN     0x03
#define ANSI_RED      0x04
#define ANSI_MAGENTA  0x05
#define ANSI_YELLOW   0x06
#define ANSI_WHITE    0x07
#define ANSI_HBLACK   0x08
#define ANSI_HBLUE    0x09
#define ANSI_HGREEN   0x0a
#define ANSI_HCYAN    0x0b
#define ANSI_HRED     0x0c
#define ANSI_HMAGENTA 0x0d
#define ANSI_HYELLOW  0x0e
#define ANSI_HWHITE   0x0f
#define ANSI_STD      0x07

#define BAR_FG ANSI_BLACK
#define BAR_BG ANSI_CYAN

typedef enum {m_normal, m_color, m_ascii} dispmode_t;

int  get_bit(char* data, long bit_offset);
void set_bit(char* data, long bit_offset, int value);
int  bitcmp(char* data, long startpos, char* find_data, long len);
char int2hexchar(int value);
int  hexchar2int(int c);
int  bitcode_string(char* buff, char* str, int granularity);
int  bitcode_value(char* buff, long offset, int val, int granularity);
long atohl(char* str);

long atohl(char* str)
{
   long accum = 0;
   char* ptr = str;

   for(;;ptr++)
   {
      if(*ptr == '\0')
         return accum;
      accum <<= 4;
      if(*ptr >= '0' && *ptr <= '9')
         accum |= (long) ((*ptr-'0') & 0x0f);
      else if(*ptr >= 'a' || *ptr <= 'f')
         accum |= (long) ((*ptr-'a'+10) & 0x0f);
      else
         return -1;
   }

}

class BitBuff;
class BitIter;
class Interface;
class DosTerm;

class BitBuff
{
public:
         BitBuff(int granularity = 4)
         : _filename(NULL), _data(NULL), _granularity(4), _len(0) {set_granularity(granularity);}
         ~BitBuff() {if(_filename != NULL) delete [] _filename;
                    if(_data != NULL) delete [] _data;}
   int   load           (char* filename);
   int   save           (char* filename = NULL);
   int   get_data       (long bit_offset);
   int   put_data       (long bit_offset, int value);
   long  find           (char* lookfor, long start_pos);
   int   set_granularity(int gran);
   int   get_granularity() {return _granularity;}
   long  get_len        () {return _len*8;}
   char* get_filename   () {return _filename;}
private:
   char*  _filename;
   char*  _data;
   int    _granularity;
   long   _len;
};

class BitIter
{
public:
       BitIter(BitBuff* buff): _buff(buff), _offset(0) {}
   int  set_offset  (long bytepos, int bitpos) {return set_offset((bytepos*8) + bitpos);}
   int  set_offset  (long offset);
   int  advance_pos (long bytepos, int bitpos) {return advance_pos((bytepos*8) + bitpos);}
   int  advance_pos (long offset);
   void reset       () {_offset = 0;}
   long get_offset  () {return _offset;}
   int  get_data    () {return _buff->get_data(_offset);}
   int  put_data    (int data) {return _buff->put_data(_offset, data);}
   int  find        (char* lookfor);
   int  set_buff    (BitBuff* buff);
private:
   long     _offset;
   BitBuff* _buff;
};

class DosTerm
{
public:
        DosTerm(BitBuff* buff, int width = 32, int length = MAX_Y, dispmode_t mode = m_normal);
   int  set_width      (int width);
   int  get_width      () {return _width;}
   int  set_length     (int length);
   int  get_length     () {return _length;}
   int  add_width      () {return set_width(_width+1);}
   int  sub_width      () {return set_width(_width-1);}
   int  add_length     () {return set_length(_length+1);}
   int  sub_length     () {return set_length(_length-1);}
   int  set_mode       (dispmode_t mode);
   dispmode_t get_mode () {return _mode;}
   int  fwd_bit        () {return _iter.advance_pos(1);}
   int  fwd_space      () {return _iter.advance_pos(_buff->get_granularity());}
   int  fwd_line       () {return _iter.advance_pos(_buff->get_granularity()*_width);}
   int  fwd_page       () {return _iter.advance_pos(_buff->get_granularity()*_width*_length);}
   int  back_bit       () {return _iter.advance_pos(-1);}
   int  back_space     () {return _iter.advance_pos(-_buff->get_granularity());}
   int  back_line      () {return _iter.advance_pos(-(_buff->get_granularity()*_width));}
   int  back_page      () {return _iter.advance_pos(-(_buff->get_granularity()*_width*_length));}
   void print_binary_data();
   void print          (int line, char* str, int fgcolor = ANSI_WHITE, int bgcolor = ANSI_BLACK);
   void erase          (int line) {print(line, "");}
   void prompt         (int line, char* str, int fgcolor = ANSI_WHITE, int bgcolor = ANSI_BLACK);
   long get_offset     () {return _iter.get_offset();}
   int  set_offset     (long offset) {return _iter.set_offset(offset);}
   int  find           (char* str) {return _iter.find(str);}
   BitBuff* get_buff   () {return _buff;}
   void update         ();
   int  crsr_set       (int x, int y);
   void crsr_left      ();
   void crsr_right     ();
   void crsr_up        ();
   void crsr_down      ();
   void crsr_reset     () {crsr_set(1, 1);}
   long crsr_get_offset();
   int  crsr_get_x     () {return _crsr_x;}
   int  crsr_get_y     () {return _crsr_y;}
   int  crsr_write     (int c);
   void crsr_park      () {gotoxy(1, 25);}
   int  get_input_string(char* str, int len);
   int  get_input_hex  (long* val, int len);
private:
   BitBuff*   _buff;
   BitIter    _iter;
   dispmode_t _mode;
   int        _width;
   int        _length;
   char       _screen[4000];
   int        _crsr_x;
   int        _crsr_y;
};

int DosTerm::get_input_string(char* str, int max_len)
{
   int i;
   int c;

   for(i=0;;i++)
   {
      switch(c = getch())
      {
         case 0x1b:	// ESC
            str[0] = '\0';
            return 0;
         case 0x0d:	// CR
	    str[i] = '\0';
            return i;
         case 0x08:	// BS
            i-=2;
            crsr_set(_crsr_x-1, _crsr_y);
            putch(' ');
         default:
            if(i < max_len+1 && c >= ' ' && c <= '~')
            {
               str[i] = c;
               putch(c);
               crsr_set(_crsr_x+1, _crsr_y);
            }
            else
               i--;
      }
   }
}

int DosTerm::get_input_hex(long* val, int max_len)
{
   int i;
   int c;
   char* str = new char[max_len];

   for(i=0;;i++)
   {
      switch(c = getch())
      {
         case 0x1b:	// ESC
            str[0] = '\0';
            return 0;
         case 0x0d:	// CR
	    str[i] = '\0';
            return (*val=atohl(str)) >= 0;
         case 0x08:	// BS
            i-=2;
            crsr_set(_crsr_x-1, _crsr_y);
            putch(' ');
         default:
            if(i < max_len+1 && ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            {
               str[i] = c;
               putch(c);
               crsr_set(_crsr_x+1, _crsr_y);
            }
            else
               i--;
      }
   }
}

class Interface
{
public:
   Interface(BitBuff* buff);
   void mode_insert() {_insert_mode = 1;}
   void mode_view  () {_insert_mode = 0;}
   void disp_error (char* msg) {_term.print(25, msg);}
   int  interp_cmd (int c);
   void print_stat_line();
   void redisplay();
private:
   int      _insert_mode;
   BitBuff* _buff;
   DosTerm  _term;
   int      _old_granularity;
};


int get_bit(char* data, long bit_offset)
{
   return ((data[(int)(bit_offset - (bit_offset%8)) / 8]
            & (1 << (int)(7-(bit_offset%8)))) != 0);
}

void set_bit(char* data, long bit_offset, int value)
{
   if(value)
      data[(int)(bit_offset - (bit_offset%8)) / 8]
         |= (1 << (int)(7-(bit_offset%8)));
   else
      data[(int)(bit_offset - (bit_offset%8)) / 8]
         &= (~(1 << (int)(7-(bit_offset%8))));
}

int bitcmp(char* data, long startpos, char* find_data, long len)
{
   long int pos_data = startpos;
   long int pos_find;

   for(pos_find=0;pos_find<len;pos_find++, pos_data++)
      if(get_bit(data, pos_data) != get_bit(find_data, pos_find))
         return 0;
   return 1;
}

char int2hexchar(int value)
{
   if(value < 0)
      return (char) ' ';
   if(value > 15)
      return (char) '~';
   if(value > 9)
      return (char) (value + 'a' - 10);
   return (char) (value + '0');
}

int hexchar2int(int c)
{
   if(c >= 'a' && c <= 'f')
      return c - 'a' + 10;
   else if(c >= '0' && c <= '9')
      return c - '0';
   else
      return -1;
}

int bitcode_string(char* buff, char* str, int granularity)
{
   int len = strlen(str);
   int i;
   int offset = 0;

   for(i=0;i<len;i++, offset+=granularity)
      if(!bitcode_value(buff, offset, str[i], granularity))
         return 0;
   return 1;
}

int bitcode_value(char* buff, long bit_offset, int val, int granularity)
{
   int i;
   long offset = bit_offset;

   if(val < 0 || val >= (1 << granularity))
      return 0;

   for(i=granularity-1;i>=0;i--)
      set_bit(buff, offset++, val & (1 << i));

   return 1;
}

int BitBuff::set_granularity(int gran)
{
   if(gran < 1 || gran > 8)
      return 0;
   _granularity = gran;
   return 1;
}

long BitBuff::find(char* lookfor, long start_offset)
{
   long len = strlen(lookfor);
   long i;
   long j;
   char search_str[1000] = {0};

   for(i=j=0;j<len;j++)
   {
      if(!bitcode_value(search_str, i, _granularity, hexchar2int(lookfor[(int)j])))
         return -1;
      i += _granularity;
   }

   len = i;

   for(i=start_offset;i<_len*8-len*8;i+=_granularity)
      if(::bitcmp(_data, i, search_str, len))
         return i;
   return -1;
}

int BitBuff::put_data(long bit_offset, int value)
{
   if(bit_offset < 0 || bit_offset >= _len*8)
      return -1;

   return bitcode_value(_data, bit_offset, value, _granularity);
}

int BitBuff::get_data(long bit_offset)
{
   long offset = bit_offset;
   int val = 0;

   if(bit_offset < 0 || bit_offset >= _len*8)
      return -1;

   switch(_granularity)
   {
      case 8:  val += (::get_bit(_data, offset++) ? 0x80 : 0);
      case 7:  val += (::get_bit(_data, offset++) ? 0x40 : 0);
      case 6:  val += (::get_bit(_data, offset++) ? 0x20 : 0);
      case 5:  val += (::get_bit(_data, offset++) ? 0x10 : 0);
      case 4:  val += (::get_bit(_data, offset++) ? 0x08 : 0);
      case 3:  val += (::get_bit(_data, offset++) ? 0x04 : 0);
      case 2:  val += (::get_bit(_data, offset++) ? 0x02 : 0);
      default: val += (::get_bit(_data, offset++) ? 0x01 : 0);
   }
   return val;
}

int BitBuff::load(char* filename)
{
   int fd;
   long len;

   if(filename != NULL)
   {
      if(_filename != NULL)
         delete [] _filename;
      _filename = new char[strlen(filename)+1];
      strcpy(_filename, filename);
      if( (fd=open(_filename, O_RDONLY|O_BINARY)) != NULL)
      {
         if( (_len=filelength(fd)) > 0)
         {
            if(_len < MAXINT)
            {
               if(_data != NULL)
                  delete [] _data;
               _data = new char[(int)_len];
               if( (len=read(fd, _data, _len)) == _len)
               {
                  close(fd);
                  return 1;
               }
               else
                  printf("Length was %d.  Should be %d", len, _len);
               delete [] _data;
               _data = NULL;
            }
            printf("File too long.\n");
         }
         else if(_len == 0)
            printf("File length was zero!\n");
      }
      delete [] _filename;
      _filename = NULL;
   }
   close(fd);
   return 0;
}

int BitBuff::save(char* filename)
{
   int fd;

   if(filename != NULL)
   {
      if(_filename != NULL)
         delete [] _filename;
      _filename = new char[strlen(filename)+1];
      strcpy(_filename, filename);
   }

   if(_filename != NULL && _data != NULL)
   {
      if( (fd=open(_filename, O_WRONLY|O_BINARY)) != NULL)
      {
         if(write(fd, _data, _len) == _len)
         {
            close(fd);
            return 1;
         }
      }
   }
   close(fd);
   return 0;
}


int BitIter::set_buff(BitBuff* buff)
{
   if(buff == NULL)
      return 0;
   _buff = buff;
   _offset = 0;
   return 1;
}

int BitIter::advance_pos(long offset)
{
   if((offset + _offset) < 0 || (offset + _offset) >= _buff->get_len())
      return 0;
   _offset += offset;
   return 1;
}

int BitIter::set_offset(long offset)
{
   if(offset < 0 || offset >= _buff->get_len())
      return 0;
   _offset = offset;
   return 1;
}

int BitIter::find(char* lookfor)
{
   long temp = _buff->find(lookfor, _offset);

   if(temp < 0)
      return 0;
   _offset = temp;
   return 1;
}

Interface::Interface(BitBuff* buff)
: _buff(buff), _term(buff), _insert_mode(0), _old_granularity(buff->get_granularity())
{
   redisplay();
}

int  Interface::interp_cmd (int c)
{
   char str[100];
   int input;

   if(_insert_mode)
   {
      switch(c)
      {
         case 0x1b:
         case 'q':
            _insert_mode = 0;
            _term.crsr_set(1, 25);
            break;
         case '.':
            _term.crsr_down();
            break;
         case 'l':
         case ';':
            _term.crsr_up();
            break;
         case '/':
            _term.crsr_right();
            break;
         case ',':
            _term.crsr_left();
            break;
         case '0': case '1': case '2': case '3':
         case '4': case '5': case '6': case '7':
         case '8': case '9': case 'a': case 'b':
         case 'c': case 'd': case 'e': case 'f':
            _term.crsr_write(c);
            break;
      }
      redisplay();
      _term.update();
   }
   else
   {
      switch(c)
      {
         case 'c':
            _term.set_mode(m_color);
            _buff->set_granularity(_old_granularity);
            break;
         case 'a':
            _term.set_mode(m_ascii);
            _buff->set_granularity(8);
            break;
         case 'n':
            _term.set_mode(m_normal);
            _buff->set_granularity(_old_granularity);
            break;
         case 's':
            _term.prompt(25, "Search for: ", BAR_FG, BAR_BG);
            _term.update();
            cscanf("%s", str);
            getch();
         case 'S':
            _term.erase(25);
            if(!_term.find(str))
            {
               _term.print(25, "Sequence not found. Press a key...", BAR_FG, BAR_BG);
               _term.update();
               getch();
               _term.erase(25);
            }
            break;
         case 'w':
            _term.prompt(25, "Save file? [y/N] ", BAR_FG, BAR_BG);
            _term.update();
            if(getch() == 'y')
               _buff->save();
            _term.erase(25);
            break;
         case 'i':
            _insert_mode = 1;
            _term.crsr_reset();
            break;
         case '1':
            if(_term.get_mode() == m_ascii)
               break;
            _old_granularity = 1;
            _buff->set_granularity(_old_granularity);
            str[0] = 0;
            break;
         case '2':
            if(_term.get_mode() == m_ascii)
               break;
            _old_granularity = 2;
            _buff->set_granularity(_old_granularity);
            str[0] = 0;
            break;
         case '3':
            if(_term.get_mode() == m_ascii)
               break;
            _old_granularity = 3;
            _buff->set_granularity(_old_granularity);
            str[0] = 0;
            break;
         case '4':
            if(_term.get_mode() == m_ascii)
               break;
            _old_granularity = 4;
            _buff->set_granularity(_old_granularity);
            str[0] = 0;
            break;
         case 'g':
            _term.prompt(25, "Goto: ", BAR_FG, BAR_BG);
            _term.update();
            cscanf("%x", &input);
            getch();
            _term.erase(25);
            _term.set_offset(input * 8);
            break;
         case 0x1b:
         case 'q':
            _term.prompt(25, "Quit? [y/N] ", BAR_FG, BAR_BG);
            _term.update();
            if(getch() == 'y')
            {
               _term.erase(25);
               _term.crsr_set(1, 24);
               _term.update();
               return 0;
            }
            _term.erase(25);
            break;
         case ' ':
            _term.fwd_page();
            break;
         case 'b':
            _term.back_page();
            break;
         case '.':
            _term.fwd_line();
            break;
         case 'l':
         case ';':
            _term.back_line();
            break;
         case '/':
            _term.fwd_space();
            break;
         case ',':
            _term.back_space();
            break;
         case ']':
            _term.fwd_bit();
            break;
         case '[':
            _term.back_bit();
            break;
         case '=':
            _term.add_width();
            break;
         case '+':
            _term.add_length();
            break;
         case '-':
            _term.sub_width();
            break;
         case '_':
            _term.sub_length();
            break;
      }
      _term.crsr_set(1, 25);
      redisplay();
      _term.update();
   }
   return 1;
}


void Interface::print_stat_line()
{
   char  buff[81];
   char* filename = _buff->get_filename();
   char* mode_list[] = {"normal", "color", "ascii"};
   long  offset = _term.get_offset();

   if(_insert_mode)
      offset = _term.crsr_get_offset();

   sprintf(buff, "File: %-.12s | POS: %.8lx.%.1x | W: %.2d | L: %.2d | %.1d Bit %.6s %.3s"
           , (filename != NULL ? filename : "(NULL)"), (long)((offset-(offset%8))/8), (int)(offset%8)
           , _term.get_width(), _term.get_length(), _buff->get_granularity()
           , mode_list[_term.get_mode()], _insert_mode ? "(I)" : "   ");

   _term.print(25, buff, BAR_FG, BAR_BG);
}

void Interface::redisplay()
{
   _term.print_binary_data();
   print_stat_line();
   _term.update();
}

int DosTerm::crsr_write(int c)
{
   return _buff->put_data(crsr_get_offset(), hexchar2int(c));
}

void DosTerm::crsr_left()
{
   if(--_crsr_x < 1)
   {
      crsr_up();
      _crsr_x = _width;
   }
   gotoxy(_crsr_x, _crsr_y);
}

void DosTerm::crsr_right()
{
   if(++_crsr_x > _width)
   {
      crsr_down();
      _crsr_x = 1;
   }
   gotoxy(_crsr_x, _crsr_y);
}

void DosTerm::crsr_up()
{
   if(--_crsr_y < 1)
      _crsr_y = _length;
   gotoxy(_crsr_x, _crsr_y);
}

void DosTerm::crsr_down()
{
   if(++_crsr_y > _length)
      _crsr_y = 1;
   gotoxy(_crsr_x, _crsr_y);
}

int DosTerm::crsr_set(int x, int y)
{
   if(x < 1 || x > MAX_X || y < 1 || y > MAX_Y)
      return 0;
   _crsr_x = x;
   _crsr_y = y;
   gotoxy(_crsr_x, _crsr_y);
   return 1;
}

long DosTerm::crsr_get_offset()
{
   return ((_crsr_y-1)*_width+_crsr_x-1)*_buff->get_granularity() + _iter.get_offset();
}


DosTerm::DosTerm(BitBuff* buff, int width, int length, dispmode_t mode)
: _buff(buff), _iter(buff), _width(60), _length(24), _mode(m_normal)
, _crsr_x(1), _crsr_y(1)
{
   int i;

   for(i=0;i<4000;i+=2)
   {
      _screen[i] = ' ';
      _screen[i+1] = ANSI_STD;
   }

   set_width(width);
   set_length(length);
   set_mode(mode);
}

void DosTerm::prompt(int line, char* str, int fgcolor, int bgcolor)
{
   int len = strlen(str);

   print(line, str, fgcolor, bgcolor);
   gotoxy(len+1, line);
   update();
}

void DosTerm::print(int line, char* str, int fgcolor, int bgcolor)
{
   int i;
   int line_begin = (line-1)*80;
   int line_end = line*80;
   int strend = strlen(str) + line_begin;
   char color = (char) (fgcolor | ((bgcolor & 7) << 4));

   if(line < 1 || line > 25)
      return;

   for(i=line_begin;i<line_end;i++)
   {
      _screen[i*2] = (i < strend ? str[i-line_begin] : ' ');
      _screen[i*2+1] = color;
   }
}

void DosTerm::print_binary_data()
{
   int  i = 0;
   long offset = _iter.get_offset();
   int  val;
   char ch;
   char at;

   for(i=0;i<2000;i++)
   {
      if(i%80 >= _width || i/80 >= _length)
      {
         ch = ' ';
         at = ANSI_STD;
      }
      else
      {
         switch(_mode)
         {
            case m_normal:
               ch = int2hexchar(_buff->get_data(offset));
               at = ANSI_STD;
               break;
            case m_color:
               ch = (char) 219;
               if( (val=_buff->get_data(offset)) < 0)
                  val = 0;
               at = val;
               break;
            case m_ascii:
               if( (val=_buff->get_data(offset)) < 0)
                  val = 0;
               if(val < ' ' || val > '~')
                  val = '.';
               ch = val;
               at = ANSI_STD;
               break;
         }
         offset += _buff->get_granularity();
      }
      _screen[i*2] = ch;
      _screen[i*2+1] = at;
   }
}

void DosTerm::update()
{
   puttext(1, 1, 80, 25, _screen);
}

int DosTerm::set_width(int width)
{
   if(width < 1 || width > MAX_X)
      return 0;
   _width = width;
   return 1;
}

int DosTerm::set_length(int length)
{
   if(length < 1 || length > MAX_Y)
      return 0;
   _length = length;
   return 1;
}

int DosTerm::set_mode(dispmode_t mode)
{
   if(mode < m_normal || mode > m_ascii)
      return 0;
   _mode = mode;
   return 1;
}

int main(int argc, char* argv[])
{
   BitBuff buff;
   Interface* interface;

   if(argc < 2)
   {
      printf("Usage: bscan <filename>\n");
      exit(-1);
   }

   if(!buff.load(argv[1]))
   {
      perror("Unable to load file");
      exit(-1);
   }
   interface = new Interface(&buff);

   while(interface->interp_cmd(getch()));

   delete interface;
   return 0;
}