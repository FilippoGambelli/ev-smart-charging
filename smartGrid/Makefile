CONTIKI_PROJECT = sensorPV-coap-server
all: $(CONTIKI_PROJECT)

# Do not try to build on Sky because of code size limitation
PLATFORMS_EXCLUDE = sky z1

CONTIKI = ../../..

MODULES_REL += ./resources

MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap

# Include optional target-specific module
include $(CONTIKI)/Makefile.identify-target
MODULES_REL += $(TARGET)

include $(CONTIKI)/Makefile.include