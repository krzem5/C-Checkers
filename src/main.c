#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
#include <checkers/checkers.h>



int main(void){
#ifdef _MSC_VER
	SetConsoleOutputCP(CP_UTF8);
	HANDLE stdout_handle=GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD old_stdout_mode=GetConsoleMode(stdout_handle);
	SetConsoleMode(stdout_handle,ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT|ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	HANDLE stdin_handle=GetStdHandle(STD_INPUT_HANDLE);
	DWORD old_stdin_mode=GetConsoleMode(stdin_handle);
	SetConsoleMode(stdin_handle,ENABLE_VIRTUAL_TERMINAL_INPUT);
#else
	struct termios old_terminal_config;
	tcgetattr(STDOUT_FILENO,&old_terminal_config);
	struct termios terminal_config=old_terminal_config;
	terminal_config.c_iflag=(terminal_config.c_iflag&(~(INLCR|IGNBRK)))|ICRNL;
	terminal_config.c_lflag=(terminal_config.c_lflag&(~(ICANON|ECHO)))|ISIG|IEXTEN;
	tcsetattr(STDIN_FILENO,TCSANOW,&terminal_config);
#endif
	checkers_board_t board;
	checkers_move_history_t history;
	checkers_init_board(&board,&history);
	while ((board.is_white_turn?checkers_make_player_move:checkers_make_ai_move)(&board,&history));
	checkers_print_board(&board,&history);
	checkers_free_history(&history);
#ifdef _MSC_VER
	SetConsoleMode(stdin_handle,old_stdin_mode);
#else
	tcsetattr(STDOUT_FILENO,TCSANOW,&old_terminal_config);
#endif
	return 0;
}
