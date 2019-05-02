// main.cpp

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include <stdlib.h>
#include <string.h>

#include "zone.h"
#include "defaults.h"

int main(int argc, char *argv[])
{
   Zone		zone;
   char		*infile		= NULL;
   char		*outfile	= NULL;
   ifstream	*instream	= NULL;
   ofstream	*outstream	= NULL;
   int		base_level	= 0;
   int		base_vnum	= -1;
   int		imm_from	= 0;
   int		imm_to		= 0;
   int		error		= 0;
   int		i		= 1;
   int		force		= FALSE;

// ************ Ugly command line parsing **************

   if(argc >= i+1)
   {
      while(argv[i][0] == '-')
      {
         switch(argv[i++][1])
         {
         case 'l':
            base_level = atoi(argv[i++]);
            break;
         case 'i':
            imm_from = atoi(argv[i++]);
            imm_to = atoi(argv[i++]);
            break;
         case 'v':
            base_vnum = atoi(argv[i++]);
            break;
         case 'f':
            force = TRUE;
            break;
         default:
            error++;
         }
      }
   }
   else error++;
   if(argc >= i+1)
      infile = argv[i++];
   else error++;
   if(argc >= i+1)
      outfile = argv[i];
   else error++;

   if(error)
   {
      cout << "Merc2rom v1.2 (28-Jun-96) by Karl Stenerud (mock@res.com)\n\n";
      cout << "Useage: merc2rom [options] <src file> <dest file>\n\n";
      cout << "Options:\n";
      cout << "   -l <value>      set base level of area\n";
      cout << "   -i <from> <to>  change imm level for #HELPS section\n";
      cout << "   -v <value>      set base vnum of area\n";
      cout << "   -f              force merc2rom to use original values\n\n";
      exit(5);
   }

   if(strcmp(outfile, infile) == 0)
   {
      cout << "Can't read and write the same file!\n\n";
      exit(5);
   }

   instream	= new ifstream(infile);
   if(!instream->is_open())
   {
      cout << "can't open " << infile << endl;
      exit(5);
   }
   outstream	= new ofstream(outfile);
   if(!outstream->is_open())
   {
      cout << "can't open " << outfile << endl;
      exit(5);
   }

// *************************** The Program ***************************

   *instream >> zone;
   instream->seekg(0);
   zone.NoChange(force);
   zone.CountObjects(*instream);
   zone.BaseVnum(base_vnum);
   zone.SetFileName(infile);
   zone.BaseLevel(base_level);
   zone.ImmLevel(imm_from, imm_to);
   *outstream << zone;

   instream->close();
   outstream->close();
   return 0;
}
