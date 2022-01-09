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
} chess_piece;

global b32 Running = 1;
global u32 MouseX = 0;
global u32 MouseY = 0;

global platform_api Platform;
global const char* const PieceToAsset[] = { // TODO(robin): Maybe factor this into a function with a local lookup
  [NONE]   = "", // TODO(robin): Read in a tiny blank bitmap in this case, maybe 2px by 2px or something
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
global const chess_piece DefaultBoard[] = {
  ROOK_B, NITE_B, BISH_B, QUEN_B, KING_B, BISH_B, NITE_B, ROOK_B,
  PAWN_B, PAWN_B, PAWN_B, PAWN_B, PAWN_B, PAWN_B, PAWN_B, PAWN_B,
  0,      0,      0,      0,      0,      0,      0,      0,
  0,      0,      0,      0,      0,      0,      0,      0,
  0,      0,      0,      0,      0,      0,      0,      0,
  0,      0,      0,      0,      0,      0,      0,      0,
  PAWN_W, PAWN_W, PAWN_W, PAWN_W, PAWN_W, PAWN_W, PAWN_W, PAWN_W,
  ROOK_W, NITE_W, BISH_W, QUEN_W, KING_W, BISH_W, NITE_W, ROOK_W,
};
global chess_piece CurrentBoard[64];

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
