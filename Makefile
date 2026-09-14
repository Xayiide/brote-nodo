#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := brote-nodo

-include credentials.mk

ifeq ($(filter clean menuconfig size,$(MAKECMDGOALS)),)

    $(foreach var,WIFI_SSID WIFI_PASS,\
      $(if $(strip $($(var))),,$(error $(var) está vacío o no definido)))

endif

export WIFI_SSID
export WIFI_PASS

include $(IDF_PATH)/make/project.mk
