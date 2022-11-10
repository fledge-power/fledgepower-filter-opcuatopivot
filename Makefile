# Note: type 'make Q=' to get make commands debug 
Q=@
# FLEDGE plugin API is incomaptible with "runtime/references"
# SONAR exclusion pattern is incomaptible with "whitespace/comments"
CPPLINT_EXCLUDE='-build/include_subdir,-build/c++11,-whitespace/comments,-runtime/references'

all: build install_plugin # insert_task
build:
	$(Q)mkdir -p build
	$(Q)cd build && cmake -DCMAKE_BUILD_TYPE=Release -DFLEDGE_INSTALL=$(FLEDGE_INSTALL) ..
	$(Q)make -C build -j4
	
clean:
	$(Q)rm -fr build
log:
	@echo "Showing logs from libpivottoopcua plugin"
	$(Q)tail -f /var/log/syslog |grep -o 'Fledge .*$$'
	
check:
	@echo "Check validity of plugin 'libpivottoopcua.so'..."
	$(Q)! [ -z "$(FLEDGE_SRC)" ] || (echo "FLEDGE_SRC not set" && false)
	$(Q)$(FLEDGE_SRC)/cmake_build/C/plugins/utils/get_plugin_info build/libpivottoopcua.so plugin_info 2> /tmp/get_plugin_info.tmp
	$(Q)! ([ -s /tmp/get_plugin_info.tmp ] && cat /tmp/get_plugin_info.tmp)

install_plugin: check
	@echo "Install plugin..."
	$(Q)sudo make -C build install
	
cpplint:
	$(Q)cpplint --output=eclipse --repository=src --linelength=120 --filter=$(CPPLINT_EXCLUDE) --exclude=include/opcua_statuscodes.h src/* include/*
	
.PHONY: all clean build check del_plugin cpplint
