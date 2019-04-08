/*
 * main.c
 *
 * Created: 05/03/2019 18:00:58
 *  Author: eduardo
 */ 

#include <asf.h>
#include "tfont.h"
#include "sourcecodepro_28.h"
#include "calibri_36.h"
#include "arial_72.h"

#define BUT_PIO_ID			  ID_PIOA
#define BUT_PIO				  PIOA
#define BUT_PIN				  11
#define BUT_PIN_MASK			  (1 << BUT_PIN)

struct ili9488_opt_t g_ili9488_display_opt;


static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses);
void BUT_init(void);
void configure_lcd(void);
void font_draw_text(tFont *font, const char *text, int x, int y, int spacing);
void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq);

volatile uint8_t pulsos = 0;
volatile uint8_t tempo = 0;
volatile uint8_t minuto = 0;
volatile uint8_t hora = 0;
volatile Bool f_rtt_alarme = false;
char buffer[32];
char buffer1[32];
char buffer2[32];


/**
*  Handle Interrupcao botao 1
*/

static void Button_callback(void)
{
	pulsos += 1;
}

/**
* @Brief Inicializa o pino do BUT
*/
void BUT_init(void){
	/* config. pino botao em modo de entrada */
	pmc_enable_periph_clk(BUT_PIO_ID);
	pio_set_input(BUT_PIO, BUT_PIN_MASK, PIO_PULLUP | PIO_DEBOUNCE);

	/* config. interrupcao em borda de descida no botao do kit */
	/* indica funcao (but_Handler) a ser chamada quando houver uma interrupção */
	pio_enable_interrupt(BUT_PIO, BUT_PIN_MASK);
	pio_handler_set(BUT_PIO, BUT_PIO_ID, BUT_PIN_MASK, PIO_IT_FALL_EDGE, Button_callback);

	/* habilita interrupçcão do PIO que controla o botao */
	/* e configura sua prioridade                        */
	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 1);
};

/**
*  Interrupt handler for TC1 interrupt.
*/
void TC0_Handler(void){
	volatile uint32_t ul_dummy;

	/****************************************************************
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	******************************************************************/
	ul_dummy = tc_get_status(TC0, 0);

	/* Avoid compiler warning */
	UNUSED(ul_dummy);
	
	tempo+=1; //em segundos
	if (tempo == 60){
		minuto += 1;
		tempo = 0;
	}
	
	if (minuto == 60){
		hora += 1;
		minuto = 0;
	}
	sprintf(buffer2, "%02d:%02d:%02d", hora,minuto,tempo);
	font_draw_text(&arial_72, buffer2, 50, 50, 1);
	
	
}

/**
* Configura TimerCounter (TC) para gerar uma interrupcao no canal (ID_TC e TC_CHANNEL)
* na taxa de especificada em freq.
*/
void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	uint32_t channel = 1;

	/* Configura o PMC */
	/* O TimerCounter é meio confuso
	o uC possui 3 TCs, cada TC possui 3 canais
	TC0 : ID_TC0, ID_TC1, ID_TC2
	TC1 : ID_TC3, ID_TC4, ID_TC5
	TC2 : ID_TC6, ID_TC7, ID_TC8
	*/
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  4Mhz e interrupçcão no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura e ativa interrupçcão no TC canal 0 */
	/* Interrupção no C */
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);

	/* Inicializa o canal 0 do TC */
	tc_start(TC, TC_CHANNEL);
}


void RTT_Handler(void)
{
	uint32_t ul_status;

	/* Get RTT status */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) { 
		//tempo+=1;
		//sprintf(buffer2, "%d", tempo);
		//font_draw_text(&arial_72, buffer2, 50, 50, 1);
	}

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		float f = pulsos/4;
		float w =  2*3.14*f;
		int vel = (int) w*(0.65/2); 
		int dist = vel*4;
		sprintf(buffer, "%02d", vel);
		sprintf(buffer1, "%02d", dist);
		font_draw_text(&arial_72, buffer, 50, 250, 1);
 		font_draw_text(&arial_72, buffer1, 50, 150, 1);
		f_rtt_alarme = true;                  // flag RTT alarme
	}
	pulsos = 0;
}

static float get_time_rtt(){
	uint ul_previous_time = rtt_read_timer_value(RTT);
}

static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses)
{
	uint32_t ul_previous_time;

	/* Configure RTT for a 1 second tick interrupt */
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	ul_previous_time = rtt_read_timer_value(RTT);
	while (ul_previous_time == rtt_read_timer_value(RTT));
	
	rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);

	/* Enable RTT interrupt */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 0);
	NVIC_EnableIRQ(RTT_IRQn);
	rtt_enable_interrupt(RTT, RTT_MR_ALMIEN);
}

void configure_lcd(void){
	/* Initialize display parameter */
	g_ili9488_display_opt.ul_width = ILI9488_LCD_WIDTH;
	g_ili9488_display_opt.ul_height = ILI9488_LCD_HEIGHT;
	g_ili9488_display_opt.foreground_color = COLOR_CONVERT(COLOR_WHITE);
	g_ili9488_display_opt.background_color = COLOR_CONVERT(COLOR_WHITE);

	/* Initialize LCD */
	ili9488_init(&g_ili9488_display_opt);
	ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
	
}


void font_draw_text(tFont *font, const char *text, int x, int y, int spacing) {
	char *p = text;
	while(*p != NULL) {
		char letter = *p;
		int letter_offset = letter - font->start_char;
		if(letter <= font->end_char) {
			tChar *current_char = font->chars + letter_offset;
			ili9488_draw_pixmap(x, y, current_char->image->width, current_char->image->height, current_char->image->data);
			x += current_char->image->width + spacing;
		}
		p++;
	}	
}


int main(void) {
	board_init();
	sysclk_init();	
	configure_lcd();

	/* Disable the watchdog */
	WDT->WDT_MR = WDT_MR_WDDIS;

	delay_init();

	/* Configura os botões */
	BUT_init();

	/** Configura timer TC0, canal 1 */
	TC_init(TC0, ID_TC0, 0, 1/4); //para contar a cada segundo
	
	// Inicializa RTT com IRQ no alarme.
	f_rtt_alarme = true;
  
	while(1) {
		//pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
			if (f_rtt_alarme){
      
				/*
				* O clock base do RTT é 32678Hz
				* Para gerar outra base de tempo é necessário
				* usar o PLL pre scale, que divide o clock base.
				*
				* Nesse exemplo, estamos operando com um clock base
				* de pllPreScale = 32768/32768/2 = 2Hz
				*
				* Quanto maior a frequência maior a resolução, porém
				* menor o tempo máximo que conseguimos contar.
				*
				* Podemos configurar uma IRQ para acontecer quando 
				* o contador do RTT atingir um determinado valor
				* aqui usamos o irqRTTvalue para isso.
				* 
				* Nesse exemplo o irqRTTvalue = 8, causando uma
				* interrupção a cada 2 segundos (lembre que usamos o 
				* pllPreScale, cada incremento do RTT leva 500ms (2Hz).
				*/
				uint16_t pllPreScale = (int) (((float) 32768) / 2.0); //2
				uint32_t irqRTTvalue  = 8; //4
      
				// reinicia RTT para gerar um novo IRQ
				RTT_init(pllPreScale, irqRTTvalue);         
      
				/*
				* caso queira ler o valor atual do RTT, basta usar a funcao
				*   rtt_read_timer_value()
				*/
      
				/*
				* CLEAR FLAG
				*/
				f_rtt_alarme = false;
			}
	}
}