CONTIKI_PROJECT = coap-server
all: $(CONTIKI_PROJECT)

# Do not try to build on Sky because of code size limitation
PLATFORMS_EXCLUDE = sky z1

CONTIKI = ../../..

MODULES_REL += ./resources

MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap

# Include optional target-specific module
include $(CONTIKI)/Makefile.identify-target
MODULES_REL += $(TARGET)

MODULES_REL += resources/solar-data

MODULES_REL += /home/iot_ubuntu_intel/.local/lib/python3.10/site-packages/emlearn 

TARGET_LIBFILES += -lm

INC += /home/iot_ubuntu_intel/.local/lib/python3.10/site-packages/emlearn

include $(CONTIKI)/Makefile.include