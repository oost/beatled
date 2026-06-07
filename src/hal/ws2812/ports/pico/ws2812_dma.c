#include <stdio.h>
#include <stdlib.h>

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/sem.h"
#include "pico/stdlib.h"

#include "ws2812_dma.h"

int dma_channel;
uint32_t dma_channel_mask = 0;

// posted when it is safe to output a new set of values
static struct semaphore reset_delay_complete_sem;
// alarm handle for handling delay
volatile alarm_id_t reset_delay_alarm_id;

int64_t reset_delay_complete(alarm_id_t id, void *user_data) {
  reset_delay_alarm_id = 0;
  sem_release(&reset_delay_complete_sem);
  // no repeat
  // puts("Release semaphore");

  return 0;
}

void __isr dma_complete_handler() {
  if (dma_hw->ints0 & dma_channel_mask) {
    // puts("Interrupt");
    // clear IRQ
    dma_hw->ints0 = dma_channel_mask;
    // when the dma is complete we start the reset delay timer
    if (reset_delay_alarm_id)
      cancel_alarm(reset_delay_alarm_id);
    reset_delay_alarm_id =
        add_alarm_in_us(400, reset_delay_complete, NULL, true);
  }
}

void dma_init(PIO pio, uint sm, uint16_t num_pixel) {
  sem_init(&reset_delay_complete_sem, 1,
           1); // initially posted so we don't block first time

  dma_channel = dma_claim_unused_channel(true);
  dma_channel_mask = 1u << dma_channel;

  // main DMA channel outputs 8 word fragments, and then chains back to the
  // chain channel
  dma_channel_config channel_config =
      dma_channel_get_default_config(dma_channel);
  channel_config_set_dreq(&channel_config, pio_get_dreq(pio, sm, true));
  // channel_config_set_chain_to(&channel_config, DMA_CB_CHANNEL);
  // channel_config_set_irq_quiet(&channel_config, true);
  dma_channel_configure(dma_channel, &channel_config, &pio->txf[sm],
                        NULL, // set by chain
                        num_pixel, false);

  irq_set_exclusive_handler(DMA_IRQ_0, dma_complete_handler);
  dma_channel_set_irq0_enabled(dma_channel, true);
  irq_set_enabled(DMA_IRQ_0, true);
}

void output_strings_dma(uint32_t *stream) {
  sem_acquire_blocking(&reset_delay_complete_sem);
  dma_channel_hw_addr(dma_channel)->al3_read_addr_trig = (uintptr_t)stream;
}