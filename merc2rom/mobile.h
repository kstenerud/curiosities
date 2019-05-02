// mobile.h

#ifndef _MY_MOBILE_H
#define _MY_MOBILE_H

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include "cstring.h"
#include "fileio.h"
#include "flags.h"
#include "defaults.h"
#include "list.h"

class Mobile
{
   public:
      		Mobile		();
      		~Mobile		();
      void	NoChange	();
      void	BaseVnum	(int, int=-1, int=-1);
      int	FindLowVnum	(int=-1);
      int	FindHighVnum	(int=-1);
      Mobile*	FindVnum	(int);
      void	BaseLevel	(int, int=-1);
      void	SetLevel	(int);
      int	FindLowLevel	(int=-1);
      int	FindHighLevel	(int=-1);
      int	GetLevel	();
      void	ListAdd		(List&);
      ifstream&	Read		(ifstream&);
      ofstream&	Write		(ofstream&);
   private:
      int	_vnum;
      Cstring	_name;
      Cstring	_short_desc;
      Cstring	_long_desc;
      Cstring	_look;
      Cstring	_race;		// rom
      Cstring	_act_flags;
      Cstring	_aff_flags;
      int	_align;
      int	_group;		// rom
      int	_level;
      int	_hit;
      int	_hpnodice;
      int	_hpsizedice;
      int	_hpplus;
      int	_mananodice;	// rom
      int	_manasizedice;	// rom
      int	_manaplus;	// rom
      int	_damnodice;
      int	_damsizedice;
      int	_damplus;
      Cstring	_dam_type;	// rom
      int	_pierce;
      int	_bash;
      int	_slash;
      int	_magic;
      Cstring	_off_flags;	// rom
      Cstring	_imm_flags;	// rom
      Cstring	_res_flags;	// rom
      Cstring	_vuln_flags;	// rom
      Cstring	_start_pos;
      Cstring	_def_pos;
      Cstring	_sex;
      int	_gold;
      Cstring	_form;		// rom flags
      Cstring	_parts;		// rom flags
      Cstring	_size;		// rom
      int	_material;	// rom
      int	_nochange;
      Mobile	*_next;
};

ifstream& operator>>(ifstream&, Mobile&);
ofstream& operator<<(ofstream&, Mobile&);

#endif
