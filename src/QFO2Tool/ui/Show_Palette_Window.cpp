#include "Show_Palette_Window.h"

#include "../City_Layer.h"
#include "../Image2Texture.h"
#include "../Maps_Txt.h"
#include "Edit_Image.h"
#include "imgui.h"

#include <cmath>

static ImGuiID g_palette_dock_id = 0;

void Show_Palette_Window(variables* My_Variables, LayerType palette_layer) {

    Palette* pal = My_Variables->shaders.FO_pal;

    // Bring palette to foreground when the active layer changes
    {
        static int prev_focus = -1;
        static int prev_layer = -1;
        int cur_focus = My_Variables->window_number_focus;
        int cur_layer = -1;
        if (cur_focus > -1) {
            cur_layer = My_Variables->F_Prop[cur_focus].active_layer;
        }
        if (cur_focus != prev_focus || cur_layer != prev_layer) {
            ImGui::SetNextWindowFocus();
        }
        prev_focus = cur_focus;
        prev_layer = cur_layer;
    }

    bool palette_window = true;
    if (palette_layer == LayerType::MSK) {
        ImGui::Begin("MSK colors###palette", &palette_window);

        brush_size_handler(My_Variables);

        ImGui::Text("Erase Mask                    Draw Mask");
        if (ImGui::ColorButton("Erase Mask", ImVec4(0, 0, 0, 1.0F), 0, ImVec2(200.0F, 200.0F))) {
            My_Variables->Color_Pick = (0);
        }
        ImGui::SameLine();
        if (ImGui::ColorButton("Mark Mask", ImVec4(1.0F, 1.0F, 1.0F, 1.0F), 0,
                               ImVec2(200.0F, 200.0F))) {
            My_Variables->Color_Pick = (1);
        }
    } else if (palette_layer == LayerType::CITY) {
        ImGui::Begin("City tools###palette", &palette_window);

        // Find city_layer_data for the focused window
        city_layer_data* city_data = nullptr;
        int focus = My_Variables->window_number_focus;
        if (focus > -1) {
            LF* focused = &My_Variables->F_Prop[focus];
            int al = focused->active_layer;
            if (al >= 0 && al < focused->img_data.overlay_count &&
                focused->img_data.overlay[al].type == LayerType::CITY) {
                city_data = (city_layer_data*)focused->img_data.overlay[al].source_data;
            }
        }

        if (city_data != nullptr) {
            CityBrush brush = city_data->active_brush;

            struct ToolEntry {
                const char* label{};
                CityBrush value{};
                ImVec4 color;
            };
            ToolEntry tools[] = {
                {"Select", CityBrush::SELECT, ImVec4(0.5F, 0.5F, 0.5F, 1.0F)},
                {"Small City", CityBrush::PLACE_SMALL, ImVec4(0.0F, 0.7F, 0.0F, 1.0F)},
                {"Medium City", CityBrush::PLACE_MEDIUM, ImVec4(0.0F, 0.85F, 0.0F, 1.0F)},
                {"Large City", CityBrush::PLACE_LARGE, ImVec4(0.0F, 1.0F, 0.0F, 1.0F)},
                {"Eraser", CityBrush::ERASER, ImVec4(0.9F, 0.2F, 0.2F, 1.0F)},
            };

            for (int i = 0; i < 5; i++) {
                ImGui::PushID(i);
                bool is_active = (brush == tools[i].value);

                if (is_active) {
                    ImVec4 border_col = ImVec4(1.0F, 1.0F, 0.0F, 1.0F);
                    ImGui::PushStyleColor(ImGuiCol_Border, border_col);
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0F);
                }

                if (ImGui::ColorButton(tools[i].label, tools[i].color, 0, ImVec2(80.0F, 40.0F))) {
                    city_data->active_brush = tools[i].value;
                }

                if (is_active) {
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor();
                }

                ImGui::SameLine();
                ImGui::Text("%s", tools[i].label);
                ImGui::PopID();
            }
        } else {
            ImGui::TextDisabled("No city layer active");
        }
    } else {
        ImGui::Begin("Default Fallout palette###palette", &palette_window);

        brush_size_handler(My_Variables);

        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {

                int index = (y * 16) + x;

                float r = static_cast<float>(pal->colors[index].r) / 255.0F;
                float g = static_cast<float>(pal->colors[index].g) / 255.0F;
                float b = static_cast<float>(pal->colors[index].b) / 255.0F;

                // give the first button an alpha channel checkerboard
                // TODO: if load_palette_from_path() is changed to use
                //       the first index as alpha = 0 always, then
                //       comment int "float a =" above and delete
                //       the below alpha switch
                float alpha = NAN;
                if (x == 0 && y == 0) {
                    alpha = 0.0;
                } else {
                    alpha = 1.0;
                }

                char color_info[12];
                snprintf(color_info, 12, "%d##aa%d", index, index);
                if (ImGui::ColorButton(color_info, ImVec4(r, g, b, alpha),
                                       ImGuiColorEditFlags_AlphaPreview)) {
                    My_Variables->Color_Pick = (uint8_t)(index);
                }

                if (index == My_Variables->Color_Pick) {
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    draw_list->AddRect(min, max, IM_COL32(0, 0, 0, 255), 0.0F, 0, 2.0F);
                    draw_list->AddRect(min, max, IM_COL32(255, 255, 255, 255), 0.0F, 0, 1.0F);
                }

                if (x < 15) {
                    ImGui::SameLine();
                }
            }
        }
    }

    g_palette_dock_id = ImGui::GetWindowDockID();
    ImGui::End();
}

void Show_City_Info_Window(variables* My_Variables) {
    static int prev_selected = -1;
    OverlayLayer* city_layer = nullptr;
    city_layer_data* city_data = nullptr;
    LF* focused = nullptr;

    if (My_Variables->window_number_focus > -1) {
        focused = &My_Variables->F_Prop[My_Variables->window_number_focus];
        int city_idx = find_overlay(focused->img_data.overlay, focused->img_data.overlay_count,
                                    LayerType::CITY);
        if (city_idx >= 0) {
            city_layer = &focused->img_data.overlay[city_idx];
            city_data = (city_layer_data*)city_layer->source_data;
        }
    }

    bool has_selected_city = (city_data != nullptr && city_data->selected_area >= 0 &&
                              city_data->selected_area < city_data->area_count);

    if (!has_selected_city) {
        prev_selected = -1;
        return;
    }

    // Look up MAPS overlay for linked map data
    maps_txt_data* maps_data = nullptr;
    if (focused != nullptr) {
        int maps_idx = find_overlay(focused->img_data.overlay, focused->img_data.overlay_count,
                                    LayerType::MAPS);
        if (maps_idx >= 0) {
            maps_data = (maps_txt_data*)focused->img_data.overlay[maps_idx].source_data;
        }
    }

    // Dock into the same node as the palette when the window first appears
    if (g_palette_dock_id != 0) {
        ImGui::SetNextWindowDockID(g_palette_dock_id, ImGuiCond_Appearing);
    }

    bool editing = focused->editing_enabled && focused->active_layer >= 0 &&
                   focused->img_data.overlay[focused->active_layer].type == LayerType::CITY;

    // Auto-focus the window when a new city is selected or editing starts
    static bool prev_editing = false;
    if (city_data->selected_area != prev_selected || (editing && !prev_editing)) {
        ImGui::SetNextWindowFocus();
    }
    prev_selected = city_data->selected_area;
    prev_editing = editing;

    bool open = true;
    ImGui::Begin("City Info", &open);
    if (draw_city_info_panel(city_layer, editing, maps_data)) {
        focused->dirty = true;
    }
    ImGui::End();
}
