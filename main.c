
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <system.h>
#include <sys/alt_alarm.h>
#include <io.h>

#include "fatfs.h"
#include "diskio.h"

#include "ff.h"
#include "monitor.h"
#include "uart.h"

#include "alt_types.h"

#include <altera_up_avalon_audio.h>
#include <altera_up_avalon_audio_and_video_config.h>

#include <altera_avalon_pio_regs.h>

int valid = 1;
int state = 0;

int previous_state = 0;
int next_state = 0;
int pause_state = 0;
int stop_state = 0;
int pause = 1;

int switch0 = 0;
int switch1 = 0;

int last = 0;

int s_index = 0;
int file_index = 0;

long p1 = 0;

alt_up_audio_dev * audio_dev;

char *ptr;
FILE* lcd;
FATFS Fatfs[_VOLUMES]; /* File system object for each logical drive */
FIL File1;
DIR Dir; /* Directory object */
uint8_t Buff[8192] __attribute__ ((aligned(4))); /* Working buffer */
char filename[20][20];
unsigned long file_size[20];
int file_order[20];
FILINFO Finfo;
uint32_t s2, cnt, blen = sizeof(Buff);

static void put_rc(FRESULT rc) {
	const char *str =
			"OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
					"INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
					"INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0"
					"LOCKED\0" "NOT_ENOUGH_CORE\0" "TOO_MANY_OPEN_FILES\0";
	FRESULT i;

	for (i = 0; i != rc && *str; i++) {
		while (*str++)
			;
	}
	xprintf("rc=%u FR_%s\n", (uint32_t) rc, str);
}


int isWav(char *filename) {
	int filename_length = (int) strlen(filename);

	if (filename[filename_length - 4] == '.'
			&& filename[filename_length - 3] == 'W'
			&& filename[filename_length - 2] == 'A'
			&& filename[filename_length - 1] == 'V') {
		return 1;
	}

	else {
		return 0;
	}
}

void song_index() {

	char* ptr = "";

	uint8_t res = f_opendir(&Dir, ptr);

	if (res) // if res in non-zero there is an error; print the error.
	{
		put_rc(res);
		return;
	}

	int order = 1;
	uint32_t s1, s2 = sizeof(Buff);
	long p1;
	
	p1 = s1 = s2 = 0; 

	while (1) {

		res = f_readdir(&Dir, &Finfo);

		if ((res != FR_OK) || !Finfo.fname[0])
			break;

		if (Finfo.fattrib & AM_DIR) {
			s2++;

		} else {
			s1++;
			p1 += Finfo.fsize;
		}

		if (isWav(&Finfo.fname[0]) == 1) {
			strcpy(filename[s_index], &(Finfo.fname[0]));
			file_size[s_index] = Finfo.fsize;
			file_order[s_index] = order;
			s_index++;
			order++;
			
		}
	}
}

void lcd_display(int song_index, char* song, char* state) {
	if (lcd != NULL) {

		fprintf(lcd, "%c%s", 27, "[2J");
		fprintf(lcd, "%d: %s\n", song_index, song);
		fprintf(lcd, "%s\n", state);
		
	}
}

void switch_helper() {
	if (!switch0 && !switch1) {
		//normal speed
		lcd_display(file_order[file_index], filename[file_index],
				"PBACK-NORM SPD");
	}
	if (switch0 && !switch1) {
		//half speed
		lcd_display(file_order[file_index], filename[file_index],
				"PBACK-HALF SPD");
	}
	if (!switch0 && switch1) {
		//double speed
		lcd_display(file_order[file_index], filename[file_index],
				"PBACK-DBL SPD");
	}
	if (switch0 && switch1) {
		//mono
		lcd_display(file_order[file_index], filename[file_index], "PBACK-MONO");
	}
}

void file_play() {
	if (pause) {
		switch0 = IORD(SWITCH_PIO_BASE, 0) & 0x1;
		switch1 = IORD(SWITCH_PIO_BASE, 0) & 0x2;
		return;
	}

	uint8_t res = 0;

	//buffer length
	const int blen = 1024;

	// used for reading from file
	int r_buf;
	int l_buf;
	// open the Audio port

	if (p1) {
		if ((uint32_t) p1 >= blen) {
			last = 0;
			cnt = blen;
			p1 -= blen;
		} else {
			cnt = p1;
			p1 = 0;
			last = 1;
		}
		res = f_read(&File1, Buff, cnt, &s2);
		if (res != FR_OK) {
			put_rc(res); 
			return;
		}
		int i = 0;
		int check_repeat = 0;

		if (!switch0 && !switch1) {
			//normal speed
			for (i = 0; i < cnt; i += 4) {

				l_buf = Buff[i] | Buff[i + 1] << 8;
				r_buf = Buff[i + 2] | Buff[i + 3] << 8;

				int fifospace = alt_up_audio_write_fifo_space(audio_dev,
				ALT_UP_AUDIO_RIGHT);
				if (fifospace > 0) {
					alt_up_audio_write_fifo(audio_dev, &(r_buf), 1,
					ALT_UP_AUDIO_RIGHT);
					alt_up_audio_write_fifo(audio_dev, &(l_buf), 1,
					ALT_UP_AUDIO_LEFT);
				} else {
					i -= 4;
				}
			}
		}
		if (switch0 && !switch1) {
			//half speed
			for (i = 0; i < cnt; i += 4) {
				l_buf = Buff[i] | Buff[i + 1] << 8;
				r_buf = Buff[i + 2] | Buff[i + 3] << 8;

				int fifospace = alt_up_audio_write_fifo_space(audio_dev,
				ALT_UP_AUDIO_RIGHT);
				if (fifospace > 0) {
					alt_up_audio_write_fifo(audio_dev, &(r_buf), 1,
					ALT_UP_AUDIO_RIGHT);
					alt_up_audio_write_fifo(audio_dev, &(l_buf), 1,
					ALT_UP_AUDIO_LEFT);
					if (check_repeat == 0) {
						i -= 4;
						check_repeat = 1;
					} else if (check_repeat == 1) {
						check_repeat = 0;
					}
				} else {
					i -= 4;
				}
			}
		}
		if (!switch0 && switch1) {
			//double speed
			for (i = 0; i < cnt; i += 8) {

				l_buf = Buff[i] | Buff[i + 1] << 8;
				r_buf = Buff[i + 2] | Buff[i + 3] << 8;

				int fifospace = alt_up_audio_write_fifo_space(audio_dev,
				ALT_UP_AUDIO_RIGHT);
				if (fifospace > 0) {
					alt_up_audio_write_fifo(audio_dev, &(r_buf), 1,
					ALT_UP_AUDIO_RIGHT);
					alt_up_audio_write_fifo(audio_dev, &(l_buf), 1,
					ALT_UP_AUDIO_LEFT);
				} else {
					i -= 8;
				}

			}
		}
		if (switch0 && switch1) {
			//mono

			for (i = 0; i < cnt; i += 4) {
				l_buf = Buff[i] | Buff[i + 1] << 8;

				int fifospace = alt_up_audio_write_fifo_space(audio_dev,
				ALT_UP_AUDIO_RIGHT);
				if (fifospace > 0) {
					r_buf = 0;
					alt_up_audio_write_fifo(audio_dev, &(r_buf), 1,
					ALT_UP_AUDIO_RIGHT);
					alt_up_audio_write_fifo(audio_dev, &(l_buf), 1,
					ALT_UP_AUDIO_LEFT);
				} else {
					i -= 4;
				}

			}
		}
		if (last == 1) {
			put_rc(f_open(&File1, &filename[file_index], 1));
			p1 = file_size[file_index];
			lcd_display(file_order[file_index], filename[file_index],
					"STOPPED");
			pause = 1;
		}
	}
}

//button ISR

static void button_ISR(void* context, alt_u32 id) {
	//disable button interrupts
	IOWR(BUTTON_PIO_BASE, 2, 0x0);
	//period low
	IOWR(TIMER_0_BASE, 2, 0xE360);
	//period high
	IOWR(TIMER_0_BASE, 3, 0x0016);
	//set bits 2 and 0 to enable START and ITO
	IOWR(TIMER_0_BASE, 1, 0x5);

}

//timer ISR
static void timer_ISR(void* context, alt_u32 id) {

	int button = IORD(BUTTON_PIO_BASE, 0);

	//previous
	if (button == 0x07) {
		state = 0x07;
		previous_state = 1;

	}
	//stop
	else if (button == 0x0B) {
		state = 0x0B;
		stop_state = 1;

	}
	//pause/play
	else if (button == 0x0D) {
		state = 0x0D;
		pause_state = 1;

	}
	//next
	else if (button == 0x0E) {
		state = 0x0E;
		next_state = 1;

	}
	//nothing
	else if (button == 0x0F) {
		if (valid) {
			next_state++;
			pause_state++;
			stop_state++;
			previous_state++;
		} else {
			next_state = pause_state = stop_state = previous_state = 0;
		}
		valid = 1;

	} else {
		valid = 0;

	}

	IOWR(TIMER_0_BASE, 1, 0x0);
	IOWR(BUTTON_PIO_BASE, 3, 0x0);
	IOWR(BUTTON_PIO_BASE, 2, 0x0F);
}


int main() {

	switch0 = IORD(SWITCH_PIO_BASE, 0) & 0X1;
	switch1 = IORD(SWITCH_PIO_BASE, 0) & 0X2;

	alt_irq_register(BUTTON_PIO_IRQ, (void *) 0, button_ISR);
	alt_irq_register(TIMER_0_IRQ, (void *) 0, timer_ISR);

	IOWR(BUTTON_PIO_BASE, 2, 0X0F); //enable button interrupts

	lcd = fopen("/dev/lcd_display", "w");

	xprintf("rc=%d\n", (uint16_t) disk_initialize((uint8_t ) p1));

	put_rc(f_mount((uint8_t) p1, &Fatfs[p1]));

	song_index();

	//song starts
	lcd_display(file_order[file_index], filename[file_index], "STOPPED");

	p1 = 1;

	put_rc(f_open(&File1, &filename[file_index], 1));
	p1 = file_size[file_index];

	audio_dev = alt_up_audio_open_dev("/dev/Audio");

	if (audio_dev == NULL) {
		alt_printf("Error: audio device could not be opened \n");
	} else {
		alt_printf("Audio device has been opened \n");
	}

	while (1) {

		//previous
		if (state == 0x07) {
			if (valid == 1 && previous_state == 2) {

				file_index--;
				if (file_index < 0) {
					file_index = 15;
				}
				if (pause) {
					lcd_display(file_order[file_index], filename[file_index],
							"STOPPED");
				} else {

					switch_helper();

				}
				previous_state = 0;

				put_rc(f_open(&File1, &filename[file_index], 1));
				p1 = file_size[file_index];
			}
		}
		// stopped
		else if (state == 0x0B) {
			if (valid == 1 && stop_state == 2) {
				lcd_display(file_order[file_index], filename[file_index],
						"STOPPED");
				stop_state = 0;

				//reset track to start
				put_rc(f_open(&File1, &filename[file_index], 1));
				p1 = file_size[file_index];
				pause = 1;
			}
		}
		// play/pause
		else if (state == 0x0D) {
			if (valid == 1 && pause_state == 2) {
				if (pause) {
					switch_helper();
					pause = 0;
				} else {
					lcd_display(file_order[file_index], filename[file_index],
							"PAUSED");
					pause = 1;
				}
				pause_state = 0;
			}
		}
		//next
		else if (state == 0x0E) {
			if (valid == 1 && next_state == 2) {
				file_index++;
				if (file_index > 15) {
					file_index = 0;
				}
				if (pause) {
					lcd_display(file_order[file_index], filename[file_index],
							"STOPPED");
				} else {
					switch_helper();
				}
				next_state = 0;

				put_rc(f_open(&File1, &filename[file_index], 1));
				p1 = file_size[file_index];
			}
		}

		file_play();
	}

	return 0;
}


