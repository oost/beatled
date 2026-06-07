#include <stdio.h>
#include <stdlib.h>

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "ws2812_dma.h"

int dma_channel;
uint32_t dma_channel_mask = 0;

static SemaphoreHandle_t reset_delay_complete_sem;
volatile alarm_id_t reset_delay_alarm_id;

int64_t reset_delay_complete(alarm_id_t id, void *user_data) {
  reset_delay_alarm_id = 0;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(reset_delay_complete_sem, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  return 0;
}

void __isr dma_complete_handler() {
  if (dma_hw->ints0 & dma_channel_mask) {
    dma_hw->ints0 = dma_channel_mask;
    if (reset_delay_alarm_id)
      cancel_alarm(reset_delay_alarm_id);
    reset_delay_alarm_id =
        add_alarm_in_us(400, reset_delay_complete, NULL, true);
  }
}

void dma_init(PIO pio, uint sm, uint16_t num_pixel) {
  reset_delay_complete_sem = xSemaphoreCreateBinary();
  xSemaphoreGive(reset_delay_complete_sem); // initially posted

  dma_channel = dma_claim_unused_channel(true);
  dma_channel_mask = 1u << dma_channel;

  dma_channel_config channel_config =
      dma_channel_get_default_config(dma_channel);
  channel_config_set_dreq(&channel_config, pio_get_dreq(pio, sm, true));
  dma_channel_configure(dma_channel, &channel_config, &pio->txf[sm],
                        NULL, // set by chain
                        num_pixel, false);

  irq_set_exclusive_handler(DMA_IRQ_0, dma_complete_handler);
  dma_channel_set_irq0_enabled(dma_channel, true);
  irq_set_enabled(DMA_IRQ_0, true);
}

void output_strings_dma(uint32_t *stream) {
  xSemaphoreTake(reset_delay_complete_sem, portMAX_DELAY);
  dma_channel_hw_addr(dma_channel)->al3_read_addr_trig = (uintptr_t)stream;
}
