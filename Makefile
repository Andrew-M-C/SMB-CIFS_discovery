
PROJ_NAME = samba-3.6.25
#SRC_DIR = $(PROJ_NAME)
SRC_DIR = $(PROJ_NAME)/source3
SMB_DISCO_DIR = $(shell pwd)/smb_discover

APP_NAME = smb_discv

CROSS_COMPILE ?= mipsel-linux-
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
CC = $(CROSS_COMPILE)cc
AR = $(CROSS_COMPILE)ar
STRIP = $(CROSS_COMPILE)strip

INSTALL_ROOT ?= /opt/RT288x_SDK/source/NC450_rootfs
LD_LIBRARY_PATH ?= $(INSTALL_ROOT)/lib

SYSTEM_INCDIR ?= /opt/RT288x_SDK/source/lib/include
SYSTEM_LIBDIR ?= /opt/RT288x_SDK/source/lib/lib
CPPFLAGS += -I$(SYSTEM_INCDIR)
CFLAGS += -I$(SYSTEM_INCDIR) -O2
LDFLAGS += -L$(SYSTEM_LIBDIR) -lm

export AS
export LD
export CC
export STRIP
export CPPFLAGS
export CFLAGS
export LDFLAGS
export LD_LIBRARY_PATH
export SYSTEM_INCDIR
export SYSTEM_LIBDIR

export SAMBA_SOURCE_DIR = $(shell pwd)/$(SRC_DIR)
export SAMBA_PROJ_DIR = $(shell pwd)/$(PROJ_NAME)
export INSTALL_ROOT

SAMBA_PREFIX = /usr/local/

CONFIGURE_ARGS = \
			--exec-prefix=$(SAMBA_PREFIX) \
			--prefix=$(SAMBA_PREFIX) \
			--with-piddir=/var/run \
			--with-configdir=/usr/local/etc \
			--disable-shared-libs \
			--with-libsmbclient \
			--without-ads \
			--without-libtalloc \
			--without-libnetapi \
			--without-libsmbsharemodes \
			--without-libaddns \
			--without-cluster-support \
			--without-krb5 \
			--without-ldap \
			--without-pam \
			--without-winbind \
			--without-libtdb \
			--disable-avahi \
			--disable-cups \
			--disable-pie \
			--disable-relro \
			--disable-static \
			--disable-swat \
			--with-included-iniparser \
			--with-included-popt \
			--with-sendfile-support \
			--with-shared-modules=pdb_tdbsam,pdb_wbc_sam,auth_winbind,auth_wbc,perfcount,perfcount_test,pdb,pdb_ads \
#			--quiet --enable-debug --enable-developer
# auto configured parameters
#			--with-codepagedir=$(SAMBA_PREFIX)/share/samba \
#			--with-configdir=$(SAMBA_PREFIX)/etc/samba \
#			--with-privatedir=$(SAMBA_PREFIX)/etc/samba \
#			--datarootdir=$(SAMBA_PREFIX)/share/samba \
#			--with-logfilebase=$(SAMBA_PREFIX)/etc/samba/log \
#			--with-nmbdsocketdir=$(SAMBA_PREFIX)/etc/samba/nmbd \
#			--with-lockdir=/tmp/samba_lock \
# unsupported modules in MTK toolchain: auth_domain,idmap_nss,nss_info_template,vfs,perfcount_onefs,pdb_ldap

ifeq ($(CROSS_COMPILE),mipsel-linux-)
CONFIGURE_ARGS += --target=mipsel-linux --build=$(shell arch)-linux --host=mipsel-linux
endif

EXT_SAMBA_CONFIGURE_ARGS = \
	samba_cv_CC_NEGATIVE_ENUM_VALUES=yes \
	libreplace_cv_HAVE_GETADDRINFO=no \
	ac_cv_file__proc_sys_kernel_core_pattern=yes \
	libreplace_cv_HAVE_IPV6=no \
	libreplace_cv_HAVE_IFACE_IFCONF=yes
# files below are to be tested
#	libreplace_cv_HAVE_C99_VSNPRINTF=yes \
#	LINUX_LFS_SUPPORT=yes \
#	samba_cv_HAVE_GETTIMEOFDAY_TZ=yes

MAKE_ARGS = CC=$(CC) $(EXT_SAMBA_CONFIGURE_ARGS)

all: $(PROJ_NAME)
	@echo "== SAMBA built sucessfully =="


INSERT_LINE_FILE = cutline.txt
.PHONY: $(PROJ_NAME)
$(PROJ_NAME):
	if [ ! -d $(PROJ_NAME) ]; then \
		tar -zxvf $(PROJ_NAME).tar.gz; \
	fi
	-mkdir $(SRC_DIR)/client/backup
	if [ ! -f $(SRC_DIR)/Makefile ]; then \
		cd $(SRC_DIR); $(MAKE_ARGS) ./configure $(CONFIGURE_ARGS); \
		grep -n "^CLIENT_OBJ1" Makefile | sed 's/:/\t/g' | cut -f 1 >$(INSERT_LINE_FILE); \
		sed -i 's/^CLIENT_OBJ1 =/\n\nCLIENT_OBJ1 ?=/g' Makefile; \
		cp -f $(SMB_DISCO_DIR)/client.in .; \
		cat $(INSERT_LINE_FILE) | xargs -I [] sed -i '[] r client.in' Makefile; \
		mv -f client/*.c client/backup || echo "move OK"; \
	fi
	cp -uf $(SMB_DISCO_DIR)/*.c $(SRC_DIR)/client
	cp -uf $(SMB_DISCO_DIR)/*.h $(SRC_DIR)/client
	cp -uf $(SRC_DIR)/client/backup/smbspool.c $(SRC_DIR)/client
	$(MAKE_ARGS) make -C $(SRC_DIR)
	@echo "== $(shell date) =="
	@find -name "smbclient" | xargs du -h
	@echo "== $(PROJ_NAME) built sucessfully =="


.PHONY:install_smb_discover
install_smb_discover:
	@find -name "smbclient" | xargs -I [] cp [] $(INSTALL_ROOT)/bin/$(APP_NAME)
	$(STRIP) $(INSTALL_ROOT)/bin/$(APP_NAME)
	@du -h $(INSTALL_ROOT)/bin/$(APP_NAME)
	@echo "== $(APP_NAME) installed =="
	#make install -C $(SMB_DISCO_DIR)


.PHONY:install
install: install_smb_discover


.PHONY: clean
clean:
	-rm -f $(SRC_DIR)/client/*.c
	@if [ ! -f $(SRC_DIR)/Makefile ]; then \
		rm -rf $(PROJ_NAME); \
		echo $(PROJ_NAME) cleaned; \
	else \
		make clean -C $(SRC_DIR); \
		echo $(PROJ_NAME) cleaned; \
	fi


.PHONY: distclean
distclean: clean
	rm -rf $(PROJ_NAME)

