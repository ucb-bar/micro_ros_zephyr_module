/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * micro-ROS serial transport — HTIF polling variant for spike_riscv64 /
 * chipyard_riscv64. The upstream version (saved as microros_transports.c.upstream)
 * targets STM32 with usart1 + interrupt-driven UART; that DT node and IRQ API
 * surface are unavailable on this Zephyr config.
 *
 * This is the bring-up minimum (step 3): make the four custom-transport hooks
 * compile and link. Real on-the-wire correctness — including HDLC framing so
 * micro-ROS bytes coexist with printk on the shared HTIF console — lands in
 * step 4 and replaces this file.
 */

#include <uxr/client/transport.h>
#include <microros_transports.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/uart.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define UART_NODE DT_NODELABEL(htif)

bool zephyr_transport_open(struct uxrCustomTransport *transport)
{
	zephyr_transport_params_t *params = (zephyr_transport_params_t *)transport->args;

	params->uart_dev = DEVICE_DT_GET(UART_NODE);
	if (!device_is_ready(params->uart_dev)) {
		printk("micro-ROS HTIF transport: device not ready\n");
		return false;
	}
	return true;
}

bool zephyr_transport_close(struct uxrCustomTransport *transport)
{
	(void)transport;
	return true;
}

size_t zephyr_transport_write(struct uxrCustomTransport *transport,
			      const uint8_t *buf, size_t len, uint8_t *err)
{
	zephyr_transport_params_t *params = (zephyr_transport_params_t *)transport->args;
	(void)err;

	for (size_t i = 0; i < len; i++) {
		uart_poll_out(params->uart_dev, buf[i]);
	}
	return len;
}

size_t zephyr_transport_read(struct uxrCustomTransport *transport,
			     uint8_t *buf, size_t len, int timeout, uint8_t *err)
{
	zephyr_transport_params_t *params = (zephyr_transport_params_t *)transport->args;
	(void)err;

	size_t read = 0;
	int waited_ms = 0;
	const int slice_ms = 1;

	while (read < len) {
		unsigned char c;
		int rc = uart_poll_in(params->uart_dev, &c);
		if (rc == 0) {
			buf[read++] = c;
			continue;
		}
		if (waited_ms >= timeout) {
			break;
		}
		k_msleep(slice_ms);
		waited_ms += slice_ms;
	}
	return read;
}
