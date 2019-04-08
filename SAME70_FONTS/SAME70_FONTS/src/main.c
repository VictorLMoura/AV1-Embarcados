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

void BUT_init(void);
void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq);
void configure_lcd(void);
void font_draw_text(tFont *font, const char *text, int x, int y, int spacing);

volatile uint8_t pulsos = 0;

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
	/* indica funcao (but_Handler) a ser chamada quando houver uma interrup��o */
	pio_enable_interrupt(BUT_PIO, BUT_PIN_MASK);
	pio_handler_set(BUT_PIO, BUT_PIO_ID, BUT_PIN_MASK, PIO_IT_FALL_EDGE, Button_callback);

	/* habilita interrup�c�o do PIO que controla o botao */
	/* e configura sua prioridade                        */
	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 1);
};

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
	
 	font_draw_text(&sourcecodepro_28, "OIMUNDO", 50, 50, 1);
 	font_draw_text(&calibri_36, "Oi Mundo! #$!@", 50, 100, 1);
 	font_draw_text(&arial_72, "102456", 50, 200, 2);

	/* Disable the watchdog */
	WDT->WDT_MR = WDT_MR_WDDIS;

	delay_init();

	/* Configura os bot�es */
	BUT_init();

	/** Configura timer TC0, canal 1 */
	TC_init(TC0, ID_TC0, 0, 1);
		
	while(1) {
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}