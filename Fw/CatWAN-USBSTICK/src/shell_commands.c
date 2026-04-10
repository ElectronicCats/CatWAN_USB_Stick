/*
 * shell_commands.c - Command Table Based Shell
 * CatWAN USB Stick — LoRa-only (SX1276/RFM95)
 */

#include "shell_commands.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <pico/bootrom.h>

#include "catwan.h"

// External functions from main.c
void shell_reply(const char *msg);
void set_status_leds(int l0, int l1, int l2);

// Command handler type
typedef void (*cmd_handler_t)(char *args);

// Command table entry
typedef struct {
	const char *name;
	cmd_handler_t handler;
	const char *help;
	bool prefix_match; // true for commands with args
} shell_cmd_t;

// Forward declarations
static void cmd_help(char *args);
static void cmd_reboot(char *args);
static void cmd_status(char *args);
static void cmd_fw_version(char *args);
static void cmd_lora_freq(char *args);
static void cmd_lora_sf(char *args);
static void cmd_lora_bw(char *args);
static void cmd_lora_cr(char *args);
static void cmd_lora_power(char *args);
static void cmd_lora_mode(char *args);
static void cmd_lora_preamble(char *args);
static void cmd_lora_syncword(char *args);
static void cmd_lora_iq(char *args);
static void cmd_lora_config(char *args);
static void cmd_lora_apply(char *args);
static void cmd_radio(char *args);
static void cmd_identify(char *args);

// Command table
static const shell_cmd_t commands[] = {
	{ "help", cmd_help, "Show available commands", false },
	{ "reboot", cmd_reboot, "RP2040 USB bootloader", false },
	{ "status", cmd_status, "Device status", false },
	{ "fw_version", cmd_fw_version, "Show firmware build version", false },
	{ "lora_freq", cmd_lora_freq, "Set frequency (Hz)", true },
	{ "lora_sf", cmd_lora_sf, "Set spreading factor", true },
	{ "lora_bw", cmd_lora_bw, "Set bandwidth (kHz)", true },
	{ "lora_cr", cmd_lora_cr, "Set coding rate", true },
	{ "lora_power", cmd_lora_power, "Set TX power (dBm)", true },
	{ "lora_mode", cmd_lora_mode, "stream|command mode", true },
	{ "lora_preamble", cmd_lora_preamble, "Set preamble length", true },
	{ "lora_syncword", cmd_lora_syncword, "Set sync word (private|public)",
	  true },
	{ "lora_iq", cmd_lora_iq, "normal|inverted IQ", true },
	{ "lora_config", cmd_lora_config, "Show LoRa config", false },
	{ "lora_apply", cmd_lora_apply, "Apply pending config", false },
	{ "identify", cmd_identify, "Blink board LED for identification", false },
	{ "radio", cmd_radio, "Forward radio cmd (TEST|TX)", true },
	{ NULL, NULL, NULL, false }
};

// Command implementations

static void cmd_help(char *args)
{
	shell_reply("Commands:\r\n");
	for (const shell_cmd_t *cmd = commands; cmd->name != NULL; cmd++) {
		char buf[80];
		snprintf(buf, sizeof(buf), "  %-14s - %s\r\n", cmd->name,
			 cmd->help);
		shell_reply(buf);
	}
}

static void cmd_reboot(char *args)
{
	shell_reply("Entering USB bootloader...\r\n");
	k_msleep(100);
	reset_usb_boot(0, 0);
}

static void cmd_status(char *args)
{
	char buf[320];
	const char *mode_str = (catwan.lora_mode == LORA_MODE_STREAM) ?
				       "Stream" :
				       "Command";
	const char *lora_status =
		catwan.lora_initialized ? "initialized" : "not initialized";
	snprintf(buf, sizeof(buf),
		 "Board: CatWAN USB Stick\r\n"
		 "Radio: SX1276/RFM95\r\n"
		 "LoRa: %s\r\n"
		 "LoRa Mode: %s\r\n"
		 "Frequency: %u Hz\r\n"
		 "FW: %s\r\n",
		 lora_status, mode_str,
		 catwan.lora_config.frequency, CATSNIFFER_FW_VERSION);
	shell_reply(buf);
}

static void cmd_fw_version(char *args)
{
	char buf[320];
	snprintf(buf, sizeof(buf),
		 "FW: %s\r\nGit: %s (%s)\r\nBuilt: %s\r\nCompiler: %s %s\r\n",
		 CATSNIFFER_FW_VERSION, CATSNIFFER_GIT_SHA,
		 CATSNIFFER_GIT_DIRTY, CATSNIFFER_BUILD_TIME_UTC,
		 CATSNIFFER_COMPILER_ID, CATSNIFFER_COMPILER_VERSION);
	shell_reply(buf);
}

#define IDENTIFY_BLINK_STEPS 20 /* 10 on + 10 off = 10 full blinks */
#define IDENTIFY_BLINK_MS 100

static int identify_steps_remaining;

static void identify_timer_expiry(struct k_timer *timer)
{
	if (identify_steps_remaining <= 0) {
		k_timer_stop(timer);
		set_status_leds(0, 0, 0);
		catwan.led_identify = false;
		return;
	}
	int on = (identify_steps_remaining % 2 == 0);
	set_status_leds(on, on, on);
	identify_steps_remaining--;
}

K_TIMER_DEFINE(identify_timer, identify_timer_expiry, NULL);

static void cmd_identify(char *args)
{
	k_timer_stop(&identify_timer);
	catwan.led_identify = true;
	identify_steps_remaining = IDENTIFY_BLINK_STEPS;
	shell_reply("Identifying board...\r\n");
	k_timer_start(&identify_timer, K_MSEC(IDENTIFY_BLINK_MS),
		      K_MSEC(IDENTIFY_BLINK_MS));
}

static void cmd_lora_freq(char *args)
{
	while (*args && *args != ' ')
		args++;
	while (*args == ' ')
		args++;

	if (*args == '\0') {
		shell_reply("Usage: lora_freq <Hz>\r\n");
		return;
	}

	uint32_t freq = (uint32_t)atoi(args);
	if (freq < 137000000 || freq > 1020000000) {
		shell_reply("Error: Frequency must be 137-1020 MHz\r\n");
		return;
	}

	catwan.lora_config.frequency = freq;
	catwan.lora_config.config_pending = true;

	char buf[64];
	snprintf(buf, sizeof(buf), "Frequency set to %u Hz (pending)\r\n",
		 freq);
	shell_reply(buf);
}

static void cmd_lora_sf(char *args)
{
	while (*args && *args != ' ')
		args++;
	while (*args == ' ')
		args++;

	if (*args == '\0') {
		shell_reply("Usage: lora_sf <7-12>\r\n");
		return;
	}

	int sf = atoi(args);
	if (sf < 7 || sf > 12) {
		shell_reply("Error: Spreading factor must be 7-12\r\n");
		return;
	}

	catwan.lora_config.spreading_factor = (uint8_t)sf;
	catwan.lora_config.config_pending = true;

	char buf[64];
	snprintf(buf, sizeof(buf), "Spreading Factor set to SF%d (pending)\r\n",
		 sf);
	shell_reply(buf);
}

static void cmd_lora_bw(char *args)
{
	while (*args && *args != ' ')
		args++;
	while (*args == ' ')
		args++;

	if (*args == '\0') {
		shell_reply("Usage: lora_bw <125|250|500>\r\n");
		return;
	}

	int bw = atoi(args);
	enum lora_signal_bandwidth bw_enum;

	switch (bw) {
	case 125:
		bw_enum = BW_125_KHZ;
		break;
	case 250:
		bw_enum = BW_250_KHZ;
		break;
	case 500:
		bw_enum = BW_500_KHZ;
		break;
	default:
		shell_reply("Error: Bandwidth must be 125, 250, or 500 "
			    "kHz\r\n");
		return;
	}

	catwan.lora_config.bandwidth = bw_enum;
	catwan.lora_config.config_pending = true;

	char buf[64];
	snprintf(buf, sizeof(buf), "Bandwidth set to %d kHz (pending)\r\n", bw);
	shell_reply(buf);
}

static void cmd_lora_cr(char *args)
{
	while (*args && *args != ' ')
		args++;
	while (*args == ' ')
		args++;

	if (*args == '\0') {
		shell_reply("Usage: lora_cr <5|6|7|8>\r\n");
		return;
	}

	int cr = atoi(args);
	uint8_t cr_enum;

	switch (cr) {
	case 5:
		cr_enum = CR_4_5;
		break;
	case 6:
		cr_enum = CR_4_6;
		break;
	case 7:
		cr_enum = CR_4_7;
		break;
	case 8:
		cr_enum = CR_4_8;
		break;
	default:
		shell_reply("Error: Coding rate must be 5, 6, 7, or 8 (for "
			    "4/5, 4/6, 4/7, 4/8)\r\n");
		return;
	}

	catwan.lora_config.coding_rate = cr_enum;
	catwan.lora_config.config_pending = true;

	char buf[64];
	snprintf(buf, sizeof(buf), "Coding Rate set to 4/%d (pending)\r\n", cr);
	shell_reply(buf);
}

static void cmd_lora_power(char *args)
{
	while (*args && *args != ' ')
		args++;
	while (*args == ' ')
		args++;

	if (*args == '\0') {
		shell_reply("Usage: lora_power <2 to 17>\r\n");
		return;
	}

	int power = atoi(args);
	if (power < 2 || power > 17) {
		shell_reply("Error: TX power must be 2 to 17 dBm "
			    "(RFM95 PA_BOOST)\r\n");
		return;
	}

	catwan.lora_config.tx_power = (int8_t)power;
	catwan.lora_config.config_pending = true;

	char buf[64];
	snprintf(buf, sizeof(buf), "TX Power set to %d dBm (pending)\r\n",
		 power);
	shell_reply(buf);
}

static void cmd_lora_mode(char *args)
{
	while (*args && *args != ' ')
		args++;
	while (*args == ' ')
		args++;

	if (*args == '\0') {
		shell_reply("Usage: lora_mode <stream|command>\r\n");
		return;
	}

	if (strncmp(args, "stream", 6) == 0) {
		catwan.lora_mode = LORA_MODE_STREAM;
		catwan.led_interval = 1000;
		shell_reply("LoRa mode set to STREAM (slow blink)\r\n");
	} else if (strncmp(args, "command", 7) == 0) {
		catwan.lora_mode = LORA_MODE_COMMAND;
		catwan.led_interval = 200;
		shell_reply("LoRa mode set to COMMAND (fast blink)\r\n");
	} else {
		shell_reply("Error: Mode must be 'stream' or 'command'\r\n");
	}
}

static void cmd_lora_preamble(char *args)
{
	while (*args && *args != ' ')
		args++;
	while (*args == ' ')
		args++;

	if (*args == '\0') {
		shell_reply("Usage: lora_preamble <6-65535>\r\n");
		return;
	}

	int preamble = atoi(args);
	if (preamble < 6 || preamble > 65535) {
		shell_reply("Error: Preamble length must be 6-65535\r\n");
		return;
	}

	catwan.lora_config.preamble_len = (uint16_t)preamble;
	catwan.lora_config.config_pending = true;

	char buf[64];
	snprintf(buf, sizeof(buf), "Preamble length set to %d (pending)\r\n",
		 preamble);
	shell_reply(buf);
}

static void cmd_lora_syncword(char *args)
{
	while (*args && *args != ' ')
		args++;
	while (*args == ' ')
		args++;

	if (*args == '\0') {
		shell_reply("Usage: lora_syncword <private|public>\r\n");
		shell_reply("  private  Private network sync word (0x12)\r\n");
		shell_reply("  public   Public/LoRaWAN sync word (0x34)\r\n");
		return;
	}

	if (strncmp(args, "private", 7) == 0) {
		catwan.lora_config.public_network = false;
		catwan.lora_config.config_pending = true;
		shell_reply("Sync word: PRIVATE (0x12) (pending)\r\n");
	} else if (strncmp(args, "public", 6) == 0) {
		catwan.lora_config.public_network = true;
		catwan.lora_config.config_pending = true;
		shell_reply("Sync word: PUBLIC/LoRaWAN (0x34) (pending)\r\n");
	} else {
		shell_reply("Error: Use 'private' or 'public'\r\n");
	}
}

static void cmd_lora_iq(char *args)
{
	while (*args && *args != ' ')
		args++;
	while (*args == ' ')
		args++;

	if (*args == '\0') {
		shell_reply("Usage: lora_iq <normal|inverted>\r\n");
		return;
	}

	if (strncmp(args, "normal", 6) == 0) {
		catwan.lora_config.iq_inverted = false;
		catwan.lora_config.config_pending = true;
		shell_reply("IQ set to NORMAL (pending)\r\n");
	} else if (strncmp(args, "inverted", 8) == 0) {
		catwan.lora_config.iq_inverted = true;
		catwan.lora_config.config_pending = true;
		shell_reply("IQ set to INVERTED (pending)\r\n");
	} else {
		shell_reply("Error: Must be 'normal' or 'inverted'\r\n");
	}
}

static void cmd_lora_config(char *args)
{
	char buf[512];
	const char *mode_str = (catwan.lora_mode == LORA_MODE_STREAM) ?
				       "Stream" :
				       "Command";
	const char *pending_str =
		catwan.lora_config.config_pending ? " (pending apply)" : "";
	const char *iq_str = catwan.lora_config.iq_inverted ? "Inverted" :
							      "Normal";
	const char *syncword_str = catwan.lora_config.public_network ?
					   "Public (0x34)" :
					   "Private (0x12)";

	snprintf(buf, sizeof(buf),
		 "LoRa Configuration:%s\r\n"
		 "  Frequency: %u Hz\r\n"
		 "  Spreading Factor: SF%d\r\n"
		 "  Bandwidth: %s kHz\r\n"
		 "  Coding Rate: 4/%d\r\n"
		 "  TX Power: %d dBm\r\n"
		 "  Preamble Length: %d\r\n"
		 "  IQ: %s\r\n"
		 "  Sync Word: %s\r\n"
		 "  Mode: %s\r\n",
		 pending_str, catwan.lora_config.frequency,
		 catwan.lora_config.spreading_factor,
		 (catwan.lora_config.bandwidth == BW_125_KHZ) ? "125" :
		 (catwan.lora_config.bandwidth == BW_250_KHZ) ? "250" :
								"500",
		 (catwan.lora_config.coding_rate == CR_4_5) ? 5 :
		 (catwan.lora_config.coding_rate == CR_4_6) ? 6 :
		 (catwan.lora_config.coding_rate == CR_4_7) ? 7 :
							      8,
		 catwan.lora_config.tx_power,
		 catwan.lora_config.preamble_len, iq_str, syncword_str,
		 mode_str);
	shell_reply(buf);
}

static void cmd_lora_apply(char *args)
{
	if (!catwan.lora_config.config_pending) {
		shell_reply("No pending configuration changes\r\n");
		return;
	}

	int ret = apply_lora_config();
	if (ret < 0) {
		char buf[64];
		snprintf(buf, sizeof(buf),
			 "Error applying configuration: %d\r\n", ret);
		shell_reply(buf);
	} else {
		catwan.lora_config.config_pending = false;
		shell_reply("LoRa configuration applied successfully\r\n");
	}
}

static void cmd_radio(char *args)
{
	char forwarded[128];
	int ret;

	while (*args && *args != ' ')
		args++;
	while (*args == ' ')
		args++;

	if (*args == '\0') {
		shell_reply("Usage: radio <TEST|TX HEX>\r\n");
		return;
	}

	snprintf(forwarded, sizeof(forwarded), "%s", args);
	ret = queue_radio_command(forwarded);
	if (ret < 0) {
		shell_reply("ERROR: Failed to queue radio command\r\n");
	}
}

// Main command processor
void process_command(char *cmd, size_t len)
{
	if (len == 0)
		return;

	for (const shell_cmd_t *entry = commands; entry->name != NULL;
	     entry++) {
		if (entry->prefix_match) {
			size_t name_len = strlen(entry->name);
			if (strncmp(cmd, entry->name, name_len) == 0) {
				entry->handler(cmd);
				return;
			}
		} else {
			if (strcmp(cmd, entry->name) == 0) {
				entry->handler(cmd);
				return;
			}
		}
	}

	shell_reply("Unknown command. Type 'help'\r\n");
}
