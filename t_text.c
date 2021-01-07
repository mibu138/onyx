#include "t_text.h"
#include <tanto/v_image.h>
#include <tanto/v_memory.h>
#include <ft2build.h>

#include FT_FREETYPE_H

FT_Library    library;
FT_Face       face;
FT_Matrix     matrix;                 /* transformation matrix */
FT_Vector     pen;                    /* untransformed origin  */
FT_Error      error;

/* origin is the upper left corner */

static void draw_bitmap( FT_Bitmap*  bitmap,
             FT_Int      x,
             FT_Int      y,
             const unsigned WIDTH,
             const unsigned HEIGHT,
             uint8_t* image)
{
  FT_Int  i, j, p, q;
  FT_Int  x_max = x + bitmap->width;
  FT_Int  y_max = y + bitmap->rows;

  /* for simplicity, we assume that `bitmap->pixel_mode' */
  /* is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)   */

  for ( i = x, p = 0; i < x_max; i++, p++ )
  {
    for ( j = y, q = 0; j < y_max; j++, q++ )
    {
      if ( i < 0      || j < 0       ||
           i >= WIDTH || j >= HEIGHT )
        continue;

      image[j * WIDTH + i] |= bitmap->buffer[q * bitmap->width + p];
    }
  }
}

static void drawString(const char* string, const size_t width, const size_t height, 
        const size_t x, const size_t y, uint8_t* buffer)
{
    pen.x = 0; pen.y = 0;
    matrix.xx = (FT_Fixed)1 * 0x10000L;
    matrix.xy = (FT_Fixed)0 * 0x10000L;
    matrix.yx = (FT_Fixed)0 * 0x10000L;
    matrix.yy = (FT_Fixed)1 * 0x10000L;


    const int len = strlen(string);
    for (int i = 0; i < len; i++) 
    {
        FT_Set_Transform(face, &matrix, &pen);

        error = FT_Load_Char(face, string[i], FT_LOAD_RENDER);
        assert(!error);

        //assert(face->glyph->bitmap.pitch > 0); // indicates buffer starts at top left pixel

        draw_bitmap(&face->glyph->bitmap, 
                x + face->glyph->bitmap_left, 
                y - face->glyph->bitmap_top,
                width, height, buffer);

        pen.x += face->glyph->advance.x;
    }
}

Tanto_V_Image tanto_CreateTextImage(const size_t width, const size_t height, 
        const size_t x, const size_t y,
        const size_t fontSize, const char* text)
{
    static bool initialized = false;
    if (!initialized)
    {
        error = FT_Init_FreeType(&library);
        assert(!error);
        error = FT_New_Face(library, "/usr/share/fonts/truetype/freefont/FreeMono.ttf", 0, &face);
        assert(!error);
        error = FT_Set_Pixel_Sizes(face, 0, fontSize);
        assert(!error);
        initialized = true;
    }

    Tanto_V_Image image = tanto_v_CreateImageAndSampler(width, height, VK_FORMAT_R8_UINT, 
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 
            VK_IMAGE_ASPECT_COLOR_BIT, 
            VK_SAMPLE_COUNT_1_BIT,
            VK_FILTER_NEAREST); 

    tanto_v_TransitionImageLayout(image.layout, VK_IMAGE_LAYOUT_GENERAL, &image);

    Tanto_V_BufferRegion region = tanto_v_RequestBufferRegion(width * height, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            TANTO_V_MEMORY_HOST_TYPE);

    drawString(text, width, height, x, y, region.hostData);

    tanto_v_CopyBufferToImage(&region, &image);

    tanto_v_FreeBufferRegion(&region);

    return image;
}
