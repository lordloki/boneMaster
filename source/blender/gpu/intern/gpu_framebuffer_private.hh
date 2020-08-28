/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2020 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup gpu
 *
 * GPU Framebuffer
 * - this is a wrapper for an OpenGL framebuffer object (FBO). in practice
 *   multiple FBO's may be created.
 * - actual FBO creation & config is deferred until GPU_framebuffer_bind or
 *   GPU_framebuffer_check_valid to allow creation & config while another
 *   opengl context is bound (since FBOs are not shared between ogl contexts).
 */

#pragma once

#include "BLI_span.hh"

#include "MEM_guardedalloc.h"

#include "GPU_framebuffer.h"

struct GPUTexture;

typedef enum GPUAttachmentType : int {
  GPU_FB_DEPTH_ATTACHMENT = 0,
  GPU_FB_DEPTH_STENCIL_ATTACHMENT,
  GPU_FB_COLOR_ATTACHMENT0,
  GPU_FB_COLOR_ATTACHMENT1,
  GPU_FB_COLOR_ATTACHMENT2,
  GPU_FB_COLOR_ATTACHMENT3,
  GPU_FB_COLOR_ATTACHMENT4,
  GPU_FB_COLOR_ATTACHMENT5,
  /* Number of maximum output slots.
   * We support 6 outputs for now (usually we wouldn't need more to preserve fill rate). */
  /* Keep in mind that GL max is GL_MAX_DRAW_BUFFERS and is at least 8, corresponding to
   * the maximum number of COLOR attachments specified by glDrawBuffers. */
  GPU_FB_MAX_ATTACHEMENT,

  GPU_FB_MAX_COLOR_ATTACHMENT = (GPU_FB_MAX_ATTACHEMENT - GPU_FB_COLOR_ATTACHMENT0),
} GPUAttachmentType;

inline constexpr GPUAttachmentType operator-(GPUAttachmentType a, int b)
{
  return static_cast<GPUAttachmentType>(static_cast<int>(a) - b);
}

inline constexpr GPUAttachmentType operator+(GPUAttachmentType a, int b)
{
  return static_cast<GPUAttachmentType>(static_cast<int>(a) + b);
}

inline GPUAttachmentType &operator++(GPUAttachmentType &a)
{
  a = a + 1;
  return a;
}

inline GPUAttachmentType &operator--(GPUAttachmentType &a)
{
  a = a - 1;
  return a;
}

namespace blender {
namespace gpu {

#ifdef DEBUG
#  define DEBUG_NAME_LEN 64
#else
#  define DEBUG_NAME_LEN 16
#endif

class FrameBuffer {
 protected:
  /** Set of texture attachements to render to. DEPTH and DEPTH_STENCIL are mutualy exclusive. */
  GPUAttachment attachments_[GPU_FB_MAX_ATTACHEMENT];
  /** Is true if internal representation need to be updated. */
  bool dirty_attachments_;
  /** Size of attachement textures. */
  int width_, height_;
  /** Debug name. */
  char name_[DEBUG_NAME_LEN];

 public:
  FrameBuffer(const char *name);
  virtual ~FrameBuffer();

  virtual void bind(bool enabled_srgb) = 0;
  virtual bool check(char err_out[256]) = 0;
  virtual void clear(eGPUFrameBufferBits buffers,
                     const float clear_col[4],
                     float clear_depth,
                     uint clear_stencil) = 0;
  virtual void clear_multi(const float (*clear_col)[4]) = 0;

  virtual void read(eGPUFrameBufferBits planes,
                    eGPUDataFormat format,
                    const int area[4],
                    int channel_len,
                    int slot,
                    void *r_data) = 0;

  virtual void blit_to(eGPUFrameBufferBits planes,
                       int src_slot,
                       FrameBuffer *dst,
                       int dst_slot,
                       int dst_offset_x,
                       int dst_offset_y) = 0;

  void attachment_set(GPUAttachmentType type, const GPUAttachment &new_attachment);

  void recursive_downsample(int max_lvl,
                            void (*callback)(void *userData, int level),
                            void *userData);

  inline GPUTexture *depth_tex(void) const
  {
    if (attachments_[GPU_FB_DEPTH_ATTACHMENT].tex) {
      return attachments_[GPU_FB_DEPTH_ATTACHMENT].tex;
    }
    return attachments_[GPU_FB_DEPTH_STENCIL_ATTACHMENT].tex;
  };

  inline GPUTexture *color_tex(int slot) const
  {
    return attachments_[GPU_FB_COLOR_ATTACHMENT0 + slot].tex;
  };
};

#undef DEBUG_NAME_LEN

}  // namespace gpu
}  // namespace blender