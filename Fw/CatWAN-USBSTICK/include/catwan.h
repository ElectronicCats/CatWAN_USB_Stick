/*
 * CatWAN USB Stick — Main Header
 * RP2040 + RFM95/SX1276 LoRa USB dongle
 * Eduardo Contreras @ Electronic Cats 2026
 */

#ifndef CATWAN_H
#define CATWAN_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <catsniffer_usbd.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <stdlib.h>
#include <fw_version.h>

// Ring buffer and command buffer sizes
#define RING_BUF_SIZE 16384
#define COMMAND_BUF_SIZE 256

// Thread priorities
#define LORA_THREAD_PRIORITY K_PRIO_COOP(5)
#define MAIN_THREAD_PRIORITY K_PRIO_COOP(7)

// LoRa mode definitions
enum LORA_MODE {
	LORA_MODE_STREAM = 0,  // Default: raw binary on CDC0
	LORA_MODE_COMMAND = 1, // Text commands on CDC0
};

// LoRa configuration structure (staged — apply with apply_lora_config())
typedef struct {
	uint32_t frequency;		      // Hz (default: 915000000)
	uint8_t spreading_factor;	      // SF7–SF12 (default: SF7)
	enum lora_signal_bandwidth bandwidth; // BW_125_KHZ, BW_250_KHZ, BW_500_KHZ
	uint8_t coding_rate;		      // CR_4_5 .. CR_4_8 (default: CR_4_5)
	int8_t tx_power;		      // +2 to +17 dBm for RFM95 PA_BOOST (default: 17)
	uint16_t preamble_len;		      // Default: 12
	bool iq_inverted;		      // IQ inversion (default: false/normal)
	bool public_network;		      // Sync word: false=private (0x12), true=public (0x34)
	bool config_pending;		      // true if changes not yet applied
} lora_config_t;

// CatWAN state structure
typedef struct {
	unsigned long led_interval;
	int64_t previous_millis;
	// Shell command parsing
	bool command_recognized;
	uint8_t command_counter;
	char command_data[COMMAND_BUF_SIZE];
	size_t command_data_len;
	// LoRa state
	uint8_t lora_mode;	   // LORA_MODE_STREAM or LORA_MODE_COMMAND
	lora_config_t lora_config; // Current LoRa configuration
	bool lora_initialized;	   // Track initialization state
	bool lora_config_lock;	   // Lock flag to pause LoRa operations during reconfiguration
	// Identify state
	bool led_identify;	   // true while identify LED blink sequence is active
} catwan_t;

// Global state
extern catwan_t catwan;

// Function prototypes
void process_lora_command(char *cmd_line);
int queue_radio_command(const char *cmd_line);
int apply_lora_config(void);

#endif /* CATWAN_H */
