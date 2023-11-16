# Note: type 'make Q=' to get make commands debug 
Q=@
# FLEDGE plugin API is incomaptible with "runtime/references"
# SONAR exclusion pattern is incomaptible with "whitespace/comments"
CPPLINT_EXCLUDE='-build/include_subdir,-build/c++11,-whitespace/comments,-runtime/references'
FLEDGE_REST=http://localhost:8081/fledge
PIPE_FILTER_NAME=pivottoopcua
OPCUA_SERVICE_NAME=injector

all: build install_plugin # insert_task
build:
	$(Q)mkdir -p build
	$(Q)cd build && cmake -DCMAKE_BUILD_TYPE=Release -DFLEDGE_INSTALL=$(FLEDGE_INSTALL) ..
	$(Q)make -C build -j4
	
unit_tests:
	$(Q)rm -rf build/tests/RunTests_coverage_html
	$(Q)mkdir -p build/tests
	$(Q)cd build && cmake -DCMAKE_BUILD_TYPE=Coverage -DFLEDGE_INSTALL=$(FLEDGE_INSTALL) ..
	$(Q)make -C build/tests RunTests_coverage_html -j4
	@echo "See unit tests coverage result in build/tests/RunTests_coverage_html/index.html"

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

create_pipeline_filter:
	@echo "Create a pipeline with '$(PIPE_FILTER_NAME)' plugin..."
	$(Q)curl -sX POST "$(FLEDGE_REST)/filter" -d "{\"name\":\"$(PIPE_FILTER_NAME)\",\"plugin\":\"$(PIPE_FILTER_NAME)\"}" && echo
del_pipeline_filter:
	@echo "Delete pipeline '$(PIPE_FILTER_NAME)'"
	$(Q)curl -sX DELETE "$(FLEDGE_REST)/filter/$(PIPE_FILTER_NAME)"
	
add_injector_filter:
	@echo "Insert filter $(PIPE_FILTER_NAME) in ${OPCUA_SERVICE_NAME} plugin..."
	$(Q)curl -sX PUT "$(FLEDGE_REST)/filter/${OPCUA_SERVICE_NAME}/pipeline" -d "{\"pipeline\":[\"$(PIPE_FILTER_NAME)\"]}"
del_injector_filter:
	@echo "Remove filter $(PIPE_FILTER_NAME) from ${OPCUA_SERVICE_NAME} plugin..."
	$(Q)curl -sX PUT "$(FLEDGE_REST)/filter/${OPCUA_SERVICE_NAME}/pipeline" -d "{\"pipeline\":[]}"
	
insert_filter: create_pipeline_filter add_injector_filter
remove_filter: del_injector_filter del_pipeline_filter
	
install_plugin: check
	@echo "Install plugin..."
	$(Q) make -C build install || sudo make -C build install
	
cpplint:
	$(Q)cpplint --output=eclipse --repository=src --linelength=120 --filter=$(CPPLINT_EXCLUDE) --exclude=include/opcua_statuscodes.h src/* include/*
	
.PHONY: all clean build check del_plugin cpplint unit_tests install_plugin add_injector_filter del_injector_filter create_pipeline_filter del_pipeline_filter
