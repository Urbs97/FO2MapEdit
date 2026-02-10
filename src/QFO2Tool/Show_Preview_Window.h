#pragma once

struct variables;
struct LF;
struct user_info;

void Show_Preview_Window(struct variables* My_Variables, LF* F_Prop, int counter);
void Preview_Tiles_Window(variables* My_Variables, LF* F_Prop, int counter);
void Show_Image_Render(variables* My_Variables, LF* F_Prop, struct user_info* usr_info,
                       int counter);
