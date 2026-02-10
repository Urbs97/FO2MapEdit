#pragma once

struct variables;
struct user_info;
struct Palette;

void ShowMainMenuBar(int* counter, struct variables* My_Variables);
void Open_Files(struct user_info* usr_info, int* counter, Palette* pxlFMT,
                struct variables* My_Variables);
void main_window_bttns(variables* My_Variables, int* counter);
