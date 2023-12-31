#
# cmonitor tools makefile
# https://github.com/f18m/cmonitor
# Francesco Montorsi (c) 2019
#

# constants
THIS_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
ROOT_DIR:=$(shell readlink -f $(THIS_DIR)/..)
include $(ROOT_DIR)/Constants.mk

TOOLS = \
	chart/cmonitor_chart.py \
	filter/cmonitor_filter.py \
	statistics/cmonitor_statistics.py
SYMLINKS = \
	chart/cmonitor_chart \
	filter/cmonitor_filter \
	statistics/cmonitor_statistics

# cmonitor_version.py has to be listed explicitly because it does not exist yet
# when the $(wilcard) runs (after a clean checkout)
COMMON_CODE = \
	$(wildcard common-code/*.py) \
	common-code/cmonitor_version.py

#
# BUILD TARGETS
#

all: $(TOOLS) 

#$(SYMLINKS)

clean:
	rm -f $(SYMLINKS)

install: $(TOOLS)
ifndef BINDIR
	@echo "*** ERROR: please call this makefile supplying explicitly the BINDIR variable"
	@exit 1
endif
ifndef PYTHON3_SITELIB
	@echo "*** ERROR: please call this makefile supplying explicitly the PYTHON3_SITELIB variable"
	@exit 1
endif
	@mkdir -p $(DESTDIR)/$(BINDIR)/
	@for tool in $(SYMLINKS); do \
		cp -fv $${tool}.py             $(DESTDIR)/$(BINDIR)/$$(basename $$tool) ; \
		chmod a+x $(DESTDIR)/$(BINDIR)/$$(basename $$tool) ; \
	done
	@mkdir -p $(DESTDIR)/$(PYTHON3_SITELIB)/
	cp -fv $(COMMON_CODE) $(DESTDIR)/$(PYTHON3_SITELIB)/

test:
	$(MAKE) -C filter        test
	$(MAKE) -C statistics    test
	# FIXME:
	# $(MAKE) -C chart         test

# TOOLS

common-code/cmonitor_version.py: common-code/cmonitor_version.py.j2
	cp common-code/cmonitor_version.py.j2 common-code/cmonitor_version.py
	sed -i 's@__RPM_VERSION__@$(CMONITOR_VERSION)@g' common-code/cmonitor_version.py
	sed -i 's@__RPM_RELEASE__@$(CMONITOR_RELEASE)@g' common-code/cmonitor_version.py
	sed -i 's@__LAST_COMMIT_HASH__@$(CMONITOR_LAST_COMMIT_HASH)@g' common-code/cmonitor_version.py

$(TOOLS): common-code/cmonitor_version.py

#$(SYMLINKS): common-code/cmonitor_version.py
#	@ln -sfv $(shell readlink -f $@.py) $@



#
# RPM TARGETS
#

srpm_tarball:
	$(MAKE) clean # make sure we don't include any build result in the source RPM!
	mkdir -p $(RPM_TMP_DIR)/ $(RPM_TARBALL_DIR)/
	rm -rf $(RPM_TMP_DIR)/* $(RPM_TARBALL_DIR)/*
	# prepare the tarball with
	#  a) the whole project inside a versioned folder
	#  b) the spec file with the version inside it replaced
	cp -arf $(ROOT_DIR)/ $(RPM_TARBALL_DIR)/ && \
		cd $(RPM_TARBALL_DIR) && \
		rm -rf cmonitor/examples && \
		rm -rf cmonitor/collector/bin && \
		rm -rf cmonitor/collector/src/tests && \
		mv cmonitor cmonitor-tools-$(CMONITOR_VERSION) && \
		sed -i 's@__RPM_VERSION__@$(CMONITOR_VERSION)@g' cmonitor-tools-$(CMONITOR_VERSION)/tools/spec/tools.spec && \
		sed -i 's@__RPM_RELEASE__@$(CMONITOR_RELEASE)@g' cmonitor-tools-$(CMONITOR_VERSION)/tools/spec/tools.spec && \
		sed -i 's@__LAST_COMMIT_HASH__@$(CMONITOR_LAST_COMMIT_HASH)@g' cmonitor-tools-$(CMONITOR_VERSION)/tools/spec/tools.spec && \
		tar cvzf $(RPM_TMP_DIR)/cmonitor-tools-$(CMONITOR_VERSION).tar.gz cmonitor-tools-$(CMONITOR_VERSION)/*
#
# This target is used by Fedora COPR to automatically produce RPMs for lots of distros.
# COPR will invoke this target like that:
#   make -f <cloned_repodir>/.copr/Makefile srpm outdir="<outdir>" spec="<spec_path>"
# See https://docs.pagure.org/copr.copr/user_documentation.html#make-srpm.
# E.g.:
#   make -f /home/francesco/work/cmonitor/.copr/Makefile srpm outdir=/tmp/cmonitor-rpm
srpm: srpm_tarball
ifndef outdir
	@echo "*** ERROR: please call this makefile supplying explicitly the outdir variable"
	@exit 1
endif
	# now build the SRPM and copy it to the $(outdir) provided by COPR
	# IMPORTANT: use the spec file that has been edited by the "srpm_tarball" to replace __RPM_VERSION__ 
	rpmbuild -bs $(RPM_TARBALL_DIR)/cmonitor-tools-$(CMONITOR_VERSION)/tools/spec/tools.spec \
	  --define "_topdir $(RPM_TMP_DIR)" \
	  --define "_sourcedir $(RPM_TMP_DIR)" \
	  --define "_builddir $(RPM_TMP_DIR)" \
	  --define "_rpmdir $(RPM_TMP_DIR)" && \
		mkdir -p $(outdir)/ && \
		rm -f $(outdir)/cmonitor-tools-*.src.rpm && \
		cp -fv $(RPM_TMP_DIR)/SRPMS/cmonitor-tools-*.src.rpm $(outdir)/

#
# This is useful to produce a binary RPM:
# (This is not used by COPR but still may be useful for local tests)
#
rpm: srpm
ifndef outdir
	@echo "*** ERROR: please call this makefile supplying explicitly the outdir variable"
	@exit 1
endif
	cd $(outdir) && \
		rpmbuild --define "_rpmdir $(RPM_TMP_DIR)" --rebuild cmonitor-tools-*.src.rpm  && \
		mkdir -p $(outdir)/ && \
		cp -fv $(RPM_TMP_DIR)/x86_64/cmonitor-*.rpm $(outdir)/






