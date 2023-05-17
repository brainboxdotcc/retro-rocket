#include <kernel.h>

const char* gtw_words[] = {
	"add",
	"amazing",
	"apple",
	"assembly",
	"banana",
	"bed",
	"blue",
	"box",
	"build",
	"builder",
	"car",
	"cat",
	"color",
	"console",
	"disk",
	"divide",
	"dog",
	"eat",
	"enter",
	"food",
	"game",
	"grammar",
	"green",
	"guess",
	"head",
	"java",
	"javascript",
	"keyboard",
	"life",
	"machine",
	"man",
	"math",
	"minus",
	"monkey",
	"mouse",
	"multiply",
	"orange",
	"paint",
	"pancake",
	"phone",
	"programmer",
	"python",
	"read",
	"room",
	"ruby",
	"rust",
	"screen",
	"size",
	"snake",
	"tail",
	"teacher",
	"user",
	"way",
	"yellow",
	"zig",
	NULL
};

int gtw_start(void)
{
	clearscreen(current_console);
	gotoxy(0, 0);
	size_t i;
	for (i = 0; gtw_words[i] != NULL; ++i);
	kprintf("Words count: %d words\n", i);
	const char* selected_word = gtw_words[mt_rand() % (i + 1)];
	unsigned int selected_word_length = strlen(selected_word);
	char* word = kmalloc(selected_word_length + 1);
	memset(word, '_', selected_word_length);
	word[selected_word_length] = '\0';
	unsigned char c;
	bool solved = false;
	for (;;) {
		unsigned char tmp;
		gotoxy(0, 2);
		setforeground(current_console, DEFAULT_COLOUR);
		kprintf("Word: ");
		setforeground(current_console, COLOUR_DARKGREEN);
		kprintf("'%s'\n", word);
		setforeground(current_console, DEFAULT_COLOUR);
		if (solved) {
			break;
		}
		while ((tmp = kgetc(current_console)) == 255);
		if (tmp == 0x1B) {
			clearscreen(current_console);
			gotoxy(0, 0);
			break;
		}
		if (c == tmp) {
			continue;
		}
		c = tmp;
		for (i = 0; i < selected_word_length; ++i) {
			if (selected_word[i] == c && word[i] != c) {
				word[i] = c;
				break;
			}
		}
		solved = !strchr(word, '_');
	}
	gotoxy(0, 3);
	if (solved) {
		setforeground(current_console, COLOUR_DARKGREEN);
		kprintf("Congratulations, you won! Word was: '%s'\n", selected_word);
		setforeground(current_console, DEFAULT_COLOUR);
	}
	kfree(word);
	return 0;
}
