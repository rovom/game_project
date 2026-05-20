#include "minimap.h"

int init_minimap(Minimap *m, const char *path, int sw, int sh, int bw, int bh) {
    (void)sh;

    if (m == NULL) {
        return -1;
    }
    
    if (path == NULL) {
        return -1;
    }


    SDL_Surface *load_temp = NULL;
    load_temp = SDL_LoadBMP(path);
    
    if (load_temp == NULL) {
        return -1;
    }

    m->thumbnail = load_temp;

    int tW = 0;
    int tH = 0;
    tW = m->thumbnail->w;
    tH = m->thumbnail->h;

    float base_scale_x = 0.0f;
    float base_scale_y = 0.0f;

    if (bw > 0) {
        base_scale_x = (float)tW / (float)bw;
        m->scaleX = base_scale_x;
    } else {
        m->scaleX = 0.0000f;
    }

    if (bh > 0) {
        base_scale_y = (float)tH / (float)bh;
        m->scaleY = base_scale_y;
    } else {
        m->scaleY = 0.0000f;
    }

    int screen_w = sw;
    int map_w = tW;
    int map_h = tH;
    int margin = MINIMAP_MARGIN;


    int calc_x = 0;
    int calc_y = 0;
    
    calc_x = screen_w - map_w - margin;
    calc_y = margin;

    m->pos.x = calc_x;
    m->pos.y = calc_y;
    m->pos.w = map_w;
    m->pos.h = map_h;

    Uint32 rmask = 0;
    Uint32 gmask = 0;
    Uint32 bmask = 0;
    Uint32 amask = 0;

    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 4278190080u;
        gmask = 16711680u;
        bmask = 65280u;
        amask = 255u;
    #else
        rmask = 255u;
        gmask = 65280u;
        bmask = 16711680u;
        amask = 4278190080u;
    #endif

    int dot_size = PLAYER_DOT_SIZE;
    SDL_Surface *dot_surface = NULL;
    dot_surface = SDL_CreateRGBSurface(0, dot_size, dot_size, 32, rmask, gmask, bmask, amask);
    
    if (dot_surface != NULL) {
        m->man = dot_surface;
        Uint32 dot_color = 0;
        SDL_PixelFormat *fmt = m->man->format;
        dot_color = SDL_MapRGB(fmt, 255, 215, 0);
        SDL_FillRect(m->man, NULL, dot_color);
    }

    m->posMan.w = dot_size;
    m->posMan.h = dot_size;

    return 0;
}

void update_minimap(Minimap *m, SDL_Rect playerPos, SDL_Rect cam) {
    if (m == NULL) {
        return;
    }

    int p_x = playerPos.x;
    int p_y = playerPos.y;
    int c_x = cam.x;
    int c_y = cam.y;

    long world_x = 0;
    long world_y = 0;
    world_x = (long)p_x + (long)c_x;
    world_y = (long)p_y + (long)c_y;

    float s_x = m->scaleX;
    float s_y = m->scaleY;

    double horizontal_res = 0.0;
    double vertical_res = 0.0;
    horizontal_res = (double)world_x * (double)s_x;
    vertical_res = (double)world_y * (double)s_y;

    int map_internal_x = 0;
    int map_internal_y = 0;
    map_internal_x = (int)horizontal_res;
    map_internal_y = (int)vertical_res;

    int bounds_w = 0;
    int bounds_h = 0;
    bounds_w = m->pos.w - m->posMan.w;
    bounds_h = m->pos.h - m->posMan.h;


    if (map_internal_x < 0) {
        map_internal_x = 0;
    }
    
    if (map_internal_x > bounds_w) {
        map_internal_x = bounds_w;
    }

    if (map_internal_y < 0) {
        map_internal_y = 0;
    }
    
    if (map_internal_y > bounds_h) {
        map_internal_y = bounds_h;
    }

    int final_screen_x = 0;
    int final_screen_y = 0;
    int offset_x = m->pos.x;
    int offset_y = m->pos.y;

    final_screen_x = offset_x + map_internal_x;
    final_screen_y = offset_y + map_internal_y;

    m->posMan.x = final_screen_x;
    m->posMan.y = final_screen_y;
}

void display_minimap(Minimap m, SDL_Surface *dest) {
    if (dest == NULL) {
        return;
    }
    
    if (m.thumbnail == NULL) {
        return;
    }

    SDL_Rect outer_frame;
    int frame_offset = 5;
    outer_frame.x = m.pos.x - frame_offset;
    outer_frame.y = m.pos.y - frame_offset;
    
    outer_frame.w = m.pos.w + (frame_offset * 2);
    outer_frame.h = m.pos.h + (frame_offset * 2);
    
    Uint32 color_outer = 0;
    color_outer = SDL_MapRGB(dest->format, 45, 45, 48);
    SDL_FillRect(dest, &outer_frame, color_outer);

    SDL_Rect inner_frame;
    int inner_offset = 2;
    inner_frame.x = m.pos.x - inner_offset;
    inner_frame.y = m.pos.y - inner_offset;
    inner_frame.w = m.pos.w + (inner_offset * 2);
    inner_frame.h = m.pos.h + (inner_offset * 2);
    
    Uint32 color_inner = 0;
    color_inner = SDL_MapRGB(dest->format, 160, 110, 60);
    SDL_FillRect(dest, &inner_frame, color_inner);


    SDL_Surface *map_surf = m.thumbnail;
    SDL_Rect map_rect = m.pos;
    SDL_BlitSurface(map_surf, NULL, dest, &map_rect);

    if (m.man != NULL) {
        SDL_Surface *player_surf = m.man;
        SDL_Rect player_rect = m.posMan;
        SDL_BlitSurface(player_surf, NULL, dest, &player_rect);
    }
}

void display_entities_on_map(Minimap m, SDL_Surface *dest, Enemy *enemies) {
    if (dest == NULL) {
        return;
    }
    
    if (enemies == NULL) {
        return;
    }

    int idx = 0;
    int total = MAX_ENEMIES;

    while (idx < total) {
        Enemy current = enemies[idx];
        
        if (current.active == 1) {
            float enemy_world_x = (float)current.pos.x;
            float enemy_world_y = (float)current.pos.y;
            
            float sc_x = m.scaleX;
            float sc_y = m.scaleY;

            float entity_map_x = 0.0f;
            float entity_map_y = 0.0f;
            entity_map_x = enemy_world_x * sc_x;
            entity_map_y = enemy_world_y * sc_y;

            SDL_Rect entity_dot;
            entity_dot.w = 4;
            entity_dot.h = 4;
            
            int draw_x = 0;
            int draw_y = 0;
            draw_x = m.pos.x + (int)entity_map_x;
            draw_y = m.pos.y + (int)entity_map_y;

            entity_dot.x = draw_x;
            entity_dot.y = draw_y;

            int is_inside_x = 0;
            int is_inside_y = 0;

            if (entity_dot.x >= m.pos.x) {
                if (entity_dot.x <= (m.pos.x + m.pos.w)) {
                    is_inside_x = 1;
                }
            }
            
            if (entity_dot.y >= m.pos.y) {
                if (entity_dot.y <= (m.pos.y + m.pos.h)) {
                    is_inside_y = 1;
                }
            }

            if (is_inside_x == 1 && is_inside_y == 1) {
                Uint32 red_val = 0;
                red_val = SDL_MapRGB(dest->format, 255, 30, 30);
                SDL_FillRect(dest, &entity_dot, red_val);
            }
        }
        idx = idx + 1;
    }
}

int check_collision_aabb(SDL_Rect a, SDL_Rect b) {
    int x1_a = a.x;
    int x2_a = a.x + a.w;
    int y1_a = a.y;
    int y2_a = a.y + a.h;

    int x1_b = b.x;
    int x2_b = b.x + b.w;
    int y1_b = b.y;
    int y2_b = b.y + b.h;

    if (y2_a <= y1_b) {
        return 0;
    }
    
    if (y1_a >= y2_b) {
        return 0;
    }
    
    if (x2_a <= x1_b) {
        return 0;
    }
    
    if (x1_a >= x2_b) {
        return 0;
    }

    return 1;
}

int check_collision_pixel(SDL_Rect pj, SDL_Surface *mask, int direction) {
    if (mask == NULL) {
        return 0;
    }

    int target_x = 0;
    int target_y = 0;
    
    int obj_x = pj.x;
    int obj_y = pj.y;
    int obj_w = pj.w;
    int obj_h = pj.h;

    if (direction == 0) {
        target_x = obj_x + (obj_w / 2);
        target_y = obj_y;
        
    } else if (direction == 1) {
        target_x = obj_x + (obj_w / 2);
        target_y = obj_y + obj_h;
        
    } else if (direction == 2) {
        target_x = obj_x;
        target_y = obj_y + (obj_h / 2);
        
    } else if (direction == 3) {
        target_x = obj_x + obj_w;
        target_y = obj_y + (obj_h / 2);
        
    } else {
        target_x = obj_x + (obj_w / 2);
        target_y = obj_y + (obj_h / 2);
    }

    int mask_w = mask->w;
    int mask_h = mask->h;

    if (target_x < 0 || target_x >= mask_w) {
        return 0;
    }
    
    if (target_y < 0 || target_y >= mask_h) {
        return 0;
    }

    int bytes_per_pixel = 0;
    bytes_per_pixel = mask->format->BytesPerPixel;
    
    Uint8 *pixel_array = (Uint8 *)mask->pixels;
    int row_pitch = mask->pitch;
    
    Uint8 *target_pixel_ptr = NULL;
    target_pixel_ptr = pixel_array + (target_y * row_pitch) + (target_x * bytes_per_pixel);
    
    Uint32 pixel_raw_value = 0;

    if (bytes_per_pixel == 1) {
        pixel_raw_value = *target_pixel_ptr;
    } else if (bytes_per_pixel == 2) {
        pixel_raw_value = *(Uint16 *)target_pixel_ptr;
    } else if (bytes_per_pixel == 3) {
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            Uint32 p1 = target_pixel_ptr[0] << 16;
            Uint32 p2 = target_pixel_ptr[1] << 8;
            Uint32 p3 = target_pixel_ptr[2];
            pixel_raw_value = p1 | p2 | p3;
        } else {
            Uint32 p1 = target_pixel_ptr[0];
            Uint32 p2 = target_pixel_ptr[1] << 8;
            Uint32 p3 = target_pixel_ptr[2] << 16;
            pixel_raw_value = p1 | p2 | p3;
        }
    } else if (bytes_per_pixel == 4) {
        pixel_raw_value = *(Uint32 *)target_pixel_ptr;
    }

    Uint8 r_comp = 0;
    Uint8 g_comp = 0;
    Uint8 b_comp = 0;
    
    SDL_PixelFormat *m_fmt = mask->format;
    SDL_GetRGB(pixel_raw_value, m_fmt, &r_comp, &g_comp, &b_comp);

    int collision_detected = 0;
    
    if (r_comp == 0) {
        if (g_comp == 0) {
            if (b_comp == 0) {
                collision_detected = 1;
            }
        }
    }

    if (collision_detected == 1) {
        return 1;
    }

    return 0;
}
