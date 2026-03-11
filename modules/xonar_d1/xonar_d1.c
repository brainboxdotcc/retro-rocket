/**
 * @file modules/xonar_d1/xonar_d1.c
 * @author Craig Edwards
 * @copyright Copyright (c) 2012-2026
 */
#include <kernel.h>
#include "xonar.h"

static xonar_d1_dev_t xonar_d1;
static int16_t *s_q_pcm = NULL;
static size_t s_q_cap_fr = 0;
static size_t s_q_len_fr = 0;
static size_t s_q_head_fr = 0;
static pci_dev_t device_zero = {0};

static inline void xonar_delay_us(unsigned us)
{
	for (unsigned i = 0; i < us; i++) {
		io_wait();
	}
}

static inline void xonar_delay_ms(unsigned ms)
{
	for (unsigned i = 0; i < ms; i++) {
		for (unsigned j = 0; j < 1000; j++) {
			io_wait();
		}
	}
}

static inline uint8_t xonar_read8(uint16_t reg)
{
	return inb(xonar_d1.io_base + reg);
}

static inline uint16_t xonar_read16(uint16_t reg)
{
	return inw(xonar_d1.io_base + reg);
}

static inline uint32_t xonar_read32(uint16_t reg)
{
	return inl(xonar_d1.io_base + reg);
}

static inline void xonar_write8(uint16_t reg, uint8_t value)
{
	outb(value, xonar_d1.io_base + reg);
}

static inline void xonar_write16(uint16_t reg, uint16_t value)
{
	outw(value, xonar_d1.io_base + reg);
}

static inline void xonar_write32(uint16_t reg, uint32_t value)
{
	outl(value, xonar_d1.io_base + reg);
}

static inline void xonar_write8_masked(uint16_t reg, uint8_t value, uint8_t mask)
{
	uint8_t tmp = xonar_read8(reg);
	tmp &= ~mask;
	tmp |= value & mask;
	xonar_write8(reg, tmp);
}

static inline void xonar_write16_masked(uint16_t reg, uint16_t value, uint16_t mask)
{
	uint16_t tmp = xonar_read16(reg);
	tmp &= ~mask;
	tmp |= value & mask;
	xonar_write16(reg, tmp);
}

static inline void xonar_set_bits8(uint16_t reg, uint8_t value)
{
	xonar_write8_masked(reg, value, value);
}

static inline void xonar_clear_bits8(uint16_t reg, uint8_t value)
{
	xonar_write8_masked(reg, 0, value);
}

static inline void xonar_set_bits16(uint16_t reg, uint16_t value)
{
	xonar_write16_masked(reg, value, value);
}

static inline void xonar_clear_bits16(uint16_t reg, uint16_t value)
{
	xonar_write16_masked(reg, 0, value);
}

static void xonar_write_i2c(uint8_t device, uint8_t map, uint8_t data)
{
	xonar_delay_ms(1);
	xonar_write8(OXYGEN_2WIRE_MAP, map);
	xonar_write8(OXYGEN_2WIRE_DATA, data);
	xonar_write8(OXYGEN_2WIRE_CONTROL, device | OXYGEN_2WIRE_DIR_WRITE);
}

static void xonar_cs4398_write(uint8_t reg, uint8_t value)
{
	xonar_write_i2c(I2C_DEVICE_CS4398, reg, value);
}

static void xonar_cs4362a_write(uint8_t reg, uint8_t value)
{
	xonar_write_i2c(I2C_DEVICE_CS4362A, reg, value);
}

static void xonar_enable_output(void)
{
	xonar_delay_ms(100);
	xonar_set_bits16(OXYGEN_GPIO_DATA, GPIO_D1_MAGIC | GPIO_D1_OUTPUT_ENABLE);
}

static void xonar_init_dacs(void)
{
	xonar_write16(OXYGEN_2WIRE_BUS_STATUS,
		     OXYGEN_2WIRE_LENGTH_8 |
		     OXYGEN_2WIRE_INTERRUPT_MASK |
		     OXYGEN_2WIRE_SPEED_FAST);

	xonar_cs4398_write(8, CS4398_CPEN | CS4398_PDN);
	xonar_cs4362a_write(0x01, CS4362A_PDN | CS4362A_CPEN);

	xonar_cs4398_write(2, CS4398_FM_SINGLE | CS4398_DEM_NONE | CS4398_DIF_LJUST);
	xonar_cs4398_write(3, CS4398_ATAPI_B_R | CS4398_ATAPI_A_L);
	xonar_cs4398_write(4, CS4398_MUTEP_LOW | CS4398_PAMUTE);
	xonar_cs4398_write(5, 0);
	xonar_cs4398_write(6, 0);
	xonar_cs4398_write(7, CS4398_RMP_DN | CS4398_RMP_UP | CS4398_ZERO_CROSS | CS4398_SOFT_RAMP);

	xonar_cs4362a_write(0x02, CS4362A_DIF_LJUST);
	xonar_cs4362a_write(0x03, CS4362A_MUTEC_6 | CS4362A_AMUTE | CS4362A_RMP_UP | CS4362A_ZERO_CROSS | CS4362A_SOFT_RAMP);
	xonar_cs4362a_write(0x04, CS4362A_RMP_DN | CS4362A_DEM_NONE);
	xonar_cs4362a_write(0x05, 0);
	xonar_cs4362a_write(0x06, CS4362A_FM_SINGLE | CS4362A_ATAPI_B_R | CS4362A_ATAPI_A_L);
	xonar_cs4362a_write(0x07, 0);
	xonar_cs4362a_write(0x08, 0);
	xonar_cs4362a_write(0x09, CS4362A_FM_SINGLE | CS4362A_ATAPI_B_R | CS4362A_ATAPI_A_L);
	xonar_cs4362a_write(0x0a, 0);
	xonar_cs4362a_write(0x0b, 0);
	xonar_cs4362a_write(0x0c, CS4362A_FM_SINGLE | CS4362A_ATAPI_B_R | CS4362A_ATAPI_A_L);
	xonar_cs4362a_write(0x0d, 0);
	xonar_cs4362a_write(0x0e, 0);

	xonar_cs4398_write(8, CS4398_CPEN);
	xonar_cs4362a_write(0x01, CS4362A_CPEN);
}

static void xonar_core_init(void)
{
	if (!(xonar_read8(OXYGEN_REVISION) & OXYGEN_REVISION_2)) {
		xonar_set_bits8(OXYGEN_MISC, OXYGEN_MISC_PCI_MEM_W_1_CLOCK);
	}

	xonar_write8_masked(OXYGEN_FUNCTION,
			    OXYGEN_FUNCTION_RESET_CODEC | OXYGEN_FUNCTION_2WIRE,
			    OXYGEN_FUNCTION_RESET_CODEC | OXYGEN_FUNCTION_2WIRE);

	xonar_write8(OXYGEN_DMA_STATUS, 0);
	xonar_write8(OXYGEN_DMA_PAUSE, 0);
	xonar_write8(OXYGEN_DMA_RESET, 0);

	xonar_write8(OXYGEN_PLAY_CHANNELS,
		     OXYGEN_PLAY_CHANNELS_2 |
		     OXYGEN_DMA_A_BURST_8 |
		     OXYGEN_DMA_MULTICH_BURST_8);

	xonar_write16(OXYGEN_INTERRUPT_MASK, 0);

	xonar_write8_masked(OXYGEN_PLAY_FORMAT,
			    OXYGEN_FORMAT_16 << OXYGEN_MULTICH_FORMAT_SHIFT,
			    OXYGEN_MULTICH_FORMAT_MASK);

	xonar_write16(OXYGEN_I2S_MULTICH_FORMAT,
		      OXYGEN_RATE_44100 |
		      OXYGEN_I2S_FORMAT_LJUST |
		      OXYGEN_I2S_MCLK(MCLK_256) |
		      OXYGEN_I2S_BITS_16 |
		      OXYGEN_I2S_MASTER |
		      OXYGEN_I2S_BCLK_64);

	xonar_write16(OXYGEN_PLAY_ROUTING,
		      OXYGEN_PLAY_MULTICH_I2S_DAC |
		      OXYGEN_PLAY_SPDIF_SPDIF |
		      (0 << OXYGEN_PLAY_DAC0_SOURCE_SHIFT) |
		      (1 << OXYGEN_PLAY_DAC1_SOURCE_SHIFT) |
		      (2 << OXYGEN_PLAY_DAC2_SOURCE_SHIFT) |
		      (3 << OXYGEN_PLAY_DAC3_SOURCE_SHIFT));

	xonar_set_bits16(OXYGEN_GPIO_CONTROL,
			 GPIO_D1_OUTPUT_ENABLE |
			 GPIO_D1_FRONT_PANEL |
			 GPIO_D1_MAGIC |
			 GPIO_D1_INPUT_ROUTE);

	xonar_clear_bits16(OXYGEN_GPIO_DATA, GPIO_D1_FRONT_PANEL | GPIO_D1_INPUT_ROUTE);
	xonar_set_bits16(OXYGEN_GPIO_DATA, GPIO_D1_MAGIC);

	xonar_init_dacs();
	xonar_enable_output();
}

static bool q_ensure_cap(size_t extra_fr)
{
	size_t live = s_q_len_fr - s_q_head_fr;
	size_t need = live + extra_fr;
	size_t new_cap;
	int16_t *new_buf;

	if (need <= s_q_cap_fr) {
		return true;
	}

	new_cap = s_q_cap_fr ? s_q_cap_fr : 4096;
	while (new_cap < need) {
		new_cap <<= 1;
	}

	new_buf = kmalloc(new_cap * 2 * sizeof(int16_t));
	if (!new_buf) {
		return false;
	}

	if (live) {
		memcpy(new_buf, s_q_pcm + (s_q_head_fr * 2), live * 2 * sizeof(int16_t));
	}

	if (s_q_pcm) {
		kfree(s_q_pcm);
	}

	s_q_pcm = new_buf;
	s_q_cap_fr = new_cap;
	s_q_len_fr = live;
	s_q_head_fr = 0;
	return true;
}

static void q_compact_if_empty(void)
{
	if (s_q_head_fr == s_q_len_fr) {
		s_q_head_fr = 0;
		s_q_len_fr = 0;
	}
}

static void xonar_fill_period(uint32_t period_index)
{
	uint8_t *dst;
	size_t pending;
	size_t chunk_frames;
	size_t bytes_copy;

	dst = xonar_d1.dma_buf + (period_index * xonar_d1.period_bytes);
	pending = s_q_len_fr - s_q_head_fr;
	chunk_frames = xonar_d1.period_frames;
	if (chunk_frames > pending) {
		chunk_frames = pending;
	}

	bytes_copy = chunk_frames * XONAR_FRAME_BYTES;

	if (bytes_copy) {
		memcpy(dst, s_q_pcm + (s_q_head_fr * 2), bytes_copy);
		s_q_head_fr += chunk_frames;
	}

	if (bytes_copy < xonar_d1.period_bytes) {
		memset(dst + bytes_copy, 0, xonar_d1.period_bytes - bytes_copy);
	}

	xonar_d1.period_valid_frames[period_index] = (uint32_t)chunk_frames;
	q_compact_if_empty();
}

static uint32_t xonar_hw_bytes(void)
{
	uint32_t curr_addr;
	uint32_t delta;

	curr_addr = xonar_read32(OXYGEN_DMA_MULTICH_ADDRESS);
	if (curr_addr < xonar_d1.dma_phys) {
		return 0;
	}

	delta = curr_addr - xonar_d1.dma_phys;
	if (delta >= xonar_d1.dma_bytes) {
		delta %= xonar_d1.dma_bytes;
	}
	return delta;
}

static void xonar_service_periods(void)
{
	uint32_t hw_bytes;
	uint32_t current_period;

	if (!xonar_d1.started) {
		return;
	}

	hw_bytes = xonar_hw_bytes();
	current_period = hw_bytes / xonar_d1.period_bytes;
	if (current_period >= xonar_d1.period_count) {
		current_period = 0;
	}

	while (xonar_d1.last_hw_period != current_period) {
		xonar_fill_period(xonar_d1.last_hw_period);
		xonar_d1.last_hw_period++;
		if (xonar_d1.last_hw_period >= xonar_d1.period_count) {
			xonar_d1.last_hw_period = 0;
		}
	}
}

static void xonar_prime_dma(void)
{
	for (uint32_t i = 0; i < xonar_d1.period_count; i++) {
		xonar_fill_period(i);
	}
	xonar_d1.last_hw_period = 0;
}

static void xonar_program_dma(void)
{
	xonar_write32(OXYGEN_DMA_MULTICH_ADDRESS, xonar_d1.dma_phys);
	xonar_write32(OXYGEN_DMA_MULTICH_COUNT, (xonar_d1.dma_bytes / 4) - 1);
	xonar_write32(OXYGEN_DMA_MULTICH_TCOUNT, (xonar_d1.period_bytes / 4) - 1);

	xonar_set_bits8(OXYGEN_DMA_FLUSH, OXYGEN_CHANNEL_MULTICH);
	xonar_clear_bits8(OXYGEN_DMA_FLUSH, OXYGEN_CHANNEL_MULTICH);
}

static void xonar_start_engine(void)
{
	xonar_d1.started = true;
	xonar_d1.paused = false;
	xonar_d1.interrupt_mask |= OXYGEN_CHANNEL_MULTICH;

	xonar_write16(OXYGEN_INTERRUPT_MASK, (uint16_t)xonar_d1.interrupt_mask);
	xonar_write8(OXYGEN_DMA_PAUSE, 0);
	xonar_write8(OXYGEN_DMA_STATUS, OXYGEN_CHANNEL_MULTICH);
}

static void xonar_stop_engine(void)
{
	xonar_d1.started = false;
	xonar_d1.paused = false;
	xonar_d1.interrupt_mask &= ~OXYGEN_CHANNEL_MULTICH;

	xonar_write8(OXYGEN_DMA_PAUSE, 0);
	xonar_write8(OXYGEN_DMA_STATUS, 0);
	xonar_write16(OXYGEN_INTERRUPT_MASK, (uint16_t)xonar_d1.interrupt_mask);

	xonar_set_bits8(OXYGEN_DMA_FLUSH, OXYGEN_CHANNEL_MULTICH);
	xonar_clear_bits8(OXYGEN_DMA_FLUSH, OXYGEN_CHANNEL_MULTICH);
}

static void xonar_prepare_stream(void)
{
	if (!xonar_d1.dma_buf) {
		return;
	}

	memset(xonar_d1.dma_buf, 0, xonar_d1.dma_bytes);
	for (uint32_t i = 0; i < xonar_d1.period_count; i++) {
		xonar_d1.period_valid_frames[i] = 0;
	}

	xonar_program_dma();
	xonar_prime_dma();
}

static size_t xonar_push_all_s16le(const int16_t *frames, size_t total_frames)
{
	if (!frames || total_frames == 0) {
		return 0;
	}

	if (!q_ensure_cap(total_frames)) {
		dprintf("xonar_d1: queue OOM (wanted %lu frames)\n", (unsigned long)total_frames);
		return 0;
	}

	memcpy(s_q_pcm + ((s_q_len_fr - s_q_head_fr) * 2), frames, total_frames * 2 * sizeof(int16_t));
	s_q_len_fr = (s_q_len_fr - s_q_head_fr) + total_frames;
	s_q_head_fr = 0;

	if (!xonar_d1.started) {
		xonar_prepare_stream();
		xonar_start_engine();
	} else if (!xonar_d1.paused) {
		xonar_service_periods();
	}

	return total_frames;
}

static void xonar_idle(void)
{
	if (!xonar_d1.started || xonar_d1.paused) {
		return;
	}

	xonar_service_periods();
}

static void xonar_pause(void)
{
	if (!xonar_d1.started) {
		return;
	}

	xonar_set_bits8(OXYGEN_DMA_PAUSE, OXYGEN_CHANNEL_MULTICH);
	xonar_d1.paused = true;
}

static void xonar_resume(void)
{
	if (!xonar_d1.started) {
		if (s_q_len_fr != s_q_head_fr) {
			xonar_prepare_stream();
			xonar_start_engine();
		}
		return;
	}

	xonar_clear_bits8(OXYGEN_DMA_PAUSE, OXYGEN_CHANNEL_MULTICH);
	xonar_d1.paused = false;
	xonar_service_periods();
}

static void xonar_stop_clear(void)
{
	xonar_stop_engine();

	s_q_head_fr = 0;
	s_q_len_fr = 0;

	for (uint32_t i = 0; i < xonar_d1.period_count; i++) {
		xonar_d1.period_valid_frames[i] = 0;
	}

	if (xonar_d1.dma_buf) {
		memset(xonar_d1.dma_buf, 0, xonar_d1.dma_bytes);
	}
}

static uint32_t xonar_buffered_ms(void)
{
	size_t sw_frames = s_q_len_fr - s_q_head_fr;
	if (!xonar_d1.started) {
		return (uint32_t)((sw_frames * 1000ULL) / XONAR_RATE_HZ);
	}

	uint32_t hw_bytes = xonar_hw_bytes();
	uint32_t current_period = hw_bytes / xonar_d1.period_bytes;
	if (current_period >= xonar_d1.period_count) {
		current_period = 0;
	}

	uint32_t played_frames_in_current = (hw_bytes % xonar_d1.period_bytes) / XONAR_FRAME_BYTES;

	size_t hw_frames = 0;
	for (uint32_t i = 0; i < xonar_d1.period_count; i++) {
		hw_frames += xonar_d1.period_valid_frames[i];
	}
	if (played_frames_in_current >= xonar_d1.period_valid_frames[current_period]) {
		hw_frames -= xonar_d1.period_valid_frames[current_period];
	} else {
		hw_frames -= played_frames_in_current;
	}

	return (uint32_t)(((sw_frames + hw_frames) * 1000ULL) / XONAR_RATE_HZ);
}

static uint32_t xonar_get_hz(void)
{
	return XONAR_RATE_HZ;
}

static const char **xonar_list_output_names(void)
{
	static const char *names[2] = {
		"Line Out",
		NULL
	};

	return names;
}

static bool xonar_select_output_by_name(const char *name)
{
	if (!name) {
		return false;
	}

	return strcasecmp(name, "Line Out") == 0;
}

static const char *xonar_get_current_output(void)
{
	return "Line Out";
}

static void xonar_irq([[maybe_unused]] uint8_t isr, [[maybe_unused]] uint64_t error, [[maybe_unused]] uint64_t irq, void *opaque)
{
	xonar_d1_dev_t *dev = opaque;
	if (!dev) {
		return;
	}

	uint16_t status = xonar_read16(OXYGEN_INTERRUPT_STATUS);
	if (!status) {
		return;
	}

	uint16_t clear = status & OXYGEN_CHANNEL_MULTICH;
	if (clear) {
		xonar_write16(OXYGEN_INTERRUPT_MASK, (uint16_t)(dev->interrupt_mask & ~clear));
		xonar_write16(OXYGEN_INTERRUPT_MASK, (uint16_t)dev->interrupt_mask);
		xonar_service_periods();
	}
}

static bool xonar_alloc_dma(void)
{
	uint8_t *raw = kmalloc_low(XONAR_BUFFER_BYTES + 32);
	if (!raw) {
		return false;
	}

	uintptr_t aligned = ((uintptr_t)raw + 31) & ~(uintptr_t)31;

	xonar_d1.dma_buf = (uint8_t *)aligned;
	xonar_d1.dma_phys = (uint32_t)aligned;
	xonar_d1.dma_bytes = XONAR_BUFFER_BYTES;
	xonar_d1.period_bytes = XONAR_PERIOD_BYTES;
	xonar_d1.period_count = XONAR_PERIOD_COUNT;
	xonar_d1.period_frames = XONAR_PERIOD_FRAMES;

	memset(xonar_d1.dma_buf, 0, xonar_d1.dma_bytes);
	return true;
}

static pci_dev_t xonar_find_device(bool *exact_match)
{
	pci_dev_t dev;
	uint16_t subvendor;
	uint16_t subdevice;

	*exact_match = false;

	dev = pci_get_device(CMEDIA_VENDOR_ID, OXYGEN_DEVICE_ID, -1);
	if (pci_not_found(dev)) {
		return device_zero;
	}

	subvendor = pci_read16(dev, PCI_SUBSYSTEM_VENDOR_ID);
	subdevice = pci_read16(dev, PCI_SUBSYSTEM_ID);

	if (subvendor == ASUS_VENDOR_ID && subdevice == XONAR_D1_SUBDEVICE_ID) {
		*exact_match = true;
	}

	return dev;
}

static audio_device_t *init_xonar_d1(void)
{
	audio_device_t *device;
	pci_dev_t dev;
	uint32_t bar0;
	bool exact_match;

	memset(&xonar_d1, 0, sizeof(xonar_d1));

	dev = xonar_find_device(&exact_match);
	if (pci_not_found(dev)) {
		dprintf("xonar_d1: no CMI8788 devices found\n");
		return NULL;
	}

	bar0 = pci_read(dev, PCI_BAR0);
	if (pci_bar_type(bar0) != PCI_BAR_TYPE_IOPORT) {
		dprintf("xonar_d1: BAR0 is not I/O space\n");
		return NULL;
	}

	xonar_d1.pci_dev = dev;
	xonar_d1.io_base = pci_io_base(bar0);

	if (!pci_bus_master(dev)) {
		dprintf("xonar_d1: failed to set bus master\n");
		return NULL;
	}

	if (!xonar_alloc_dma()) {
		dprintf("xonar_d1: failed to allocate DMA buffer\n");
		return NULL;
	}

	xonar_core_init();

	xonar_d1.irq_vector = pci_setup_interrupt("xonar_d1", dev, 0, xonar_irq, &xonar_d1);
	if (!xonar_d1.irq_vector) {
		dprintf("xonar_d1: failed to set interrupt\n");
		return NULL;
	}

	proc_register_idle(xonar_idle, IDLE_FOREGROUND, 1);

	device = kmalloc(sizeof(audio_device_t));
	if (!device) {
		return NULL;
	}

	make_unique_device_name("audio", device->name, MAX_AUDIO_DEVICE_NAME);
	device->opaque = &xonar_d1;
	device->next = NULL;
	device->play = xonar_push_all_s16le;
	device->frequency = xonar_get_hz;
	device->pause = xonar_pause;
	device->resume = xonar_resume;
	device->stop = xonar_stop_clear;
	device->queue_length = xonar_buffered_ms;
	device->get_outputs = xonar_list_output_names;
	device->select_output = xonar_select_output_by_name;
	device->get_current_output = xonar_get_current_output;

	xonar_d1.audio_dev = device;
	xonar_d1.initialised = true;

	if (exact_match) {
		kprintf("xonar_d1: Asus Xonar D1 detected at I/O 0x%04x\n", xonar_d1.io_base);
	} else {
		kprintf("xonar_d1: CMI8788-compatible device detected at I/O 0x%04x\n", xonar_d1.io_base);
	}

	return register_audio_device(device) ? device : NULL;
}

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void)
{
	audio_device_t *dev;

	dprintf("xonar_d1: module loaded\n");

	dev = init_xonar_d1();
	if (!dev) {
		return false;
	}

	if (!mixer_init(dev, 50, 25, 64)) {
		dprintf("xonar_d1: mixer init failed\n");
		return false;
	}

	kprintf("xonar_d1: started\n");
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void)
{
	return false;
}