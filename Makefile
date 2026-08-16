#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := brote-nodo

-include credentials.mk

ifeq ($(filter clean menuconfig size,$(MAKECMDGOALS)),)

    ifeq ($(strip $(NODE)),)
        $(warning NODE no está definido. Usa: make NODE=terraza)
        NODE := terraza
    endif

    $(foreach var,WIFI_SSID WIFI_PASS DST_IP DATA_PORT LOG_PORT,\
      $(if $(strip $($(var))),,$(error $(var) está vacío o no definido)))

    ifeq ($(NODE),terraza)
        CFLAGS += -DNODE_TERRAZA
    else ifeq ($(NODE),interior)
        CFLAGS += -DNODE_INTERIOR
    else
        $(error NODE='$(NODE)' no reconocido. Valores válidos: terraza, interior)
    endif

endif

export WIFI_SSID
export WIFI_PASS
export DST_IP
export DATA_PORT
export LOG_PORT

include $(IDF_PATH)/make/project.mk