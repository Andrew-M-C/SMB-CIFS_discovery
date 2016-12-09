/*******************************************************************************
	
	File name:    util_smbtree.h
	Version:      1.0
	Description:  
	    Interfaces for nmblookup utility.

	Author:		Andrew Chang
	Create Date: 	2016-12-01
--------------------------------------------------------------
	History: 
		2016-12-01: File created as "util_nmblookup.h".

--------------------------------------------------------------
	GPL declaration:

	     This program is free software: you can redistribute it and/or modify it 
	 under the terms of the GNU General Public License as published by the Free 
	 Software Foundation, either version 3 of the License, or any later version.
	     This program is distributed in the hope that it will be useful, but WITH-
	 OUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
	 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
	 more details. 
	     You should have received a copy of the GNU General Public License along
	 with this program. If not, see <http://www.gnu.org/licenses/>.

********************************************************************************/

#ifndef __UTIL_NMBLOOKUP_H__
#define __UTIL_NMBLOOKUP_H__

#include "smb_discover.h"

cJSON *util_nmblookup(SmbDiscoverContext_st *pAppConf);
int util_nmblookup_check_config(SmbDiscoverContext_st *pAppConf);

#endif	/* EOF */

