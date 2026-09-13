COMPONENT_ADD_INCLUDEDIRS := include
COMPONENT_REQUIRES := log

CFLAGS += -DIP=\"$(strip $(IP))\"
CFLAGS += -DGW=\"$(strip $(GW))\"
CFLAGS += -DSUBMASK=\"$(strip $(SUBMASK))\"
CFLAGS += -DDST_IP=\"$(strip $(DST_IP))\"
CFLAGS += -DDATA_PORT=$(strip $(DATA_PORT))
CFLAGS += -DLOG_PORT=$(strip $(LOG_PORT))
CFLAGS += -DLISTEN_PORT=$(strip $(LISTEN_PORT))