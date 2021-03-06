#define Assert(Expression) if (!(Expression)) {*(volatile s32*)0 = 0;}
#define InvalidCodePath Assert(0)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
// TODO(robin): CopyMemory
#define CopyArray(Dest, Source) for (u64 i = 0; i < ArrayCount(Source); i++) {(Dest)[i] = (Source)[i];}
#define CreateMask(Start, End) (-1ULL >> (sizeof(umm) * 8 - 1 - (End) + (Start)) << (Start))
// NOTE(robin): Used to take the two's compliment of an int that is Size bytes
#define TwosComplement(Value, Size) ((~(Value) & CreateMask(0, (Size) * 8 - 1)) + 1)
#define GetBit(Value, Bit) ((Value) & (1 << (Bit)))
#define SetBit(Value, Bit) ((Value) |= 1 << (Bit))
#define ClearBit(Value, Bit) ((Value) &= ~(1 << (Bit)))
#define ToggleBit(Value, Bit) ((Value) ^= 1 << (Bit))
#define GetBits(Value, Start, End) (((Value) & CreateMask(Start, End)) >> (Start))

#define Kilobytes(Value) ((Value)*1024ULL)
#define Megabytes(Value) (Kilobytes(Value)*1024ULL)
#define Gigabytes(Value) (Megabytes(Value)*1024ULL)
#define Terabytes(Value) (Gigabytes(Value)*1024ULL)

#define global           // NOTE(robin): So we can grep for globals
#define local static
#define internal static
#define inline static inline

#if _WIN64

typedef char* va_list;
#define PtrAlignedSizeOf(Type) ((sizeof(Type) + sizeof(int*) - 1) & ~(sizeof(int*) - 1))
#define VAStart(ArgPtr, LastNamedArg) (ArgPtr = (byte*)&LastNamedArg + PtrAlignedSizeOf(LastNamedArg))
#define VAGet(ArgPtr, Type) (*(Type*)((ArgPtr += PtrAlignedSizeOf(Type)) - PtrAlignedSizeOf(Type)))
#define VAEnd(ArgPtr) (ArgPtr = 0)

#else

// TODO(robin): Figure out what va_start does on mac
#include <stdarg.h>
#define VAStart va_start
#define VAGet va_arg
#define VAEnd va_end

#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef float f32;
typedef double f64;
typedef u8 byte;
typedef s32 b32;
typedef u64 umm;

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
} entire_file;

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
    b32 RotatedUp : 1;
    b32 RotatedDown : 1;
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

typedef struct
{
  recti Rect;
  char* Text;
  colour Colour;
  colour TextColour;
  colour HoveredColour;
  colour HoveredTextColour;
} button;

typedef struct
{
  b32 LClicked;
  b32 Hovered;
} do_button_result;

typedef struct
{
  umm Size;
  umm Used;

  byte* Base;
} memory_arena;

typedef struct
{
  u32 Size;
  u32 Count;

  move* Moves;
} move_history_array;

typedef struct
{
  u32 Size;
  u32 Count;

  piece (*Boards)[64];
} board_hisory_array;

typedef struct
{
  v2i Offset;
  v2i Increment;
  v2i XClip;
  v2i YClip;

  // TODO(robin): Do we actually want these?
  recti ElementSize;
  v2i Start;
  v2i End;
} viewport;

typedef struct
{
  b32 Running;

  piece CurrentBoard[64];
  piece LastBoard[64]; // NOTE(robin): Board last frame
  piece MousePiece; // NOTE(robin): Piece currently being held by the mouse
  u32 LastMouseTileIndex; // NOTE(robin): The last tile index that the user interacted with

  s32 EnPassantTileIndex;
  b32 CanCastle[4]; // NOTE(robin): BlackQueenSide, WhiteQueenSide, BlackKingSide, WhiteKingSide

  b32 WhiteToMove;
  move_history_array MoveHistory;
  board_hisory_array BoardHistory;
} program_state;

typedef entire_file platform_read_entire_file(char*);
typedef b32 platform_write_entire_file(char*, void*, umm);
typedef void platform_write_to_std_out(char*);
typedef void* platform_allocate_memory(umm);
typedef void platform_deallocate_memory(void*);

typedef struct
{
  platform_read_entire_file* ReadEntireFile;
  platform_write_entire_file* WriteEntireFile;
  platform_write_to_std_out* StdOut;
  platform_allocate_memory* AllocateMemory;
  platform_deallocate_memory* DeallocateMemory;

  char* ExecutableFilename;
  char* ExecutableDirectory;
} platform_api;

typedef struct
{
  memory_arena PermArena;

  platform_api Platform;
  program_state State;
  program_input Input;
  program_input LastInput;

  loaded_bitmap* GlyphTable;
  render_group* RenderGroup;
  viewport MoveHistoryViewport;
  u64 FrameCounter;
} program_memory;

global platform_api Platform;
global loaded_bitmap* GlyphTable;
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

inline b32 InRect(v2i Coord, recti Quad);
inline b32 MouseLClickedIn(program_memory*, recti Rect);

inline s32
RectWidth(recti Rect)
{
  s32 Result = Rect.Right - Rect.Left;
  return Result;
}

inline s32
RectHeight(recti Rect)
{
  s32 Result = Rect.Bottom - Rect.Top;
  return Result;
}

inline f64
Clip(f64 Value, f64 Min, f64 Max)
{
  f64 Result = Value > Max ? Max : Value < Min ? Min : Value;
  return Result;
}

void ScrollViewport(viewport* Viewport, v2i ScrollCount)
{
  Viewport->Offset.X += ScrollCount.X * Viewport->Increment.X;
  Viewport->Offset.Y += ScrollCount.Y * Viewport->Increment.Y;

  Viewport->Offset.X = Clip(Viewport->Offset.X, Viewport->XClip.E[0], Viewport->XClip.E[1]);
  Viewport->Offset.Y = Clip(Viewport->Offset.Y, Viewport->YClip.E[0], Viewport->YClip.E[1]);

  v2i LastStart = Viewport->Start;
  Viewport->Start.Y = Viewport->Offset.Y / Viewport->ElementSize.Bottom;
  Viewport->End.Y += Viewport->Start.Y - LastStart.Y;
}

inline f64 Abs(f64 x)
{
  f64 Result = x < 0 ? -x : x;
  return Result;
}

inline f64 PowI(f64 Value, s32 Exponent)
{
  f64 Result = Value;

  if (Exponent == 0)
  {
    Result = 1;
  }

  for (s32 i = 0; i < Abs(Exponent); i++)
  {
    Result *= Value;
  }

  if (Exponent < 0)
  {
    Result = 1/Result;
  }
  return Result;
}

inline u64 StringLength(char* String)
{
  u64 Result = 0;
  while (*String++)
    Result++;
  return Result;
}

char* CopyString(char* Dest, char* Source)
{
  char* Result = Dest;
  while (*Source) *Dest++ = *Source++;
  return Result;
}

internal void*
CopyMemory(void* DestInit, void* SourceInit, umm Size)
{
  char* Dest = (char*)DestInit;
  char* Source = (char*)SourceInit;
  while (Size--) *Dest++ = *Source++;

  return(DestInit);
}

inline u32
Digits10(umm Value, b32 Signed, u32 Size)
{
  u64 PowerOfTen[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
    100000000, 1000000000, 10000000000, 100000000000, 1000000000000};

  b32 Negative = Signed && GetBit(Value, Size * 8 - 1);

  if (Negative)
  {
    Value = TwosComplement(Value, Size);
  }

  u32 Power;
  for (Power = 1; Power < ArrayCount(PowerOfTen); Power++)
  {
    if (Value < PowerOfTen[Power])
    {
      return Power;
    }
  }

  Power--;
  return Power + Digits10(Value / PowerOfTen[Power], Signed, Size / 2);
}

inline char*
IntString(char* Buffer, umm Value, b32 Signed, u32 Size, u32 Padding)
{
  u32 DigitsRequired = Digits10(Value, Signed, Size);
  DigitsRequired += Padding;

  b32 Negative = Signed && GetBit(Value, Size * 8 - 1);

  if (Negative)
  {
    DigitsRequired++;
    Value = TwosComplement(Value, Size);
  }

  u64 Quotient = Value;
  u64 Remainder = 0;

  for (s32 Digit = DigitsRequired - 1; Digit >= 0; Digit--)
  {
    if (Digit == 0 && Negative)
    {
      Buffer[Digit] = '-';
    }
    else if (Digit < Padding)
    {
      Buffer[Digit] = '0';
    }
    else
    {
      Quotient = Value / 10;
      Remainder = Value % 10;
      Value = Quotient;
      Buffer[Digit] = '0' + Remainder;
    }
  }

  return Buffer;
}

inline char*
S32String(char* Buffer, s32 Value)
{
  char* Result = IntString(Buffer, Value, 1, sizeof(Value), 0);
  return Result;
}

inline char*
U32String(char* Buffer, u32 Value)
{
  char* Result = IntString(Buffer, Value, 0, sizeof(Value), 0);
  return Result;
}

inline char*
U64String(char* Buffer, u64 Value)
{
  char* Result = IntString(Buffer, Value, 0, sizeof(Value), 0);
  return Result;
}

inline char*
S64String(char* Buffer, s64 Value)
{
  char* Result = IntString(Buffer, Value, 1, sizeof(Value), 0);
  return Result;
}

// Stolen from https://www.geeksforgeeks.org/convert-floating-point-number-string/
inline void
F64String(char* Buffer, f64 Value, u32 DecimalPlaces)
{
  s32 Offset = 0;
  if (Value < 0)
  {
    Offset++;
    Buffer[0] = '-';
    Value = -Value;
  }

  s64 IntegerPart = Value;
  f64 FloatPart = Value - (f64)IntegerPart;

  S64String(Buffer + Offset, IntegerPart);
  int i = Digits10(IntegerPart, 1, sizeof(s64));

  if (DecimalPlaces != 0)
  {
    Buffer[i + Offset] = '.';
    FloatPart = Abs(FloatPart * PowI(10, DecimalPlaces));
    IntString(Buffer + i + Offset + 1, FloatPart, 0, 8,
        DecimalPlaces - Digits10((u64)FloatPart, 1, sizeof(u64)));
  }
}

char* FormatStringVA(char* Buffer, const char* Format, va_list Args)
{
  u32 BufferIndex = 0;
  b32 LongModifier = 0;
  while (*Format)
  {
    char TempBuffer[1024] = {0};
    char* ValueBuffer = TempBuffer;
    if (*Format != '%' && !LongModifier)
    {
      Buffer[BufferIndex++] = *Format++;
    }
    else
    {
      if (!LongModifier)
        Format++;

      switch (*Format)
      {
        case 'l':
        {
          LongModifier++;
        } break;

        case 'c':
        {
          char Value = VAGet(Args, int);
          ValueBuffer[0] = Value;
          ValueBuffer[1] = 0;
        } break;

        case 'd':
        {
          if (!LongModifier)
          {
            s32 Value = VAGet(Args, s32);
            S32String(ValueBuffer, Value);
          }
          else
          {
            s64 Value = VAGet(Args, s64);
            S64String(ValueBuffer, Value);
          }
          LongModifier = 0;
        } break;

        case 'u':
        {
          if (!LongModifier)
          {
            u32 Value = VAGet(Args, u32);
            U32String(ValueBuffer, Value);
          }
          else
          {
            u64 Value = VAGet(Args, u64);
            U64String(ValueBuffer, Value);
          }
          LongModifier = 0;
        } break;

        case 'f':
        {
          f64 Value = VAGet(Args, f64);
          F64String(ValueBuffer, Value, 6);
        } break;

        case 's':
        {
          ValueBuffer = VAGet(Args, char*);
        } break;
      }

      CopyString(Buffer + BufferIndex, ValueBuffer);
      BufferIndex += StringLength(ValueBuffer);

      Format++;
    }
  }

  // NOTE(robin): Null terminate
  Buffer[BufferIndex] = 0;

  return Buffer;
}

inline char*
FormatString(char* Buffer, const char* Format, ...)
{
  va_list Args;
  VAStart(Args, Format);

  FormatStringVA(Buffer, Format, Args);

  VAEnd(Args);

  return Buffer;
}

#define PushType(Arena, Type) (Type*)PushSize(Arena, sizeof(Type))
#define PushArray(Arena, Type, Count) (Type*)PushSize(Arena, (Count)*sizeof(Type))

inline void*
PushSize(memory_arena* Arena, umm Size)
{
  // TODO(robin): Alignment
  Assert(Arena->Used + Size <= Arena->Size);

  void* Result = Arena->Base + Arena->Used;
  Arena->Used += Size;

  return Result;
}

inline void
BoardIndexToFAN(char* FANString, u32 Value)
{
  char Ranks[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
  char Files[] = {'1', '2', '3', '4', '5', '6', '7', '8'};

  u32 Rank = Value % 8;
  u32 File = 7 - Value / 8;

  FANString[0] = Ranks[Rank];
  FANString[1] = Files[File];
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

internal void*
PushString(render_group* RenderGroup, s32 X, s32 Y, char* Text, colour Colour)
{
  // TODO(robin): Figure out how we want to handle \n and \r
  byte* Result = RenderGroup->Buffer + RenderGroup->Used;
  for (;*Text; Text++)
  {
    loaded_bitmap* Glyph = &GlyphTable[*Text];
    render_entry_header Header = {RENDER_BITMAP};
    render_entry_bitmap Entry = {X, Y, X + (s32)Glyph->Width, Y + (s32)Glyph->Height, Colour, Glyph};
    X += Glyph->Width;

    Assert(RenderGroup->Used + sizeof(Header) + sizeof(Entry) <= RenderGroup->Size);

    byte* Base = RenderGroup->Buffer + RenderGroup->Used;

    CopyMemory(Base, &Header, sizeof(Header));
    CopyMemory(Base + sizeof(Header), &Entry, sizeof(Entry));

    RenderGroup->Used += sizeof(Header) + sizeof(Entry);
  }

  return Result;
}

void PushBoard(render_group* RenderGroup, piece Board[64], recti Rect)
{
  PushRect(RenderGroup, (recti){Rect.Left - 5, Rect.Top - 5, Rect.Right + 5, Rect.Bottom + 5},
      (colour){0xFF1B1B1B});

  u32 TileSize = (Rect.Right - Rect.Left) / 8;
  for (s32 Y = 0; Y < 8; Y++)
  {
    for (s32 X = 0; X < 8; X++)
    {
      u32 TileIndex = Y * 8 + X;

      // NOTE(robin): Alternate tile colours
      colour TileColour = (TileIndex + (Y & 1)) & 1 ? (colour){0xFF333333} : (colour){0xFFFFFFFF};

      recti Tile = {
        Rect.Left + TileSize * X,
        Rect.Top + TileSize * Y,
        Rect.Left + TileSize * (X + 1),
        Rect.Top + TileSize * (Y + 1),
      };

      PushRect(RenderGroup, Tile, TileColour);

      piece Piece = Board[TileIndex];

      if (Piece)
      {
        u32 TileMargin = TileSize / 8;
        recti TileRect =
        {
          TileMargin + Tile.Left,
          TileMargin + Tile.Top,
          Tile.Right - TileMargin,
          Tile.Bottom - TileMargin,
        };

        PushBitmap(RenderGroup, &PieceBitmap[Piece], TileRect, (colour){~0L});
      }
    }
  }
}

internal colour
ColourAdd(colour Colour, s32 Value)
{
  colour Result = Colour;

  Result.R += Value;
  Result.G += Value;
  Result.B += Value;

  return Result;
}

// TODO(robin): Figure out a better way to get text bounds
internal recti
TextRect(char* Text)
{
  recti Result = {0};

  for (;*Text; Text++)
  {
    loaded_bitmap* Glyph = &GlyphTable[*Text];
    Result.Right += Glyph->Width;
    if (Glyph->Height > Result.Bottom)
      Result.Bottom = Glyph->Height;
  }

  return Result;
}

internal void*
PushStringCentred(render_group* RenderGroup, v2i Position, char* Text, colour Colour)
{
  byte* Result = RenderGroup->Buffer + RenderGroup->Used;

  recti TextBounds = TextRect(Text);
  s32 X = Position.X - TextBounds.Right/2.0;
  s32 Y = Position.Y - TextBounds.Bottom/2.0;
  PushString(RenderGroup, X, Y, Text, Colour);

  return Result;
}

internal void*
PushFormattedString(render_group* RenderGroup, s32 X, s32 Y, colour Colour, const char* Format, ...)
{
  va_list VArgs;
  VAStart(VArgs, Format);

  char Buffer[2048];
  FormatStringVA(Buffer, Format, VArgs);
  void* Result = PushString(RenderGroup, X, Y, Buffer, Colour);

  VAEnd(VArgs);

  return Result;
}

internal void*
PushButton(render_group* RenderGroup, recti Rect, char* Text, colour ButtonColour, colour TextColor)
{
  byte* Result = RenderGroup->Buffer + RenderGroup->Used;
  PushRect(RenderGroup, Rect, ButtonColour);
  s32 Width = Rect.Right - Rect.Left;
  s32 Height = Rect.Bottom - Rect.Top;
  recti TextBounds = TextRect(Text);
  s32 X = Rect.Left + Width/2 - TextBounds.Right/2;
  s32 Y = Rect.Top + Height/2 - TextBounds.Bottom/2;
  PushString(RenderGroup, X, Y, Text, TextColor);
  return Result;
}

internal do_button_result
DoButton(program_memory* Memory, button Button)
{
  render_group* RenderGroup = Memory->RenderGroup;
  do_button_result Result = {0};

  Result.LClicked = MouseLClickedIn(Memory, Button.Rect);
  Result.Hovered = InRect(Memory->Input.Mouse.XY, Button.Rect);

  colour ButtonColour = Result.Hovered ? Button.HoveredColour : Button.Colour;
  colour TextColour = Result.Hovered ? Button.HoveredTextColour : Button.TextColour;

  PushButton(RenderGroup, Button.Rect, Button.Text, ButtonColour, TextColour);

  return Result;
}

void PushMove(move_history_array* MoveHistory, move Move)
{
  Assert(MoveHistory->Count + 1 <= MoveHistory->Size);
  MoveHistory->Moves[MoveHistory->Count++] = Move;
}

void SerialiseGlyphTable(char* Filename)
{
  umm BytesToWrite = 0;

  // NOTE(robin): Compute how much memory we need
  for (u32 Glyph = 0; Glyph < 256; Glyph++)
  {
    BytesToWrite += sizeof(GlyphTable[0]);
    BytesToWrite += GlyphTable[Glyph].Width * GlyphTable[Glyph].Height * 4;
  }

  byte* Data = Platform.AllocateMemory(BytesToWrite);
  byte* Base = Data;

  for (u32 Glyph = 0; Glyph < 256; Glyph++)
  {
    loaded_bitmap Bitmap = GlyphTable[Glyph];
    CopyMemory(Base, &Bitmap, sizeof(Bitmap));
    Base += sizeof(Bitmap);

    umm PixelByteCount = Bitmap.Width * Bitmap.Height * 4;
    CopyMemory(Base, Bitmap.Pixels, PixelByteCount);
    Base += PixelByteCount;
  }

  Platform.WriteEntireFile(Filename, Data, BytesToWrite);

  Platform.DeallocateMemory(Data);
}

void DeserialiseGlyphTable(loaded_bitmap* GlyphTable, char* Filename)
{
  byte* Data = Platform.ReadEntireFile(Filename).Data;
  byte* Base = Data;

  for (u32 Glyph = 0; Glyph < 256; Glyph++)
  {
    loaded_bitmap* Bitmap = (loaded_bitmap*)Base + Glyph;
    Bitmap->Pixels = (byte*)(Bitmap + 1);
    Base += Bitmap->Width * Bitmap->Height * 4;
    GlyphTable[Glyph] = *Bitmap;
  }
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

inline b32 StringsAreEqual(char* String1, char* String2)
{
  b32 Result = String1 == String2;

  if (!Result && String1 && String2)
  {
    while (*String1 && *String2 && (*String1 == *String2))
      String1++, String2++;

    Result = *String1 == 0 && *String2 == 0;
  }

  return Result;
}

inline b32 InRect(v2i Coord, recti Quad)
{
  b32 Result = Coord.E[0] > Quad.Left && Coord.E[0] < Quad.Right
    && Coord.E[1] > Quad.Top && Coord.E[1] < Quad.Bottom;
  return Result;
}

inline b32 MouseLReleased(program_memory* Memory)
{
  b32 Result = Memory->LastInput.Mouse.LButtonDown && !Memory->Input.Mouse.LButtonDown;
  return Result;
}

inline b32 MouseLReleasedIn(program_memory* Memory, recti Rect)
{
  b32 Result = MouseLReleased(Memory) && InRect(Memory->Input.Mouse.XY, Rect);
  return Result;
}

inline b32 MouseLReleasedOutside(program_memory* Memory, recti Rect)
{
  b32 Result = MouseLReleased(Memory) && !InRect(Memory->Input.Mouse.XY, Rect);
  return Result;
}

inline b32 MouseLClicked(program_memory* Memory)
{
  b32 Result = Memory->Input.Mouse.LButtonDown && !Memory->LastInput.Mouse.LButtonDown;
  return Result;
}

inline b32 MouseLClickedIn(program_memory* Memory, recti Rect)
{
  b32 Result = MouseLClicked(Memory) && InRect(Memory->Input.Mouse.XY, Rect);
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

inline b32 IsSelfCapture(program_state* State, u8 SourceTileIndex, u8 TargetTileIndex)
{
  // TODO(robin): Fix using MousePiece everywhere!!!
  piece SourcePiece = State->MousePiece;
  piece TargetPiece = State->CurrentBoard[TargetTileIndex];

  b32 Result = !(TargetPiece == NONE || (IsWhite(SourcePiece) != IsWhite(TargetPiece)));

  return Result;
}

move_array GetMoves(program_state* State, u8 TileIndex)
{
  move_array Result = {0};

  s32 Rank = TileIndex & 7; // TileIndex % 8;
  s32 File = TileIndex >> 3; // TileIndex / 8;
  u8 TileIndex88 = Pos88(TileIndex);

  piece SourcePiece = State->MousePiece;

  if (State->WhiteToMove == IsWhite(SourcePiece))
  {
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

        b32 CanMove1Square = State->CurrentBoard[Tile1Ahead] == NONE;
        b32 CanMove2Squares = CanMove1Square && File == DoubleMoveFile && State->CurrentBoard[Tile2Ahead] == NONE;

        if (CanMove1Square)
        {
          if (!IsSelfCapture(State, TileIndex, Tile1Ahead))
            Result.Moves[Result.Count++] = (move){TileIndex, Tile1Ahead};
        }

        if (CanMove2Squares)
        {
          if (!IsSelfCapture(State, TileIndex, Tile2Ahead))
            Result.Moves[Result.Count++] = (move){TileIndex, Tile2Ahead};
        }

        s32 Diagonals[] = {15, 17};
        for (s32 i = 0; i < ArrayCount(Diagonals); i++)
        {
          u8 TargetIndex88 = TileIndex88 + DirectionModifier * Diagonals[i];
          if (!(TargetIndex88 & 0x88))
          {
            u8 TargetIndex = Pos64(TargetIndex88);
            b32 CanCapture = State->CurrentBoard[TargetIndex] && !IsSelfCapture(State, TileIndex, TargetIndex);

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
            if (!IsSelfCapture(State, TileIndex, TargetIndex))
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
            if (!IsSelfCapture(State, TileIndex, TargetIndex))
              Result.Moves[Result.Count++] = (move){TileIndex, TargetIndex};

            if (State->CurrentBoard[TargetIndex])
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
          State->CurrentBoard[KingSide ? White ? 63 : 7 : White ? 56 : 0] == (White ? ROOK_W : ROOK_B);

        if (State->CanCastle[White + 2 * KingSide] && CastleInCorner)
        {
          b32 CastleIsValid = 1;
          s32 DirectionModifier = KingSide ? 1 : -1;
          for (s32 i = 1; i < (KingSide ? 3 : 4); i++)
          {
            s32 TargetIndex = TileIndex + DirectionModifier * i;

            if (State->CurrentBoard[TargetIndex] != NONE)
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
  }

  return Result;
}

inline b32 LegalMove(program_state* State, u8 From, u8 To)
{
  b32 Result = 0;

  move_array Moves = GetMoves(State, From);
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

  entire_file File = Platform.ReadEntireFile(Filename);

  bmp_header* Header = (bmp_header*)File.Data;

  // TODO(robin): We only support BitmapV5Header currently
  Assert(Header->HeaderSize == 124);

  bmp_header_v5* Header5 = (bmp_header_v5*)(Header + 1);
  Result.Width = Header5->Width;
  Result.Height = Header5->Height;

  Result.Pixels = File.Data + Header->DataOffset;

  return Result;
}

inline recti
RectBorder(recti Rect, s32 Value)
{
  recti Result = Rect;

  Result.Left -= Value;
  Result.Top -= Value;
  Result.Right += Value;
  Result.Bottom += Value;

  return Result;
}

inline recti
RectWH(s32 X, s32 Y, u32 Width, u32 Height)
{
  recti Result = {
    .Left = X,
    .Top = Y,
    .Right = X + Width,
    .Bottom = Y + Height,
  };
  return Result;
}

b32 MoveMade(piece LastBoard[64], piece CurrentBoard[64])
{
  b32 Result = 0;

  for (u32 i = 0; i < 64; i++)
  {
    if (LastBoard[i] != CurrentBoard[i])
    {
      Result = 1;
      break;
    }
  }

  return Result;
}

move MoveFromBoardDelta(piece LastBoard[64], piece CurrentBoard[64])
{
  move Result = {0};

  s32 TileIndices[2] = {0};
  s32 TileIndex = 0;
  for (u32 i = 0; i < 64; i++)
  {
    if (LastBoard[i] != CurrentBoard[i])
    {
      TileIndices[TileIndex++] = i;
    }
  }

  if (CurrentBoard[TileIndices[0]])
  {
    Result.To = TileIndices[0];
    Result.From = TileIndices[1];
  }
  else
  {
    Result.From = TileIndices[0];
    Result.To = TileIndices[1];
  }

  return Result;
}

void Update(program_memory* Memory)
{
  // NOTE(robin): Don't update boards if we're holding a piece
  if (!Memory->State.MousePiece)
  {
    // TODO(robin): Fix this scuffed ass code
    b32 CastledKingSide[2] = {
      Memory->State.LastBoard[4] == KING_B && Memory->State.CurrentBoard[6] == KING_B,
      Memory->State.LastBoard[60] == KING_W && Memory->State.CurrentBoard[62] == KING_W,
    };
    b32 CastledQueenSide[2] = {
      Memory->State.LastBoard[4] == KING_B && Memory->State.CurrentBoard[2] == KING_B,
      Memory->State.LastBoard[60] == KING_W && Memory->State.CurrentBoard[58] == KING_W,
    };

    b32 RookMoved[4] = {
      Memory->State.LastBoard[0] == ROOK_B && Memory->State.CurrentBoard[0] != ROOK_B,
      Memory->State.LastBoard[56] == ROOK_W && Memory->State.CurrentBoard[56] != ROOK_W,
      Memory->State.LastBoard[7] == ROOK_B && Memory->State.CurrentBoard[7] != ROOK_B,
      Memory->State.LastBoard[63] == ROOK_W && Memory->State.CurrentBoard[63] != ROOK_W,
    };

    b32 KingMoved[2] = {
      Memory->State.LastBoard[4] == KING_B && Memory->State.CurrentBoard[4] != KING_B,
      Memory->State.LastBoard[60] == KING_W && Memory->State.CurrentBoard[60] != KING_W,
    };

    if (KingMoved[0])
    {
      Memory->State.CanCastle[0] = Memory->State.CanCastle[2] = 0;
    }

    if (KingMoved[1])
    {
      Memory->State.CanCastle[1] = Memory->State.CanCastle[3] = 0;
    }

    for (u32 i = 0; i < ArrayCount(Memory->State.CanCastle); i++)
    {
      if (RookMoved[i])
      {
        Memory->State.CanCastle[i] = 0;
      }
    }

    for (u32 White = 0; White < ArrayCount(CastledKingSide); White++)
    {
      if (CastledKingSide[White])
      {
        Memory->State.CurrentBoard[White ? 63 : 7] = NONE;
        Memory->State.CurrentBoard[White ? 61 : 5] = White ? ROOK_W : ROOK_B;
        Memory->State.CanCastle[White + 2] = 0;
        Memory->State.CanCastle[White] = 0;
      }

      if (CastledQueenSide[White])
      {
        Memory->State.CurrentBoard[White ? 56 : 0] = NONE;
        Memory->State.CurrentBoard[White ? 59 : 3] = White ? ROOK_W : ROOK_B;
        Memory->State.CanCastle[White + 2] = 0;
        Memory->State.CanCastle[White] = 0;
      }
    }

    if (MoveMade(Memory->State.LastBoard, Memory->State.CurrentBoard))
    {
      move Move = MoveFromBoardDelta(Memory->State.LastBoard, Memory->State.CurrentBoard);
      Memory->State.WhiteToMove = !Memory->State.WhiteToMove;
      PushMove(&Memory->State.MoveHistory, Move);
    }

    CopyArray(Memory->State.LastBoard, Memory->State.CurrentBoard);
  }
}

inline f64 Max(f64 X, f64 Y)
{
  f64 Result = X > Y ? X : Y;
  return Result;
}

inline f64 Min(f64 X, f64 Y)
{
  f64 Result = X < Y ? X : Y;
  return Result;
}

void Render(program_memory* Memory, recti ClipRect)
{
  render_group* RenderGroup = Memory->RenderGroup;
  s32 RenderWidth = ClipRect.Right - ClipRect.Left;
  s32 RenderHeight = ClipRect.Bottom - ClipRect.Top;

  typedef enum
  {
    SCENE_PLAY,
    SCENE_HISTORY,
  } scene;

  local scene CurrentScene = SCENE_PLAY;

  colour ClearColour = {0xFF312EBB};
  ClearColour = ColourAdd(ClearColour, 15);
  PushClear(RenderGroup, ClearColour);

  recti MenuRect = {0, 0, RenderWidth / 5.0, RenderHeight};
  PushRect(RenderGroup, MenuRect, (colour){0xFF323240});
  PushStringCentred(RenderGroup, (v2i){MenuRect.Right/2.0, MenuRect.Top + 30}, "Chess", (colour){~0U});

  // TODO(robin): Find a better way of doing layouts
  u32 ButtonCount = 0;
  button Buttons[32] = {0};

  colour ButtonColour = {0xFF282833};
  colour HoveredButtonColour = ColourAdd(ButtonColour, 20);
  colour TextColour = {0xFFFFFFFF};
  colour HoveredTextColour = {0xFFFFFFFF};

  Buttons[ButtonCount++] = (button){
    {20, 70, MenuRect.Right - 20, 120}, "Play",
      ButtonColour, TextColour,
      HoveredButtonColour, HoveredTextColour,
  };
  Buttons[ButtonCount++] = (button){
    {
      .Left = 20,
        .Top = Buttons[0].Rect.Bottom+20,
        .Right = MenuRect.Right-20,
        .Bottom = Buttons[0].Rect.Bottom + 20 + 50
    },
      "Match History",
      ButtonColour, TextColour,
      HoveredButtonColour, HoveredTextColour,
  };

  for (u32 ButtonIndex = 0; ButtonIndex < ButtonCount; ButtonIndex++)
  {
    button* Button = &Buttons[ButtonIndex];
    do_button_result ButtonResult = DoButton(Memory, *Button);

    if (ButtonResult.LClicked)
    {
      if (Button == &Buttons[0])
      {
        CurrentScene = SCENE_PLAY;
      }
      if (Button == &Buttons[1])
      {
        CurrentScene = SCENE_HISTORY;
      }
    }

  }

  switch (CurrentScene)
  {
    case SCENE_PLAY:
    {
      s32 XOffset = MenuRect.Right + 50;
      s32 YOffset = 50;
      s32 TileSize = 80;
      s32 TileMargin = 10;

      recti BoardRect =
      {
        .Left = XOffset,
        .Top = YOffset,
        .Right = XOffset + 8 * TileSize,
        .Bottom = YOffset + 8 * TileSize,
      };

      recti MatchInfoRect =
      {
        .Left = BoardRect.Right + 20,
        .Top = BoardRect.Top,
        .Right = MatchInfoRect.Left + 220,
        .Bottom = BoardRect.Bottom,
      };

      recti TopInfoRect =
      {
        .Left = MatchInfoRect.Left,
        .Top = MatchInfoRect.Top,
        .Right = MatchInfoRect.Right,
        .Bottom = MatchInfoRect.Top + 100,
      };

      recti BottomInfoRect =
      {
        .Left = MatchInfoRect.Left,
        .Top = MatchInfoRect.Bottom - 100,
        .Right = MatchInfoRect.Right,
        .Bottom = MatchInfoRect.Bottom,
      };

      // NOTE(robin): Put the staged piece back if we release the mouse outside the board
      if (MouseLReleasedOutside(Memory, BoardRect) && Memory->State.MousePiece)
      {
        Memory->State.CurrentBoard[Memory->State.LastMouseTileIndex] = Memory->State.MousePiece;
        Memory->State.MousePiece = NONE;
      }

      for (s32 Y = 0; Y < 8; Y++)
      {
        for (s32 X = 0; X < 8; X++)
        {
          u32 TileIndex = Y * 8 + X;

          recti Tile = {
            BoardRect.Left + TileSize * X,
            BoardRect.Top + TileSize * Y,
            BoardRect.Left + TileSize * (X + 1),
            BoardRect.Top + TileSize * (Y + 1),
          };

          if (MouseLClickedIn(Memory, Tile))
          {
            Memory->State.MousePiece = Memory->State.CurrentBoard[TileIndex];
            Memory->State.CurrentBoard[TileIndex] = NONE;
            Memory->State.LastMouseTileIndex = TileIndex;
          }

          if (MouseLReleasedIn(Memory, Tile))
          {
            if (LegalMove(&Memory->State, Memory->State.LastMouseTileIndex, TileIndex))
            {
              Memory->State.CurrentBoard[TileIndex] = Memory->State.MousePiece;
              Memory->State.MousePiece = NONE;
            }
            else
            {
              Memory->State.CurrentBoard[Memory->State.LastMouseTileIndex] = Memory->State.MousePiece;
              Memory->State.MousePiece = NONE;
            }
          }
        }
      }

      PushBoard(RenderGroup, Memory->State.CurrentBoard, BoardRect);

      // {{{ NOTE(robin): Draw the move history panel

      PushRect(RenderGroup, RectBorder(MatchInfoRect, 5), (colour){0xFF1B1B1B});
      PushRect(RenderGroup, MatchInfoRect, (colour){0xFF1B1B1B});

      recti MoveHistoryRect = {
        .Left = MatchInfoRect.Left,
        .Right = MatchInfoRect.Right,
        .Top = TopInfoRect.Bottom,
        .Bottom = BottomInfoRect.Top,
      };

      recti TextBounds = TextRect("a1a3");
      // NOTE(robin): +1 because we are masking off an extra row of moves
      u32 MaxTurnsToDraw = 1 + RectHeight(MoveHistoryRect) / TextBounds.Bottom;
      u32 TurnIndexOffset = (Memory->State.MoveHistory.Count & 1) ? 1 : 0;
      v2i ScrollCount = {
        .Y = Memory->Input.Mouse.RotatedDown ? 1 : Memory->Input.Mouse.RotatedUp ? -1 : 0
      };

      viewport* Viewport = &Memory->MoveHistoryViewport;
      Viewport->Increment.Y = 10;
      Viewport->YClip = (v2i){
        0,
          0.5 * TextBounds.Bottom * ((Memory->State.MoveHistory.Count - 2) + TurnIndexOffset),
      };
      Viewport->ElementSize = TextBounds;
      Viewport->Start.Y = 0;
      Viewport->End.Y = MaxTurnsToDraw;

      ScrollViewport(Viewport, ScrollCount);

      s32 StartTurnIndex = Viewport->Start.Y;
      s32 EndTurnIndex = Min(Memory->State.MoveHistory.Count / 2 + TurnIndexOffset, Viewport->End.Y);
      for (u32 TurnIndex = StartTurnIndex; TurnIndex < EndTurnIndex; TurnIndex++)
      {
        for (u32 i = 0; i < 2; i++)
        {
          if (TurnIndex * 2 + i < Memory->State.MoveHistory.Count)
          {
            move Move = Memory->State.MoveHistory.Moves[TurnIndex * 2 + i];
            char MoveText[5] = {0};
            BoardIndexToFAN(MoveText, Move.From);
            BoardIndexToFAN(&MoveText[2], Move.To);
            recti TextBounds = TextRect(MoveText);
            s32 X = i == 0 ? MatchInfoRect.Left : MatchInfoRect.Right - TextBounds.Right;
            s32 Y = MoveHistoryRect.Top + (TextBounds.Bottom * (TurnIndex - StartTurnIndex)) - (Viewport->Offset.Y % TextBounds.Bottom);

            PushString(RenderGroup, X, Y, MoveText, (colour){~0U});
          }
        }
      }

      PushRect(RenderGroup, TopInfoRect, ColourAdd(ClearColour, 30));
      PushRect(RenderGroup, BottomInfoRect, (colour){0xFFFFFFFF});

      // }}}

      // NOTE(robin): The texture lags the cursor a bit because the window callback is not called every frame.

      if (Memory->State.MousePiece) // NOTE(robin): We're holding a piece with the mouse
      {
        // NOTE(robin): Highlight the possible moves in red
        move_array PossibleMoves = GetMoves(&Memory->State, Memory->State.LastMouseTileIndex);
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

        // NOTE(robin): Draw the piece bitmap over the cursor
        recti BitmapRect = {
          Memory->Input.Mouse.X - TileSize/2 + TileMargin,
          Memory->Input.Mouse.Y - TileSize/2 + TileMargin,
          Memory->Input.Mouse.X + TileSize/2 - TileMargin,
          Memory->Input.Mouse.Y + TileSize/2 - TileMargin,
        };

        PushBitmap(RenderGroup, &PieceBitmap[Memory->State.MousePiece], BitmapRect, (colour){~0U});
      }

    } break;

    case SCENE_HISTORY:
    {
      PushBoard(RenderGroup, Memory->State.LastBoard, RectWH(MenuRect.Right + 30, 30, 160, 160));
      PushStringCentred(RenderGroup,
          (v2i){MenuRect.Right + (RenderWidth - MenuRect.Right)/2,
          RenderHeight/2.0}, "Some match history stuff", (colour){~0U});
    } break;
  }

  Memory->FrameCounter++;

}

void Init(program_memory* Memory)
{
  Platform = Memory->Platform;

  Memory->PermArena.Size = Megabytes(512);
  Memory->PermArena.Base = Memory->Platform.AllocateMemory(Memory->PermArena.Size);

  char FontPath[2048];
  Memory->GlyphTable = PushArray(&Memory->PermArena, loaded_bitmap, 256);
  GlyphTable = Memory->GlyphTable;
  DeserialiseGlyphTable(Memory->GlyphTable,
      CatStrings(FontPath, 2, Memory->Platform.ExecutableDirectory, "../assets/font.bin"));

  Memory->RenderGroup = PushType(&Memory->PermArena, render_group);
  Memory->RenderGroup->Size = Megabytes(8);
  Memory->RenderGroup->Buffer = PushArray(&Memory->PermArena, byte, Memory->RenderGroup->Size);

  Memory->State.WhiteToMove = 1;
  Memory->State.MoveHistory.Size = 256;
  Memory->State.MoveHistory.Moves = PushArray(&Memory->PermArena, move, Memory->State.MoveHistory.Size);
  Memory->State.BoardHistory.Size = 256;
  Memory->State.BoardHistory.Boards = (piece (*)[64])PushArray(&Memory->PermArena, piece, Memory->State.BoardHistory.Size);

  CopyArray(Memory->State.CurrentBoard, DefaultBoard);
  CopyArray(Memory->State.LastBoard, DefaultBoard);
  for (u32 i = 0; i < ArrayCount(Memory->State.CanCastle); i++)
    Memory->State.CanCastle[i] = 1;

  Memory->State.Running = 1;
}
