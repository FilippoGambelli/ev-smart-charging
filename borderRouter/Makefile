CONTIKI_PROJECT = border-router
all: $(CONTIKI_PROJECT)

# Do not try to build on Sky because of code size limitation
PLATFORMS_EXCLUDE = sky z1

CONTIKI = ../../..

MODULES_REL += ./resources
MODULES_REL += ./power-manager

# Include RPL BR module
include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_SERVICES_DIR)/rpl-border-router
# Include webserver module
MODULES_REL += webserver

MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap

# Include optional target-specific module
include $(CONTIKI)/Makefile.identify-target
MODULES_REL += $(TARGET)

MAKE_NET_IPV6 = 1

MAKE_NET ?= MAKE_NET_IPV6
ifeq ($(MAKE_NET),MAKE_NET_IPV6)
CFLAGS += -DNETSTACK_CONF_WITH_IPV6=1
MODULES += $(CONTIKI_NG_NET_DIR)/ipv6
endif

ifeq ($(MAKE_NET),MAKE_NET_IPV6)
MAKE_ROUTING ?= MAKE_ROUTING_RPL_LITE
else
MAKE_ROUTING ?= MAKE_ROUTING_NULLROUTING
endif 

include $(CONTIKI)/Makefile.include