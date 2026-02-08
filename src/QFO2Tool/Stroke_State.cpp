#include "Stroke_State.h"

#include <string.h>

void stroke_begin(StrokeState* state, Surface* surface) {
    if (state->stroke_active) {
        return;
    }
    state->pre_stroke_snapshot = Copy8BitSurface(surface);
    state->pre_stroke_target = surface;
    state->stroke_active = true;
}

void stroke_commit(StrokeState* state) {
    if (!state->stroke_active) {
        return;
    }

    // Push the pre-stroke snapshot onto the undo stack
    UndoEntry entry;
    entry.snapshot = state->pre_stroke_snapshot;
    entry.target = state->pre_stroke_target;

    if ((int)state->undo_stack.size() >= UNDO_STACK_MAX) {
        // Remove oldest entry
        FreeSurface(state->undo_stack[0].snapshot);
        state->undo_stack.erase(state->undo_stack.begin());
    }
    state->undo_stack.push_back(entry);

    // New stroke invalidates redo history
    for (size_t i = 0; i < state->redo_stack.size(); i++) {
        FreeSurface(state->redo_stack[i].snapshot);
    }
    state->redo_stack.clear();

    state->pre_stroke_snapshot = nullptr;
    state->pre_stroke_target = nullptr;
    state->stroke_active = false;
}

void stroke_cancel(StrokeState* state) {
    if (!state->stroke_active) {
        return;
    }
    // Restore pixels from snapshot
    if (state->pre_stroke_snapshot && state->pre_stroke_target) {
        memcpy(state->pre_stroke_target->pxls, state->pre_stroke_snapshot->pxls,
               state->pre_stroke_target->w * state->pre_stroke_target->h);
        FreeSurface(state->pre_stroke_snapshot);
    }
    state->pre_stroke_snapshot = nullptr;
    state->pre_stroke_target = nullptr;
    state->stroke_active = false;
}

bool stroke_undo(StrokeState* state) {
    if (state->stroke_active || state->undo_stack.empty()) {
        return false;
    }
    UndoEntry entry = state->undo_stack.back();
    state->undo_stack.pop_back();

    // Save current state to redo stack before restoring
    UndoEntry redo_entry;
    redo_entry.snapshot = Copy8BitSurface(entry.target);
    redo_entry.target = entry.target;
    state->redo_stack.push_back(redo_entry);

    // Restore pixels to the target surface
    memcpy(entry.target->pxls, entry.snapshot->pxls, entry.target->w * entry.target->h);

    FreeSurface(entry.snapshot);
    return true;
}

bool stroke_redo(StrokeState* state) {
    if (state->stroke_active || state->redo_stack.empty()) {
        return false;
    }
    UndoEntry entry = state->redo_stack.back();
    state->redo_stack.pop_back();

    // Save current state to undo stack before restoring
    UndoEntry undo_entry;
    undo_entry.snapshot = Copy8BitSurface(entry.target);
    undo_entry.target = entry.target;
    if ((int)state->undo_stack.size() >= UNDO_STACK_MAX) {
        FreeSurface(state->undo_stack[0].snapshot);
        state->undo_stack.erase(state->undo_stack.begin());
    }
    state->undo_stack.push_back(undo_entry);

    // Restore pixels to the target surface
    memcpy(entry.target->pxls, entry.snapshot->pxls, entry.target->w * entry.target->h);

    FreeSurface(entry.snapshot);
    return true;
}

void stroke_state_cleanup(StrokeState* state) {
    if (state->pre_stroke_snapshot) {
        FreeSurface(state->pre_stroke_snapshot);
        state->pre_stroke_snapshot = nullptr;
    }
    state->pre_stroke_target = nullptr;
    state->stroke_active = false;

    for (size_t i = 0; i < state->undo_stack.size(); i++) {
        FreeSurface(state->undo_stack[i].snapshot);
    }
    state->undo_stack.clear();

    for (size_t i = 0; i < state->redo_stack.size(); i++) {
        FreeSurface(state->redo_stack[i].snapshot);
    }
    state->redo_stack.clear();

    state->cursor_visible = false;
}
