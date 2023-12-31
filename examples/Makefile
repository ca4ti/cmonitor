#
# Makefile to regenerate example data
# 
TOPDIR=..
DOCKER_EXAMPLE_FOLDER:=$(shell readlink -f .)
THIS_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
ROOT_DIR:=$(shell readlink -f $(THIS_DIR)/..)
SHELL:=/bin/bash 
include $(ROOT_DIR)/Constants.mk


# ------------------------------------------------------------------------------------------------------------------------------
# List of examples JSONs
# ------------------------------------------------------------------------------------------------------------------------------

BAREMETAL_MONITORING_EXAMPLE_JSON_FILES=\
	baremetal1.json.gz \
	docker-collecting-baremetal-stats.json.gz

# FIXME: baremetal2.json.gz needs to be re-generated 
#	baremetal2.json.gz

DOCKER_MONITORING_EXAMPLE_JSON_FILES=\
	docker-userapp.json.gz \
	docker-stress-test-cpu.json.gz \
	docker-stress-test-mem.json.gz \
	docker-collecting-docker-stats.json.gz

EXAMPLE_JSON_FILES=$(BAREMETAL_MONITORING_EXAMPLE_JSON_FILES) $(DOCKER_MONITORING_EXAMPLE_JSON_FILES)


# ------------------------------------------------------------------------------------------------------------------------------
# Constants to launch cmonitor_collector from baremetal or as a Docker
# ------------------------------------------------------------------------------------------------------------------------------

CMONITOR_COLLECTOR=$(TOPDIR)/collector/bin/glibc/cmonitor_collector

COMMON_CMONITOR_DOCKER_OPTIONS=\
	--rm \
	--name=cmonitor-baremetal-collector \
	--network=host \
	--pid=host \
	--volume=/sys:/sys:ro \
	--volume=/etc/os-release:/etc/os-release:ro \
	--volume=$(DOCKER_EXAMPLE_FOLDER):/perf:rw

# ------------------------------------------------------------------------------------------------------------------------------
# Python tools constants
# ------------------------------------------------------------------------------------------------------------------------------

PYTHON_COMMON_CODE=$(ROOT_DIR)/tools/common-code
PYTHON_TOOLS_DIR=../tools

# ------------------------------------------------------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------------------------------------------------------

all:
	@echo "Now using cmonitor tools on the gzipped example JSON files"
	export PYTHONPATH=$(PYTHON_COMMON_CODE) ; \
	for jsonExample in $(EXAMPLE_JSON_FILES); do \
		echo "-- TOOL: cmonitor_chart ------------------------------------------------------" ; \
		$(PYTHON_TOOLS_DIR)/chart/cmonitor_chart.py $$jsonExample --verbose || exit 10; \
		echo "-- VALIDATING $${jsonExample/.json.gz/.html} ------------------------------------------------------" ; \
		tidy -o /dev/null $${jsonExample/.json.gz/.html} 2>/dev/null && echo "$${jsonExample/.json.gz/.html} is valid HTML" || exit 11; \
		echo "-- TOOL: cmonitor_statistics -------------------------------------------------" ; \
		$(PYTHON_TOOLS_DIR)/statistics/cmonitor_statistics.py --output=/tmp/stats_$$jsonExample $$jsonExample || exit 12; \
		echo "-- TOOL: cmonitor_filter -------------------------------------------------" ; \
		$(PYTHON_TOOLS_DIR)/filter/cmonitor_filter.py --verbose --task_name=TEST --output=/tmp/filtered_$$jsonExample $$jsonExample || exit 12; \
	done

regen_all_example_jsons:
	# baremetal monitoring:
	$(MAKE) regen_baremetal1                              # takes about 10mins to run
	$(MAKE) regen_docker_collecting_baremetal_stats       # takes about 3mins to run
	# docker monitoring:
	$(MAKE) regen_docker_userapp                          # takes about 3mins to run
	$(MAKE) regen_docker_stress_test_cpu                  # takes about 15secs to run
	$(MAKE) regen_docker_stress_test_mem                  # takes about 15secs to run
	$(MAKE) regen_docker_collecting_docker_stats          # takes about 3mins to run

clean:
	rm -f *.err docker-*/example-load.sh
	
distclean: clean
	rm -f *.html


# ------------------------------------------------------------------------------------------------------------------------------
# BAREMETAL MONITORING
# ------------------------------------------------------------------------------------------------------------------------------

#
# baremetal1 example:
#  example of graph generated with the performance stats collected from a physical (baremetal) server running Centos7
#

regen_baremetal1:
	@echo "Launching load simulator to have some CPU at 100% and so on..."
	@-pkill example-load.sh # if any
	./example-load.sh &
	@echo "Regenerating example JSON... this will take about 10minutes (60 samples every 10secs)."
	# IMPORTANT: the sampling interval has to be small enough to let cmonitor_collector detect the PIDs of the transient "stress"
	#            instances produced at random by the example-load.sh script
	@$(CMONITOR_COLLECTOR) \
		--sampling-interval=1 \
		--num-samples=600 \
		--output-pretty \
		--output-filename baremetal1 \
		--foreground \
		--debug
	@pkill example-load || true
	@jq -e . baremetal1.json >/dev/null && ( echo "Valid JSON produced" ) || ( echo "INVALID JSON WAS PRODUCED" ; exit 10 )
	@gzip -f baremetal1.json

#
# NOTE: baremetal2 is a real-world example and thus cannot be regenerated automatically just like others;
#       however here's a quick guide on how to do that on a real server:
#
#   - git clone cmonitor repo
#   - make -j -C collector
#   - collector/bin/glibc/cmonitor_collector --sampling-interval=10 --num-samples=1440 --output-filename baremetal2
#

regen_docker_collecting_baremetal_stats:
	docker run \
		$(COMMON_CMONITOR_DOCKER_OPTIONS) \
		f18m/cmonitor:$(DOCKER_TAG) \
		--sampling-interval=1 \
		--num-samples=180 \
		--collect=cpu,memory,disk,network \
		--output-filename=docker-collecting-baremetal-stats
	@gzip -f docker-collecting-baremetal-stats.json


# ------------------------------------------------------------------------------------------------------------------------------
# DOCKER MONITORING
# ------------------------------------------------------------------------------------------------------------------------------

# docker_userapp:
#  monitoring of a docker container from the baremetal
#
# NOTES:
# - Redis + example_load_redis.py is used as "model" of a generic user application 
# - cpuset cgroup is used (--cpuset-cpus): 2 cpus allowed regardless of how many are available
# - cpuacct cgroup is used (--cpus): 0.9 cpu time max can be consumed on each allowed cpu
# - memory cgroup is used (--memory) max 10MB can be consumed
# - cgroup_thread collection is enabled and score-filtering is disabled to make sure to capture
#   all 5 threads of Redis software, despite most of these having 0% CPU usage
#
DOCKER_EXAMPLE_CPUS:=0,1
DOCKER_EXAMPLE_CPU_QUOTA:=0.9
DOCKER_EXAMPLE_MEMORY_LIMIT:=10m

ifeq ($(PODMAN),)
# by default assume Moby Docker is in use; provide PODMAN=1 to instead assume podman implementation is in use
PODMAN:=0
endif

ifeq ($(PODMAN),0)
# NOTE: we know that docker creates a cgroup always named docker/container-ID
CGROUP_NAME=docker/$$(docker ps --no-trunc -aqf name=userapp)
else
CGROUP_NAME=machine.slice/libpod-$$(docker ps --no-trunc -aqf name=userapp).scope/container/
endif

__run_redis_docker_as_userapp:
	-docker stop userapp >/dev/null 2>&1
	docker run \
		--detach \
		--interactive \
		--tty \
		--rm \
		--cpuset-cpus=$(DOCKER_EXAMPLE_CPUS) \
		--cpus=$(DOCKER_EXAMPLE_CPU_QUOTA) \
		--memory=$(DOCKER_EXAMPLE_MEMORY_LIMIT) \
		--volume=$(DOCKER_EXAMPLE_FOLDER):/perf \
		-P \
		--name userapp \
		redis:latest

regen_docker_userapp:
	-pkill example_load_re >/dev/null 2>&1
	$(MAKE) __run_redis_docker_as_userapp
	sleep 3
	for i in $$(seq 1 3); do \
		echo "Spawning instance $i of the simple Redis load application" ; \
		./example_load_redis.py userapp & \
		sleep 1 ; \
	done
	@echo "Now running cmonitor_collector on the baremetal to monitor for 2 minutes the 'userapp' container:"
	$(CMONITOR_COLLECTOR) \
		--sampling-interval=0.5 \
		--num-samples=240 \
		--collect=cgroup_cpu,cgroup_memory,cgroup_threads,cgroup_network \
		--foreground \
		--output-filename=docker-userapp.json \
		--cgroup-name=$(CGROUP_NAME) \
		--score-threshold=0 \
		--custom-metadata="cmonitor_chart_name:userapp (Redis docker)" \
		--custom-metadata="some_other_useful_data:that can contain spaces as well"
	pkill example_load_re
	docker stop userapp
	gzip -f docker-userapp.json


STRESS_TEST_DOCKER_CPUS:=0,1
STRESS_TEST_DOCKER_CPU_QUOTA:=2

ifeq ($(PODMAN),0)
# NOTE: we know that docker creates a cgroup always named docker/container-ID
STRESS_TEST_CGROUP_NAME=docker/$$(docker ps --no-trunc -aqf name=stress)
else
STRESS_TEST_CGROUP_NAME=machine.slice/libpod-$$(docker ps --no-trunc -aqf name=stress).scope/container/
endif

regen_docker_stress_test_cpu:
	-docker stop stress >/dev/null 2>&1
	docker run \
		--detach \
		--interactive \
		--tty \
		--rm \
		--cpuset-cpus=$(STRESS_TEST_DOCKER_CPUS) \
		--cpus=$(STRESS_TEST_DOCKER_CPU_QUOTA) \
		--name stress \
		alexeiled/stress-ng \
		--cpu $(STRESS_TEST_DOCKER_CPU_QUOTA) --metrics-brief
	sleep 3
	@echo "Now running cmonitor_collector on the baremetal to monitor for 15secs the 'stress' docker:"
	$(CMONITOR_COLLECTOR) \
		--sampling-interval=0.5 \
		--num-samples=30 \
		--collect=cgroup_cpu,cgroup_memory,cgroup_threads,cgroup_network \
		--foreground \
		--output-filename=docker-stress-test-cpu.json \
		--cgroup-name=$(STRESS_TEST_CGROUP_NAME) \
		--score-threshold=0 \
		--custom-metadata="cmonitor_chart_name:stress (Redis docker)"
	docker stop stress
	gzip -f docker-stress-test-cpu.json


STRESS_TEST_DOCKER_MEMORY:=200M

regen_docker_stress_test_mem:
	-docker stop stress >/dev/null 2>&1
	docker run \
		--detach \
		--interactive \
		--tty \
		--memory=$(STRESS_TEST_DOCKER_MEMORY) \
		--name stress \
		alexeiled/stress-ng \
		--vm 1 --vm-bytes $(STRESS_TEST_DOCKER_MEMORY) --metrics-brief
	sleep 3
	@echo "Now running cmonitor_collector on the baremetal to monitor for 15secs the 'stress' docker:"
	$(CMONITOR_COLLECTOR) \
		--sampling-interval=0.5 \
		--num-samples=30 \
		--collect=cgroup_cpu,cgroup_memory,cgroup_threads,cgroup_network \
		--foreground \
		--output-filename=docker-stress-test-mem.json \
		--cgroup-name=$(STRESS_TEST_CGROUP_NAME) \
		--score-threshold=0 \
		--custom-metadata="cmonitor_chart_name:stress (Redis docker)"
	docker stop stress
	docker logs stress
	docker rm stress
	gzip -f docker-stress-test-mem.json


regen_docker_collecting_docker_stats:
	$(MAKE) __run_redis_docker_as_userapp
	docker run \
		$(COMMON_CMONITOR_DOCKER_OPTIONS) \
		f18m/cmonitor:$(DOCKER_TAG) \
		--sampling-interval=1 \
		--num-samples=180 \
		--collect=cgroup_threads,cgroup_cpu,cgroup_memory,cgroup_network --score-threshold=0 \
		--custom-metadata="cmonitor_chart_name:userapp" \
		--cgroup-name=$(CGROUP_NAME) \
		--output-filename=docker-collecting-docker-stats
	@gzip -f docker-collecting-docker-stats.json


# ------------------------------------------------------------------------------------------------------------------------------
# INFLUXDB & GRAFANA example:
# ------------------------------------------------------------------------------------------------------------------------------

reset_influxdb:
	# assuming you started in another console or in background the "influxd" daemon:
	influx -execute "DROP DATABASE cmonitor"
	influx -execute "CREATE DATABASE cmonitor"

show_cmonitor_data_in_influxdb:
	influx -execute 'SHOW SERIES' -database="cmonitor"
	influx -execute 'SELECT * FROM "stat_cpu0" LIMIT 20' -database="cmonitor"

regen_grafana_screenshots:
	@echo "Running InfluxDB inside a Docker"
	@docker stop influxdb || true
	rm -rf /tmp/influxdb/*   # remove data from previous runs
	docker pull influxdb
	docker run -d \
		--rm \
		-p 8086:8086 \
		-v /tmp/influxdb:/var/lib/influxdb \
		--name=influxdb \
		influxdb
	@echo "Running Grafana inside a Docker"
	@# Note that with --link it is possible to configure InfluxDB as source with link "http://influxdb:8086"
	@docker stop grafana || true
	docker pull grafana/grafana
	docker run -d \
		--rm \
		-p 3000:3000 \
		--link=influxdb \
		--name=grafana \
		grafana/grafana
	@# IMPORTANT: create the 'cmonitor' database before running the collector:
	#influx -execute "CREATE DATABASE cmonitor"
	sleep 10 # give time to influxDB to start!
	curl -i -XPOST http://localhost:8086/query --data-urlencode "q=CREATE DATABASE cmonitor"    # do not depend on "influx" CLI utility
	@echo "InfluxDB and Grafana dockers should be up and running... launching cmonitor to collect 10min of data"
	@$(CMONITOR_COLLECTOR) \
		--sampling-interval=2 --num-samples=300 \
		--remote-ip=localhost --remote-port=8086 \
		--output-filename=none \
		--debug \
		--foreground
	@echo "Now you can login from your browser at http://localhost:3000/login and create a Grafana dashboard to pick"
	@echo "data from the 'http://influxdb:8086' datasource, database 'cmonitor'..."
 
.PHONY: reset_influxdb show_cmonitor_data_in_influxdb regen_grafana_screenshots

