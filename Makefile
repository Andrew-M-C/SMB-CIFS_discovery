
PROJ_NAME = samba-3.6.25
#SRC_DIR = $(PROJ_NAME)
SRC_DIR = $(PROJ_NAME)/source3
SMB_DISCO_DIR = $(shell pwd)/smb_discover

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
LDFLAGS += -L$(SYSTEM_LIBDIR)

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

all: $(PROJ_NAME) static_libs smb_discover
	@echo "== SAMBA built sucessfully =="

.PHONY:smb_discover
smb_discover:
	make -C $(SMB_DISCO_DIR)

.PHONY:libsmbclient
libsmbclient:
	@rm -f $@.a
	find ./$(PROJ_NAME) -name "libsmbclient.a" | xargs -I__ cp -a __ ./

.PHONY:libwbclient
libwbclient:
	@rm -f $@.a
	find ./$(PROJ_NAME) -name "libwbclient.a" | xargs -I__ cp -a __ ./

.PHONY:libsambalibs
libsambalibs:
	@rm -f $@.a
	@rm -f $@.txt
	find ./$(PROJ_NAME)/lib -name "*.o" | xargs -I__ echo -n "__ " >> $@.txt
	find ./$(SRC_DIR)/lib -name "*.o" | xargs -I__ echo -n "__ " >> $@.txt
	find ./$(SRC_DIR)/libsmb -name "*.o" | xargs -I__ echo -n "__ " >> $@.txt
	cat $@.txt | xargs $(AR) r $@.a
	@rm $@.txt

.PHONY:libsambarpc
libsambarpc:
	@rm -f $@.a
	@rm -f $@.txt
	find ./$(PROJ_NAME)/librpc -name "*.o" | xargs -I__ echo -n "__ " >> $@.txt
	cat $@.txt | xargs $(AR) r $@.a
	@rm $@.txt

.PHONY:static_libs
static_libs: libsmbclient libwbclient libsambalibs libsambarpc
	@echo "=="
	@find ./ -name "*.a" -maxdepth 1 | xargs du -h
	@echo "== samba static lib built sucessfully =="


.PHONY: $(PROJ_NAME)
$(PROJ_NAME):
	if [ ! -d $(PROJ_NAME) ]; then \
		tar -zxvf $(PROJ_NAME).tar.gz; \
	fi
	if [ ! -f $(SRC_DIR)/Makefile ]; then \
		cd $(SRC_DIR); $(MAKE_ARGS) ./configure $(CONFIGURE_ARGS); \
	fi
	$(MAKE_ARGS) make -s -C $(SRC_DIR)
	@echo "== $(shell date) =="
	@echo "== $(PROJ_NAME) built sucessfully =="


.PHONY:install_smb_discover
install_smb_discover:
	make install -C $(SMB_DISCO_DIR)


.PHONY:install
install: install_smb_discover


.PHONY: clean_smb_discover
clean_smb_discover:
	make clean -C $(SMB_DISCO_DIR)


.PHONY: clean
clean: clean_smb_discover
	@rm -rf *.a
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

