#define Assert(Expression) if (!(Expression)) {*(volatile s32*)0 = 0;}
#define InvalidCodePath Assert(0)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
// TODO(robin): CopyMemory
#define CopyArray(Dest, Source) for (u64 i = 0; i < ArrayCount(Source); i++) {(Dest)[i] = (Source)[i];}

#define Kilobytes(Value) ((Value)*1024ULL)
#define Megabytes(Value) (Kilobytes(Value)*1024ULL)
#define Gigabytes(Value) (Megabytes(Value)*1024ULL)
#define Terabytes(Value) (Gigabytes(Value)*1024ULL)

#define global static
#define local static
#define internal static

// typedef char* va_list;
// #define PtrAlignedSizeOf(Type) ((sizeof(Type) + sizeof(int*) - 1) & ~(sizeof(int*) - 1))
// #define VAStart(ArgPtr, LastNamedArg) (ArgPtr = (va_list)&LastNamedArg + PtrAlignedSizeOf(LastNamedArg))
// #define VAGet(ArgPtr, Type) (*(Type*)((ArgPtr += PtrAlignedSizeOf(Type)) - PtrAlignedSizeOf(Type)))
// #define VAEnd(ArgPtr) (ArgPtr = 0)

#define VAStart va_start
#define VAGet va_arg
#define VAEnd va_end
#define inline static inline

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef u8 byte;
typedef s32 b32;
typedef u64 umm;

const u32 TileSize = 80;

enum
{
  NONE = 0,
};

typedef enum
{
  PAWN_W = NONE + 1,
  KING_W,
  QUEN_W,
  ROOK_W,
  BISH_W,
  NITE_W,
  PAWN_B,
  KING_B,
  QUEN_B,
  ROOK_B,
  BISH_B,
  NITE_B,

  CHESS_PIECE_COUNT,
} piece;

enum
{
  RENDER_LINE,
  RENDER_RECT,
  RENDER_BITMAP,
  RENDER_CLEAR,
};

enum
{
  FLIP_HORIZONTAL = 1 << 0,
  FLIP_VERTICAL = 1 << 1,
};

typedef union
{
  u32 Value;
  struct
  {
    u8 B;
    u8 G;
    u8 R;
    u8 A;
  };
} colour;

typedef union
{
  s32 E[2];
  struct
  {
    s32 X;
    s32 Y;
  };
} v2i;

typedef union
{
  s32 E[4];
  struct
  {
    s32 Left;
    s32 Top;
    s32 Right;
    s32 Bottom;
  };
} recti;

typedef struct
{
  u32 Size;
  byte* Data;
} file_read_result;

#pragma pack(push, 1)
typedef struct
{
  char Type[2];
  u32  FileSize;
  u16  Reserved1;
  u16  Reserved2;
  u32  DataOffset;
  u32  HeaderSize;
} bmp_header;

typedef struct
{
  s32 Width;
  s32 Height;
} bmp_header_v5;
#pragma pack(pop)

typedef struct
{
  struct
  {
    union
    {
      v2i;
      v2i XY;
    };

    b32 LButtonDown : 1;
    b32 RButtonDown : 1;
    b32 MButtonDown : 1;
  } Mouse;
} program_input;

typedef struct
{
  u32 Width;
  u32 Height;
  u32 TextureHandle;
  u32 Transform; /* NOTE(robin): Some bitmaps are stored bottom-up, others top-down, etc.
                    This allows us to specify how we want to draw the bitmap on screen. */
  byte* Pixels;
} loaded_bitmap;

typedef struct
{
  u32 Type;
} render_entry_header;

typedef struct
{
  s32 Left;
  s32 Top;
  s32 Right;
  s32 Bottom;
  colour Colour;
} render_entry_rect;

typedef struct
{
  v2i Point1;
  v2i Point2;
  colour Colour;
} render_entry_line;

typedef struct
{
  s32 Left;
  s32 Top;
  s32 Right;
  s32 Bottom;
  colour Colour;
  loaded_bitmap* Bitmap;
} render_entry_bitmap;

typedef struct
{
  colour Colour;
} render_entry_clear;

typedef struct
{
  umm Size;
  umm Used;
  byte* Buffer;
} render_group;

typedef struct
{
  u8 From;
  u8 To;
} move;

typedef struct
{
  u32 Count;
  move Moves[64];
} move_array;

typedef file_read_result (platform_read_entire_file)(char*);
typedef void (platform_write_to_std_out)(char*);
typedef void* (platform_allocate_memory)(umm);

typedef struct
{
  platform_read_entire_file* ReadEntireFile;
  platform_write_to_std_out* StdOut;
  platform_allocate_memory* AllocateMemory;
} platform_api;

global b32 Running = 1;
global piece MousePiece;
global u32 LastMouseTileIndex;
global render_group* RenderGroup;

global program_input Input;
global program_input LastInput;

global platform_api Platform;
global const char* const PieceAsset[] = { // TODO(robin): Maybe factor this into a function with a local lookup
  [NONE]   = "assets/blank.bmp",
  [PAWN_W] = "assets/piece_white_pawn.bmp",
  [KING_W] = "assets/piece_white_king.bmp",
  [QUEN_W] = "assets/piece_white_queen.bmp",
  [ROOK_W] = "assets/piece_white_rook.bmp",
  [BISH_W] = "assets/piece_white_bishop.bmp",
  [NITE_W] = "assets/piece_white_knight.bmp",
  [PAWN_B] = "assets/piece_black_pawn.bmp",
  [KING_B] = "assets/piece_black_king.bmp",
  [QUEN_B] = "assets/piece_black_queen.bmp",
  [ROOK_B] = "assets/piece_black_rook.bmp",
  [BISH_B] = "assets/piece_black_bishop.bmp",
  [NITE_B] = "assets/piece_black_knight.bmp",
};
global loaded_bitmap PieceBitmap[ArrayCount(PieceAsset)];
global const piece DefaultBoard[] = {
  ROOK_B, NITE_B, BISH_B, QUEN_B, KING_B, BISH_B, NITE_B, ROOK_B,
  PAWN_B, PAWN_B, PAWN_B, PAWN_B, PAWN_B, PAWN_B, PAWN_B, PAWN_B,
  0,      0,      0,      0,      0,      0,      0,      0,
  0,      0,      0,      0,      0,      0,      0,      0,
  0,      0,      0,      0,      0,      0,      0,      0,
  0,      0,      0,      0,      0,      0,      0,      0,
  PAWN_W, PAWN_W, PAWN_W, PAWN_W, PAWN_W, PAWN_W, PAWN_W, PAWN_W,
  ROOK_W, NITE_W, BISH_W, QUEN_W, KING_W, BISH_W, NITE_W, ROOK_W,
};
global s32 EnPassantTileIndex = -1;
global b32 CanCastle[4] = {1, 1, 1, 1}; // NOTE(robin): BlackQueenSide, WhiteQueenSide, BlackKingSide, WhiteKingSide
global piece LastBoard[64];
global piece CurrentBoard[64];

internal void*
CopyMemory(void* DestInit, void* SourceInit, umm Size)
{
  char* Dest = (char*)DestInit;
  char* Source = (char*)SourceInit;
  while (Size--) *Dest++ = *Source++;

  return(DestInit);
}

internal render_group*
AllocateRenderGroup(umm Size)
{
  render_group* Result = Platform.AllocateMemory(sizeof(render_group) + Size);
  Result->Buffer = (byte*)(Result + 1);
  Result->Size = Size;

  return Result;
}

internal void*
PushRect(render_group* RenderGroup, recti Rect, colour Colour)
{
  render_entry_header Header = {RENDER_RECT};
  render_entry_rect Entry = {Rect.Left, Rect.Top, Rect.Right, Rect.Bottom, Colour};

  Assert(RenderGroup->Used + sizeof(Header) + sizeof(Entry) <= RenderGroup->Size);

  byte* Base = RenderGroup->Buffer + RenderGroup->Used;

  CopyMemory(Base, &Header, sizeof(Header));
  CopyMemory(Base + sizeof(Header), &Entry, sizeof(Entry));

  RenderGroup->Used += sizeof(Header) + sizeof(Entry);

  return Base;
}

internal void*
PushLine(render_group* RenderGroup, v2i Point1, v2i Point2, colour Colour)
{
  render_entry_header Header = {RENDER_LINE};
  render_entry_line Entry = {Point1, Point2, Colour};

  Assert(RenderGroup->Used + sizeof(Header) + sizeof(Entry) <= RenderGroup->Size);

  byte* Base = RenderGroup->Buffer + RenderGroup->Used;

  CopyMemory(Base, &Header, sizeof(Header));
  CopyMemory(Base + sizeof(Header), &Entry, sizeof(Entry));

  RenderGroup->Used += sizeof(Header) + sizeof(Entry);

  return Base;
}

internal void*
PushBitmap(render_group* RenderGroup, loaded_bitmap* Bitmap, recti Rect, colour Colour)
{
  render_entry_header Header = {RENDER_BITMAP};
  render_entry_bitmap Entry = {Rect.Left, Rect.Top, Rect.Right, Rect.Bottom, Colour, Bitmap};

  Assert(RenderGroup->Used + sizeof(Header) + sizeof(Entry) <= RenderGroup->Size);

  byte* Base = RenderGroup->Buffer + RenderGroup->Used;

  CopyMemory(Base, &Header, sizeof(Header));
  CopyMemory(Base + sizeof(Header), &Entry, sizeof(Entry));

  RenderGroup->Used += sizeof(Header) + sizeof(Entry);

  return Base;
}

internal void*
PushClear(render_group* RenderGroup, colour Colour)
{
  render_entry_header Header = {RENDER_CLEAR};
  render_entry_clear Entry = {Colour};

  Assert(RenderGroup->Used + sizeof(Header) + sizeof(Entry) <= RenderGroup->Size);

  byte* Base = RenderGroup->Buffer + RenderGroup->Used;

  CopyMemory(Base, &Header, sizeof(Header));
  CopyMemory(Base + sizeof(Header), &Entry, sizeof(Entry));

  RenderGroup->Used += sizeof(Header) + sizeof(Entry);

  return Base;
}

inline u64 StringLength(char* String)
{
  u64 Result = 0;
  while (*String++)
    Result++;
  return Result;
}

char* CatStrings(char* Dest, s32 Count, ...)
{
  char* Result = Dest;

  va_list VArgs;
  VAStart(VArgs, Count);

  while (Count--)
  {
    char* Source = VAGet(VArgs, char*);
    while (*Source)
      *Dest++ = *Source++;
  }

  *Dest = 0;

  return Result;
}

inline b32 InRect(v2i Coord, recti Quad)
{
  b32 Result = Coord.E[0] > Quad.Left && Coord.E[0] < Quad.Right
    && Coord.E[1] > Quad.Top && Coord.E[1] < Quad.Bottom;
  return Result;
}

inline b32 MouseLReleased(void)
{
  b32 Result = LastInput.Mouse.LButtonDown && !Input.Mouse.LButtonDown;
  return Result;
}

inline b32 MouseLReleasedIn(recti Rect)
{
  b32 Result = MouseLReleased() && InRect(Input.Mouse.XY, Rect);
  return Result;
}

inline b32 MouseLReleasedOutside(recti Rect)
{
  b32 Result = MouseLReleased() && !InRect(Input.Mouse.XY, Rect);
  return Result;
}

inline b32 MouseLClicked(void)
{
  b32 Result = Input.Mouse.LButtonDown && !LastInput.Mouse.LButtonDown;
  return Result;
}

inline b32 MouseLClickedIn(recti Rect)
{
  b32 Result = MouseLClicked() && InRect(Input.Mouse.XY, Rect);
  return Result;
}

inline u8 Pos64(u8 TileIndex88)
{
  u8 Result = (TileIndex88 + (TileIndex88 & 7)) >> 1;
  return Result;
}

inline u8 Pos88(u8 TileIndex64)
{
  u8 Result = TileIndex64 + (TileIndex64 & ~7);
  return Result;
}

inline b32 IsWhite(piece Piece)
{
  b32 Result = 0;
  switch (Piece)
  {
    case PAWN_W: case ROOK_W:
    case NITE_W: case BISH_W:
    case QUEN_W: case KING_W:
    {
      Result = 1;
    } break;
  }
  return Result;
}

inline b32 IsSelfCapture(u8 SourceTileIndex, u8 TargetTileIndex)
{
  // TODO(robin): Fix using MousePiece everywhere!!!
  piece SourcePiece = MousePiece;
  piece TargetPiece = CurrentBoard[TargetTileIndex];

  b32 Result = !(TargetPiece == NONE || (IsWhite(SourcePiece) != IsWhite(TargetPiece)));

  return Result;
}

move_array GetMoves(u8 TileIndex)
{
  move_array Result = {0};

  s32 Rank = TileIndex & 7; // TileIndex % 8;
  s32 File = TileIndex >> 3; // TileIndex / 8;
  u8 TileIndex88 = Pos88(TileIndex);

  piece SourcePiece = MousePiece;

  // TODO(robin): en passant and check
  switch (SourcePiece)
  {
    case PAWN_B:
    case PAWN_W:
    {
      b32 White = SourcePiece == PAWN_W;
      s32 DirectionModifier = White ? -1 : 1;
      s32 DoubleMoveFile = White ? 6 : 1;

      u8 Tile1Ahead = TileIndex + DirectionModifier * 8;
      u8 Tile2Ahead = TileIndex + DirectionModifier * 16;

      b32 CanMove1Square = CurrentBoard[Tile1Ahead] == NONE;
      b32 CanMove2Squares = CanMove1Square && File == DoubleMoveFile && CurrentBoard[Tile2Ahead] == NONE;

      if (CanMove1Square)
      {
        if (!IsSelfCapture(TileIndex, Tile1Ahead))
          Result.Moves[Result.Count++] = (move){TileIndex, Tile1Ahead};
      }

      if (CanMove2Squares)
      {
        if (!IsSelfCapture(TileIndex, Tile2Ahead))
          Result.Moves[Result.Count++] = (move){TileIndex, Tile2Ahead};
      }

      s32 Diagonals[] = {15, 17};
      for (s32 i = 0; i < ArrayCount(Diagonals); i++)
      {
        u8 TargetIndex88 = TileIndex88 + DirectionModifier * Diagonals[i];
        if (!(TargetIndex88 & 0x88))
        {
          u8 TargetIndex = Pos64(TargetIndex88);
          b32 CanCapture = CurrentBoard[TargetIndex] && !IsSelfCapture(TileIndex, TargetIndex);

          if (CanCapture)
          {
            Result.Moves[Result.Count++] = (move){TileIndex, TargetIndex};
          }
        }
      }

    } break;

    case NITE_B:
    case NITE_W:
    {
      // NOTE(robin): We use the 0x88 offsets here to check for off-board wrapping
      s32 MoveOffsets[] = {14, 18, -14, -18, 33, 31, -33, -31};
      for (s32 i = 0; i < 8; i++)
      {
        u8 TargetIndex88 = TileIndex88 + MoveOffsets[i];

        if (!(TargetIndex88 & 0x88))
        {
          u8 TargetIndex = Pos64(TargetIndex88);
          if (!IsSelfCapture(TileIndex, TargetIndex))
            Result.Moves[Result.Count++] = (move){TileIndex, TargetIndex};
        }
      }
    } break;

    // NOTE(robin): Sliding moves (no castling)
    case BISH_B:
    case BISH_W:
    case ROOK_B:
    case ROOK_W:
    case QUEN_B:
    case QUEN_W:
    case KING_B:
    case KING_W:
    {
      s32 SlideOffset[] = {-16, 16, -1, 1, 17, 15, -17, -15};

      s32 Start = 0, End = 8;

      switch (SourcePiece)
      {
        case BISH_B:
        case BISH_W:
        {
          Start = 4;
        } break;

        case ROOK_B:
        case ROOK_W:
        {
          End = 4;
        } break;
      }

      for (s32 i = Start; i < End; i++)
      {
        u8 TargetIndex88 = TileIndex88 + SlideOffset[i];
        while (!(TargetIndex88 & 0x88))
        {
          u8 TargetIndex = Pos64(TargetIndex88);
          if (!IsSelfCapture(TileIndex, TargetIndex))
            Result.Moves[Result.Count++] = (move){TileIndex, TargetIndex};

          if (CurrentBoard[TargetIndex])
            break; // NOTE(robin): Moving to this tile will take a piece so we stop

          TargetIndex88 += SlideOffset[i];

          if (SourcePiece == KING_B || SourcePiece == KING_W)
            break; // NOTE(robin): King can only move one square
        }
      }

    } break;
  }

  // NOTE(robin): Castling
  if (SourcePiece == KING_B || SourcePiece == KING_W)
  {
    b32 White = IsWhite(SourcePiece);
    for (s32 KingSide = 0; KingSide < 2; KingSide++)
    {
      b32 CastleInCorner =
        CurrentBoard[KingSide ? White ? 63 : 7 : White ? 56 : 0] == (White ? ROOK_W : ROOK_B);

      if (CanCastle[White + 2 * KingSide] && CastleInCorner)
      {
        b32 CastleIsValid = 1;
        s32 DirectionModifier = KingSide ? 1 : -1;
        for (s32 i = 1; i < (KingSide ? 3 : 4); i++)
        {
          s32 TargetIndex = TileIndex + DirectionModifier * i;

          if (CurrentBoard[TargetIndex] != NONE)
            CastleIsValid = 0;
        }

        if (CastleIsValid)
        {
          u8 TargetIndex = TileIndex + DirectionModifier * 2;
          Result.Moves[Result.Count++] = (move){TileIndex, TargetIndex};
        }
      }

    }
  }

  return Result;
}

inline b32 LegalMove(u8 From, u8 To)
{
  b32 Result = 0;

  move_array Moves = GetMoves(From);
  for (u32 MoveIndex = 0; MoveIndex < Moves.Count; MoveIndex++)
  {
    move Move = Moves.Moves[MoveIndex];
    if (Move.To == To)
    {
      Result = 1;
      break;
    }
  }

  return Result;
}

loaded_bitmap ReadBitmap(char* Filename)
{
  loaded_bitmap Result = {0};

  file_read_result File = Platform.ReadEntireFile(Filename);

  bmp_header* Header = (bmp_header*)File.Data;

  // TODO(robin): We only support BitmapV5Header currently
  Assert(Header->HeaderSize == 124);

  bmp_header_v5* Header5 = (bmp_header_v5*)(Header + 1);
  Result.Width = Header5->Width;
  Result.Height = Header5->Height;

  Result.Pixels = File.Data + Header->DataOffset;

  return Result;
}

void Update(void)
{
  // NOTE(robin): Don't update boards if we're holding a piece
  if (!MousePiece)
  {
    // TODO(robin): Fix this scuffed ass code
    b32 CastledKingSide[2] = {
      LastBoard[4] == KING_B && CurrentBoard[6] == KING_B,
      LastBoard[60] == KING_W && CurrentBoard[62] == KING_W,
    };
    b32 CastledQueenSide[2] = {
      LastBoard[4] == KING_B && CurrentBoard[2] == KING_B,
      LastBoard[60] == KING_W && CurrentBoard[58] == KING_W,
    };

    b32 RookMoved[4] = {
      LastBoard[0] == ROOK_B && CurrentBoard[0] != ROOK_B,
      LastBoard[56] == ROOK_W && CurrentBoard[56] != ROOK_W,
      LastBoard[7] == ROOK_B && CurrentBoard[7] != ROOK_B,
      LastBoard[63] == ROOK_W && CurrentBoard[63] != ROOK_W,
    };

    b32 KingMoved[2] = {
      LastBoard[4] == KING_B && CurrentBoard[4] != KING_B,
      LastBoard[60] == KING_W && CurrentBoard[60] != KING_W,
    };

    if (KingMoved[0])
    {
      CanCastle[0] = CanCastle[2] = 0;
    }

    if (KingMoved[1])
    {
      CanCastle[1] = CanCastle[3] = 0;
    }

    for (u32 i = 0; i < ArrayCount(CanCastle); i++)
    {
      if (RookMoved[i])
      {
        CanCastle[i] = 0;
      }
    }

    for (u32 White = 0; White < ArrayCount(CastledKingSide); White++)
    {
      if (CastledKingSide[White])
      {
        CurrentBoard[White ? 63 : 7] = NONE;
        CurrentBoard[White ? 61 : 5] = White ? ROOK_W : ROOK_B;
        CanCastle[White + 2] = 0;
        CanCastle[White] = 0;
      }

      if (CastledQueenSide[White])
      {
        CurrentBoard[White ? 56 : 0] = NONE;
        CurrentBoard[White ? 59 : 3] = White ? ROOK_W : ROOK_B;
        CanCastle[White + 2] = 0;
        CanCastle[White] = 0;
      }
    }

    CopyArray(LastBoard, CurrentBoard);
  }

  LastInput = Input;
}

void Render(render_group* RenderGroup)
{
  colour ClearColour = {0xFF312EBB};
  PushClear(RenderGroup, ClearColour);

  s32 XOffset = 50;
  s32 YOffset = 50;
  s32 TileMargin = 10;

  recti BoardClipRect = {
    XOffset, YOffset,
    XOffset + 8 * TileSize, YOffset + 8 * TileSize,
  };

  // NOTE(robin): Put the staged piece back if we release the mouse outside the board
  if (MouseLReleasedOutside(BoardClipRect) && MousePiece)
  {
    CurrentBoard[LastMouseTileIndex] = MousePiece;
    MousePiece = NONE;
  }

  for (s32 Y = 0; Y < 8; Y++)
  {
    for (s32 X = 0; X < 8; X++)
    {
      u32 TileIndex = Y * 8 + X;

      // NOTE(robin): Alternate tile colours
      colour TileColour = (TileIndex + (Y & 1)) & 1 ? (colour){0xFF333333} : (colour){0xFFFFFFFF};

      recti Tile = {
        XOffset + TileSize * X,
        YOffset + TileSize * Y,
        XOffset + TileSize * (X + 1),
        YOffset + TileSize * (Y + 1),
      };

      PushRect(RenderGroup, Tile, TileColour);

      piece Piece = CurrentBoard[TileIndex];

      if (MouseLClickedIn(Tile))
      {
        MousePiece = CurrentBoard[TileIndex];
        CurrentBoard[TileIndex] = NONE;
        LastMouseTileIndex = TileIndex;
      }

      if (MouseLReleasedIn(Tile))
      {
        if (LegalMove(LastMouseTileIndex, TileIndex))
        {
          CurrentBoard[TileIndex] = MousePiece;
          MousePiece = NONE;
        }
        else
        {
          CurrentBoard[LastMouseTileIndex] = MousePiece;
          MousePiece = NONE;
        }
      }

      if (Piece)
      {
        recti TileRect = {
          TileMargin + Tile.Left,
          TileMargin + Tile.Top,
          Tile.Right - TileMargin,
          Tile.Bottom - TileMargin,
        };

        PushBitmap(RenderGroup, &PieceBitmap[Piece], TileRect, (colour){~0L});
      }
    }
  }

  // NOTE(robin): The texture lags the cursor a bit because the window callback is not called every frame.

  if (MousePiece) // NOTE(robin): We're holding a piece with the mouse
  {
    // NOTE(robin): Draw the piece bitmap over the cursor
    recti BitmapRect = {
      Input.Mouse.X - TileSize/2 + TileMargin,
      Input.Mouse.Y - TileSize/2 + TileMargin,
      Input.Mouse.X + TileSize/2 - TileMargin,
      Input.Mouse.Y + TileSize/2 - TileMargin,
    };

    PushBitmap(RenderGroup, &PieceBitmap[MousePiece], BitmapRect, (colour){~0U});

    // NOTE(robin): Also highlight the possible moves in red
    move_array PossibleMoves = GetMoves(LastMouseTileIndex);
    for (u32 MoveIndex = 0; MoveIndex < PossibleMoves.Count; MoveIndex++)
    {
      u8 TileIndex = PossibleMoves.Moves[MoveIndex].To;
      s32 X = TileIndex & 7;
      s32 Y = TileIndex >> 3;
      recti Tile = {
        XOffset + TileSize * X,
        YOffset + TileSize * Y,
        XOffset + TileSize * (X + 1),
        YOffset + TileSize * (Y + 1),
      };
      PushRect(RenderGroup, Tile, (colour){0x7FCC0000});
    }
  }

}
