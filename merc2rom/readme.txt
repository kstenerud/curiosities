28-Jun-96

Merc2rom 1.2 by Karl Stenerud (mock@res.com)


Disclaimer: This software holds no warranty whatsoever.  Use at your own risk.

This software is niceware, which means that if you use it, send me something
you've developed on rom =)

What is it?
  Merc2rom takes a (working!) area file from merc (a mud based on diku) and
converts it to a Rom 2.4 area file.


What cool things does it do?
- It lets you set the base level of the area you are converting, so you can
  make a level 100 smurf village if you want =P.
- It lets you set the base vnum of the entire zone.
- It automatically sets mobile and object stats to suit the level in rom.
  Any objects that are not equipped by a mobile are set to the level of
  the lowest level mob in the area.
- You can tell merc2rom not to set object and mobile stats.
- There are default values you can set in defaults.h
- It outputs in the order AREA, HELPS, MOBILES, OBJECTS, ROOMS, RESETS, SHOPS,
  SPECIALS no matter what the input order was.
- It strips out stuff like mobprograms (since theres no mobprogram standard).
- Error checking: makes sure you got ALL the mobiles, objects, and rooms that
  were in the original.

How do I compile it?
1. Make sure you have a c++ compiler. (I use g++)
2. Extract the archive (cat merc2rom1.1.tar.gz|gunzip|tar xf -)
3. set your favorite compiler and compile flags in Makefile
4. have a look at defaults.h and change things if you want
5. type "make"


How do I run it?
Merc2rom takes the syntax: merc2rom [options] <src file> <dest file>
The options are:
   -l <value>      lets you set the base level of the area.
   -i <from> <to>  lets you change the base imm level in case you get an area
                   with #HELPS that has imm help set at level 32 or something.
                   I have a 100 level mud, so I do "-i 52 102".
   -v              Lets you set the base vnum of the area
   -f              Force merc2rom to use the loaded values instead of
                   calculating its own.

If you get "Error: EOF encountered", it means either your area file is invalid
or there's another bug in my program =P.


Known bugs:
  There seems to be a memory leak in the filestreams.  I noticed this because
some data in the mobiles and objects section would be corrupted if I opened
a file or did a seekg() after having read data into my objects.
I've been able to minimize the problem, and now the only quirk is in
hitower.are, where room 1317 will disappear unless you pad the long description
of the shadow guardian 1 or 2 characters.

I've put in some checking routines to look for missing objects, so if you have
problems, try padding the long description of the first mob.


TODO:

23to24   - Converts rom 2.3 areas to 2.4.
romfixer - cleans up rom 2.4 area files

If anyone has rom 2.2, I'd appreciate a copy of the tables.c and save.c and
some areas for it so I can make a 2.2 to 2.4 converter.


HISTORY:

1.0 (20-Jun-96) - First non-beta release.
                - Included -b option.

1.1 (25-Jun-96) - Major code cleanup (this was the first official release)
                - Included new option -i

1.2 (28-Jun-96) - Found out that some compilers don't like multiple instances
                  of default arguments.  Oops!
1.3 (02-Jul-96) - Slight bug in objects.c was causing negative values where
                  they shouldn't!
                - added opetions -v and -f
