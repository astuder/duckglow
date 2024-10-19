#include "ch32v003fun.h"
#include "i2c_slave.h"
#include "lookup.h"

#define PWM_STEPS 1024
#define PWM_SCALE 2		// left-shifts to scale 8-bit RGB to PWM resolution
#define I2C_ADDR 0x6c

// LED pins
// 	background: PA1, T1CH2, anode (+)
//	LED R: PC3, T1CH3, cathode (-)
//	LED G: PC4, T1CH4, cathode (-)
//	LED B: PD2, T1CH1, cathode (-)
#define LED_BG_PIN	PA1
#define LED_R_PIN	PC3
#define LED_G_PIN	PC4
#define LED_B_PIN	PD2

#define LED_BG_CH	2
#define LED_R_CH	3
#define LED_G_CH	4
#define LED_B_CH	1

#define WS2812_PIN	PC0
#define WS2812_CH	3

uint8_t pwm_chan[] = { LED_R_CH, LED_G_CH, LED_B_CH, LED_BG_CH };

// setup timer 1 for PWM to drive LEDs
void t1pwm_init(void)
{
	// enable GPIOC, GPIOD and TIM1
	RCC->APB2PCENR |= 	RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC |
						RCC_APB2Periph_TIM1;
	
	// configure LED pins
	funPinMode(LED_BG_PIN, GPIO_CFGLR_OUT_10Mhz_AF_PP);
	funPinMode(LED_R_PIN, GPIO_CFGLR_OUT_10Mhz_AF_PP);
	funPinMode(LED_G_PIN, GPIO_CFGLR_OUT_10Mhz_AF_PP);
	funPinMode(LED_B_PIN, GPIO_CFGLR_OUT_10Mhz_AF_PP);

	// reset TIM1 to init all regs
	RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
	RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;
	
	// CTLR1: default is up, events generated, edge align
	// SMCFGR: default clk input is CK_INT
	
	// prescaler 
	TIM1->PSC = 0x0000;
	
	// auto reload - sets period
	TIM1->ATRLR = PWM_STEPS;
	
	// reload immediately
	TIM1->SWEVGR |= TIM_UG;
	
	// enable CH1-4 output, positive pol
	TIM1->CCER |= TIM_CC1E | TIM_CC1P
				| TIM_CC2E // | TIM_CC2P background LEDs are inverted
				| TIM_CC3E | TIM_CC3P
				| TIM_CC4E | TIM_CC4P;

	// CH1+2 mode is output, PWM1 (CC1S = 00, OC1M = 110)
	TIM1->CHCTLR1 |= TIM_OC1M_2 | TIM_OC1M_1 | TIM_OC2M_2 | TIM_OC2M_1;

	// CH1+2 mode is output, PWM1 (CC1S = 00, OC1M = 110)
	TIM1->CHCTLR2 |= TIM_OC3M_2 | TIM_OC3M_1 | TIM_OC4M_2 | TIM_OC4M_1;

	// set the Capture Compare Register value to 50% initially
	TIM1->CH1CVR = 0;
	TIM1->CH2CVR = 0;
	TIM1->CH3CVR = 0;
	TIM1->CH4CVR = 0;
	
	// enable TIM1 outputs
	TIM1->BDTR |= TIM_MOE;
	
	// enable TIM1
	TIM1->CTLR1 |= TIM_CEN;
}

// set PWM for a given channel
void t1pwm_setpw(uint8_t channel, uint16_t width)
{
	switch(channel)
	{
		case 1: TIM1->CH1CVR = width; break;
		case 2: TIM1->CH2CVR = width; break;
		case 3: TIM1->CH3CVR = width; break;
		case 4: TIM1->CH4CVR = width; break;
		default: break;
	}
}

// setup timer 2 to decode WS2812 protocol
void t2cap_init(void)
{
	// WS2812 timing
	// datasheet:
	// 0: 0.35us / 0.8us
	// 1: 0.7us / 0.6us
	// 1.25us +/- 0.6us per bit
	// latch: > 50us
	// https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
	// 0: 0.2-0.5us high pulse
	// 1: >0.5us high pulse
	// latch: > 6us low
	// simiplified
	// if high pulse <= 5: 0, else: 1
	// if low for more than 6us, latch

	// configure pin as input with pull-down
	funPinMode(WS2812_PIN, GPIO_CFGLR_IN_PUPD);
	funDigitalWrite(WS2812_PIN, 0);

	// enable TIM2
	RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

	// Reset TIM2 to init all regs
	RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
	RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;

	// Prescaler 48MHz/5 -> ~0.1us resolution
	TIM2->PSC = 5;



}


// address of first R/W I2C register
#define I2C_REG_START 0xe0

// structure of I2C registers
struct i2c_regs_t {
	union {
		struct regs_t {
			uint8_t poem[I2C_REG_START];
			volatile uint8_t max[4];		// max LED brightness RGB,BG
			volatile uint8_t min[4];		// min LED brightness RGB,BG
			volatile uint8_t speed[4];		// LED fader speed RGB,BG
			volatile uint8_t phase[4];		// LED fader phase RGB,BG (0-255, offset into lookup-table)
			volatile uint8_t save;			// 0x42 = save current register settings to flash
		} regs;
		uint8_t data[255];
	};
};

// default content of I2C registers
struct i2c_regs_t i2c_regs = {
 	{
		{
			"Behold the duck.\n"
			"It does not cluck.\n"
			"A cluck it lacks.\n"
			"It quacks.\n"
			"It is specially fond\n"
			"Of a puddle or a pond.\n"
			"When it dines or sups,\n"
			"It bottoms ups.\n\n"
			"Regs (R,G,B,BG):\n"
			"E0 max\n"
			"E4 min\n"
			"E8 spd\n"
			"EC ph\n"
			"F0 42=save\0",
			{ 64, 64, 0, 255 },		// RGB, BG max
			{ 10, 10, 0, 0 },		// RGB, BG min
			{ 2, 2, 0, 2 },			// RGB, BG speed
			{ 0, 0, 0, 128 },		// RGB, BG phase
			0						// save
		}
	}
};

// callback after I2C write transaction
void on_i2c_write(uint8_t reg, uint8_t length)
{
	// do something?
}

// callback after I2C read 
void on_i2c_read(uint8_t reg)
{
	// do something?
}

// structure of configuration data saved to flash
typedef struct {
	union {
		struct fields_t {
			uint32_t magic;
			uint8_t version;
			char regs[sizeof(i2c_regs.regs)-1];		// exclude last register (save command reg)
		} fields;
		uint32_t raw[16];
	};
} presets_t;

#define PRESETS_MAGIC 0x6475636b	// "duck"
#define PRESETS_VERSION 2		// increment when modifing I2C register structure
#define PRESETS_REGS_SIZE (sizeof(i2c_regs.regs) - sizeof(i2c_regs.regs.poem) - 1)
#define PRESETS_ADDR 0x08003fc0	// points to page 255 in flash, we hope our code will never use that much flash

presets_t *flash_presets = (presets_t*) PRESETS_ADDR;

// load register settings from flash
bool presets_load(void)
{
	// verify that magic and version do match
	if (flash_presets->fields.magic != PRESETS_MAGIC || flash_presets->fields.version != PRESETS_VERSION) {
		return 0;
	}

	// initialize I2C registers from flash
	for (uint8_t i=0; i<PRESETS_REGS_SIZE; i++) {
		i2c_regs.data[I2C_REG_START+i] = flash_presets->fields.regs[i];
	}
	return 1;
}

// save current register settings to flash
bool presets_save(void)
{
	// fill in presets structure
	presets_t p;
	p.fields.magic = PRESETS_MAGIC;
	p.fields.version = PRESETS_VERSION;
	for (uint8_t i=0; i<PRESETS_REGS_SIZE; i++) {
		p.fields.regs[i] = i2c_regs.data[I2C_REG_START+i];
	}

	// unlock flash
	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xCDEF89AB;
	FLASH->MODEKEYR = 0x45670123;
	FLASH->MODEKEYR = 0xCDEF89AB;
	if (FLASH->CTLR & 0x8080) {
		// failed to unlock flash
		return 0;
	}

	// erase page
	FLASH->CTLR = CR_PAGE_ER;
	FLASH->ADDR = (intptr_t) PRESETS_ADDR;
	FLASH->CTLR = CR_STRT_Set | CR_PAGE_ER;
	while (FLASH->STATR & FLASH_STATR_BSY);

	// write page
	FLASH->CTLR = CR_PAGE_PG;
	FLASH->CTLR = CR_BUF_RST | CR_PAGE_PG;
	FLASH->ADDR = (intptr_t) PRESETS_ADDR;
	while (FLASH->STATR & FLASH_STATR_BSY);
	for (uint8_t i=0; i<16; i++) {
		flash_presets->raw[i] = p.raw[i]; 				// write to the memory
		FLASH->CTLR = CR_PAGE_PG | FLASH_CTLR_BUF_LOAD; // load the buffer
		while (FLASH->STATR & FLASH_STATR_BSY);  		// wait for completion
	}
	FLASH->CTLR = CR_PAGE_PG | CR_STRT_Set;
	while (FLASH->STATR & FLASH_STATR_BSY);

	// lock flash
	FLASH->CTLR = CR_LOCK_Set;

	return 1;
}

int main()
{
	SystemInit();
	funGpioInitAll();

	// set I2C address based on jumper configuration
	// PD5 connected to GND adds 1 to I2C_ADDR
	// PD6 connected to GND adds 2 to I2C_ADDR
	funPinMode(PD5, GPIO_CFGLR_IN_PUPD);
	funPinMode(PD6, GPIO_CFGLR_IN_PUPD);
	funDigitalWrite(PD5, 1);
	funDigitalWrite(PD6, 1);
	Delay_Ms(100);
	uint8_t i2c_addr = I2C_ADDR + (((funDigitalRead(PD6) << 1) + funDigitalRead(PD5)) ^ 3);

	// load presets from flash
	presets_load();

	// setup I2C	
	funPinMode(PC1, GPIO_CFGLR_OUT_10Mhz_AF_OD);
	funPinMode(PC2, GPIO_CFGLR_OUT_10Mhz_AF_OD);
	SetupI2CSlave(i2c_addr, i2c_regs.data, sizeof(i2c_regs.data), on_i2c_write, on_i2c_read, false);

	// init TIM1 for PWM
	t1pwm_init();

	// init TIM2 for input capture mode
	t2cap_init();

	uint16_t idx[4] = { 0, 0, 0, 0 };
	uint16_t pos[4] = { i2c_regs.regs.phase[0]<<2, i2c_regs.regs.phase[1]<<2, i2c_regs.regs.phase[2]<<2, i2c_regs.regs.phase[3]<<2 };
	uint32_t val;

	while(1)
	{
		if (i2c_regs.regs.save == 42) {
			presets_save();
			i2c_regs.regs.save = 0;
		}

		for (uint8_t ch = 0; ch < 4; ch++)
		{
			if (i2c_regs.regs.speed[ch] == 0)
			{
				// use fixed color if speed = 0
				val = i2c_regs.regs.max[ch] << PWM_SCALE;
				if (val >= PWM_STEPS)
				{
					val = PWM_STEPS-1;
				}
				t1pwm_setpw(pwm_chan[ch], val);
			}
			else
			{
				// do sine fading if speed is set
				uint16_t max = i2c_regs.regs.max[ch] << PWM_SCALE;
				uint16_t min = i2c_regs.regs.min[ch] << PWM_SCALE;
				val = ((((uint32_t) sine[pos[ch]]) * (max - min)) >> 16) + min;
				if (val >= PWM_STEPS)
				{
					val = PWM_STEPS-1;
				}
				t1pwm_setpw(pwm_chan[ch], val);
				idx[ch] += i2c_regs.regs.speed[ch];
				if (idx[ch] >= sine_size)
				{
					idx[ch] -= sine_size;
				}
				pos[ch] = idx[ch] + (i2c_regs.regs.phase[ch]<<2);
				if (pos[ch] >= sine_size)
				{
					pos[ch] -= sine_size;
				}
			}
		}

		Delay_Ms(20);
	}
}
