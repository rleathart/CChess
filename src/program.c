#define Assert(Expression) if (!(Expression)) {*(volatile int*)0 = 0;}
#define InvalidCodePath Assert(0)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define global static
#define local static
#define internal static

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

global const u32 TileSize = 80;

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
  u32 Width;
  u32 Height;

  byte* Pixels;
  u32 TextureHandle;
} loaded_bitmap;

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

typedef struct
{
  platform_read_entire_file* ReadEntireFile;
} platform_api;

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

global b32 Running = 1;
global v2i Mouse;
global b32 MouseDown;
global b32 LastMouseDown;
global piece MousePiece;
global u32 LastMouseTileIndex;

global platform_api Platform;
global const char* const PieceToAsset[] = { // TODO(robin): Maybe factor this into a function with a local lookup
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
global loaded_bitmap PieceBitmap[ArrayCount(PieceToAsset)];
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
global piece CurrentBoard[64];

inline b32 InRect(v2i Coord, recti Quad)
{
  b32 Result = Coord.E[0] > Quad.Left && Coord.E[0] < Quad.Right
    && Coord.E[1] > Quad.Top && Coord.E[1] < Quad.Bottom;
  return Result;
}

inline b32 MouseReleased(void)
{
  b32 Result = LastMouseDown && !MouseDown;
  return Result;
}

inline b32 MouseReleasedIn(recti Rect)
{
  b32 Result = MouseReleased() && InRect(Mouse, Rect);
  return Result;
}

inline b32 MouseClicked(void)
{
  b32 Result = MouseDown && !LastMouseDown;
  return Result;
}

inline b32 MouseClickedIn(recti Rect)
{
  b32 Result = MouseClicked() && InRect(Mouse, Rect);
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

move_array GetMoves(u8 TileIndex)
{
  move_array Result = {0};

  s32 Rank = TileIndex & 7; // TileIndex % 8;
  s32 File = TileIndex >> 3; // TileIndex / 8;
  u8 TileIndex88 = Pos88(TileIndex);

  piece SourcePiece = MousePiece;

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
        Result.Moves[Result.Count++] = (move){TileIndex, Tile1Ahead};
      }

      if (CanMove2Squares)
      {
        Result.Moves[Result.Count++] = (move){TileIndex, Tile2Ahead};
      }

      s32 Diagonals[] = {15, 17};
      for (s32 i = 0; i < ArrayCount(Diagonals); i++)
      {
        u8 TargetIndex88 = TileIndex88 + DirectionModifier * Diagonals[i];
        if (!(TargetIndex88 & 0x88))
        {
          u8 TargetIndex = Pos64(TargetIndex88);
          // TODO(robin): Self capture and en passant
          b32 CanCapture = CurrentBoard[TargetIndex];

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
    } break;
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
