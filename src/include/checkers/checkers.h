#ifndef _CHECKERS_CHECKERS_H_
#define _CHECKERS_CHECKERS_H_ 1



#define POS_FROM_XY(x,y) ((((x)>>1)<<3)|(y))
#define POS_GET_X(i) ((((i)>>3)<<1)|((i)&1))
#define POS_GET_Y(i) ((i)&7)

#define CHECKERS_MAX_MOVE_COUNT 256

#define CHECKERS_HISTORY_MAX_PREVIEW_SIZE 5

#define CHECKERS_AI_MAX_DEPTH 8
#define CHECKERS_AI_SCORE_SUCCESS 10000.0f
#define CHECKERS_AI_FACTOR_GAME_SCORE 6.0f
#define CHECKERS_AI_FACTOR_KING_COUNT 2.0f
#define CHECKERS_AI_FACTOR_MOVE_COUNT 0.15f
#define CHECKERS_AI_FACTOR_ITERATION 0.15f



typedef struct _CHECKERS_BOARD{
	_Bool is_white_turn;
	unsigned char score_white;
	unsigned char score_black;
	unsigned int state_is_white;
	unsigned int state_is_black;
	unsigned int state_is_king;
} checkers_board_t;



typedef struct _CHECKERS_RAW_BOARD_MOVES{
	unsigned int down[2];
	unsigned int up[2];
	unsigned int down_capture[2];
	unsigned int up_capture[2];
	_Bool is_capture;
} checkers_raw_board_moves_t;



typedef struct _CHECKERS_BOARD_MOVE{
	unsigned char length;
	unsigned char points[17];
} checkers_board_move_t;



typedef struct _CHECKERS_BOARD_MOVES{
	unsigned int count;
	checkers_board_move_t data[CHECKERS_MAX_MOVE_COUNT];
} checkers_board_moves_t;



typedef struct _CHECKERS_HISTORY_MOVE{
	unsigned char from;
	unsigned char delete;
	unsigned char to;
} checkers_history_move_t;



typedef struct _CHECKERS_MOVE_HISTORY{
	unsigned int white_count;
	unsigned int black_count;
	checkers_history_move_t* white_moves;
	checkers_history_move_t* black_moves;
} checkers_move_history_t;



void checkers_init_board(checkers_board_t* out,checkers_move_history_t* history);



void checkers_free_history(checkers_move_history_t* history);



void checkers_get_raw_moves(const checkers_board_t* board,checkers_raw_board_moves_t* out);



void checkers_get_moves(const checkers_board_t* board,const checkers_raw_board_moves_t* raw_moves,checkers_board_moves_t* out);



_Bool checkers_make_player_move(checkers_board_t* board,checkers_move_history_t* history);



_Bool checkers_make_ai_move(checkers_board_t* board,checkers_move_history_t* history);



void checkers_print_board(const checkers_board_t* board,const checkers_move_history_t* history);



void checkers_print_moves(const checkers_board_moves_t* moves);




#endif
