#include <checkers/checkers.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



#define FIND_FIRST_SET_BIT(m) (__builtin_ffs(m)-1)
#define COUNT_SET_BITS(m) __builtin_popcount(m)



#define BUILD_COMMAND(a,b) ((((unsigned int)(a))<<8)|(b))



typedef struct _REDUCED_BOARD_STATE{
	unsigned int current;
	unsigned int opponent;
} reduced_board_state_t;



static inline void _get_capture_moves(unsigned int current_down,unsigned int current_up,unsigned int opponent,unsigned int empty,checkers_raw_board_moves_t* out){
	out->down_capture[0]=current_down&(((opponent<<7)&0x15151500)|((opponent>>1)&0x2a2a2a00))&(empty<<6);
	out->down_capture[1]=current_down&(((opponent>>1)&0x00151515)|((opponent>>9)&0x002a2a2a))&(empty>>10);
	out->up_capture[0]=current_up&(((opponent<<1)&0x00545454)|((opponent>>7)&0x00a8a8a8))&(empty>>6);
	out->up_capture[1]=current_up&(((opponent<<9)&0x54545400)|((opponent<<1)&0xa8a8a800))&(empty<<10);
	out->is_capture=!!(out->down_capture[0]|out->down_capture[1]|out->up_capture[0]|out->up_capture[1]);
}



static void _make_move(checkers_board_t* board,const checkers_board_move_t* move){
	unsigned int mask_src=1<<move->points[0];
	unsigned int mask_dst=1<<move->points[move->length-1];
	if (board->is_white_turn){
		board->state_is_white&=~mask_src;
		board->state_is_white|=mask_dst;
		if (POS_GET_Y(move->points[move->length-1])==7){
			board->state_is_king|=mask_src;
		}
	}
	else{
		board->state_is_black&=~mask_src;
		board->state_is_black|=mask_dst;
		if (!POS_GET_Y(move->points[move->length-1])){
			board->state_is_king|=mask_src;
		}
	}
	if (board->state_is_king&mask_src){
		board->state_is_king|=mask_dst;
	}
	else{
		board->state_is_king&=~mask_dst;
	}
	if (move->length>2){
		if (board->is_white_turn){
			board->score_white+=move->length>>1;
		}
		else{
			board->score_black+=move->length>>1;
		}
		for (unsigned int i=1;i<move->length;i+=2){
			unsigned int mask=~(1<<move->points[i]);
			board->state_is_white&=mask;
			board->state_is_black&=mask;
		}
	}
	board->is_white_turn^=1;
}



static float _minmax(const checkers_board_t* board,unsigned int depth,float alpha,float beta){
	checkers_raw_board_moves_t raw_moves;
	checkers_get_raw_moves(board,&raw_moves);
	checkers_board_moves_t moves;
	checkers_get_moves(board,&raw_moves,&moves);
	float current_score=CHECKERS_AI_FACTOR_GAME_SCORE*(board->score_black-board->score_white)+CHECKERS_AI_FACTOR_KING_COUNT*(COUNT_SET_BITS(board->state_is_black&board->state_is_king)-COUNT_SET_BITS(board->state_is_white&board->state_is_king))+(board->state_is_white?-CHECKERS_AI_FACTOR_MOVE_COUNT:CHECKERS_AI_FACTOR_MOVE_COUNT)*moves.count;
	if (!depth){
		return current_score;
	}
	float out=0.0f;
	if (board->is_white_turn){
		if (!moves.count){
			return CHECKERS_AI_SCORE_SUCCESS;
		}
		for (unsigned int i=0;i<moves.count;i++){
			checkers_board_t move_board=*board;
			_make_move(&move_board,moves.data+i);
			float score=_minmax(&move_board,depth-1,alpha,beta);
			if (!i||score<out){
				out=score;
				if (out>alpha){
					alpha=out;
					if (alpha>=beta){
						return beta;
					}
				}
			}
		}
	}
	else{
		if (!moves.count){
			return -CHECKERS_AI_SCORE_SUCCESS;
		}
		for (unsigned int i=0;i<moves.count;i++){
			checkers_board_t move_board=*board;
			_make_move(&move_board,moves.data+i);
			float score=_minmax(&move_board,depth-1,alpha,beta);
			if (!i||score>out){
				out=score;
				if (out<beta){
					beta=out;
					if (beta<=alpha){
						return alpha;
					}
				}
			}
		}
	}
	return out*CHECKERS_AI_FACTOR_ITERATION+current_score;
}



static void _add_move_to_history(checkers_move_history_t* history,_Bool is_white_turn,const checkers_board_move_t* move){
	if (!history){
		return;
	}
	if (is_white_turn){
		history->white_count++;
		history->white_moves=realloc(history->white_moves,history->white_count*sizeof(checkers_history_move_t));
		(history->white_moves+history->white_count-1)->from=move->points[0];
		(history->white_moves+history->white_count-1)->delete=(move->length>2?move->length>>1:0);
		(history->white_moves+history->white_count-1)->to=move->points[move->length-1];
	}
	else{
		history->black_count++;
		history->black_moves=realloc(history->black_moves,history->black_count*sizeof(checkers_history_move_t));
		(history->black_moves+history->black_count-1)->from=move->points[0];
		(history->black_moves+history->black_count-1)->delete=(move->length>2?move->length>>1:0);
		(history->black_moves+history->black_count-1)->to=move->points[move->length-1];
	}
}



void checkers_init_board(checkers_board_t* out,checkers_move_history_t* history){
	out->is_white_turn=1;
	out->score_white=0;
	out->score_black=0;
	out->state_is_white=0x03030303;
	out->state_is_black=0xc0c0c0c0;
	out->state_is_king=0x00000000;
	if (history){
		history->white_count=0;
		history->black_count=0;
		history->white_moves=NULL;
		history->black_moves=NULL;
	}
}



void checkers_free_history(checkers_move_history_t* history){
	history->white_count=0;
	history->black_count=0;
	free(history->white_moves);
	history->white_moves=NULL;
	free(history->black_moves);
	history->black_moves=NULL;
}



void checkers_get_raw_moves(const checkers_board_t* board,checkers_raw_board_moves_t* out){
	unsigned int current_down;
	unsigned int current_up;
	unsigned int opponent;
	if (board->is_white_turn){
		current_down=board->state_is_white;
		current_up=current_down&board->state_is_king;
		opponent=board->state_is_black;
	}
	else{
		current_up=board->state_is_black;
		current_down=current_up&board->state_is_king;
		opponent=board->state_is_white;
	}
	unsigned int empty=~(board->state_is_white|board->state_is_black);
	_get_capture_moves(current_down,current_up,opponent,empty,out);
	if (out->is_capture){
		out->down[0]=0;
		out->down[1]=0;
		out->up[0]=0;
		out->up[1]=0;
	}
	else{
		out->down[0]=current_down&(empty>>1)&0x7f7f7f7f;
		out->down[1]=current_down&(((empty<<7)&0x55555500)|((empty>>9)&0x002a2a2a));
		out->up[0]=current_up&(empty<<1)&0xfefefefe;
		out->up[1]=current_up&(((empty>>7)&0x00aaaaaa)|((empty<<9)&0x54545400));
	}
}



void checkers_get_moves(const checkers_board_t* board,const checkers_raw_board_moves_t* raw_moves,checkers_board_moves_t* out){
	out->count=0;
	checkers_board_move_t* move=out->data;
	if (!raw_moves->is_capture){
		unsigned int piece_mask=raw_moves->down[0];
		while (piece_mask){
			unsigned int i=FIND_FIRST_SET_BIT(piece_mask);
			move->length=2;
			move->points[0]=i;
			move->points[1]=i+1;
			move++;
			out->count++;
			piece_mask&=piece_mask-1;
		}
		piece_mask=raw_moves->down[1];
		while (piece_mask){
			unsigned int i=FIND_FIRST_SET_BIT(piece_mask);
			move->length=2;
			move->points[0]=i;
			move->points[1]=((i&1)?i+9:i-7);
			move++;
			out->count++;
			piece_mask&=piece_mask-1;
		}
		piece_mask=raw_moves->up[0];
		while (piece_mask){
			unsigned int i=FIND_FIRST_SET_BIT(piece_mask);
			move->length=2;
			move->points[0]=i;
			move->points[1]=i-1;
			move++;
			out->count++;
			piece_mask&=piece_mask-1;
		}
		piece_mask=raw_moves->up[1];
		while (piece_mask){
			unsigned int i=FIND_FIRST_SET_BIT(piece_mask);
			move->length=2;
			move->points[0]=i;
			move->points[1]=((i&1)?i+7:i-9);
			move++;
			out->count++;
			piece_mask&=piece_mask-1;
		}
		return;
	}
	reduced_board_state_t move_boards[CHECKERS_MAX_MOVE_COUNT];
	unsigned int piece_mask=raw_moves->down_capture[0];
	while (piece_mask){
		unsigned int i=FIND_FIRST_SET_BIT(piece_mask);
		move->length=3;
		move->points[0]=i;
		move->points[1]=((i&1)?i+1:i-7);
		move->points[2]=i-6;
		move++;
		out->count++;
		piece_mask&=piece_mask-1;
	}
	piece_mask=raw_moves->down_capture[1];
	while (piece_mask){
		unsigned int i=FIND_FIRST_SET_BIT(piece_mask);
		move->length=3;
		move->points[0]=i;
		move->points[1]=((i&1)?i+9:i+1);
		move->points[2]=i+10;
		move++;
		out->count++;
		piece_mask&=piece_mask-1;
	}
	piece_mask=raw_moves->up_capture[0];
	while (piece_mask){
		unsigned int i=FIND_FIRST_SET_BIT(piece_mask);
		move->length=3;
		move->points[0]=i;
		move->points[1]=((i&1)?i+7:i-1);
		move->points[2]=i+6;
		move++;
		out->count++;
		piece_mask&=piece_mask-1;
	}
	piece_mask=raw_moves->up_capture[1];
	while (piece_mask){
		unsigned int i=FIND_FIRST_SET_BIT(piece_mask);
		move->length=3;
		move->points[0]=i;
		move->points[1]=((i&1)?i-1:i-9);
		move->points[2]=i-10;
		move++;
		out->count++;
		piece_mask&=piece_mask-1;
	}
	checkers_raw_board_moves_t tmp;
	checkers_board_move_t* current_move;
	checkers_board_move_t* last_move=move;
	move=out->data;
	unsigned int current_init;
	unsigned int opponent_init;
	if (board->is_white_turn){
		current_init=board->state_is_white;
		opponent_init=board->state_is_black;
	}
	else{
		current_init=board->state_is_black;
		opponent_init=board->state_is_white;
	}
	for (unsigned int i=0;i<out->count;i++){
		move_boards[i].current=current_init;
		move_boards[i].opponent=opponent_init;
	}
	for (unsigned int i=0;i<out->count;){
		unsigned int last_move_mask=1<<move->points[move->length-1];
		move_boards[i].current=(move_boards[i].current&(~(1<<move->points[move->length-3])))|last_move_mask;
		move_boards[i].opponent&=~(1<<move->points[move->length-2]);
		_get_capture_moves(last_move_mask,last_move_mask,move_boards[i].opponent,~(move_boards[i].current|move_boards[i].opponent),&tmp);
		if (!tmp.is_capture){
			i++;
			move++;
			continue;
		}
		_Bool first=1;
		piece_mask=tmp.down_capture[0];
		while (piece_mask){
			unsigned int j=FIND_FIRST_SET_BIT(piece_mask);
			if (first){
				first=0;
				move->length+=2;
				current_move=move;
			}
			else{
				*last_move=*move;
				current_move=last_move;
				move_boards[out->count]=move_boards[i];
				out->count++;
				last_move++;
			}
			current_move->points[move->length-2]=((j&1)?j+1:j-7);
			current_move->points[move->length-1]=j-6;
			piece_mask&=piece_mask-1;
		}
		piece_mask=tmp.down_capture[1];
		while (piece_mask){
			unsigned int j=FIND_FIRST_SET_BIT(piece_mask);
			if (first){
				first=0;
				move->length+=2;
				current_move=move;
			}
			else{
				*last_move=*move;
				current_move=last_move;
				move_boards[out->count]=move_boards[i];
				out->count++;
				last_move++;
			}
			current_move->points[move->length-2]=((j&1)?j+9:j+1);
			current_move->points[move->length-1]=j+10;
			piece_mask&=piece_mask-1;
		}
		piece_mask=tmp.up_capture[0];
		while (piece_mask){
			unsigned int j=FIND_FIRST_SET_BIT(piece_mask);
			if (first){
				first=0;
				move->length+=2;
				current_move=move;
			}
			else{
				*last_move=*move;
				current_move=last_move;
				move_boards[out->count]=move_boards[i];
				out->count++;
				last_move++;
			}
			current_move->points[move->length-2]=((j&1)?j+7:j-1);
			current_move->points[move->length-1]=j+6;
			piece_mask&=piece_mask-1;
		}
		piece_mask=tmp.up_capture[1];
		while (piece_mask){
			unsigned int j=FIND_FIRST_SET_BIT(piece_mask);
			if (first){
				first=0;
				move->length+=2;
				current_move=move;
			}
			else{
				*last_move=*move;
				current_move=last_move;
				move_boards[out->count]=move_boards[i];
				out->count++;
				last_move++;
			}
			current_move->points[move->length-2]=((j&1)?j-1:j-9);
			current_move->points[move->length-1]=j-10;
			piece_mask&=piece_mask-1;
		}
	}
}



_Bool checkers_make_player_move(checkers_board_t* board,checkers_move_history_t* history){
	printf("\x1b[?25l");
	checkers_raw_board_moves_t raw_moves;
	checkers_get_raw_moves(board,&raw_moves);
	checkers_board_moves_t moves;
	checkers_get_moves(board,&raw_moves,&moves);
	if (!moves.count){
		return 0;
	}
	unsigned int selected_index=0;
	char command[4];
	while (1){
		printf("\x1b[0;0H\x1b[2J\x1b[48;2;40;40;40m\x1b[38;2;59;108;255m  A B C D E F G H   \x1b[0m\n");
		const checkers_board_move_t* move=moves.data+selected_index;
		for (unsigned int i=0;i<8;i++){
			printf("\x1b[48;2;40;40;40m\x1b[38;2;25;230;100m%c ",i+49);
			for (unsigned int j=0;j<8;j++){
				if ((i^j)&1){
					printf("\x1b[48;2;240;217;181m  ");
				}
				else{
					unsigned char idx=POS_FROM_XY(j,i);
					for (unsigned int k=0;k<move->length;k++){
						if (move->points[k]==idx){
							if (move->length>2&&(k&1)){
								printf("\x1b[48;2;211;140;159m");
							}
							else{
								printf("\x1b[48;2;102;115;197m");
							}
							goto _print_piece;
						}
					}
					printf("\x1b[48;2;181;136;99m");
_print_piece:
					unsigned int mask=1<<idx;
					if (board->state_is_white&mask){
						printf("\x1b[38;2;255;255;255m");
						if (board->state_is_king&mask){
							printf("K ");
						}
						else{
							printf("P ");
						}
					}
					else if (board->state_is_black&mask){
						printf("\x1b[38;2;0;0;0m");
						if (board->state_is_king&mask){
							printf("K ");
						}
						else{
							printf("P ");
						}
					}
					else{
						printf("  ");
					}
				}
			}
			printf("\x1b[48;2;40;40;40m  \x1b[0m");
			if (history){
				if (i==2){
					printf(" \x1b[38;2;255;255;255m%u\x1b[38;2;190;190;190m:",history->white_count);
					for (int j=history->white_count-1;j>=0&&j>=((int)history->white_count)-CHECKERS_HISTORY_MAX_PREVIEW_SIZE;j--){
						const checkers_history_move_t* history_move=history->white_moves+j;
						if (j<history->white_count-1){
							printf("\x1b[38;2;190;190;190m,");
						}
						if (history_move->delete){
							printf(" \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c \x1b[38;2;190;190;190m-\x1b[38;2;255;108;59m%u\x1b[38;2;190;190;190m-> \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c",POS_GET_X(history_move->from)+65,POS_GET_Y(history_move->from)+49,history_move->delete,POS_GET_X(history_move->to)+65,POS_GET_Y(history_move->to)+49);
						}
						else{
							printf(" \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c \x1b[38;2;190;190;190m-> \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c",POS_GET_X(history_move->from)+65,POS_GET_Y(history_move->from)+49,POS_GET_X(history_move->to)+65,POS_GET_Y(history_move->to)+49);
						}
					}
				}
				else if (i==5){
					printf(" \x1b[38;2;130;130;130m%u\x1b[38;2;190;190;190m:",history->black_count);
					for (int j=history->black_count-1;j>=0&&j>=((int)history->black_count)-CHECKERS_HISTORY_MAX_PREVIEW_SIZE;j--){
						const checkers_history_move_t* history_move=history->black_moves+j;
						if (j<history->black_count-1){
							printf("\x1b[38;2;190;190;190m,");
						}
						if (history_move->delete){
							printf(" \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c \x1b[38;2;190;190;190m-\x1b[38;2;255;108;59m%u\x1b[38;2;190;190;190m-> \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c",POS_GET_X(history_move->from)+65,POS_GET_Y(history_move->from)+49,history_move->delete,POS_GET_X(history_move->to)+65,POS_GET_Y(history_move->to)+49);
						}
						else{
							printf(" \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c \x1b[38;2;190;190;190m-> \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c",POS_GET_X(history_move->from)+65,POS_GET_Y(history_move->from)+49,POS_GET_X(history_move->to)+65,POS_GET_Y(history_move->to)+49);
						}
					}
				}
			}
			putchar('\n');
		}
		printf("\x1b[48;2;40;40;40m       \x1b[38;2;255;255;255m%2u  \x1b[38;2;130;130;130m%-2u       \x1b[0m\n",board->score_white,board->score_black);
		move=moves.data;
		for (unsigned int i=0;i<moves.count;i++){
			if (i==selected_index){
				printf("\x1b[38;2;190;190;190m> ");
				for (unsigned int j=0;j<move->length;j+=1+(move->length>2)){
					if (j){
						printf("\x1b[38;2;190;190;190m -> ");
					}
					printf("\x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c",POS_GET_X(move->points[j])+65,POS_GET_Y(move->points[j])+49);
				}
			}
			else{
				printf("  ");
				for (unsigned int j=0;j<move->length;j+=1+(move->length>2)){
					if (j){
						printf("\x1b[38;2;80;80;80m -> ");
					}
					printf("\x1b[38;2;33;60;143m%c\x1b[38;2;13;117;51m%c",POS_GET_X(move->points[j])+65,POS_GET_Y(move->points[j])+49);
				}
			}
			printf("\x1b[0m\n");
			move++;
		}
		int count=read(STDIN_FILENO,command,4);
		command[1]=(count>2?command[2]:0);
		switch (BUILD_COMMAND(command[0],command[1])){
			case BUILD_COMMAND(10,0):
				goto _execute_move;
			case BUILD_COMMAND(27,65):
				selected_index=(selected_index?selected_index:moves.count)-1;
				break;
			case BUILD_COMMAND(27,66):
				selected_index++;
				if (selected_index==moves.count){
					selected_index=0;
				}
				break;
		}
	}
_execute_move:
	printf("\x1b[0;0H\x1b[2J\x1b[?25h");
	_add_move_to_history(history,board->is_white_turn,moves.data+selected_index);
	_make_move(board,moves.data+selected_index);
	return 1;
}



_Bool checkers_make_ai_move(checkers_board_t* board,checkers_move_history_t* history){
	checkers_raw_board_moves_t raw_moves;
	checkers_get_raw_moves(board,&raw_moves);
	checkers_board_moves_t moves;
	checkers_get_moves(board,&raw_moves,&moves);
	if (!moves.count){
		return 0;
	}
	checkers_board_t adjusted_board=*board;
	if (board->is_white_turn){
		adjusted_board.is_white_turn=0;
		adjusted_board.score_white=board->score_black;
		adjusted_board.score_black=board->score_white;
		adjusted_board.state_is_white=board->state_is_black;
		adjusted_board.state_is_black=board->state_is_white;
	}
	float max_score=0.0f;
	unsigned int best_move_index=0;
	for (unsigned int i=0;i<moves.count;i++){
		checkers_board_t move_board=adjusted_board;
		_make_move(&move_board,moves.data+i);
		float score=_minmax(&move_board,CHECKERS_AI_MAX_DEPTH,-CHECKERS_AI_SCORE_SUCCESS*2,CHECKERS_AI_SCORE_SUCCESS*2);
		if (!i||score>max_score){
			max_score=score;
			best_move_index=i;
		}
	}
	_add_move_to_history(history,board->is_white_turn,moves.data+best_move_index);
	_make_move(board,moves.data+best_move_index);
	return 1;
}



void checkers_print_board(const checkers_board_t* board,const checkers_move_history_t* history){
	printf("\x1b[48;2;40;40;40m\x1b[38;2;59;108;255m  A B C D E F G H   \n");
	for (unsigned int i=0;i<8;i++){
		printf("\x1b[38;2;25;230;100m%c ",i+49);
		for (unsigned int j=0;j<8;j++){
			if ((i^j)&1){
				printf("\x1b[48;2;240;217;181m  ");
			}
			else{
				printf("\x1b[48;2;181;136;99m");
				unsigned int mask=1<<POS_FROM_XY(j,i);
				if (board->state_is_white&mask){
					printf("\x1b[38;2;255;255;255m");
					if (board->state_is_king&mask){
						printf("K ");
					}
					else{
						printf("P ");
					}
				}
				else if (board->state_is_black&mask){
					printf("\x1b[38;2;0;0;0m");
					if (board->state_is_king&mask){
						printf("K ");
					}
					else{
						printf("P ");
					}
				}
				else{
					printf("  ");
				}
			}
		}
		printf("\x1b[48;2;40;40;40m  \x1b[0m");
		if (history){
			if (i==2){
				printf(" \x1b[38;2;255;255;255m%u\x1b[38;2;190;190;190m:",history->white_count);
				for (int j=history->white_count-1;j>=0&&j>=((int)history->white_count)-CHECKERS_HISTORY_MAX_PREVIEW_SIZE;j--){
					const checkers_history_move_t* history_move=history->white_moves+j;
					if (j<history->white_count-1){
						printf("\x1b[38;2;190;190;190m,");
					}
					if (history_move->delete){
						printf(" \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c \x1b[38;2;190;190;190m-\x1b[38;2;255;108;59m%u\x1b[38;2;190;190;190m-> \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c",POS_GET_X(history_move->from)+65,POS_GET_Y(history_move->from)+49,history_move->delete,POS_GET_X(history_move->to)+65,POS_GET_Y(history_move->to)+49);
					}
					else{
						printf(" \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c \x1b[38;2;190;190;190m-> \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c",POS_GET_X(history_move->from)+65,POS_GET_Y(history_move->from)+49,POS_GET_X(history_move->to)+65,POS_GET_Y(history_move->to)+49);
					}
				}
			}
			else if (i==5){
				printf(" \x1b[38;2;130;130;130m%u\x1b[38;2;190;190;190m:",history->black_count);
				for (int j=history->black_count-1;j>=0&&j>=((int)history->black_count)-CHECKERS_HISTORY_MAX_PREVIEW_SIZE;j--){
					const checkers_history_move_t* history_move=history->black_moves+j;
					if (j<history->black_count-1){
						printf("\x1b[38;2;190;190;190m,");
					}
					if (history_move->delete){
						printf(" \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c \x1b[38;2;190;190;190m-\x1b[38;2;255;108;59m%u\x1b[38;2;190;190;190m-> \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c",POS_GET_X(history_move->from)+65,POS_GET_Y(history_move->from)+49,history_move->delete,POS_GET_X(history_move->to)+65,POS_GET_Y(history_move->to)+49);
					}
					else{
						printf(" \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c \x1b[38;2;190;190;190m-> \x1b[38;2;59;108;255m%c\x1b[38;2;25;230;100m%c",POS_GET_X(history_move->from)+65,POS_GET_Y(history_move->from)+49,POS_GET_X(history_move->to)+65,POS_GET_Y(history_move->to)+49);
					}
				}
			}
		}
		putchar('\n');
	}
	printf("       \x1b[38;2;255;255;255m%2u  \x1b[38;2;130;130;130m%-2u       \x1b[0m\n",board->score_white,board->score_black);
}



void checkers_print_moves(const checkers_board_moves_t* moves){
	printf("Moves: (%u)\n",moves->count);
	if (!moves->count){
		printf("  <no moves>\n");
		return;
	}
	const checkers_board_move_t* move=moves->data;
	for (unsigned int i=0;i<moves->count;i++){
		printf("  ");
		for (unsigned int j=0;j<move->length;j++){
			if (j){
				printf(" -> ");
			}
			printf("%c%c",POS_GET_X(move->points[j])+65,POS_GET_Y(move->points[j])+49);
		}
		putchar('\n');
		move++;
	}
}
