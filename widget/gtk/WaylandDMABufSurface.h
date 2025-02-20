/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WaylandDMABufSurface_h__
#define WaylandDMABufSurface_h__

#include <stdint.h>
#include "GLContext.h"
#include "GLContextTypes.h"
#include "mozilla/widget/nsWaylandDisplay.h"
#include "mozilla/widget/va_drmcommon.h"

typedef void* EGLImageKHR;
typedef void* EGLSyncKHR;

#define DMABUF_BUFFER_PLANES 4

namespace mozilla {
namespace layers {
class SurfaceDescriptor;
class SurfaceDescriptorDMABuf;
}  // namespace layers
}  // namespace mozilla

typedef enum {
  // Use alpha pixel format
  DMABUF_ALPHA = 1 << 0,
  // Surface is used as texture and may be also shared
  DMABUF_TEXTURE = 1 << 1,
  // Automatically create wl_buffer / EGLImage in Create routines.
  DMABUF_CREATE_WL_BUFFER = 1 << 2,
  // Use modifiers. Such dmabuf surface may have more planes
  // and complex internal structure (tiling/compression/etc.)
  // so we can't do direct rendering to it.
  DMABUF_USE_MODIFIERS = 1 << 3,
} WaylandDMABufSurfaceFlags;

class WaylandDMABufSurfaceRGBA;

class WaylandDMABufSurface {
 public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(WaylandDMABufSurface)

  enum SurfaceType {
    SURFACE_RGBA,
    SURFACE_NV12,
  };

  static already_AddRefed<WaylandDMABufSurface> CreateDMABufSurface(
      const mozilla::layers::SurfaceDescriptor& aDesc);

  virtual bool Serialize(
      mozilla::layers::SurfaceDescriptor& aOutDescriptor) = 0;

  virtual int GetWidth(int aPlane = 0) = 0;
  virtual int GetHeight(int aPlane = 0) = 0;
  virtual mozilla::gfx::SurfaceFormat GetFormat() = 0;
  virtual mozilla::gfx::SurfaceFormat GetFormatGL() = 0;

  virtual bool CreateTexture(mozilla::gl::GLContext* aGLContext,
                             int aPlane = 0) = 0;
  virtual void ReleaseTextures() = 0;
  virtual GLuint GetTexture(int aPlane = 0) = 0;
  virtual EGLImageKHR GetEGLImage(int aPlane = 0) = 0;

  SurfaceType GetSurfaceType() { return mSurfaceType; };
  virtual uint32_t GetTextureCount() = 0;

  virtual WaylandDMABufSurfaceRGBA* GetAsWaylandDMABufSurfaceRGBA() {
    return nullptr;
  }

  virtual mozilla::gfx::YUVColorSpace GetYUVColorSpace() {
    return mozilla::gfx::YUVColorSpace::UNKNOWN;
  };
  virtual bool IsFullRange() { return false; };

  void FenceSet();
  void FenceWait();
  void FenceDelete();

  WaylandDMABufSurface(SurfaceType aSurfaceType)
      : mSurfaceType(aSurfaceType),
        mBufferModifier(0),
        mBufferPlaneCount(0),
        mStrides(),
        mOffsets(),
        mSync(0) {
    for (auto& slot : mDmabufFds) {
      slot = -1;
    }
  }

 protected:
  virtual bool Create(const mozilla::layers::SurfaceDescriptor& aDesc) = 0;
  virtual void ReleaseSurface() = 0;
  bool FenceCreate(int aFd);

  virtual ~WaylandDMABufSurface() { FenceDelete(); };

  SurfaceType mSurfaceType;
  uint64_t mBufferModifier;

  int mBufferPlaneCount;
  int mDmabufFds[DMABUF_BUFFER_PLANES];
  uint32_t mStrides[DMABUF_BUFFER_PLANES];
  uint32_t mOffsets[DMABUF_BUFFER_PLANES];

  EGLSyncKHR mSync;
  RefPtr<mozilla::gl::GLContext> mGL;
};

class WaylandDMABufSurfaceRGBA : public WaylandDMABufSurface {
 public:
  static already_AddRefed<WaylandDMABufSurfaceRGBA> CreateDMABufSurface(
      int aWidth, int aHeight, int aWaylandDMABufSurfaceFlags);

  bool Serialize(mozilla::layers::SurfaceDescriptor& aOutDescriptor);

  WaylandDMABufSurfaceRGBA* GetAsWaylandDMABufSurfaceRGBA() { return this; }

  bool Resize(int aWidth, int aHeight);
  void Clear();

  bool CopyFrom(class WaylandDMABufSurface* aSourceSurface);

  int GetWidth(int aPlane = 0) { return mWidth; };
  int GetHeight(int aPlane = 0) { return mHeight; };
  mozilla::gfx::SurfaceFormat GetFormat();
  mozilla::gfx::SurfaceFormat GetFormatGL();
  bool HasAlpha();

  void* MapReadOnly(uint32_t aX, uint32_t aY, uint32_t aWidth, uint32_t aHeight,
                    uint32_t* aStride = nullptr);
  void* MapReadOnly(uint32_t* aStride = nullptr);
  void* Map(uint32_t aX, uint32_t aY, uint32_t aWidth, uint32_t aHeight,
            uint32_t* aStride = nullptr);
  void* Map(uint32_t* aStride = nullptr);
  void* GetMappedRegion() { return mMappedRegion; };
  uint32_t GetMappedRegionStride() { return mMappedRegionStride; };
  bool IsMapped() { return (mMappedRegion != nullptr); };
  void Unmap();

  bool CreateTexture(mozilla::gl::GLContext* aGLContext, int aPlane = 0);
  void ReleaseTextures();
  GLuint GetTexture(int aPlane = 0) { return mTexture; };
  EGLImageKHR GetEGLImage(int aPlane = 0) { return mEGLImage; };

  uint32_t GetTextureCount() { return 1; };

  void SetWLBuffer(struct wl_buffer* aWLBuffer);
  wl_buffer* GetWLBuffer();
  void WLBufferDetach() { mWLBufferAttached = false; };
  bool WLBufferIsAttached() { return mWLBufferAttached; };
  void WLBufferSetAttached() { mWLBufferAttached = true; };

  WaylandDMABufSurfaceRGBA();

 private:
  ~WaylandDMABufSurfaceRGBA();

  bool Create(int aWidth, int aHeight, int aWaylandDMABufSurfaceFlags);
  bool Create(const mozilla::layers::SurfaceDescriptor& aDesc);
  void ReleaseSurface();

  bool CreateWLBuffer();

  void FillFdData(struct gbm_import_fd_data& aData);
  void FillFdData(struct gbm_import_fd_modifier_data& aData);
  void ImportSurfaceDescriptor(const mozilla::layers::SurfaceDescriptor& aDesc);

  void* MapInternal(uint32_t aX, uint32_t aY, uint32_t aWidth, uint32_t aHeight,
                    uint32_t* aStride, int aGbmFlags);

 private:
  int mSurfaceFlags;

  int mWidth;
  int mHeight;
  mozilla::widget::GbmFormat* mGmbFormat;

  wl_buffer* mWLBuffer;
  void* mMappedRegion;
  uint32_t mMappedRegionStride;

  struct gbm_bo* mGbmBufferObject;
  uint32_t mGbmBufferFlags;

  EGLImageKHR mEGLImage;
  GLuint mTexture;

  bool mWLBufferAttached;
  bool mFastWLBufferCreation;
};

class WaylandDMABufSurfaceNV12 : public WaylandDMABufSurface {
 public:
  static already_AddRefed<WaylandDMABufSurfaceNV12> CreateNV12Surface(
      const VADRMPRIMESurfaceDescriptor& aDesc);

  bool Create(const VADRMPRIMESurfaceDescriptor& aDesc);

  bool Serialize(mozilla::layers::SurfaceDescriptor& aOutDescriptor);

  int GetWidth(int aPlane = 0) { return mWidth[aPlane]; }
  int GetHeight(int aPlane = 0) { return mHeight[aPlane]; }
  mozilla::gfx::SurfaceFormat GetFormat();
  mozilla::gfx::SurfaceFormat GetFormatGL();

  bool CreateTexture(mozilla::gl::GLContext* aGLContext, int aPlane = 0);
  void ReleaseTextures();
  GLuint GetTexture(int aPlane = 0) { return mTexture[aPlane]; };
  EGLImageKHR GetEGLImage(int aPlane = 0) { return mEGLImage[aPlane]; };

  uint32_t GetTextureCount() { return 2; };

  void SetYUVColorSpace(mozilla::gfx::YUVColorSpace aColorSpace) {
    mColorSpace = aColorSpace;
  }
  mozilla::gfx::YUVColorSpace GetYUVColorSpace() { return mColorSpace; }

  bool IsFullRange() { return true; }

  WaylandDMABufSurfaceNV12();

 private:
  ~WaylandDMABufSurfaceNV12();

  bool Create(const mozilla::layers::SurfaceDescriptor& aDesc);
  void ReleaseSurface();

  void ImportSurfaceDescriptor(
      const mozilla::layers::SurfaceDescriptorDMABuf& aDesc);

  mozilla::gfx::SurfaceFormat mSurfaceFormat;

  int mWidth[DMABUF_BUFFER_PLANES];
  int mHeight[DMABUF_BUFFER_PLANES];
  uint32_t mDrmFormats[DMABUF_BUFFER_PLANES];
  EGLImageKHR mEGLImage[DMABUF_BUFFER_PLANES];
  GLuint mTexture[DMABUF_BUFFER_PLANES];
  mozilla::gfx::YUVColorSpace mColorSpace;
};

#endif
