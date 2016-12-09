
# application name
APP_NAME = smb_discv
CROSS_COMPILE ?= mipsel-linux-

# Comment out next line to disable debug mode
_DEBUG = 1

# basic compile and link settings
ifdef _DEBUG
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
CC = $(CROSS_COMPILE)cc
AR = $(CROSS_COMPILE)ar
STRIP = $(CROSS_COMPILE)strip
CFLAGS += -DSMB_DISCO_DEBUG -Werror

else
AS ?= $(CROSS_COMPILE)as
LD ?= $(CROSS_COMPILE)ld
CC ?= $(CROSS_COMPILE)cc
AR ?= $(CROSS_COMPILE)ar
STRIP ?= $(CROSS_COMPILE)strip

endif

export AS
export LD
export CC
export STRIP
export CPPFLAGS
export CFLAGS
export LDFLAGS

# check samba lib definition
ifndef SAMBA_SOURCE_DIR
$(error Error: No samba 3.x source directory specified. Maybe you should make at upper directory.)
endif

ifndef SAMBA_PROJ_DIR
$(error Error: No samba 3.x project directory specified. Maybe you should make at upper directory.)
endif

ifndef INSTALL_ROOT
$(error Error: No install destination defined.)
endif

$(warning Samba source:  "$(SAMBA_SOURCE_DIR)")
$(warning Samba project: "$(SAMBA_PROJ_DIR)")

#prepare for build
SAMBA_LINK_DIR = $(shell pwd)/../
SAMBA_INC_FLAGS = -I$(SAMBA_SOURCE_DIR) -I$(SAMBA_SOURCE_DIR)/include \
				-I$(SAMBA_SOURCE_DIR)/librpc \
				-I$(SAMBA_PROJ_DIR) \
				-I$(SAMBA_PROJ_DIR)/lib/talloc -I$(SAMBA_PROJ_DIR)/lib/util -I$(SAMBA_PROJ_DIR)/lib/replace \
				-I$(SAMBA_PROJ_DIR)/lib/tevent -I$(SAMBA_PROJ_DIR)/lib/popt -I$(SAMBA_PROJ_DIR)/librpc -I$(SAMBA_PROJ_DIR)/libcli/util

CFLAGS += -Wall -I$(SYSTEM_INCDIR) $(SAMBA_INC_FLAGS) -I$(shell pwd)/ -O2
CPPFLAGS += $(CFLAGS)
LDFLAGS += -L$(SYSTEM_LIBDIR) -L$(SAMBA_LINK_DIR) -lsmbclient -lz -lsambarpc -static -lwbclient -lsambalibs -lm

C_OBJS =$(shell ls *.c > clist.txt 2>/dev/null; sed 's/\.c/\.o/g' < clist.txt) 
CPP_OBJS =$(shell ls *.cpp > cpplist.txt 2>/dev/null; sed 's/\.cpp/\.o/g' < cpplist.txt) 

# make rules
ifdef _DEBUG

all: $(APP_NAME)
	@echo "== $(shell date) =="
	@du -h $(APP_NAME)
	@echo "== $(APP_NAME)(DEBUG ver) built sucessfully =="
	sudo cp -a $(APP_NAME) $(INSTALL_ROOT)/bin
	
else
all: $(APP_NAME)
	$(STRIP) $(APP_NAME)
	@echo "== $(shell date) =="
	@du -h $(APP_NAME)
	@echo "== $(APP_NAME) built sucessfully =="
endif

$(APP_NAME): $(C_OBJS) $(CPP_OBJS)
	@rm -f clist.txt cpplist.txt
	$(LD) -r -o $@.o $(C_OBJS) $(CPP_OBJS)
	$(CC) $@.o -o $@ $(LDFLAGS)


-include $(C_OBJS:.o=.d)
-include $(CPP_OBJS:.o=.d)

$(CPP_OBJS): $(CPP_OBJS:.o=.cpp)
	$(CPP) -c $(CPPFLAGS) $*.cpp -o $*.o  
	$(CPP) -MM $(CPPFLAGS) $*.cpp > $*.d  
	@mv -f $*.d $*.d.tmp  
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d  
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp 

$(C_OBJS): $(C_OBJS:.o=.c)
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	$(CC) -MM $(CFLAGS) $*.c > $*.d  
	@mv -f $*.d $*.d.tmp  
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d  
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp 
	

.PHONY: clean
clean:
	@rm -f $(C_OBJS) $(CPP_OBJS) $(APP_NAME) clist.txt cpplist.txt *.d *.d.* *.o
	@echo "== $(APP_NAME) cleaned =="

.PHONY: distclean
distclean: clean

.PHONY: install
install:
	$(STRIP) $(APP_NAME)
	@du -h $(APP_NAME)
	cp $(APP_NAME) $(INSTALL_ROOT)/bin
	@echo "== $(APP_NAME) installed =="


