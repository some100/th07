#include "TextHelper.hpp"

#include <d3d8.h>
#include <d3dx8tex.h>
#include <wingdi.h>

#include "Supervisor.hpp"
#include "dxutil.hpp"
#include "inttypes.hpp"

// GLOBAL: TH07 0x0049fe28
IDirect3DSurface8 *g_TextBufferSurface;

// GLOBAL: TH07 0x0049ed98
FormatInfo g_FormatInfoArray[7] = {
    {
        D3DFMT_X8R8G8B8,
        0x20,
        0,
        0x00FF0000,
        0x0000FF00,
        0x000000FF,
    },
    {
        D3DFMT_A8R8G8B8,
        0x20,
        0xFF000000,
        0x00FF0000,
        0x0000FF00,
        0x000000FF,
    },
    {
        D3DFMT_X1R5G5B5,
        0x10,
        0,
        0x00007C00,
        0x000003E0,
        0x0000001F,
    },
    {
        D3DFMT_R5G6B5,
        0x10,
        0,
        0x0000F800,
        0x000007E0,
        0x0000001F,
    },
    {
        D3DFMT_A1R5G5B5,
        0x10,
        0x00008000,
        0x00007C00,
        0x000003E0,
        0x0000001F,
    },
    {
        D3DFMT_A4R4G4B4,
        0x10,
        0x0000F000,
        0x00000F00,
        0x000000F0,
        0x0000000F,
    },
    {
        (D3DFORMAT)-1,
        0x0,
        0,
        0,
        0,
        0,
    },
};

// FUNCTION: TH07 0x00431a0f
TextHelper::TextHelper()

{
  this->format = (D3DFORMAT)-1;
  this->width = 0;
  this->height = 0;
  this->hdc = NULL;
  this->gdiobj2 = NULL;
  this->gdiobj = NULL;
  this->buffer = NULL;
}

// FUNCTION: TH07 0x00431a4b
TextHelper::~TextHelper() { ReleaseBuffer(); }

// FUNCTION: TH07 0x00431a5c
bool TextHelper::ReleaseBuffer()

{
  bool isNotNull = this->hdc != NULL;
  if (isNotNull) {
    SelectObject((HDC)this->hdc, this->gdiobj);
    DeleteDC((HDC)this->hdc);
    DeleteObject(this->gdiobj2);
    this->format = (D3DFORMAT)-1;
    this->width = 0;
    this->height = 0;
    this->hdc = NULL;
    this->gdiobj2 = NULL;
    this->gdiobj = NULL;
    this->buffer = NULL;
  }
  return isNotNull;
}

// FUNCTION: TH07 0x00431ace
bool TextHelper::AllocateBufferWithFallback(i32 width, i32 height,
                                            D3DFORMAT format)

{
  bool bVar1;

  bVar1 = TryAllocateBuffer(width, height, format);
  if (bVar1) {
    bVar1 = true;
  } else if ((format == D3DFMT_A1R5G5B5) || (format == D3DFMT_A4R4G4B4)) {
    bVar1 = TryAllocateBuffer(width, height, D3DFMT_A8R8G8B8);
  } else if (format == D3DFMT_R5G6B5) {
    bVar1 = TryAllocateBuffer(width, height, D3DFMT_X8R8G8B8);
  } else {
    bVar1 = false;
  }
  return bVar1;
}

struct ThBitmapInfo {
  BITMAPINFOHEADER bmiHeader;
  RGBQUAD bmiColors[17];
};

// FUNCTION: TH07 0x00431b2d
bool TextHelper::TryAllocateBuffer(i32 width, i32 height, D3DFORMAT format)

{
  bool bVar1;
  i32 imageWidthInBytes;
  HDC hdc;
  HGDIOBJ pvVar2;
  u16 *bitmapData;
  HBITMAP bitmapObj;
  FormatInfo *formatInfo;
  ThBitmapInfo bitmapInfo;

  ReleaseBuffer();
  memset(&bitmapInfo, 0, 0x6c);
  formatInfo = GetFormatInfo(format);
  if (formatInfo == NULL) {
    bVar1 = false;
  } else {
    imageWidthInBytes = (((width * formatInfo->bitCount) / 8 + 3) / 4) * 4;
    bitmapInfo.bmiHeader.biSize = 0x6c;
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -(height + 1);
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = (WORD)formatInfo->bitCount;
    bitmapInfo.bmiHeader.biSizeImage = height * imageWidthInBytes;
    if ((format != D3DFMT_X1R5G5B5) && (format != D3DFMT_X8R8G8B8)) {
      bitmapInfo.bmiHeader.biCompression = 3;
      ((u32 *)bitmapInfo.bmiColors)[0] = formatInfo->redMask;
      ((u32 *)bitmapInfo.bmiColors)[1] = formatInfo->greenMask;
      ((u32 *)bitmapInfo.bmiColors)[2] = formatInfo->blueMask;
      ((u32 *)bitmapInfo.bmiColors)[3] = formatInfo->alphaMask;
    }
    bitmapObj = CreateDIBSection(NULL, (BITMAPINFO *)&bitmapInfo, 0,
                                 (void **)&bitmapData, NULL, 0);
    if (bitmapObj == NULL) {
      bVar1 = false;
    } else {
      memset(bitmapData, 0, bitmapInfo.bmiHeader.biSizeImage);
      hdc = CreateCompatibleDC(NULL);
      pvVar2 = SelectObject(hdc, bitmapObj);
      this->hdc = (HDC)hdc;
      this->gdiobj2 = bitmapObj;
      this->buffer = bitmapData;
      this->imageSizeInBytes = bitmapInfo.bmiHeader.biSizeImage;
      this->gdiobj = pvVar2;
      this->width = width;
      this->height = height;
      this->format = format;
      this->imageWidthInBytes = imageWidthInBytes;
      bVar1 = true;
    }
  }
  return bVar1;
}

// FUNCTION: TH07 0x00431cec
FormatInfo *TextHelper::GetFormatInfo(D3DFORMAT format)

{
  FormatInfo *ret;
  i32 i;

  for (i = 0; (g_FormatInfoArray[i].format != -1 &&
               (g_FormatInfoArray[i].format != format));
       i = i + 1) {
  }
  if (format == -1) {
    ret = NULL;
  } else {
    ret = g_FormatInfoArray + i;
  }
  return ret;
}

struct A1R5G5B5 {
  u16 blue : 5;
  u16 green : 5;
  u16 red : 5;
  u16 alpha : 1;
};

// FUNCTION: TH07 0x00431d3c
bool TextHelper::InvertAlpha(i32 x, i32 y, i32 spriteWidth, i32 fontHeight,
                             i32 param5)

{
  i32 doubleArea;
  u16 *bufferRegion;
  i32 idx;
  A1R5G5B5 *bufferCursor;

  i32 tempColor;

  doubleArea = spriteWidth * fontHeight * 2;
  bufferRegion = &this->buffer[y * spriteWidth];
  switch (this->format) {
  case D3DFMT_A8R8G8B8:
    for (idx = 3; idx < doubleArea; idx += 4) {
      ((u8 *)bufferRegion)[idx] ^= 0xff;
    }
    break;
  case D3DFMT_A1R5G5B5:
    for (bufferCursor = (A1R5G5B5 *)bufferRegion, idx = 0; idx < doubleArea;
         idx += 2, bufferCursor += 1) {
      bufferCursor->alpha ^= 1;
      if (bufferCursor->alpha) {
        if (!param5) {
          if (bufferCursor->red < bufferCursor->blue) {
            tempColor =
                bufferCursor->blue - bufferCursor->blue * idx / doubleArea / 2;
            bufferCursor->blue = tempColor & 0x1f;

            tempColor = bufferCursor->green -
                        bufferCursor->green * idx / doubleArea / 2;
            bufferCursor->green = tempColor & 0x1f;
          } else {
            tempColor = bufferCursor->red -
                        bufferCursor->red * idx * 2 / doubleArea / 3;
            bufferCursor->red = tempColor & 0x1f;

            tempColor = bufferCursor->green -
                        bufferCursor->green * idx * 2 / doubleArea / 3;
            bufferCursor->green = tempColor & 0x1f;
          }
        } else {
          if (bufferCursor->red < bufferCursor->blue) {
            tempColor =
                bufferCursor->red - bufferCursor->red * idx / doubleArea / 4;
            bufferCursor->red = tempColor & 0x1f;

            tempColor = bufferCursor->green -
                        bufferCursor->green * idx / doubleArea / 4;
            bufferCursor->green = tempColor & 0x1f;
          } else {
            tempColor =
                bufferCursor->blue - bufferCursor->blue * idx / doubleArea / 4;
            bufferCursor->blue = tempColor & 0x1f;

            tempColor = bufferCursor->green -
                        bufferCursor->green * idx / doubleArea / 4;
            bufferCursor->green = tempColor & 0x1f;
          }
        }
      } else {
        bufferCursor->red = 0;
        bufferCursor->green = 0;
        bufferCursor->blue = 0;
      }
    }
    break;
  case D3DFMT_A4R4G4B4:
    for (idx = 1; idx < doubleArea; idx = idx + 2) {
      ((u8 *)bufferRegion)[idx] ^= 0xf0;
    }
    break;
  default:
    return false;
  }
  return true;
}

// FUNCTION: TH07 0x00432164
bool TextHelper::CopyTextToSurface(IDirect3DSurface8 *surface)

{
  D3DLOCKED_RECT lockedRect;
  u16 *srcBuf;
  D3DSURFACE_DESC format;
  size_t local_24;
  i32 i;
  RECT local_1c;
  i32 local_c;
  void *dstBuf;

  if (this->gdiobj2 == NULL) {
    return false;
  } else {
    surface->GetDesc(&format);
    local_1c.left = 0;
    local_1c.top = 0;
    local_1c.right = this->width;
    local_1c.bottom = this->height;
    if (surface->LockRect(&lockedRect, &local_1c, 0) == 0) {
      local_c = lockedRect.Pitch;
      local_24 = this->imageWidthInBytes;
      srcBuf = this->buffer;
      dstBuf = lockedRect.pBits;
      if (format.Format == this->format) {
        for (i = 0; i < this->height; i = i + 1) {
          memcpy(dstBuf, srcBuf, local_24);
          srcBuf = (u16 *)((i32)srcBuf + local_24);
          dstBuf = (void *)((i32)dstBuf + local_c);
        }
      }
      surface->UnlockRect();
      return true;
    } else {
      return false;
    }
  }
}

// FUNCTION: TH07 0x0043225b
void TextHelper::CreateTextBuffer()

{
  g_Supervisor.d3dDevice->CreateImageSurface(0x400, 0x40, D3DFMT_A1R5G5B5,
                                             &g_TextBufferSurface);
}

// FUNCTION: TH07 0x0043227e
void TextHelper::ReleaseTextBuffer()

{
  SAFE_RELEASE(g_TextBufferSurface);
}

// FUNCTION: TH07 0x004322a3
void TextHelper::RenderTextToTextureBold(i32 xPos, i32 yPos, i32 spriteWidth,
                                         i32 spriteHeight, i32 fontHeight,
                                         i32 fontWidth, D3DCOLOR textColor,
                                         u32 outlineType, char *string,
                                         IDirect3DTexture8 *outTexture)

{
  IDirect3DSurface8 *dstSurface;
  RECT dstRect;
  RECT srcRect;
  HGDIOBJ h;
  D3DSURFACE_DESC textSurfaceDesc;
  HFONT hfont;
  HDC hdc;

  hfont =
      CreateFontA(fontHeight * 2 - 2, 0, 0, 0, FW_BOLD, false, false, false,
                  SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                  ANTIALIASED_QUALITY, FF_ROMAN | FIXED_PITCH, "ＭＳ ゴシック");
  TextHelper textHelper;
  g_TextBufferSurface->GetDesc(&textSurfaceDesc);
  textHelper.AllocateBufferWithFallback(
      textSurfaceDesc.Width, textSurfaceDesc.Height, textSurfaceDesc.Format);
  hdc = (HDC)textHelper.hdc;
  h = SelectObject((HDC)textHelper.hdc, hfont);
  textHelper.InvertAlpha(0, 0, spriteWidth << 1, fontHeight * 2 + 6, 0);
  SetBkMode(hdc, 1);
  if (outlineType == 0xffffffff) {
    SetTextColor(hdc, 0);
    TextOutA(hdc, xPos * 2 + 3, 2, string, strlen(string));
    TextOutA(hdc, xPos * 2 + 1, 2, string, strlen(string));
    TextOutA(hdc, xPos * 2 + 2, 1, string, strlen(string));
    TextOutA(hdc, xPos * 2 + 2, 3, string, strlen(string));
  } else {
    SetTextColor(hdc, 0);
    TextOutA(hdc, xPos * 2 + 4, 2, string, strlen(string));
    TextOutA(hdc, xPos << 1, 2, string, strlen(string));
    TextOutA(hdc, xPos * 2 + 2, 0, string, strlen(string));
    TextOutA(hdc, xPos * 2 + 2, 4, string, strlen(string));
  }
  SetTextColor(hdc, textColor);
  TextOutA(hdc, xPos * 2 + 2, 2, string, strlen(string));
  SelectObject(hdc, h);
  textHelper.InvertAlpha(0, 0, spriteWidth << 1, fontHeight * 2 + 6,
                         (u32)(outlineType == 0xffffffff));
  textHelper.CopyTextToSurface(g_TextBufferSurface);
  SelectObject(hdc, h);
  DeleteObject(hfont);
  dstRect.left = 0;
  dstRect.right = spriteWidth;
  dstRect.bottom = yPos + fontWidth;
  srcRect.left = 0;
  srcRect.top = 0;
  srcRect.right = spriteWidth << 1;
  srcRect.bottom = fontHeight << 1;
  if (1024 < srcRect.right) {
    srcRect.right = 1024;
  }
  dstRect.top = yPos;
  outTexture->GetSurfaceLevel(0, &dstSurface);
  D3DXLoadSurfaceFromSurface(dstSurface, 0, &dstRect, g_TextBufferSurface, 0,
                             &srcRect, 4, 0);
  SAFE_RELEASE(dstSurface);
}
