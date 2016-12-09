/*******************************************************************************
	
	File name:    util_smbtree.h
	Version:      1.0
	Description:  
	    Interfaces for smbtree utility.

	Author:		Andrew Chang
	Create Date: 	2016-11-29
--------------------------------------------------------------
	History: 
		2016-11-29: File created as "util_smbtree.h".

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

#ifndef __UTIL_SMBTREE_H__
#define __UTIL_SMBTREE_H__

#include "smb_discover.h"

cJSON *util_smbtree(SmbDiscoverContext_st *pAppConf);
int util_smbtree_check_config(SmbDiscoverContext_st *pAppConf);

#endif	/* EOF */

