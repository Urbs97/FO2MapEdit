#include "display_FRM_OpenGL.h"

#include "Load_Files.h"

constexpr uint16_t ms_PER_sec = 1000;

mesh load_giant_triangle() {
    float vertices[] = {// giant triangle     uv coordinates?
                        -1.0F, -1.0F, 0.0F,  0.0F, 0.0F, 3.0F, -1.0F, 0.0F,
                        2.0F,  0.0F,  -1.0F, 3.0F, 0.0F, 0.0F, 2.0F};

    mesh triangle;
    triangle.vertexCount = 3;

    glGenVertexArrays(1, &triangle.VAO);
    glBindVertexArray(triangle.VAO);

    glGenBuffers(1, &triangle.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, triangle.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return triangle;
}

// TODO: this is still being used for regular images, refactor
void render_OTHER_OpenGL(image_data* img_data, int width, int height) {
    int orient = img_data->display_orient_num; //(img_data->ANIM_hdr->Frame_0_Offset[1] > 0) ?
                                               // img_data->display_orient_num : 0;
    int frame_num = img_data->display_frame_num;
    ANM_Dir* anm_dir = img_data->ANM_dir;

    uint8_t* pxls = nullptr;
    if (anm_dir[orient].frame_data[frame_num] == nullptr) {
        width = 0;
        height = 0;
        pxls = nullptr;
    } else {
        pxls = anm_dir[orient].frame_data[frame_num]->pxls;
    }
    // Change alignment with glPixelStorei() (this change is global/permanent until changed back)
    // FRM's are aligned to 1-byte, SDL_Surfaces are typically 4-byte
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    // bind data to FRM_texture for display
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pxls);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

// TODO: this is still being used, maybe refactor?
void animate_OTHER_to_framebuff(Shader* shader, mesh* triangle, image_data* img_data,
                                uint64_t current_time) {
    float constexpr static playback_speeds[5] = {0.0F, .25f, 0.5F, 1.0F, 2.0F};

    float fps = 10 * playback_speeds[img_data->playback_speed];

    int dir = img_data->display_orient_num;
    int num = img_data->display_frame_num;
    ANM_Dir* anm_dir = img_data->ANM_dir;

    int img_width = 0;
    int img_height = 0;
    // TODO: probably want to make the image display
    //       more flexible, manage the case where
    //       frame_data[num] doesn't have a surface
    //       but we can add one with a button if we
    //       want, also can add a "num" entry at the
    //       end (or possibly in between frames?)
    if (anm_dir[dir].frame_data[num] != nullptr) {
        img_width = img_data->ANM_dir[dir].frame_data[num]->w;
        img_height = img_data->ANM_dir[dir].frame_data[num]->h;
    } else {
        // TODO: for now this assigns frame[0]'s w/h
        //       to force glViewport() to be full image size
        //       for clearing, in case current frame is NULL
        img_width = img_data->ANM_dir[dir].frame_data[0]->w;
        img_height = img_data->ANM_dir[dir].frame_data[0]->h;
    }

    glViewport(0, 0, img_width, img_height);
    glBindFramebuffer(GL_FRAMEBUFFER, img_data->framebuffer);
    glBindVertexArray(triangle->VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, img_data->FRM_texture);

    static uint64_t last_time = 0;
    static int last_frame;
    static int last_orient;

    if ((fps != 0) && ((float)(current_time - last_time) / ms_PER_sec > 1 / fps)) {
        last_time = current_time;

        img_data->display_frame_num += 1;
        if (img_data->display_frame_num >= img_data->ANM_dir[dir].num_frames) {
            img_data->display_frame_num = 0;
        }
        render_OTHER_OpenGL(img_data, img_width, img_height);
    } else if ((last_frame != img_data->display_frame_num) ||
               (last_orient != img_data->display_orient_num)) {
        last_frame = img_data->display_frame_num;
        last_orient = img_data->display_orient_num;
        render_OTHER_OpenGL(img_data, img_width, img_height);
    }

    // shader
    shader->use();
    // draw image to framebuffer
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(triangle->vertexCount));

    // bind framebuffer back to default
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SURFACE_to_texture(Surface* src, GLuint texture, int width, int height, int alignment) {
    if (src == nullptr) {
        return;
    }

    // //print surface out as ascii hex
    // for (int i = 0; i < src->h; i++) {
    //     for (int j = 0; j < src->w; j++) {
    //         printf("%02x", src->pxls[i*src->pitch + j]);
    //     }
    //     printf("\n");
    // }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    // Change alignment with glPixelStorei() (this change is global/permanent until changed back)
    // MSK & FRM are aligned to 1-byte
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

    int pxl_type = GL_RED;
    if (alignment == 3) {
        pxl_type = GL_RGB;
    } else if (alignment == 4) {
        pxl_type = GL_RGBA;
    }

    // If the surface is smaller than the requested texture size (e.g. animation frame
    // vs bounding box), allocate empty texture then upload just the surface portion.
    if (src->w == width && src->h == height) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, pxl_type, GL_UNSIGNED_BYTE,
                     src->pxls);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, pxl_type, GL_UNSIGNED_BYTE,
                     nullptr);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, src->w, src->h, pxl_type, GL_UNSIGNED_BYTE,
                        src->pxls);
    }
}

void PAL_SURFACE_to_sub_texture(uint8_t* pxls, GLuint texture, int x_offset, int y_offset,
                                int frm_width, int frm_height, int total_width, int total_height) {
    GLuint alignment = 1;
    int pxl_type = GL_RED;
    // if (type == OTHER) {
    //     alignment = 4;
    //     pxl_type = GL_RGBA;
    // }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    // Change alignment with glPixelStorei() (this change is global/permanent until changed back)
    // FRM/MSK are aligned to 1-byte
    glPixelStorei(GL_UNPACK_ALIGNMENT, static_cast<GLint>(alignment));
    // bind blank background to FRM_texture for display, then paint data onto texture
    // use static buffer to avoid calloc/free every frame
    static uint8_t* blank = nullptr;
    static int blank_size = 0;
    int needed = total_width * total_height;
    if (needed > blank_size) {
        free(blank);
        blank = (uint8_t*)calloc(1, needed);
        if (blank == nullptr) {
            blank_size = 0;
            return;
        }
        blank_size = needed;
    } else if (blank != nullptr) {
        memset(blank, 0, needed);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, total_width, total_height, 0, pxl_type, GL_UNSIGNED_BYTE,
                 blank);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x_offset, y_offset, frm_width, frm_height, GL_RED,
                    GL_UNSIGNED_BYTE, pxls);
}

// TODO: handle the current_time break outside this code
//       to prevent this from being called when not directly
//       changing either a frame or a palette color-cycle
void animate_SURFACE_to_sub_texture(image_data* img_data, Surface* edit_srfc,
                                    uint64_t current_time) {
    if (edit_srfc == nullptr) {
        return;
    }
    // TODO: maybe handle single image FRM's slightly differently with dropdown?
    // int orient = (img_data->FRM_hdr->Frame_0_Offset[1] > 0) ? img_data->display_orient_num : 0;
    int frame_num = img_data->display_frame_num;
    int dir = img_data->display_orient_num;

    int total_w = img_data->ANM_bounding_box[dir].x2 - img_data->ANM_bounding_box[dir].x1;
    int total_h = img_data->ANM_bounding_box[dir].y2 - img_data->ANM_bounding_box[dir].y1;

    // int frame_w = img_data->ANM_dir[dir].frame_data[frame_num]->w;
    // int frame_h = img_data->ANM_dir[dir].frame_data[frame_num]->h;
    int frame_w = edit_srfc->w;
    int frame_h = edit_srfc->h;

    int x_offset =
        img_data->ANM_dir[dir].frame_box[frame_num].x1 - img_data->ANM_bounding_box[dir].x1;
    int y_offset =
        img_data->ANM_dir[dir].frame_box[frame_num].y1 - img_data->ANM_bounding_box[dir].y1;

    float constexpr static playback_speeds[5] = {0.0F, .25f, 0.5F, 1.0F, 2.0F};
    int FRM_fps = (img_data->FRM_hdr->FPS == 0 && img_data->ANM_dir[dir].num_frames > 1)
                      ? 10
                      : img_data->FRM_hdr->FPS;
    float fps = static_cast<float>(FRM_fps) * playback_speeds[img_data->playback_speed];

    static uint64_t last_time = 0;
    if ((fps != 0) && ((float)(current_time - last_time) / ms_PER_sec > 1 / fps)) {
        last_time = current_time;

        img_data->display_frame_num += 1;
        if (img_data->display_frame_num >= img_data->FRM_hdr->Frames_Per_Orient) {
            img_data->display_frame_num = 0;
        }
    }
    PAL_SURFACE_to_sub_texture(edit_srfc->pxls, img_data->FRM_texture, x_offset, y_offset, frame_w,
                               frame_h, total_w, total_h);
}

// TODO: delete? replaced by draw_PAL_to_framebuffer()?
void draw_FRM_to_framebuffer(shader_info* shader_i, int width, int height, GLuint framebuffer,
                             GLuint texture) {
    glViewport(0, 0, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glBindVertexArray(shader_i->giant_triangle.VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    // shader
    shader_i->render_FRM_shader->use();
    GLint t = glGetUniformLocation(shader_i->render_FRM_shader->ID, "ColorPaletteUINT");
    glUniform1uiv(t, 256, (GLuint*)shader_i->FO_pal->colors);

    shader_i->render_FRM_shader->setInt("Indexed_FRM", 0);

    int err = glGetError();
    if (err != 0) {
        printf("draw_FRM_to_framebuffer() glGetError: %d\n", err);
    }
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(shader_i->giant_triangle.vertexCount));

    // bind framebuffer back to default
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void draw_PAL_to_framebuffer(Palette* pal, Shader* shader, mesh* triangle,
                             struct image_data* img_data, OverlayLayer* overlays,
                             int overlay_count) {
    glViewport(0, 0, img_data->width, img_data->height);

    // bind framebuffer to draw to
    glBindFramebuffer(GL_FRAMEBUFFER, img_data->framebuffer);

    glBindVertexArray(triangle->VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, img_data->FRM_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, img_data->PAL_texture);

    // Bind overlay textures to units 2..2+N-1
    static const char* overlay_names[MAX_OVERLAY_LAYERS] = {
        "Overlay0", "Overlay1", "Overlay2", "Overlay3",
        "Overlay4", "Overlay5", "Overlay6", "Overlay7",
    };
    int blend_arr[MAX_OVERLAY_LAYERS] = {};
    float color_arr[MAX_OVERLAY_LAYERS * 4] = {};

    for (int i = 0; i < overlay_count && i < MAX_OVERLAY_LAYERS; i++) {
        glActiveTexture(GL_TEXTURE2 + i);
        glBindTexture(GL_TEXTURE_2D, overlays[i].texture);
        blend_arr[i] = static_cast<int>(overlays[i].blend);
        color_arr[(i * 4) + 0] = overlays[i].color[0];
        color_arr[(i * 4) + 1] = overlays[i].color[1];
        color_arr[(i * 4) + 2] = overlays[i].color[2];
        color_arr[(i * 4) + 3] = overlays[i].color[3];
    }

    // shader
    shader->use();
    uint32_t ID = shader->ID;
    glUniform1uiv(glGetUniformLocation(ID, "ColorPaletteUINT"), 256, (GLuint*)pal->colors);

    shader->setInt("Indexed_FRM", 0);
    shader->setInt("Indexed_PAL", 1);

    // Set overlay sampler units
    for (int i = 0; i < MAX_OVERLAY_LAYERS; i++) {
        glUniform1i(glGetUniformLocation(ID, overlay_names[i]), 2 + i);
    }

    // Set overlay uniforms
    shader->setInt("overlay_count", overlay_count);
    glUniform1iv(glGetUniformLocation(ID, "overlay_blend"), MAX_OVERLAY_LAYERS, blend_arr);
    glUniform4fv(glGetUniformLocation(ID, "overlay_color"), MAX_OVERLAY_LAYERS, color_arr);

    int err = glGetError();
    if (err != 0) {
        printf("draw_PAL_to_framebuffer() glGetError: %d\n", err);
    }

    // draw to framebuffer
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(triangle->vertexCount));

    // bind framebuffer back to default
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// TODO: this should be replaced by draw_PAL_to_framebuffer()?
void draw_texture_to_framebuffer(Palette* pal, Shader* shader, mesh* triangle, GLuint framebuffer,
                                 GLuint texture, int w, int h) {
    glViewport(0, 0, w, h);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glBindVertexArray(triangle->VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    shader->use();

    GLint t = glGetUniformLocation(shader->ID, "ColorPaletteUINT");
    glUniform1uiv(t, 256, (GLuint*)pal->colors);

    GLenum err = glGetError();
    if (err != 0U) {
        printf("draw_texture_to_framebuffer() glGetError: %d\n", err);
        // GL_INVALID_OPERATION
    }

    shader->setInt("Indexed_FRM", 0);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(triangle->vertexCount));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
