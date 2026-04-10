/*
 * CatWAN USB Stick Firmware
 * RP2040 + RFM95/SX1276 LoRa USB dongle
 * Eduardo Contreras @ Electronic Cats 2026
 */
#include "catwan.h"
#include "shell_commands.h"

LOG_MODULE_REGISTER(catwan_main, LOG_LEVEL_INF);

#define CDC0_NODE DT_NODELABEL(cdc_acm_uart0)
#define CDC1_NODE DT_NODELABEL(cdc_acm_uart1)

// Device definitions
#define CDC0_DEV DEVICE_DT_GET(CDC0_NODE) // LoRa data
#define CDC1_DEV DEVICE_DT_GET(CDC1_NODE) // Shell

// Communication buffers (4 ring buffers: LoRa<->USB, Shell<->USB)
uint8_t ring_lora_to_usb[RING_BUF_SIZE];
uint8_t ring_usb_to_lora[RING_BUF_SIZE];
uint8_t ring_config_to_usb[RING_BUF_SIZE];
uint8_t ring_usb_to_config[RING_BUF_SIZE];

struct ring_buf rb_lora_to_usb;
struct ring_buf rb_usb_to_lora;
struct ring_buf rb_config_to_usb;
struct ring_buf rb_usb_to_config;

// Device references
const struct device *cdc0_dev; // LoRa data port
const struct device *cdc1_dev; // Shell port

// Global state
catwan_t catwan = { 0 };

// GPIO for LED
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

// USB context
static struct usbd_context *catwan_usbd;

static const struct device *lora_dev;

// Thread definitions
#define LORA_THREAD_STACK_SIZE 4096
K_THREAD_STACK_DEFINE(lora_thread_stack, LORA_THREAD_STACK_SIZE);
static struct k_thread lora_thread;
K_SEM_DEFINE(lora_data_sem, 0, 1);
static bool lora_async_rx_active;

// --- Ring buffer helpers ---

static inline uint32_t safe_ring_buf_put(struct ring_buf *rb,
					 const uint8_t *data, uint32_t size)
{
	unsigned int key = irq_lock();
	uint32_t result = ring_buf_put(rb, data, size);
	irq_unlock(key);
	return result;
}

static inline uint32_t safe_ring_buf_get(struct ring_buf *rb, uint8_t *data,
					 uint32_t size)
{
	unsigned int key = irq_lock();
	uint32_t result = ring_buf_get(rb, data, size);
	irq_unlock(key);
	return result;
}

// --- LoRa RX callback ---

void lora_rx_cb(const struct device *dev, uint8_t *data, uint16_t size,
		int16_t rssi, int8_t snr, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (catwan.lora_mode == LORA_MODE_STREAM) {
		// Stream mode: length-prefixed binary frame
		// [length:1][payload:N][RSSI+128:1][SNR+128:1]
		uint8_t header = (uint8_t)(size > 255 ? 255 : size);
		safe_ring_buf_put(&rb_lora_to_usb, &header, 1);
		safe_ring_buf_put(&rb_lora_to_usb, data, header);
		uint8_t rssi_byte = (uint8_t)(rssi + 128);
		uint8_t snr_byte = (uint8_t)(snr + 128);
		safe_ring_buf_put(&rb_lora_to_usb, &rssi_byte, 1);
		safe_ring_buf_put(&rb_lora_to_usb, &snr_byte, 1);
	} else {
		// Command mode: text format
		char rx_msg[384];
		char data_str[128] = { 0 };
		int display_len = (size > 40) ? 40 : size;

		for (int i = 0; i < display_len; i++) {
			char byte_str[4];
			snprintf(byte_str, sizeof(byte_str), "%02X", data[i]);
			strcat(data_str, byte_str);
		}
		if (size > 40)
			strcat(data_str, "...");

		snprintf(rx_msg, sizeof(rx_msg),
			 "RX: %s | RSSI: %d | SNR: %d\r\n", data_str, rssi, snr);
		safe_ring_buf_put(&rb_lora_to_usb, (uint8_t *)rx_msg,
				  strlen(rx_msg));
	}

	if (cdc0_dev)
		uart_irq_tx_enable(cdc0_dev);
}

// --- USB callbacks ---

static void catwan_usb_msg_cb(struct usbd_context *const ctx,
			      const struct usbd_msg *msg)
{
	if (usbd_can_detect_vbus(ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			usbd_enable(ctx);
		} else if (msg->type == USBD_MSG_VBUS_REMOVED) {
			usbd_disable(ctx);
		}
	}
}

static int enable_usb_device_next(void)
{
	catwan_usbd = usbd_init_device(catwan_usb_msg_cb);
	if (catwan_usbd == NULL) {
		return -ENODEV;
	}
	if (!usbd_can_detect_vbus(catwan_usbd)) {
		return usbd_enable(catwan_usbd);
	}
	return 0;
}

// --- CDC0 (LoRa data) interrupt handler ---

static void cdc0_interrupt_handler(const struct device *dev, void *user_data)
{
	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			uint8_t buf[64];
			int len = uart_fifo_read(dev, buf, sizeof(buf));
			if (len > 0) {
				safe_ring_buf_put(&rb_usb_to_lora, buf, len);
				k_sem_give(&lora_data_sem);
			}
		}

		if (uart_irq_tx_ready(dev)) {
			uint8_t buf[64];
			int len = safe_ring_buf_get(&rb_lora_to_usb, buf,
						    sizeof(buf));
			if (len > 0) {
				uart_fifo_fill(dev, buf, len);
			} else {
				uart_irq_tx_disable(dev);
			}
		}
	}
}

// --- CDC1 (Shell) interrupt handler ---

static void cdc1_interrupt_handler(const struct device *dev, void *user_data)
{
	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			uint8_t buf[64];
			int len = uart_fifo_read(dev, buf, sizeof(buf));

			for (int i = 0; i < len; i++) {
				uint8_t data = buf[i];

				// Echo back
				safe_ring_buf_put(&rb_config_to_usb, &data, 1);
				uart_irq_tx_enable(dev);

				// Line buffering
				if (data == '\n' || data == '\r') {
					if (catwan.command_data_len > 0) {
						catwan.command_data
							[catwan.command_data_len] =
							'\0';
						process_command(
							catwan.command_data,
							catwan.command_data_len);
						catwan.command_data_len = 0;
					}
				} else if (catwan.command_data_len <
					   COMMAND_BUF_SIZE - 1) {
					catwan.command_data
						[catwan.command_data_len++] =
						data;
				}
			}
		}

		if (uart_irq_tx_ready(dev)) {
			uint8_t buf[64];
			int len = safe_ring_buf_get(&rb_config_to_usb, buf,
						    sizeof(buf));
			if (len > 0) {
				uart_fifo_fill(dev, buf, len);
			} else {
				uart_irq_tx_disable(dev);
			}
		}
	}
}

// --- Shell helpers (exposed to shell_commands.c) ---

void shell_reply(const char *msg)
{
	if (!msg)
		return;
	safe_ring_buf_put(&rb_config_to_usb, (uint8_t *)msg, strlen(msg));
	if (cdc1_dev)
		uart_irq_tx_enable(cdc1_dev);
}

int queue_radio_command(const char *cmd_line)
{
	uint8_t nl = '\n';
	size_t len;
	uint32_t written;

	if (!cmd_line)
		return -EINVAL;

	len = strlen(cmd_line);
	if (len == 0 || len >= COMMAND_BUF_SIZE)
		return -EINVAL;

	written = safe_ring_buf_put(&rb_usb_to_lora,
				    (const uint8_t *)cmd_line, len);
	if (written != len)
		return -ENOSPC;

	written = safe_ring_buf_put(&rb_usb_to_lora, &nl, 1);
	if (written != 1)
		return -ENOSPC;

	k_sem_give(&lora_data_sem);
	return 0;
}

void set_status_leds(int l0, int l1, int l2)
{
	ARG_UNUSED(l1);
	ARG_UNUSED(l2);
	gpio_pin_set_dt(&led0, l0);
}

// --- LoRa helpers ---

static const char *get_error_string(int error)
{
	switch (error) {
	case 0:
		return "Success";
	case -EAGAIN:
		return "EAGAIN - Resource temporarily unavailable";
	case -EBUSY:
		return "EBUSY - Device busy";
	case -EIO:
		return "EIO - I/O error";
	case -EINVAL:
		return "EINVAL - Invalid argument";
	case -ENODEV:
		return "ENODEV - Device not ready";
	case -ENOTSUP:
		return "ENOTSUP - Operation not supported";
	case -ETIMEDOUT:
		return "ETIMEDOUT - Timeout";
	default:
		return "Unknown error";
	}
}

static void lora_stop_rx(void)
{
	lora_recv_async(lora_dev, NULL, NULL);
	lora_async_rx_active = false;
	k_sleep(K_MSEC(5));
}

static int lora_set_tx_mode(void)
{
	struct lora_modem_config config = { 0 };
	config.frequency = catwan.lora_config.frequency;
	config.bandwidth = catwan.lora_config.bandwidth;
	config.datarate = catwan.lora_config.spreading_factor;
	config.preamble_len = catwan.lora_config.preamble_len;
	config.coding_rate = catwan.lora_config.coding_rate;
	config.tx_power = catwan.lora_config.tx_power;
	config.tx = true;
	config.iq_inverted = catwan.lora_config.iq_inverted;
	config.public_network = catwan.lora_config.public_network;
	return lora_config(lora_dev, &config);
}

static int lora_start_rx_async(void)
{
	if (lora_async_rx_active)
		return 0;

	int ret = lora_recv_async(lora_dev, lora_rx_cb, NULL);
	if (ret == 0) {
		lora_async_rx_active = true;
		return 0;
	}

	if (ret == -EBUSY) {
		lora_recv_async(lora_dev, NULL, NULL);
		k_sleep(K_MSEC(2));
		ret = lora_recv_async(lora_dev, lora_rx_cb, NULL);
		if (ret == 0) {
			lora_async_rx_active = true;
		}
	}
	return ret;
}

// --- LoRa init and config ---

int initialize_lora(void)
{
	const char *status_msg;

	if (catwan.lora_initialized) {
		shell_reply("LoRa: Already initialized\r\n");
		return 0;
	}

	shell_reply("LoRa: Starting initialization...\r\n");

	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		shell_reply("ERROR: LoRa device not ready\r\n");
		return -ENODEV;
	}

	shell_reply("LoRa: Device ready\r\n");

	struct lora_modem_config config = { 0 };
	config.frequency = catwan.lora_config.frequency;
	config.bandwidth = catwan.lora_config.bandwidth;
	config.datarate = catwan.lora_config.spreading_factor;
	config.preamble_len = catwan.lora_config.preamble_len;
	config.coding_rate = catwan.lora_config.coding_rate;
	config.tx_power = catwan.lora_config.tx_power;
	config.tx = false;
	config.iq_inverted = catwan.lora_config.iq_inverted;
	config.public_network = catwan.lora_config.public_network;

	int ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		shell_reply("ERROR: LoRa configuration failed\r\n");
		return ret;
	}

	catwan.lora_initialized = true;
	catwan.lora_config_lock = false;

	ret = lora_start_rx_async();
	if (ret < 0 && ret != -EBUSY) {
		char err_buf[96];
		snprintf(err_buf, sizeof(err_buf),
			 "WARN: LoRa RX start failed (%d: %s)\r\n", ret,
			 get_error_string(ret));
		shell_reply(err_buf);
	}

	shell_reply("LoRa: Initialization completed (RX mode)!\r\n");
	return 0;
}

int apply_lora_config(void)
{
	if (!catwan.lora_initialized) {
		shell_reply("Error: LoRa not initialized. Use TEST command first.\r\n");
		return -ENODEV;
	}

	catwan.lora_config_lock = true;
	k_msleep(50);

	lora_recv_async(lora_dev, NULL, NULL);
	lora_async_rx_active = false;

	shell_reply("Applying LoRa configuration...\r\n");

	struct lora_modem_config config = { 0 };
	config.frequency = catwan.lora_config.frequency;
	config.bandwidth = catwan.lora_config.bandwidth;
	config.datarate = catwan.lora_config.spreading_factor;
	config.preamble_len = catwan.lora_config.preamble_len;
	config.coding_rate = catwan.lora_config.coding_rate;
	config.tx_power = catwan.lora_config.tx_power;
	config.tx = false;
	config.iq_inverted = catwan.lora_config.iq_inverted;
	config.public_network = catwan.lora_config.public_network;

	int ret = lora_config(lora_dev, &config);
	catwan.lora_config_lock = false;

	if (ret < 0) {
		char err_buf[96];
		snprintf(err_buf, sizeof(err_buf),
			 "ERROR: LoRa configuration failed (%d: %s)\r\n", ret,
			 get_error_string(ret));
		shell_reply(err_buf);
		return ret;
	}

	shell_reply("LoRa configuration applied successfully (RX mode)\r\n");

	ret = lora_start_rx_async();
	if (ret < 0 && ret != -EBUSY) {
		char err_buf[96];
		snprintf(err_buf, sizeof(err_buf),
			 "WARN: LoRa RX re-arm failed (%d: %s)\r\n", ret,
			 get_error_string(ret));
		shell_reply(err_buf);
	}

	return 0;
}

// --- LoRa command processing (CDC0 command mode) ---

void process_lora_command(char *cmd_line)
{
	char response[384];

	if (!catwan.lora_initialized) {
		int ret = initialize_lora();
		if (ret < 0)
			return;
	}

	if (strncmp(cmd_line, "TEST", 4) == 0) {
		shell_reply("TEST: LoRa ready!\r\n");
		return;
	}

	if (strncmp(cmd_line, "TXTEST", 6) == 0) {
		if (!catwan.lora_initialized) {
			shell_reply("ERROR: LoRa not initialized\r\n");
			return;
		}

		uint8_t tx_data[] = "PING";
		shell_reply("Sending PING packet...\r\n");

		lora_stop_rx();
		lora_set_tx_mode();
		int ret = lora_send(lora_dev, tx_data, sizeof(tx_data));
		lora_start_rx_async();

		snprintf(response, sizeof(response), "TX Result: %d (%s)\r\n",
			 ret, get_error_string(ret));
		shell_reply(response);
		return;
	}

	if (strncmp(cmd_line, "TX ", 3) == 0) {
		if (!catwan.lora_initialized) {
			shell_reply("ERROR: LoRa not initialized\r\n");
			return;
		}

		char *hex_data = &cmd_line[3];
		uint8_t tx_data[128];
		size_t hex_len = strlen(hex_data);

		if (hex_len % 2 == 0 && hex_len > 0) {
			size_t data_len = hex_len / 2;

			for (size_t i = 0; i < data_len; i++) {
				char hex_byte[3] = { hex_data[i * 2],
						     hex_data[i * 2 + 1],
						     '\0' };
				tx_data[i] =
					(uint8_t)strtoul(hex_byte, NULL, 16);
			}

			lora_stop_rx();
			lora_set_tx_mode();
			int ret = lora_send(lora_dev, tx_data, data_len);
			lora_start_rx_async();

			snprintf(response, sizeof(response),
				 "TX Result: %d (%s)\r\n", ret,
				 get_error_string(ret));
		} else {
			snprintf(response, sizeof(response),
				 "ERROR: Invalid hex data\r\n");
		}

		shell_reply(response);
		return;
	}

	shell_reply("Available: TEST, TXTEST, TX <hex>\r\n");
}

// --- LoRa thread ---

static void lora_thread_func(void *p1, void *p2, void *p3)
{
	char command_buffer[128];
	size_t cmd_len = 0;

	while (1) {
		(void)k_sem_take(&lora_data_sem, K_MSEC(100));

		if (catwan.lora_config_lock) {
			k_msleep(10);
			continue;
		}

		lora_start_rx_async();

		if (catwan.lora_mode == LORA_MODE_COMMAND) {
			// Command mode: parse text commands from CDC0
			uint8_t usb_buf[64];
			int usb_len = safe_ring_buf_get(&rb_usb_to_lora,
							usb_buf, sizeof(usb_buf));

			if (usb_len > 0) {
				for (int i = 0; i < usb_len; i++) {
					char c = usb_buf[i];
					if (c == '\n' || c == '\r') {
						if (cmd_len > 0) {
							command_buffer[cmd_len] = '\0';
							process_lora_command(
								command_buffer);
							cmd_len = 0;
						}
					} else if (cmd_len <
						   sizeof(command_buffer) - 1) {
						command_buffer[cmd_len++] = c;
					}
				}
			}
		} else {
			// Stream mode: raw binary TX/RX
			uint8_t tx_buffer[255];
			int tx_len = safe_ring_buf_get(&rb_usb_to_lora,
						       tx_buffer, sizeof(tx_buffer));

			if (tx_len > 0 && catwan.lora_initialized) {
				lora_stop_rx();
				lora_set_tx_mode();
				lora_send(lora_dev, tx_buffer, tx_len);
				lora_start_rx_async();
			}
		}
	}
}

// --- Main ---

int main(void)
{
	int ret;

	// Get device references
	cdc0_dev = CDC0_DEV;
	cdc1_dev = CDC1_DEV;

	// Initialize LED
	if (!device_is_ready(led0.port)) {
		return -ENODEV;
	}
	gpio_pin_configure_dt(&led0, GPIO_OUTPUT);
	gpio_pin_set_dt(&led0, 0);

	// Initialize LoRa configuration defaults
	catwan.led_interval = 1000;
	catwan.lora_mode = LORA_MODE_STREAM;
	catwan.lora_config.frequency = 915000000;
	catwan.lora_config.spreading_factor = SF_7;
	catwan.lora_config.bandwidth = BW_125_KHZ;
	catwan.lora_config.coding_rate = CR_4_5;
	catwan.lora_config.tx_power = 17;
	catwan.lora_config.preamble_len = 12;
	catwan.lora_config.iq_inverted = false;
	catwan.lora_config.public_network = false;
	catwan.lora_config.config_pending = false;
	catwan.lora_initialized = false;
	catwan.lora_config_lock = false;

	// Initialize USB
	ret = enable_usb_device_next();
	if (ret < 0) {
		return ret;
	}
	gpio_pin_set_dt(&led0, 1);

	// Initialize ring buffers
	ring_buf_init(&rb_lora_to_usb, sizeof(ring_lora_to_usb),
		      ring_lora_to_usb);
	ring_buf_init(&rb_usb_to_lora, sizeof(ring_usb_to_lora),
		      ring_usb_to_lora);
	ring_buf_init(&rb_config_to_usb, sizeof(ring_config_to_usb),
		      ring_config_to_usb);
	ring_buf_init(&rb_usb_to_config, sizeof(ring_usb_to_config),
		      ring_usb_to_config);

	// Set up CDC interrupt handlers
	uart_irq_callback_set(cdc0_dev, cdc0_interrupt_handler);
	uart_irq_rx_enable(cdc0_dev);

	if (cdc1_dev) {
		uart_irq_callback_set(cdc1_dev, cdc1_interrupt_handler);
		uart_irq_rx_enable(cdc1_dev);
	}

	gpio_pin_set_dt(&led0, 0);

	// Wait for USB to be ready, then send startup messages
	k_msleep(1000);

	char startup_msg[200];
	snprintf(startup_msg, sizeof(startup_msg),
		 "CatWAN USB Stick [%s] - Shell\r\n", CATSNIFFER_FW_VERSION);
	shell_reply(startup_msg);

	const char *lora_welcome = "CatWAN LoRa Data Port\r\n";
	safe_ring_buf_put(&rb_lora_to_usb, (uint8_t *)lora_welcome,
			  strlen(lora_welcome));
	uart_irq_tx_enable(cdc0_dev);

	// Initialize LoRa
	ret = initialize_lora();
	if (ret < 0) {
		shell_reply("LoRa initialization failed\r\n");
	}

	// Start LoRa thread
	k_thread_create(&lora_thread, lora_thread_stack, LORA_THREAD_STACK_SIZE,
			lora_thread_func, NULL, NULL, NULL,
			LORA_THREAD_PRIORITY, 0, K_NO_WAIT);

	// Main loop: LED blink
	while (1) {
		if (catwan.led_identify) {
			k_msleep(10);
			continue;
		}

		int64_t current_time = k_uptime_get();
		if (current_time - catwan.previous_millis > catwan.led_interval) {
			catwan.previous_millis = current_time;
			gpio_pin_toggle_dt(&led0);
		}

		k_msleep(10);
	}

	return 0;
}
