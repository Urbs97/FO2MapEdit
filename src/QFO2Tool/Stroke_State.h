#pragma once
#include "MiniSDL.h"
#include "imgui.h"

#include <stdint.h>
#include <vector>

static const int UNDO_STACK_MAX = 50;

struct UndoEntry {
    Surface* snapshot;
    Surface* target;
};

struct StrokeState {
    bool stroke_active = false;
    Surface* pre_stroke_snapshot = nullptr;
    Surface* pre_stroke_target = nullptr;

    std::vector<UndoEntry> undo_stack;
    std::vector<UndoEntry> redo_stack;

    // Brush cursor screen-space rectangle (set every frame when hovering)
    bool cursor_visible = false;
    ImVec2 cursor_min;
    ImVec2 cursor_max;
};

void stroke_begin(StrokeState* state, Surface* surface);
void stroke_commit(StrokeState* state);
void stroke_cancel(StrokeState* state);
bool stroke_undo(StrokeState* state);
bool stroke_redo(StrokeState* state);
void stroke_state_cleanup(StrokeState* state);
