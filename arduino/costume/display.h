#ifndef __DISPLAY_H
#define DISPLAY_H

void display_off(void);
void display_fixed(void);
void display_strobe(void);
void display_fade(void);
void display_two_color(void);
void display_top_down(void);
void display_bottom_up(void);
void display_rainbow(void);
void display_bi_pride(void);

extern void (*display_funcs[N_MODES])(void);

#endif
