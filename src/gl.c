void InitGL(program_memory* Memory)
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  for (u32 Piece = NONE; Piece < CHESS_PIECE_COUNT; Piece++)
  {
    char BitmapPath[1024] = {0};
    PieceBitmap[Piece] = ReadBitmap(CatStrings(BitmapPath, 3,
          Memory->Platform.ExecutableDirectory, "../", (char*)PieceAsset[Piece]));
    PieceBitmap[Piece].Transform = FLIP_VERTICAL;
    glGenTextures(1, &PieceBitmap[Piece].TextureHandle);
    glBindTexture(GL_TEXTURE_2D, PieceBitmap[Piece].TextureHandle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PieceBitmap[Piece].Width, PieceBitmap[Piece].Height,
        0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, PieceBitmap[Piece].Pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  for (u32 Glyph = 0; Glyph < 256; Glyph++)
  {
    glGenTextures(1, &Memory->GlyphTable[Glyph].TextureHandle);
    glBindTexture(GL_TEXTURE_2D, Memory->GlyphTable[Glyph].TextureHandle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Memory->GlyphTable[Glyph].Width, Memory->GlyphTable[Glyph].Height,
        0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, Memory->GlyphTable[Glyph].Pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

}

void DrawRenderGroupOpenGL(render_group* RenderGroup)
{
  // NOTE(robin): Push-buffer based renderer. The program layer pushes primitives like RECT, LINE, BITMAP
  // into an array and we render the commands here with any backend we like.
  for (umm BaseAddress = 0; BaseAddress < RenderGroup->Used;)
  {
    render_entry_header* Header = (render_entry_header*)(RenderGroup->Buffer + BaseAddress);
    BaseAddress += sizeof(*Header);
    void* Data = (byte*)Header + sizeof(*Header);

    switch (Header->Type)
    {
      case RENDER_RECT:
      {
        render_entry_rect* Entry = (render_entry_rect*)Data;
        colour Colour = Entry->Colour;
        glColor4f(Colour.R/255.0, Colour.G/255.0, Colour.B/255.0, Colour.A/255.0);
        glRectf(Entry->Left, Entry->Top, Entry->Right, Entry->Bottom);
        BaseAddress += sizeof(*Entry);
      } break;

      case RENDER_LINE:
      {
        render_entry_line* Entry = (render_entry_line*)Data;
        colour Colour = Entry->Colour;
        glBegin(GL_LINES);
        glColor4f(Colour.R/255.0, Colour.G/255.0, Colour.B/255.0, Colour.A/255.0);
        glVertex2f(Entry->Point1.X, Entry->Point1.Y);
        glVertex2f(Entry->Point2.X, Entry->Point2.Y);
        glEnd();
        BaseAddress += sizeof(*Entry);
      } break;

      case RENDER_BITMAP:
      {
        render_entry_bitmap* Entry = (render_entry_bitmap*)Data;

        s32 Width = Entry->Bitmap->Width;
        s32 Height = Entry->Bitmap->Height;
        colour Colour = Entry->Colour;

        glEnable(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->TextureHandle);

        glBegin(GL_QUADS);
        glColor4f(Colour.R/255.0, Colour.G/255.0, Colour.B/255.0, Colour.A/255.0);
        switch (Entry->Bitmap->Transform)
        {
          case NONE:
          {
            glTexCoord2i(0, 0); glVertex2i(Entry->Left, Entry->Top);
            glTexCoord2i(0, 1); glVertex2i(Entry->Left, Entry->Bottom);
            glTexCoord2i(1, 1); glVertex2i(Entry->Right, Entry->Bottom);
            glTexCoord2i(1, 0); glVertex2i(Entry->Right, Entry->Top);
          } break;

          case FLIP_VERTICAL:
          {
            glTexCoord2i(1, 1); glVertex2i(Entry->Left, Entry->Top);
            glTexCoord2i(1, 0); glVertex2i(Entry->Left, Entry->Bottom);
            glTexCoord2i(0, 0); glVertex2i(Entry->Right, Entry->Bottom);
            glTexCoord2i(0, 1); glVertex2i(Entry->Right, Entry->Top);
          } break;
        }
        glEnd();

        glBindTexture(GL_TEXTURE_2D, 0);

        glDisable(GL_TEXTURE_2D);

        BaseAddress += sizeof(*Entry);
      } break;

      case RENDER_CLEAR:
      {
        render_entry_clear* Entry = (render_entry_clear*)Data;
        colour Colour = Entry->Colour;
        glClearColor(Colour.R/255.0, Colour.G/255.0, Colour.B/255.0, Colour.A/255.0);
        glClear(GL_COLOR_BUFFER_BIT);
        BaseAddress += sizeof(*Entry);
      } break;

      default:
      {
        InvalidCodePath;
      } break;
    }
  }
}
