#include "Show_DAT_Window.h"

#include "App_State.h"
#include "ImGui_Warning.h"
#include "Image2Texture.h"
#include "Save_Files.h"
#include "dat2/dat2_tree_view.h"
#include "dat2/dat2_writer.h"
#include "file_types/File_Type_Registry.h"
#include "imgui.h"
#include "platform_io.h"

#include <ImFileDialog.h>
#include <filesystem>

// ── DAT archive window ──────────────────────────────────────────────────

static void draw_dat2_tree_node(const Dat2TreeNode& node, dat_info* info) {
    for (const auto& child : node.children) {
        if (child.entry == nullptr) {
            // directory node
            if (ImGui::TreeNode(child.name.c_str())) {
                draw_dat2_tree_node(child, info);
                ImGui::TreePop();
            }
        } else {
            // file (leaf) node
            ImGui::TreeNodeEx(child.name.c_str(),
                              ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);

            // right-click context menu — must follow TreeNodeEx before other items
            if (ImGui::BeginPopupContextItem()) {
                if (dat2_entry_is_previewable(child.name.c_str())) {
                    if (ImGui::Selectable("Preview")) {
                        info->pending_preview = child.entry;
                    }
                }
                if (ImGui::Selectable("Export...")) {
                    info->pending_export = child.entry;
                    ifd::FileDialog::Instance().Save("DATExportDialog", "Export File",
                                                     "All files (*.*){.*}",
                                                     usr_info.default_save_path);
                    ifd::FileDialog::Instance().SetFilename(child.name.c_str());
                }
                ImGui::EndPopup();
            }

            char size_buf[32];
            format_file_size(size_buf, sizeof(size_buf), child.entry->decompressed_size);
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", size_buf);
        }
    }
}

void Show_DAT_Window(variables* My_Variables, LF* F_Prop, int slot_index, int* open_count) {
    dat_info* info = F_Prop->dat;
    if (info == nullptr) {
        return;
    }

    char name[MAX_PATH + 32];
    snprintf(name, sizeof(name), "%s###preview%02d", F_Prop->c_name, slot_index);

    if (ImGui::Begin(name, &F_Prop->file_open_window)) {
        ImGui::Text("Archive: %s", F_Prop->c_name);
        ImGui::Text("Files: %u", info->archive.file_count());

        ImGui::Checkbox("Compress", &info->repack_compress);
        ImGui::SameLine();
        if (ImGui::Button("Repack Archive...")) {
            info->pending_repack = true;
            init_IFD();
            ifd::FileDialog::Instance().Save("DATRepackDialog", "Save Repacked Archive",
                                             "DAT2 Archive (*.dat){.dat,.DAT}",
                                             usr_info.default_save_path);
        }
        ImGui::Separator();

        draw_dat2_tree_node(info->tree_root, info);
    }
    ImGui::End();

    // handle preview request — extract to temp file and open in a new window
    if (info->pending_preview != nullptr) {
        const dat2::Dat2Entry* entry = info->pending_preview;
        info->pending_preview = nullptr;

        auto result = info->archive.extract(*entry);
        if (!result.ok()) {
            char msg[512];
            snprintf(msg, sizeof(msg),
                     "[ERROR] Preview DAT entry\n\n"
                     "Failed to extract file:\n%s",
                     dat2::dat2_error_str(result.error));
            set_popup_warning(msg);
        } else {
            // write to temp file preserving original filename
            const char* basename = strrchr(entry->filename.c_str(), '\\');
            basename = (basename != nullptr) ? basename + 1 : entry->filename.c_str();

            char tmp_path[MAX_PATH];
            snprintf(tmp_path, sizeof(tmp_path), "%s%cdat2_preview_%s",
                     std::filesystem::temp_directory_path().u8string().c_str(), PLATFORM_SLASH,
                     basename);

            FILE* f = fopen(tmp_path, "wb");
            if (f != nullptr) {
                fwrite(result.value.data(), 1, result.value.size(), f);
                fclose(f);

                LF* new_slot = &My_Variables->F_Prop[*open_count];
                new_slot->file_open_window = File_Type_Check(new_slot, &My_Variables->shaders,
                                                             &new_slot->img_data, tmp_path);
                if (new_slot->file_open_window) {
                    (*open_count)++;
                }
            } else {
                set_popup_warning("[ERROR] Preview DAT entry\n\n"
                                  "Failed to write temp file for preview.");
            }
        }
    }

    // handle export save dialog
    if (ifd::FileDialog::Instance().IsDone("DATExportDialog")) {
        if (ifd::FileDialog::Instance().HasResult() && info->pending_export != nullptr) {
            std::string save_path = ifd::FileDialog::Instance().GetResult().u8string();
            auto result = info->archive.extract(*info->pending_export);
            if (result.ok()) {
                FILE* f = fopen(save_path.c_str(), "wb");
                if (f != nullptr) {
                    fwrite(result.value.data(), 1, result.value.size(), f);
                    fclose(f);
                } else {
                    set_popup_warning("[ERROR] Export DAT entry\n\n"
                                      "Failed to write exported file.");
                }
            } else {
                char msg[512];
                snprintf(msg, sizeof(msg),
                         "[ERROR] Export DAT entry\n\n"
                         "Failed to extract file:\n%s",
                         dat2::dat2_error_str(result.error));
                set_popup_warning(msg);
            }
        }
        info->pending_export = nullptr;
        ifd::FileDialog::Instance().Close();
    }

    // handle repack save dialog
    if (ifd::FileDialog::Instance().IsDone("DATRepackDialog")) {
        if (ifd::FileDialog::Instance().HasResult() && info->pending_repack) {
            std::string save_path = ifd::FileDialog::Instance().GetResult().u8string();
            auto collected = dat2::collect_from_archive(info->archive);
            if (collected.ok()) {
                dat2::Dat2WriteOptions opts;
                opts.compress = info->repack_compress;
                auto ws = dat2::write_archive(save_path.c_str(), collected.value, opts);
                if (!ws.ok()) {
                    char msg[512];
                    snprintf(msg, sizeof(msg),
                             "[ERROR] Repack DAT2 Archive\n\n"
                             "Failed to write archive:\n%s",
                             dat2::dat2_error_str(ws.error));
                    set_popup_warning(msg);
                }
            } else {
                char msg[512];
                snprintf(msg, sizeof(msg),
                         "[ERROR] Repack DAT2 Archive\n\n"
                         "Failed to extract entries:\n%s",
                         dat2::dat2_error_str(collected.error));
                set_popup_warning(msg);
            }
        }
        info->pending_repack = false;
        ifd::FileDialog::Instance().Close();
    }

    if (!F_Prop->file_open_window) {
        delete F_Prop->dat;
        F_Prop->dat = nullptr;
    }
}
