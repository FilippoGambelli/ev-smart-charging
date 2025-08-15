#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-debug.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

#define MAX_DEVICES 20  // Maximum number of devices that can be registered

// Structure to store information about a registered device
struct device_info {
  uint8_t id;           // Device ID
  uip_ipaddr_t addr;    // Device IPv6 address
};

static struct device_info devices[MAX_DEVICES];  // Array to store registered devices
static uint8_t device_count = 0;                 // Current number of registered devices

// Function prototypes
static void res_register_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static uint8_t register_device(const uip_ipaddr_t *src_addr);

// Define the CoAP resource "res_register" for device registration
RESOURCE(res_register,
        "title=\"Register Device\";rt=\"Control\"",
        NULL,                       // GET handler (not used)
        res_register_post_handler,  // POST handler
        NULL,                       // PUT handler (not used)
        NULL);                      // DELETE handler (not used)

// Handler for POST requests to register a new device
static void res_register_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  uip_ipaddr_t src_addr;
  // Get the source IP address of the device making the request
  uip_ipaddr_copy(&src_addr, &UIP_IP_BUF->srcipaddr);

  // Attempt to register the device and get its assigned ID
  uint8_t id = register_device(&src_addr);
  if(id > 0) {
    // If registration was successful, return the assigned ID in the response payload
    char id_str[4];
    snprintf(id_str, sizeof(id_str), "%u", id);
    memcpy(buffer, id_str, strlen(id_str));
    coap_set_payload(response, buffer, strlen(id_str));
    coap_set_status_code(response, CREATED_2_01); // HTTP 201 Created equivalent
  } else {
    // If registration failed (list full), send Service Unavailable
    coap_set_status_code(response, SERVICE_UNAVAILABLE_5_03);
  }
}

// Register a device by storing its IP address and assigning an ID
static uint8_t register_device(const uip_ipaddr_t *src_addr) {
  if(device_count < MAX_DEVICES) {
    uint8_t new_id = device_count + 1;
    devices[device_count].id = new_id;
    uip_ipaddr_copy(&devices[device_count].addr, src_addr);
    device_count++;
    LOG_INFO("New device registered: ID = %d from ", new_id);
    //uip_debug_ipaddr_print(src_addr);
    LOG_INFO_6ADDR(src_addr);
    LOG_INFO_("\n");
    return new_id;
  } else {
    LOG_INFO("Device list is full");
    LOG_INFO_("\n");
    return 0; // Registration failed
  }
}