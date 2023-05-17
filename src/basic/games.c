/**
 * @file basic/string.c
 * @brief BASIC game launcher
 */
#include <games/guess_the_word.h>
#include <kernel.h>

typedef int (*game_t)(void);

const game_t games[] = {
	gtw_start,
	NULL
};

int64_t basic_game(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t game_index = intval;
	PARAMS_END("GAME", 0);
	size_t games_count;
	for (games_count = 0; games[games_count] != NULL; ++games_count);
	if (game_index < games_count) {
		return games[game_index]();
	}
	tokenizer_error_print(ctx, "Invalid game number");
	return -256;
}
