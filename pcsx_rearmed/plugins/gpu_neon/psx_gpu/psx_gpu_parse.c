/*
 * Copyright (C) 2011 Gilead Kutnick "Exophase" <exophase@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <stdio.h>

#include "common.h"

#ifndef command_lengths
const u8 command_lengths[256] =
{
	0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 00
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 10
	3,  3,  3,  3,  6,  6,  6,  6,  4,  4,  4,  4,  8,  8,  8,  8,   // 20
	5,  5,  5,  5,  8,  8,  8,  8,  7,  7,  7,  7,  11, 11, 11, 11,  // 30
	2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,   // 40
	3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,   // 50
	2,  2,  2,  2,  3,  3,  3,  3,  1,  1,  1,  1,  0,  0,  0,  0,   // 60
	1,  1,  1,  1,  2,  2,  2,  2,  1,  1,  1,  1,  2,  2,  2,  2,   // 70
	3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 80
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 90
	2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // a0
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // b0
	2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // c0
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // d0
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // e0
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0    // f0
};
#endif

void update_texture_ptr(psx_gpu_struct *psx_gpu)
{
  u8 *texture_base;
  u8 *texture_ptr;

  switch((psx_gpu->render_state_base >> 8) & 0x3)
  {
    case TEXTURE_MODE_4BPP:
      texture_base = psx_gpu->texture_4bpp_cache[psx_gpu->current_texture_page];

      texture_ptr = texture_base;
      texture_ptr += psx_gpu->texture_window_x & 0xF;
      texture_ptr += (psx_gpu->texture_window_y & 0xF) << 4;
      texture_ptr += (psx_gpu->texture_window_x >> 4) << 8;
      texture_ptr += (psx_gpu->texture_window_y >> 4) << 12;
      break;

    case TEXTURE_MODE_8BPP:
      if(psx_gpu->current_texture_page & 0x1)
      {
        texture_base =
         psx_gpu->texture_8bpp_odd_cache[psx_gpu->current_texture_page >> 1];
      }
      else
      {
        texture_base =
         psx_gpu->texture_8bpp_even_cache[psx_gpu->current_texture_page >> 1];
      }
      
      texture_ptr = texture_base;
      texture_ptr += psx_gpu->texture_window_x & 0xF;
      texture_ptr += (psx_gpu->texture_window_y & 0xF) << 4;
      texture_ptr += (psx_gpu->texture_window_x >> 4) << 8;
      texture_ptr += (psx_gpu->texture_window_y >> 4) << 12;
      break;

    default:
    case TEXTURE_MODE_16BPP:
      texture_base = (u8 *)(psx_gpu->vram_ptr);
      texture_base += (psx_gpu->current_texture_page & 0xF) * 128;
      texture_base += ((psx_gpu->current_texture_page >> 4) * 256) * 2048;

      texture_ptr = texture_base;
      texture_ptr += psx_gpu->texture_window_x * 2;
      texture_ptr += (psx_gpu->texture_window_y) * 2048;
      break;
  }

  psx_gpu->texture_page_base = texture_base;
  psx_gpu->texture_page_ptr = texture_ptr;  
}

void set_texture(psx_gpu_struct *psx_gpu, u32 texture_settings)
{
  texture_settings &= 0x1FF;
  if(psx_gpu->texture_settings != texture_settings)
  {
    u32 new_texture_page = texture_settings & 0x1F;
    u32 texture_mode = (texture_settings >> 7) & 0x3;
    u32 render_state_base = psx_gpu->render_state_base;

    flush_render_block_buffer(psx_gpu);

    render_state_base &= ~(0xF << 6);
    render_state_base |= ((texture_settings >> 5) & 0xF) << 6;

    psx_gpu->render_state_base = render_state_base;

    psx_gpu->current_texture_mask = 0x1 << new_texture_page;

    if(texture_mode == TEXTURE_MODE_8BPP)
    {     
      // In 8bpp mode 256x256 takes up two pages. If it's on the right edge it
      // wraps back around to the left edge.
      u32 adjacent_texture_page = ((texture_settings + 1) & 0xF) | (texture_settings & 0x10);
      psx_gpu->current_texture_mask |= 0x1 << adjacent_texture_page;

      if((psx_gpu->last_8bpp_texture_page ^ new_texture_page) & 0x1)
      {
        u32 dirty_textures_8bpp_alternate_mask =
         psx_gpu->dirty_textures_8bpp_alternate_mask;
        psx_gpu->dirty_textures_8bpp_alternate_mask =
         psx_gpu->dirty_textures_8bpp_mask;
        psx_gpu->dirty_textures_8bpp_mask = dirty_textures_8bpp_alternate_mask;
      }

      psx_gpu->last_8bpp_texture_page = new_texture_page;
    }

    psx_gpu->current_texture_page = new_texture_page;
    psx_gpu->texture_settings = texture_settings;

    update_texture_ptr(psx_gpu);
  }
}

void set_clut(psx_gpu_struct *psx_gpu, u32 clut_settings)
{
  if(psx_gpu->clut_settings != clut_settings)
  {
    flush_render_block_buffer(psx_gpu);
    psx_gpu->clut_settings = clut_settings;
    psx_gpu->clut_ptr = psx_gpu->vram_ptr + ((clut_settings & 0x7FFF) * 16);
  }
}

void set_triangle_color(psx_gpu_struct *psx_gpu, u32 triangle_color)
{
  if(psx_gpu->triangle_color != triangle_color)
  {
    flush_render_block_buffer(psx_gpu);
    psx_gpu->triangle_color = triangle_color;
  }
}

static void do_fill(psx_gpu_struct *psx_gpu, u32 x, u32 y,
 u32 width, u32 height, u32 color)
{
  x &= ~0xF;
  width = ((width + 0xF) & ~0xF);

  flush_render_block_buffer(psx_gpu);

  if(unlikely((x + width) > 1024))
  {
    u32 width_a = 1024 - x;
    u32 width_b = width - width_a;

    if(unlikely((y + height) > 512))
    {
      u32 height_a = 512 - y;
      u32 height_b = height - height_a;

      render_block_fill(psx_gpu, color, x, y, width_a, height_a);
      render_block_fill(psx_gpu, color, 0, y, width_b, height_a);
      render_block_fill(psx_gpu, color, x, 0, width_a, height_b);
      render_block_fill(psx_gpu, color, 0, 0, width_b, height_b);
    }
    else
    {
      render_block_fill(psx_gpu, color, x, y, width_a, height);
      render_block_fill(psx_gpu, color, 0, y, width_b, height);
    }
  }
  else
  {
    if(unlikely((y + height) > 512))
    {
      u32 height_a = 512 - y;
      u32 height_b = height - height_a;

      render_block_fill(psx_gpu, color, x, y, width, height_a);
      render_block_fill(psx_gpu, color, x, 0, width, height_b);
    }
    else
    {
      render_block_fill(psx_gpu, color, x, y, width, height);
    }
  }
}

#define sign_extend_12bit(value)                                               \
  (((s32)((value) << 20)) >> 20)                                               \

#define sign_extend_11bit(value)                                               \
  (((s32)((value) << 21)) >> 21)                                               \

#define sign_extend_10bit(value)                                               \
  (((s32)((value) << 22)) >> 22)                                               \


#define get_vertex_data_xy(vertex_number, offset16)                            \
  vertexes[vertex_number].x =                                                  \
   sign_extend_12bit(list_s16[offset16]) + psx_gpu->offset_x;                  \
  vertexes[vertex_number].y =                                                  \
   sign_extend_12bit(list_s16[(offset16) + 1]) + psx_gpu->offset_y;            \

#define get_vertex_data_uv(vertex_number, offset16)                            \
  vertexes[vertex_number].u = list_s16[offset16] & 0xFF;                       \
  vertexes[vertex_number].v = (list_s16[offset16] >> 8) & 0xFF                 \

#define get_vertex_data_rgb(vertex_number, offset32)                           \
  vertexes[vertex_number].r = list[offset32] & 0xFF;                           \
  vertexes[vertex_number].g = (list[offset32] >> 8) & 0xFF;                    \
  vertexes[vertex_number].b = (list[offset32] >> 16) & 0xFF                    \

#define get_vertex_data_xy_uv(vertex_number, offset16)                         \
  get_vertex_data_xy(vertex_number, offset16);                                 \
  get_vertex_data_uv(vertex_number, (offset16) + 2)                            \

#define get_vertex_data_xy_rgb(vertex_number, offset16)                        \
  get_vertex_data_rgb(vertex_number, (offset16) / 2);                          \
  get_vertex_data_xy(vertex_number, (offset16) + 2);                           \

#define get_vertex_data_xy_uv_rgb(vertex_number, offset16)                     \
  get_vertex_data_rgb(vertex_number, (offset16) / 2);                          \
  get_vertex_data_xy(vertex_number, (offset16) + 2);                           \
  get_vertex_data_uv(vertex_number, (offset16) + 4);                           \

#define set_vertex_color_constant(vertex_number, color)                        \
  vertexes[vertex_number].r = color & 0xFF;                                    \
  vertexes[vertex_number].g = (color >> 8) & 0xFF;                             \
  vertexes[vertex_number].b = (color >> 16) & 0xFF                             \

#define get_vertex_data_xy_rgb_constant(vertex_number, offset16, color)        \
  get_vertex_data_xy(vertex_number, offset16);                                 \
  set_vertex_color_constant(vertex_number, color)                              \

#ifndef SET_Ex
#define SET_Ex(r, v)
#endif

u32 gpu_parse(psx_gpu_struct *psx_gpu, u32 *list, u32 size, u32 *last_command)
{
  vertex_struct vertexes[4] __attribute__((aligned(16))) = {};
  u32 current_command = 0, command_length;

  u32 *list_start = list;
  u32 *list_end = list + (size / 4);

  for(; list < list_end; list += 1 + command_length)
  {
  	s16 *list_s16 = (void *)list;
  	current_command = *list >> 24;
  	command_length = command_lengths[current_command];
  	if (list + 1 + command_length > list_end) {
  	  current_command = (u32)-1;
  	  break;
  	}

  	switch(current_command)
  	{
  		case 0x00:
  			break;
  
  		case 0x02:
      {
        u32 x = list_s16[2] & 0x3FF;
        u32 y = list_s16[3] & 0x1FF;
        u32 width = list_s16[4] & 0x3FF;
        u32 height = list_s16[5] & 0x1FF;
        u32 color = list[0] & 0xFFFFFF;

        do_fill(psx_gpu, x, y, width, height, color);
  			break;
      }
  
  		case 0x20 ... 0x23:
      {
        set_triangle_color(psx_gpu, list[0] & 0xFFFFFF);
  
        get_vertex_data_xy(0, 2);
        get_vertex_data_xy(1, 4);
        get_vertex_data_xy(2, 6);
          
        render_triangle(psx_gpu, vertexes, current_command);
  			break;
      }
  
  		case 0x24 ... 0x27:
      {
        set_clut(psx_gpu, list_s16[5]);
        set_texture(psx_gpu, list_s16[9]);
        set_triangle_color(psx_gpu, list[0] & 0xFFFFFF);
  
        get_vertex_data_xy_uv(0, 2);
        get_vertex_data_xy_uv(1, 6);
        get_vertex_data_xy_uv(2, 10);
  
        render_triangle(psx_gpu, vertexes, current_command);
  			break;
      }
  
  		case 0x28 ... 0x2B:
      {
        set_triangle_color(psx_gpu, list[0] & 0xFFFFFF);
  
        get_vertex_data_xy(0, 2);
        get_vertex_data_xy(1, 4);
        get_vertex_data_xy(2, 6);
        get_vertex_data_xy(3, 8);
  
        render_triangle(psx_gpu, vertexes, current_command);
        render_triangle(psx_gpu, &(vertexes[1]), current_command);
  			break;
      }
  
  		case 0x2C ... 0x2F:
      {
        set_clut(psx_gpu, list_s16[5]);
        set_texture(psx_gpu, list_s16[9]);
        set_triangle_color(psx_gpu, list[0] & 0xFFFFFF);
  
        get_vertex_data_xy_uv(0, 2);   
        get_vertex_data_xy_uv(1, 6);   
        get_vertex_data_xy_uv(2, 10);  
        get_vertex_data_xy_uv(3, 14);
  
        render_triangle(psx_gpu, vertexes, current_command);
        render_triangle(psx_gpu, &(vertexes[1]), current_command);
  			break;
      }
  
  		case 0x30 ... 0x33:
      {
        get_vertex_data_xy_rgb(0, 0);
        get_vertex_data_xy_rgb(1, 4);
        get_vertex_data_xy_rgb(2, 8);
  
        render_triangle(psx_gpu, vertexes, current_command);
  			break;
      }
  
  		case 0x34:
  		case 0x35:
  		case 0x36:
  		case 0x37:
      {
        set_clut(psx_gpu, list_s16[5]);
        set_texture(psx_gpu, list_s16[11]);
  
        get_vertex_data_xy_uv_rgb(0, 0);
        get_vertex_data_xy_uv_rgb(1, 6);
        get_vertex_data_xy_uv_rgb(2, 12);

        render_triangle(psx_gpu, vertexes, current_command);
  			break;
      }
  
  		case 0x38:
  		case 0x39:
  		case 0x3A:
  		case 0x3B:
      {
        get_vertex_data_xy_rgb(0, 0);
        get_vertex_data_xy_rgb(1, 4);
        get_vertex_data_xy_rgb(2, 8);
        get_vertex_data_xy_rgb(3, 12);
  
        render_triangle(psx_gpu, vertexes, current_command);
        render_triangle(psx_gpu, &(vertexes[1]), current_command);
  			break;
      }
  
  		case 0x3C:
  		case 0x3D:
  		case 0x3E:
  		case 0x3F:
      {
        set_clut(psx_gpu, list_s16[5]);
        set_texture(psx_gpu, list_s16[11]);
  
        get_vertex_data_xy_uv_rgb(0, 0);
        get_vertex_data_xy_uv_rgb(1, 6);
        get_vertex_data_xy_uv_rgb(2, 12);
        get_vertex_data_xy_uv_rgb(3, 18);
  
        render_triangle(psx_gpu, vertexes, current_command);
        render_triangle(psx_gpu, &(vertexes[1]), current_command);
  			break;
      }
  
  		case 0x40 ... 0x47:
      {
        vertexes[0].x = list_s16[2] + psx_gpu->offset_x;
        vertexes[0].y = list_s16[3] + psx_gpu->offset_y;
        vertexes[1].x = list_s16[4] + psx_gpu->offset_x;
        vertexes[1].y = list_s16[5] + psx_gpu->offset_y;

        render_line(psx_gpu, vertexes, current_command, list[0], 0);
  			break;
      }
  
  		case 0x48 ... 0x4F:
      {
        u32 num_vertexes = 1;
        u32 *list_position = &(list[2]);
        u32 xy = list[1];

        vertexes[1].x = (xy & 0xFFFF) + psx_gpu->offset_x;
        vertexes[1].y = (xy >> 16) + psx_gpu->offset_y;
      
        xy = *list_position;
        while(1)
        {
          vertexes[0] = vertexes[1];

          vertexes[1].x = (xy & 0xFFFF) + psx_gpu->offset_x;
          vertexes[1].y = (xy >> 16) + psx_gpu->offset_y;

          render_line(psx_gpu, vertexes, current_command, list[0], 0);

          list_position++;
          num_vertexes++;

          if(list_position >= list_end)
          {
            current_command = (u32)-1;
            goto breakloop;
          }

          xy = *list_position;
          if((xy & 0xF000F000) == 0x50005000)
            break;
        }

        command_length += (num_vertexes - 2);
        break;
      }
  
  		case 0x50 ... 0x57:
      {
        vertexes[0].r = list[0] & 0xFF;
        vertexes[0].g = (list[0] >> 8) & 0xFF;
        vertexes[0].b = (list[0] >> 16) & 0xFF;
        vertexes[0].x = list_s16[2] + psx_gpu->offset_x;
        vertexes[0].y = list_s16[3] + psx_gpu->offset_y;

        vertexes[1].r = list[2] & 0xFF;
        vertexes[1].g = (list[2] >> 8) & 0xFF;
        vertexes[1].b = (list[2] >> 16) & 0xFF;
        vertexes[1].x = list_s16[6] + psx_gpu->offset_x;
        vertexes[1].y = list_s16[7] + psx_gpu->offset_y;

        render_line(psx_gpu, vertexes, current_command, 0, 0);
  			break;
      }
 
      case 0x58 ... 0x5F:
      {
        u32 num_vertexes = 1;
        u32 *list_position = &(list[2]);
        u32 color = list[0];
        u32 xy = list[1];

        vertexes[1].r = color & 0xFF;
        vertexes[1].g = (color >> 8) & 0xFF;
        vertexes[1].b = (color >> 16) & 0xFF;
        vertexes[1].x = (xy & 0xFFFF) + psx_gpu->offset_x;
        vertexes[1].y = (xy >> 16) + psx_gpu->offset_y;
      
        color = list_position[0];
        while(1)
        {
          xy = list_position[1];

          vertexes[0] = vertexes[1];

          vertexes[1].r = color & 0xFF;
          vertexes[1].g = (color >> 8) & 0xFF;
          vertexes[1].b = (color >> 16) & 0xFF;
          vertexes[1].x = (xy & 0xFFFF) + psx_gpu->offset_x;
          vertexes[1].y = (xy >> 16) + psx_gpu->offset_y;

          render_line(psx_gpu, vertexes, current_command, 0, 0);

          list_position += 2;
          num_vertexes++;

          if(list_position >= list_end)
          {
            current_command = (u32)-1;
            goto breakloop;
          }

          color = list_position[0];
          if((color & 0xF000F000) == 0x50005000)
            break;
        }

        command_length += ((num_vertexes - 2) * 2);
        break;
      }
  
  		case 0x60 ... 0x63:
      {        
        u32 x = sign_extend_11bit(list_s16[2] + psx_gpu->offset_x);
        u32 y = sign_extend_11bit(list_s16[3] + psx_gpu->offset_y);
        u32 width = list_s16[4] & 0x3FF;
        u32 height = list_s16[5] & 0x1FF;

        render_sprite(psx_gpu, x, y, 0, 0, width, height, current_command, list[0]);
  			break;
      }
  
  		case 0x64 ... 0x67:
      {        
        u32 x = sign_extend_11bit(list_s16[2] + psx_gpu->offset_x);
        u32 y = sign_extend_11bit(list_s16[3] + psx_gpu->offset_y);
        u32 uv = list_s16[4];
        u32 width = list_s16[6] & 0x3FF;
        u32 height = list_s16[7] & 0x1FF;

        set_clut(psx_gpu, list_s16[5]);

        render_sprite(psx_gpu, x, y, uv & 0xFF, (uv >> 8) & 0xFF, width, height,
         current_command, list[0]);
  			break;
      }
  
  		case 0x68:
  		case 0x69:
  		case 0x6A:
  		case 0x6B:
      {
        s32 x = sign_extend_11bit(list_s16[2] + psx_gpu->offset_x);
        s32 y = sign_extend_11bit(list_s16[3] + psx_gpu->offset_y);

        render_sprite(psx_gpu, x, y, 0, 0, 1, 1, current_command, list[0]);
  			break;
      }
  
  		case 0x70:
  		case 0x71:
  		case 0x72:
  		case 0x73:
      {        
        s32 x = sign_extend_11bit(list_s16[2] + psx_gpu->offset_x);
        s32 y = sign_extend_11bit(list_s16[3] + psx_gpu->offset_y);

        render_sprite(psx_gpu, x, y, 0, 0, 8, 8, current_command, list[0]);
  			break;
      }
  
  		case 0x74:
  		case 0x75:
  		case 0x76:
  		case 0x77:
      {        
        s32 x = sign_extend_11bit(list_s16[2] + psx_gpu->offset_x);
        s32 y = sign_extend_11bit(list_s16[3] + psx_gpu->offset_y);
        u32 uv = list_s16[4];

        set_clut(psx_gpu, list_s16[5]);

        render_sprite(psx_gpu, x, y, uv & 0xFF, (uv >> 8) & 0xFF, 8, 8,
         current_command, list[0]);
  			break;
      }
  
  		case 0x78:
  		case 0x79:
  		case 0x7A:
  		case 0x7B:
      {        
        s32 x = sign_extend_11bit(list_s16[2] + psx_gpu->offset_x);
        s32 y = sign_extend_11bit(list_s16[3] + psx_gpu->offset_y);

        render_sprite(psx_gpu, x, y, 0, 0, 16, 16, current_command, list[0]);
  			break;
      }
  
  		case 0x7C:
  		case 0x7D:
  		case 0x7E:
  		case 0x7F:
      {        
        s32 x = sign_extend_11bit(list_s16[2] + psx_gpu->offset_x);
        s32 y = sign_extend_11bit(list_s16[3] + psx_gpu->offset_y);
        u32 uv = list_s16[4];

        set_clut(psx_gpu, list_s16[5]);

        render_sprite(psx_gpu, x, y, uv & 0xFF, (uv >> 8) & 0xFF, 16, 16,
         current_command, list[0]);
  			break;
      }
  
#ifdef PCSX
      case 0x80 ... 0x9F:          //  vid -> vid
      case 0xA0 ... 0xBF:          //  sys -> vid
      case 0xC0 ... 0xDF:          //  vid -> sys
        goto breakloop;
#else
      case 0x80 ... 0x9F:          //  vid -> vid
      {
        u32 sx = list_s16[2] & 0x3FF;
        u32 sy = list_s16[3] & 0x1FF;
        u32 dx = list_s16[4] & 0x3FF;
        u32 dy = list_s16[5] & 0x1FF;
        u32 w = ((list_s16[6] - 1) & 0x3FF) + 1;
        u32 h = ((list_s16[7] - 1) & 0x1FF) + 1;

        if (sx == dx && sy == dy && psx_gpu->mask_msb == 0)
          break;

        render_block_move(psx_gpu, sx, sy, dx, dy, w, h);
        break;
      } 

      case 0xA0 ... 0xBF:          //  sys -> vid
      {
        u32 load_x = list_s16[2] & 0x3FF;
        u32 load_y = list_s16[3] & 0x1FF;
        u32 load_width = list_s16[4] & 0x3FF;
        u32 load_height = list_s16[5] & 0x1FF;
        u32 load_size = load_width * load_height;
  
        command_length += load_size / 2;

        if(load_size & 1)
          command_length++;

        render_block_copy(psx_gpu, (u16 *)&(list_s16[6]), load_x, load_y,
         load_width, load_height, load_width);
  			break;
      }

      case 0xC0 ... 0xDF:          //  vid -> sys
        break;
#endif

  		case 0xE1:
        set_texture(psx_gpu, list[0]);

        if(list[0] & (1 << 9))
          psx_gpu->render_state_base |= RENDER_STATE_DITHER;
        else
          psx_gpu->render_state_base &= ~RENDER_STATE_DITHER;

        psx_gpu->display_area_draw_enable = (list[0] >> 10) & 0x1;
  			SET_Ex(1, list[0]);
  			break;
  
  		case 0xE2:
      {
        // TODO: Clean
        u32 texture_window_settings = list[0];
        u32 tmp, x, y, w, h;

        if(texture_window_settings != psx_gpu->texture_window_settings)
        {
          tmp = (texture_window_settings & 0x1F) | 0x20;
          for(w = 8; (tmp & 1) == 0; tmp >>= 1, w <<= 1);

          tmp = ((texture_window_settings >> 5) & 0x1f) | 0x20;
          for (h = 8; (tmp & 1) == 0; tmp >>= 1, h <<= 1);

          tmp = 32 - (w >> 3);
          x = ((texture_window_settings >> 10) & tmp) << 3;

          tmp = 32 - (h >> 3);
          y = ((texture_window_settings >> 15) & tmp) << 3;

          flush_render_block_buffer(psx_gpu);
          
          psx_gpu->texture_window_settings = texture_window_settings;
          psx_gpu->texture_window_x = x;
          psx_gpu->texture_window_y = y;
          psx_gpu->texture_mask_width = w - 1;
          psx_gpu->texture_mask_height = h - 1;

          update_texture_ptr(psx_gpu);
        }
        SET_Ex(2, list[0]);
        break;
      }

      case 0xE3:
      {
        s16 viewport_start_x = list[0] & 0x3FF;
        s16 viewport_start_y = (list[0] >> 10) & 0x1FF;

        if(viewport_start_x == psx_gpu->viewport_start_x &&
         viewport_start_y == psx_gpu->viewport_start_y)
        {
          break;
        }
  
        psx_gpu->viewport_start_x = viewport_start_x;
        psx_gpu->viewport_start_y = viewport_start_y;

#ifdef TEXTURE_CACHE_4BPP
        psx_gpu->viewport_mask =
         texture_region_mask(psx_gpu->viewport_start_x,
         psx_gpu->viewport_start_y, psx_gpu->viewport_end_x,
         psx_gpu->viewport_end_y);
#endif
        SET_Ex(3, list[0]);
        break;
      }

      case 0xE4:
      {
        s16 viewport_end_x = list[0] & 0x3FF;
        s16 viewport_end_y = (list[0] >> 10) & 0x1FF;

        if(viewport_end_x == psx_gpu->viewport_end_x &&
         viewport_end_y == psx_gpu->viewport_end_y)
        {
          break;
        }

        psx_gpu->viewport_end_x = viewport_end_x;
        psx_gpu->viewport_end_y = viewport_end_y;

#ifdef TEXTURE_CACHE_4BPP
        psx_gpu->viewport_mask =
         texture_region_mask(psx_gpu->viewport_start_x,
         psx_gpu->viewport_start_y, psx_gpu->viewport_end_x,
         psx_gpu->viewport_end_y);
#endif
        SET_Ex(4, list[0]);
        break;
      }
  
      case 0xE5:
      {
        s32 offset_x = list[0] << 21;
        s32 offset_y = list[0] << 10;
        psx_gpu->offset_x = offset_x >> 21;
        psx_gpu->offset_y = offset_y >> 21; 
  
  			SET_Ex(5, list[0]);
  			break;
  		}

  		case 0xE6:
      {
        u32 mask_settings = list[0];
        u16 mask_msb = mask_settings << 15;

        if(list[0] & 0x2)
          psx_gpu->render_state_base |= RENDER_STATE_MASK_EVALUATE;
        else
          psx_gpu->render_state_base &= ~RENDER_STATE_MASK_EVALUATE;

        if(mask_msb != psx_gpu->mask_msb)
        {
          flush_render_block_buffer(psx_gpu);
          psx_gpu->mask_msb = mask_msb;
        }

  			SET_Ex(6, list[0]);
  			break;
      }
  
  		default:
  			break;
  	}
  }

breakloop:
  if (last_command != NULL)
    *last_command = current_command;
  return list - list_start;
}

#ifdef PCSX

#define ENH_BUF_TABLE_STEP (1024 / sizeof(psx_gpu->enhancement_buf_by_x16))

static int is_new_scanout(psx_gpu_struct *psx_gpu, int x)
{
  int i, scanout_x;
  for (i = 0; i < ARRAY_SIZE(psx_gpu->enhancement_scanout_x); i++)
  {
    scanout_x = psx_gpu->enhancement_scanout_x[i];
    if (x <= scanout_x && scanout_x < x + ENH_BUF_TABLE_STEP)
    {
      if (x != scanout_x)
        log_anomaly("unaligned scanout x: %d,%d\n", scanout_x, x);
      return 1;
    }
  }
  return 0;
}

static void update_enhancement_buf_table_from_hres(psx_gpu_struct *psx_gpu)
{
  u32 b, x;

  b = 0;
  psx_gpu->enhancement_buf_by_x16[0] = b;
  psx_gpu->enhancement_buf_start[0] = 0;
  for (x = 1; x < sizeof(psx_gpu->enhancement_buf_by_x16); x++)
  {
    if (b < 3 && is_new_scanout(psx_gpu, x * ENH_BUF_TABLE_STEP)) {
      b++;
      psx_gpu->enhancement_buf_start[b] = x * ENH_BUF_TABLE_STEP;
    }

    psx_gpu->enhancement_buf_by_x16[x] = b;
  }
#if 0
  printf("buf_by_x16:\n");
  for (b = 0; b < 3; b++) {
    int first = -1, count = 0;
    for (x = 0; x < sizeof(psx_gpu->enhancement_buf_by_x16); x++) {
      if (psx_gpu->enhancement_buf_by_x16[x] == b) {
        if (first < 0) first = x;
        count++;
      }
    }
    if (count) {
      assert(first * ENH_BUF_TABLE_STEP == psx_gpu->enhancement_buf_start[b]);
      printf("%d: %3zd-%zd\n", b, first * ENH_BUF_TABLE_STEP,
          (first + count) * ENH_BUF_TABLE_STEP);
    }
  }
#endif
}

static void update_enhancement_buf_table_from_x(psx_gpu_struct *psx_gpu,
 u32 x0, u32 len)
{
#if 0
  u32 x, b;

  for (x = x0, b = 0; x >= len; b++)
    x -= len;
  if (b > 3)
    b = 3;

  memset(psx_gpu->enhancement_buf_by_x16 + x0 / ENH_BUF_TABLE_STEP,
   b, (len + ENH_BUF_TABLE_STEP - 1) / ENH_BUF_TABLE_STEP);
#endif
}

#define select_enhancement_buf(psx_gpu) \
  psx_gpu->enhancement_current_buf_ptr = \
    select_enhancement_buf_ptr(psx_gpu, psx_gpu->saved_viewport_start_x)

#define enhancement_disable() { \
  psx_gpu->vram_out_ptr = psx_gpu->vram_ptr; \
  psx_gpu->viewport_start_x = psx_gpu->saved_viewport_start_x; \
  psx_gpu->viewport_start_y = psx_gpu->saved_viewport_start_y; \
  psx_gpu->viewport_end_x = psx_gpu->saved_viewport_end_x; \
  psx_gpu->viewport_end_y = psx_gpu->saved_viewport_end_y; \
  psx_gpu->uvrgb_phase = 0x8000; \
}

#define enhancement_enable() { \
  psx_gpu->vram_out_ptr = psx_gpu->enhancement_current_buf_ptr; \
  psx_gpu->viewport_start_x = psx_gpu->saved_viewport_start_x * 2; \
  psx_gpu->viewport_start_y = psx_gpu->saved_viewport_start_y * 2; \
  psx_gpu->viewport_end_x = psx_gpu->saved_viewport_end_x * 2 + 1; \
  psx_gpu->viewport_end_y = psx_gpu->saved_viewport_end_y * 2 + 1; \
  psx_gpu->uvrgb_phase = 0x7fff; \
}

#define shift_vertices3(v) { \
  v[0]->x <<= 1; \
  v[0]->y <<= 1; \
  v[1]->x <<= 1; \
  v[1]->y <<= 1; \
  v[2]->x <<= 1; \
  v[2]->y <<= 1; \
}

#define unshift_vertices3(v) { \
  v[0]->x >>= 1; \
  v[0]->y >>= 1; \
  v[1]->x >>= 1; \
  v[1]->y >>= 1; \
  v[2]->x >>= 1; \
  v[2]->y >>= 1; \
}

#define shift_triangle_area() \
  psx_gpu->triangle_area *= 4

#ifndef NEON_BUILD
void scale2x_tiles8(void *dst, const void *src, int w8, int h)
{
  uint16_t* d = (uint16_t*)dst;
  const uint16_t* s = (const uint16_t*)src;

  while ( h-- )
  {
    uint16_t* d_save = d;
    const uint16_t* s_save = s;
    int w = w8;

    while ( w-- )
    {
      d[    0 ] = *s;
      d[    1 ] = *s;
      d[ 1024 ] = *s;
      d[ 1025 ] = *s;
      d += 2; s++;

      d[    0 ] = *s;
      d[    1 ] = *s;
      d[ 1024 ] = *s;
      d[ 1025 ] = *s;
      d += 2; s++;

      d[    0 ] = *s;
      d[    1 ] = *s;
      d[ 1024 ] = *s;
      d[ 1025 ] = *s;
      d += 2; s++;

      d[    0 ] = *s;
      d[    1 ] = *s;
      d[ 1024 ] = *s;
      d[ 1025 ] = *s;
      d += 2; s++;

      d[    0 ] = *s;
      d[    1 ] = *s;
      d[ 1024 ] = *s;
      d[ 1025 ] = *s;
      d += 2; s++;

      d[    0 ] = *s;
      d[    1 ] = *s;
      d[ 1024 ] = *s;
      d[ 1025 ] = *s;
      d += 2; s++;

      d[    0 ] = *s;
      d[    1 ] = *s;
      d[ 1024 ] = *s;
      d[ 1025 ] = *s;
      d += 2; s++;

      d[    0 ] = *s;
      d[    1 ] = *s;
      d[ 1024 ] = *s;
      d[ 1025 ] = *s;
      d += 2; s++;
    }

    d = d_save + 2048;
    s = s_save + 1024; /* or 512? */
  }
}
#endif

static int disable_main_render;

static int check_enhanced_range(psx_gpu_struct *psx_gpu, int x, int x_end)
{
  // reject to avoid oveflowing the 1024 width
  // (assume some offscreen render-to-texture thing)
  int fb_index;
  if (x < 0)
    return 1;
  fb_index = select_enhancement_buf_index(psx_gpu, x);
  if (x >= psx_gpu->enhancement_buf_start[fb_index] + 512)
    return 0;

  return 1;
}

static int is_in_array(int val, int array[], int len)
{
  int i;
  for (i = 0; i < len; i++)
    if (array[i] == val)
      return 1;
  return 0;
}

static int make_members_unique(int array[], int len)
{
  int i, j;
  for (i = j = 1; i < len; i++)
    if (!is_in_array(array[i], array, j))
      array[j++] = array[i];

  if (array[0] > array[1]) {
    i = array[0]; array[0] = array[1]; array[1] = i;
  }
  return j;
}

static void patch_u(vertex_struct *vertex_ptrs, int count, int old, int new)
{
  int i;
  for (i = 0; i < count; i++)
    if (vertex_ptrs[i].u == old)
      vertex_ptrs[i].u = new;
}

static void patch_v(vertex_struct *vertex_ptrs, int count, int old, int new)
{
  int i;
  for (i = 0; i < count; i++)
    if (vertex_ptrs[i].v == old)
      vertex_ptrs[i].v = new;
}

static void uv_hack(vertex_struct *vertex_ptrs, int vertex_count)
{
  int i, u[4], v[4];

  for (i = 0; i < vertex_count; i++) {
    u[i] = vertex_ptrs[i].u;
    v[i] = vertex_ptrs[i].v;
  }
  if (make_members_unique(u, vertex_count) == 2 && u[1] - u[0] >= 8) {
    if ((u[0] & 7) == 7) {
      patch_u(vertex_ptrs, vertex_count, u[0], u[0] + 1);
      //printf("u hack: %3u-%3u -> %3u-%3u\n", u[0], u[1], u[0]+1, u[1]);
    }
    else if ((u[1] & 7) == 0 || u[1] - u[0] > 128) {
      patch_u(vertex_ptrs, vertex_count, u[1], u[1] - 1);
      //printf("u hack: %3u-%3u -> %3u-%3u\n", u[0], u[1], u[0], u[1]-1);
    }
  }
  if (make_members_unique(v, vertex_count) == 2 && ((v[0] - v[1]) & 7) == 0) {
    if ((v[0] & 7) == 7) {
      patch_v(vertex_ptrs, vertex_count, v[0], v[0] + 1);
      //printf("v hack: %3u-%3u -> %3u-%3u\n", v[0], v[1], v[0]+1, v[1]);
    }
    else if ((v[1] & 7) == 0) {
      patch_v(vertex_ptrs, vertex_count, v[1], v[1] - 1);
      //printf("v hack: %3u-%3u -> %3u-%3u\n", v[0], v[1], v[0], v[1]-1);
    }
  }
}

static void do_triangle_enhanced(psx_gpu_struct *psx_gpu,
 vertex_struct *vertexes, u32 current_command)
{
  vertex_struct *vertex_ptrs[3];

  if (!prepare_triangle(psx_gpu, vertexes, vertex_ptrs))
    return;

  if (!disable_main_render)
    render_triangle_p(psx_gpu, vertex_ptrs, current_command);

  if (!check_enhanced_range(psx_gpu, vertex_ptrs[0]->x, vertex_ptrs[2]->x))
    return;

  enhancement_enable();
  shift_vertices3(vertex_ptrs);
  shift_triangle_area();
  render_triangle_p(psx_gpu, vertex_ptrs, current_command);
  unshift_vertices3(vertex_ptrs);
}

static void do_quad_enhanced(psx_gpu_struct *psx_gpu, vertex_struct *vertexes,
 u32 current_command)
{
  do_triangle_enhanced(psx_gpu, vertexes, current_command);
  enhancement_disable();
  do_triangle_enhanced(psx_gpu, &vertexes[1], current_command);
}

#if 0

#define fill_vertex(i, x_, y_, u_, v_, rgb_) \
  vertexes[i].x = x_; \
  vertexes[i].y = y_; \
  vertexes[i].u = u_; \
  vertexes[i].v = v_; \
  vertexes[i].r = rgb_; \
  vertexes[i].g = (rgb_) >> 8; \
  vertexes[i].b = (rgb_) >> 16

static void do_sprite_enhanced(psx_gpu_struct *psx_gpu, int x, int y,
 u32 u, u32 v, u32 w, u32 h, u32 cmd_rgb)
{
  vertex_struct *vertex_ptrs[3];
  u32 flags = (cmd_rgb >> 24);
  u32 color = cmd_rgb & 0xffffff;
  u32 render_state_base_saved = psx_gpu->render_state_base;
  int x1, y1;
  u8 u1, v1;

  flags &=
   (RENDER_FLAGS_MODULATE_TEXELS | RENDER_FLAGS_BLEND |
   RENDER_FLAGS_TEXTURE_MAP);

  set_triangle_color(psx_gpu, color);
  if(color == 0x808080)
    flags |= RENDER_FLAGS_MODULATE_TEXELS;

  psx_gpu->render_state_base &= ~RENDER_STATE_DITHER;
  enhancement_enable();

  x1 = x + w;
  y1 = y + h;
  u1 = u + w;
  v1 = v + h;
  // FIXME..
  if (u1 < u) u1 = 0xff;
  if (v1 < v) v1 = 0xff;

  // 0-2
  // |/
  // 1
  fill_vertex(0, x,  y,  u,  v,  color);
  fill_vertex(1, x,  y1, u,  v1, color);
  fill_vertex(2, x1, y,  u1, v,  color);
  if (prepare_triangle(psx_gpu, vertexes, vertex_ptrs)) {
    shift_vertices3(vertex_ptrs);
    shift_triangle_area();
    render_triangle_p(psx_gpu, vertex_ptrs, flags);
  }

  //   0
  //  /|
  // 1-2
  fill_vertex(0, x1, y,  u1, v,  color);
  fill_vertex(1, x,  y1, u,  v1, color);
  fill_vertex(2, x1, y1, u1, v1, color);
  if (prepare_triangle(psx_gpu, vertexes, vertex_ptrs)) {
    shift_vertices3(vertex_ptrs);
    shift_triangle_area();
    render_triangle_p(psx_gpu, vertex_ptrs, flags);
  }

  psx_gpu->render_state_base = render_state_base_saved;
}
#else
static void do_sprite_enhanced(psx_gpu_struct *psx_gpu, int x, int y,
 u32 u, u32 v, u32 w, u32 h, u32 cmd_rgb)
{
  u32 flags = (cmd_rgb >> 24);
  u32 color = cmd_rgb & 0xffffff;

  render_sprite_4x(psx_gpu, x, y, u, v, w, h, flags, color);
}
#endif

u32 gpu_parse_enhanced(psx_gpu_struct *psx_gpu, u32 *list, u32 size,
 u32 *last_command)
{
  vertex_struct vertexes[4] __attribute__((aligned(16))) = {};
  u32 current_command = 0, command_length;

  u32 *list_start = list;
  u32 *list_end = list + (size / 4);

  psx_gpu->saved_viewport_start_x = psx_gpu->viewport_start_x;
  psx_gpu->saved_viewport_start_y = psx_gpu->viewport_start_y;
  psx_gpu->saved_viewport_end_x = psx_gpu->viewport_end_x;
  psx_gpu->saved_viewport_end_y = psx_gpu->viewport_end_y;
  select_enhancement_buf(psx_gpu);

  for(; list < list_end; list += 1 + command_length)
  {
    s16 *list_s16 = (void *)list;
    current_command = *list >> 24;
    command_length = command_lengths[current_command];
    if (list + 1 + command_length > list_end) {
      current_command = (u32)-1;
      break;
    }

    enhancement_disable();

    switch(current_command)
    {
      case 0x00:
        break;
  
      case 0x02:
      {
        u32 x = list_s16[2] & 0x3FF;
        u32 y = list_s16[3] & 0x1FF;
        u32 width = list_s16[4] & 0x3FF;
        u32 height = list_s16[5] & 0x1FF;
        u32 color = list[0] & 0xFFFFFF;
        u32 i1, i2;

        x &= ~0xF;
        width = ((width + 0xF) & ~0xF);
        if (width == 0 || height == 0)
          break;

        do_fill(psx_gpu, x, y, width, height, color);

        i1 = select_enhancement_buf_index(psx_gpu, x);
        i2 = select_enhancement_buf_index(psx_gpu, x + width - 1);
        if (i1 != i2) {
          sync_enhancement_buffers(x, y, width, height);
          break;
        }
        if (x >= psx_gpu->enhancement_buf_start[i1] + psx_gpu->saved_hres)
          break;

        psx_gpu->vram_out_ptr = select_enhancement_buf_ptr(psx_gpu, x);
        x *= 2;
        y *= 2;
        width *= 2;
        height *= 2;
        render_block_fill_enh(psx_gpu, color, x, y, width, height);
        break;
      }
  
      case 0x20 ... 0x23:
      {
        set_triangle_color(psx_gpu, list[0] & 0xFFFFFF);
  
        get_vertex_data_xy(0, 2);
        get_vertex_data_xy(1, 4);
        get_vertex_data_xy(2, 6);

        do_triangle_enhanced(psx_gpu, vertexes, current_command);
        break;
      }
  
      case 0x24 ... 0x27:
      {
        set_clut(psx_gpu, list_s16[5]);
        set_texture(psx_gpu, list_s16[9]);
        set_triangle_color(psx_gpu, list[0] & 0xFFFFFF);
  
        get_vertex_data_xy_uv(0, 2);
        get_vertex_data_xy_uv(1, 6);
        get_vertex_data_xy_uv(2, 10);
  
        do_triangle_enhanced(psx_gpu, vertexes, current_command);
        break;
      }
  
      case 0x28 ... 0x2B:
      {
        set_triangle_color(psx_gpu, list[0] & 0xFFFFFF);
  
        get_vertex_data_xy(0, 2);
        get_vertex_data_xy(1, 4);
        get_vertex_data_xy(2, 6);
        get_vertex_data_xy(3, 8);

        do_quad_enhanced(psx_gpu, vertexes, current_command);
        break;
      }
  
      case 0x2C ... 0x2F:
      {
        set_clut(psx_gpu, list_s16[5]);
        set_texture(psx_gpu, list_s16[9]);
        set_triangle_color(psx_gpu, list[0] & 0xFFFFFF);
  
        get_vertex_data_xy_uv(0, 2);   
        get_vertex_data_xy_uv(1, 6);   
        get_vertex_data_xy_uv(2, 10);  
        get_vertex_data_xy_uv(3, 14);
  
        uv_hack(vertexes, 4);
        do_quad_enhanced(psx_gpu, vertexes, current_command);
        break;
      }
  
      case 0x30 ... 0x33:
      {
        get_vertex_data_xy_rgb(0, 0);
        get_vertex_data_xy_rgb(1, 4);
        get_vertex_data_xy_rgb(2, 8);
  
        do_triangle_enhanced(psx_gpu, vertexes, current_command);
        break;
      }
  
      case 0x34:
      case 0x35:
      case 0x36:
      case 0x37:
      {
        set_clut(psx_gpu, list_s16[5]);
        set_texture(psx_gpu, list_s16[11]);
  
        get_vertex_data_xy_uv_rgb(0, 0);
        get_vertex_data_xy_uv_rgb(1, 6);
        get_vertex_data_xy_uv_rgb(2, 12);

        do_triangle_enhanced(psx_gpu, vertexes, current_command);
        break;
      }
  
      case 0x38:
      case 0x39:
      case 0x3A:
      case 0x3B:
      {
        get_vertex_data_xy_rgb(0, 0);
        get_vertex_data_xy_rgb(1, 4);
        get_vertex_data_xy_rgb(2, 8);
        get_vertex_data_xy_rgb(3, 12);
  
        do_quad_enhanced(psx_gpu, vertexes, current_command);
        break;
      }
  
      case 0x3C:
      case 0x3D:
      case 0x3E:
      case 0x3F:
      {
        set_clut(psx_gpu, list_s16[5]);
        set_texture(psx_gpu, list_s16[11]);
  
        get_vertex_data_xy_uv_rgb(0, 0);
        get_vertex_data_xy_uv_rgb(1, 6);
        get_vertex_data_xy_uv_rgb(2, 12);
        get_vertex_data_xy_uv_rgb(3, 18);

        uv_hack(vertexes, 4);
        do_quad_enhanced(psx_gpu, vertexes, current_command);
        break;
      }
  
      case 0x40 ... 0x47:
      {
        vertexes[0].x = list_s16[2] + psx_gpu->offset_x;
        vertexes[0].y = list_s16[3] + psx_gpu->offset_y;
        vertexes[1].x = list_s16[4] + psx_gpu->offset_x;
        vertexes[1].y = list_s16[5] + psx_gpu->offset_y;

        render_line(psx_gpu, vertexes, current_command, list[0], 0);
        enhancement_enable();
        render_line(psx_gpu, vertexes, current_command, list[0], 1);
        break;
      }
  
      case 0x48 ... 0x4F:
      {
        u32 num_vertexes = 1;
        u32 *list_position = &(list[2]);
        u32 xy = list[1];

        vertexes[1].x = (xy & 0xFFFF) + psx_gpu->offset_x;
        vertexes[1].y = (xy >> 16) + psx_gpu->offset_y;
      
        xy = *list_position;
        while(1)
        {
          vertexes[0] = vertexes[1];

          vertexes[1].x = (xy & 0xFFFF) + psx_gpu->offset_x;
          vertexes[1].y = (xy >> 16) + psx_gpu->offset_y;

          enhancement_disable();
          render_line(psx_gpu, vertexes, current_command, list[0], 0);
          enhancement_enable();
          render_line(psx_gpu, vertexes, current_command, list[0], 1);

          list_position++;
          num_vertexes++;

          if(list_position >= list_end)
          {
            current_command = (u32)-1;
            goto breakloop;
          }

          xy = *list_position;
          if((xy & 0xF000F000) == 0x50005000)
            break;
        }

        command_length += (num_vertexes - 2);
        break;
      }
  
      case 0x50 ... 0x57:
      {
        vertexes[0].r = list[0] & 0xFF;
        vertexes[0].g = (list[0] >> 8) & 0xFF;
        vertexes[0].b = (list[0] >> 16) & 0xFF;
        vertexes[0].x = list_s16[2] + psx_gpu->offset_x;
        vertexes[0].y = list_s16[3] + psx_gpu->offset_y;

        vertexes[1].r = list[2] & 0xFF;
        vertexes[1].g = (list[2] >> 8) & 0xFF;
        vertexes[1].b = (list[2] >> 16) & 0xFF;
        vertexes[1].x = list_s16[6] + psx_gpu->offset_x;
        vertexes[1].y = list_s16[7] + psx_gpu->offset_y;

        render_line(psx_gpu, vertexes, current_command, 0, 0);
        enhancement_enable();
        render_line(psx_gpu, vertexes, current_command, 0, 1);
        break;
      }
 
      case 0x58 ... 0x5F:
      {
        u32 num_vertexes = 1;
        u32 *list_position = &(list[2]);
        u32 color = list[0];
        u32 xy = list[1];

        vertexes[1].r = color & 0xFF;
        vertexes[1].g = (color >> 8) & 0xFF;
        vertexes[1].b = (color >> 16) & 0xFF;
        vertexes[1].x = (xy & 0xFFFF) + psx_gpu->offset_x;
        vertexes[1].y = (xy >> 16) + psx_gpu->offset_y;
      
        color = list_position[0];
        while(1)
        {
          xy = list_position[1];

          vertexes[0] = vertexes[1];

          vertexes[1].r = color & 0xFF;
          vertexes[1].g = (color >> 8) & 0xFF;
          vertexes[1].b = (color >> 16) & 0xFF;
          vertexes[1].x = (xy & 0xFFFF) + psx_gpu->offset_x;
          vertexes[1].y = (xy >> 16) + psx_gpu->offset_y;

          enhancement_disable();
          render_line(psx_gpu, vertexes, current_command, 0, 0);
          enhancement_enable();
          render_line(psx_gpu, vertexes, current_command, 0, 1);

          list_position += 2;
          num_vertexes++;

          if(list_position >= list_end)
          {
            current_command = (u32)-1;
            goto breakloop;
          }

          color = list_position[0];
          if((color & 0xF000F000) == 0x50005000)
            break;
        }

        command_length += ((num_vertexes - 2) * 2);
        break;
      }
  
      case 0x60 ... 0x63:
      {        
        u32 x = sign_extend_11bit(list_s16[2] + psx_gpu->offset_x);
        u32 y = sign_extend_11bit(list_s16[3] + psx_gpu->offset_y);
        u32 width = list_s16[4] & 0x3FF;
        u32 height = list_s16[5] & 0x1FF;

        render_sprite(psx_gpu, x, y, 0, 0, width, height, current_command, list[0]);

        if (check_enhanced_range(psx_gpu, x, x + width))
          do_sprite_enhanced(psx_gpu, x, y, 0, 0, width, height, list[0]);
        break;
      }
  
      case 0x64 ... 0x67:
      {        
        u32 x = sign_extend_11bit(list_s16[2] + psx_gpu->offset_x);
        u32 y = sign_extend_11bit(list_s16[3] + psx_gpu->offset_y);
        u8 u = list_s16[4];
        u8 v = list_s16[4] >> 8;
        u32 width = list_s16[6] & 0x3FF;
        u32 height = list_s16[7] & 0x1FF;

        set_clut(psx_gpu, list_s16[5]);

        render_sprite(psx_gpu, x, y, u, v, width, height,
         current_command, list[0]);

        if (check_enhanced_range(psx_gpu, x, x + width))
          do_sprite_enhanced(psx_gpu, x, y, u, v, width, height, list[0]);
        break;
      }
  
      case 0x68:
      case 0x69:
      case 0x6A:
      case 0x6B:
      {
        s32 x = sign_extend_11bit(list_s16[2] + psx_gpu->offset_x);
        s32 y = sign_extend_11bit(list_s16[3] + psx_gpu->offset_y);

        render_sprite(psx_gpu, x, y, 0, 0, 1, 1, current_command, list[0]);

        if (check_enhanced_range(psx_gpu, x, x + 1))
          do_sprite_enhanced(psx_gpu, x, y, 0, 0, 1, 1, list[0]);
        break;
      }
  
      case 0x70:
      case 0x71:
      case 0x72:
      case 0x73:
      {        
        s32 x = sign_extend_11bit(list_s16[2] + psx_gpu->offset_x);
        s32 y = sign_extend_11bit(list_s16[3] + psx_gpu->offset_y);

        render_sprite(psx_gpu, x, y, 0, 0, 8, 8, current_command, list[0]);

        if (check_enhanced_range(psx_gpu, x, x + 8))
          do_sprite_enhanced(psx_gpu, x, y, 0, 0, 8, 8, list[0]);
        break;
      }
  
      case 0x74:
      case 0x75:
      case 0x76:
      case 0x77:
      {        
        s32 x = sign_extend_11bit(list_s16[2] + psx_gpu->offset_x);
        s32 y = sign_extend_11bit(list_s16[3] + psx_gpu->offset_y);
        u8 u = list_s16[4];
        u8 v = list_s16[4] >> 8;

        set_clut(psx_gpu, list_s16[5]);

        render_sprite(psx_gpu, x, y, u, v, 8, 8,
         current_command, list[0]);

        if (check_enhanced_range(psx_gpu, x, x + 8))
          do_sprite_enhanced(psx_gpu, x, y, u, v, 8, 8, list[0]);
        break;
      }
  
      case 0x78:
      case 0x79:
      case 0x7A:
      case 0x7B:
      {        
        s32 x = sign_extend_11bit(list_s16[2] + psx_gpu->offset_x);
        s32 y = sign_extend_11bit(list_s16[3] + psx_gpu->offset_y);

        render_sprite(psx_gpu, x, y, 0, 0, 16, 16, current_command, list[0]);

        if (check_enhanced_range(psx_gpu, x, x + 16))
          do_sprite_enhanced(psx_gpu, x, y, 0, 0, 16, 16, list[0]);
        break;
      }
  
      case 0x7C:
      case 0x7D:
      case 0x7E:
      case 0x7F:
      {        
        s32 x = sign_extend_11bit(list_s16[2] + psx_gpu->offset_x);
        s32 y = sign_extend_11bit(list_s16[3] + psx_gpu->offset_y);
        u8 u = list_s16[4];
        u8 v = list_s16[4] >> 8;

        set_clut(psx_gpu, list_s16[5]);

        render_sprite(psx_gpu, x, y, u, v, 16, 16, current_command, list[0]);

        if (check_enhanced_range(psx_gpu, x, x + 16))
          do_sprite_enhanced(psx_gpu, x, y, u, v, 16, 16, list[0]);
        break;
      }

      case 0x80 ... 0x9F:          //  vid -> vid
      case 0xA0 ... 0xBF:          //  sys -> vid
      case 0xC0 ... 0xDF:          //  vid -> sys
        goto breakloop;

      case 0xE1:
        set_texture(psx_gpu, list[0]);

        if(list[0] & (1 << 9))
          psx_gpu->render_state_base |= RENDER_STATE_DITHER;
        else
          psx_gpu->render_state_base &= ~RENDER_STATE_DITHER;

        psx_gpu->display_area_draw_enable = (list[0] >> 10) & 0x1;
        SET_Ex(1, list[0]);
        break;
  
      case 0xE2:
      {
        // TODO: Clean
        u32 texture_window_settings = list[0];
        u32 tmp, x, y, w, h;

        if(texture_window_settings != psx_gpu->texture_window_settings)
        {
          tmp = (texture_window_settings & 0x1F) | 0x20;
          for(w = 8; (tmp & 1) == 0; tmp >>= 1, w <<= 1);

          tmp = ((texture_window_settings >> 5) & 0x1f) | 0x20;
          for (h = 8; (tmp & 1) == 0; tmp >>= 1, h <<= 1);

          tmp = 32 - (w >> 3);
          x = ((texture_window_settings >> 10) & tmp) << 3;

          tmp = 32 - (h >> 3);
          y = ((texture_window_settings >> 15) & tmp) << 3;

          flush_render_block_buffer(psx_gpu);
          
          psx_gpu->texture_window_settings = texture_window_settings;
          psx_gpu->texture_window_x = x;
          psx_gpu->texture_window_y = y;
          psx_gpu->texture_mask_width = w - 1;
          psx_gpu->texture_mask_height = h - 1;

          update_texture_ptr(psx_gpu);
        }
        SET_Ex(2, list[0]);
        break;
      }
  
      case 0xE3:
      {
        s16 viewport_start_x = list[0] & 0x3FF;
        s16 viewport_start_y = (list[0] >> 10) & 0x1FF;
        u32 w;
        s32 d;

        if(viewport_start_x == psx_gpu->viewport_start_x &&
         viewport_start_y == psx_gpu->viewport_start_y)
        {
          break;
        }
        psx_gpu->viewport_start_x = viewport_start_x;
        psx_gpu->viewport_start_y = viewport_start_y;
        psx_gpu->saved_viewport_start_x = viewport_start_x;
        psx_gpu->saved_viewport_start_y = viewport_start_y;

        w = (u32)psx_gpu->viewport_end_x - (u32)viewport_start_x + 1;
        d = psx_gpu->saved_hres - w;
        if(-16 <= d && d <= 16)
        {
          update_enhancement_buf_table_from_x(psx_gpu,
           viewport_start_x, w);
        }
        select_enhancement_buf(psx_gpu);

#ifdef TEXTURE_CACHE_4BPP
        psx_gpu->viewport_mask =
         texture_region_mask(psx_gpu->viewport_start_x,
         psx_gpu->viewport_start_y, psx_gpu->viewport_end_x,
         psx_gpu->viewport_end_y);
#endif
        SET_Ex(3, list[0]);
        break;
      }

      case 0xE4:
      {
        s16 viewport_end_x = list[0] & 0x3FF;
        s16 viewport_end_y = (list[0] >> 10) & 0x1FF;
        u32 w;
        s32 d;

        if(viewport_end_x == psx_gpu->viewport_end_x &&
         viewport_end_y == psx_gpu->viewport_end_y)
        {
          break;
        }

        psx_gpu->viewport_end_x = viewport_end_x;
        psx_gpu->viewport_end_y = viewport_end_y;
        psx_gpu->saved_viewport_end_x = viewport_end_x;
        psx_gpu->saved_viewport_end_y = viewport_end_y;

        w = (u32)viewport_end_x - (u32)psx_gpu->viewport_start_x + 1;
        d = psx_gpu->saved_hres - w;
        if(-16 <= d && d <= 16)
        {
          update_enhancement_buf_table_from_x(psx_gpu,
           psx_gpu->viewport_start_x, w);
        }
        select_enhancement_buf(psx_gpu);

#ifdef TEXTURE_CACHE_4BPP
        psx_gpu->viewport_mask =
         texture_region_mask(psx_gpu->viewport_start_x,
         psx_gpu->viewport_start_y, psx_gpu->viewport_end_x,
         psx_gpu->viewport_end_y);
#endif
        SET_Ex(4, list[0]);
        break;
      }
  
      case 0xE5:
      {
        s32 offset_x = list[0] << 21;
        s32 offset_y = list[0] << 10;
        psx_gpu->offset_x = offset_x >> 21;
        psx_gpu->offset_y = offset_y >> 21; 
  
        SET_Ex(5, list[0]);
        break;
      }

      case 0xE6:
      {
        u32 mask_settings = list[0];
        u16 mask_msb = mask_settings << 15;

        if(list[0] & 0x2)
          psx_gpu->render_state_base |= RENDER_STATE_MASK_EVALUATE;
        else
          psx_gpu->render_state_base &= ~RENDER_STATE_MASK_EVALUATE;

        if(mask_msb != psx_gpu->mask_msb)
        {
          flush_render_block_buffer(psx_gpu);
          psx_gpu->mask_msb = mask_msb;
        }

        SET_Ex(6, list[0]);
        break;
      }
  
      default:
        break;
    }
  }

  enhancement_disable();

breakloop:
  if (last_command != NULL)
    *last_command = current_command;
  return list - list_start;
}

#endif /* PCSX */

// vim:ts=2:shiftwidth=2:expandtab
