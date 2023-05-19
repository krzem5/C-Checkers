#include <checkers/checkers.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>



int main(void){
	struct termios old_terminal_config;
	tcgetattr(STDOUT_FILENO,&old_terminal_config);
	struct termios terminal_config=old_terminal_config;
	terminal_config.c_iflag=(terminal_config.c_iflag&(~(INLCR|IGNBRK)))|ICRNL;
	terminal_config.c_lflag=(terminal_config.c_lflag&(~(ICANON|ECHO)))|ISIG|IEXTEN;
	tcsetattr(STDIN_FILENO,TCSANOW,&terminal_config);
	checkers_board_t board;
	checkers_move_history_t history;
	checkers_init_board(&board,&history);
	while ((!board.is_white_turn?checkers_make_player_move:checkers_make_ai_move)(&board,&history));
	checkers_print_board(&board,&history);
	checkers_free_history(&history);
	tcsetattr(STDOUT_FILENO,TCSANOW,&old_terminal_config);
	return 0;
}
