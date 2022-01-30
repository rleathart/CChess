#include "program.c"

// NOTE(robin): Make sure to include and platform specific headers afer program code
// so as to cause compile errors if platform code is used in the program layer.

// TODO(robin): Replace windows and gl headers
#include <windows.h>
#include <windowsx.h>
#include <gl/gl.h>

#include "gl.c"

inline void* Win64AllocateMemory(umm Bytes)
{
  void* Result = VirtualAlloc(0, Bytes, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  return Result;
}

inline void Win64DeallocateMemory(void* Memory)
{
  VirtualFree(Memory, 0, MEM_RELEASE);
}

void Win64WriteToStdOut(char* Buffer)
{
  u32 BytesWritten = 0;
  u32 BytesToWrite = StringLength(Buffer);
  WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), Buffer, BytesToWrite, &BytesWritten, 0);
  Assert(BytesWritten == BytesToWrite);
}

entire_file Win64ReadEntireFile(char* Filename)
{
  entire_file Result = {0};
  HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if (FileHandle != INVALID_HANDLE_VALUE)
  {
    if (GetFileSizeEx(FileHandle, (PLARGE_INTEGER)&Result.Size))
    {
      Result.Data = (byte*)VirtualAlloc(0, Result.Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
      u32 bytes_read = 0;
      if (ReadFile(FileHandle, Result.Data, Result.Size, &bytes_read, 0))
      {
        Assert(bytes_read == Result.Size);
      }
      else
      {
        Assert(!"Failed to read file");
      }
    }
    else
    {
      Assert(!"Failed to get file size");
    }

    CloseHandle(FileHandle);
  }
  else
  {
    Assert(!"Failed to open file");
  }

  return Result;
}

b32 Win64WriteEntireFile(char* Filename, void* Data, umm BytesToWrite)
{
  b32 Result = 0;
  HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  if (FileHandle != INVALID_HANDLE_VALUE)
  {
    u32 BytesWritten = 0;
    if (WriteFile(FileHandle, Data, BytesToWrite, &BytesWritten, 0))
    {
      Result = BytesWritten == BytesToWrite;
    }
    else
    {
      Assert(!"Failed to write file");
    }

    CloseHandle(FileHandle);
  }
  else
  {
    Assert(!"Failed to open file");
  }

  return Result;
}

void Win64ResizeWindow(s32 Width, s32 Height)
{
  glViewport(0, 0, Width, Height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, Width, Height, 0, 0, -1);
}

void Win64Dirname(char* OutPath, char* Path)
{
  char* OnePastLastSlash = Path;
  for (char* Scan = Path; *Scan; ++Scan)
  {
    if (*Scan == '\\')
    {
      OnePastLastSlash = Scan + 1;
    }
  }
  CopyMemory(OutPath, Path, OnePastLastSlash - Path);
}

internal loaded_bitmap
Win64LoadGlyphBitmap(u32 Glyph)
{
  loaded_bitmap Result = {0};

  local HDC DeviceContext;
  if (!DeviceContext)
  {
    DeviceContext = CreateCompatibleDC(0);
    HBITMAP Bitmap = CreateCompatibleBitmap(DeviceContext, 1024, 1024);
    SelectObject(DeviceContext, Bitmap);

    s32 Height = 20;
    HFONT Font = CreateFontW(Height, 0, 0, 0,
        400, // FW_NORMAL
        0,
        0,
        0,
        1, // DEFAULT_CHARSET
        0, // OUT_DEFAULT_PRECIS
        0, // CLIP_DEFAULT_PRECIS
        4, // ANTIALIASED_QUALITY
        0|(0 << 4), // DEFAULT_PITCH|FF_DONTCARE
        L"Consolas"
        );
    SelectObject(DeviceContext, Font);
  }

  u16 CheesePoint = Glyph;

  SIZE Size;
  GetTextExtentPoint32W(DeviceContext, &CheesePoint, 1, &Size);

  s32 Width = Size.cx;
  s32 Height = Size.cy;

  TextOutW(DeviceContext, 0, 0, &CheesePoint, 1);

  Result.Width = Width;
  Result.Height = Height;
  Result.Pixels = (byte*)Platform.AllocateMemory(Width * Height * 4);

  u32* Dest = (u32*)Result.Pixels;
  for (s32 Y = 0; Y < Height; Y++)
  {
    for (s32 X = 0; X < Width; X++)
    {
      u8 Alpha = 0xFF - (GetPixel(DeviceContext, X, Y) & 0xFF);
      *Dest++ = Alpha << 24 | Alpha << 16 | Alpha << 8  | Alpha << 0;
    }
  }

  // NOTE(robin): Upload our texture to the GPU
  glGenTextures(1, &Result.TextureHandle);

  glBindTexture(GL_TEXTURE_2D, Result.TextureHandle);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height,
      0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, Result.Pixels);
  glBindTexture(GL_TEXTURE_2D, 0);

  // TODO(robin): Do we want to free the pixel memory here?

  return Result;
}

internal void
Win64InitGlyphTable(void)
{
  for (u16 Glyph = 0; Glyph < 256; Glyph++)
  {
    GlyphTable[Glyph] = Win64LoadGlyphBitmap(Glyph);
  }
}

void Win64InitOpenGL(HWND Window)
{
  HDC WindowDC = GetDC(Window);

  PIXELFORMATDESCRIPTOR DesiredPixelFormat = {0};
  DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
  DesiredPixelFormat.nVersion = 1;
  DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
  DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
  DesiredPixelFormat.cColorBits = 32;
  DesiredPixelFormat.cAlphaBits = 8;
  DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;

  s32 SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
  PIXELFORMATDESCRIPTOR SuggestedPixelFormat = {0};
  DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex, sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
  SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);

  HGLRC OpenGLRC = wglCreateContext(WindowDC);
  wglMakeCurrent(WindowDC, OpenGLRC);

  RECT ClientRect;
  GetClientRect(Window, &ClientRect);

  s32 Width = ClientRect.right - ClientRect.left;
  s32 Height = ClientRect.bottom - ClientRect.top;

  Win64ResizeWindow(Width, Height);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  ReleaseDC(Window, WindowDC);
}

LRESULT MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
  LRESULT Result = 0;

  local program_memory* Memory;

  switch (Message)
  {
    case WM_CREATE:
    {
      CREATESTRUCTA* Create = (CREATESTRUCTA*)LParam;
      Memory = (program_memory*)Create->lpCreateParams;
    } break;

    case WM_DESTROY:
    case WM_QUIT:
    {
      Memory->State.Running = 0;
    } break;

    case WM_SIZE:
    {
      recti ClientRect;
      GetClientRect(Window, (RECT*)&ClientRect);

      s32 Width = ClientRect.Right - ClientRect.Left;
      s32 Height = ClientRect.Bottom - ClientRect.Top;
      Win64ResizeWindow(Width, Height);
      if (Memory->RenderGroup)
      {
        Memory->RenderGroup->Used = 0;
        Render(Memory, ClientRect);
        DrawRenderGroupOpenGL(Memory->RenderGroup);
        SwapBuffers(GetDC(Window));
      }
    } break;

    default:
    {
      Result = DefWindowProcA(Window, Message, WParam, LParam);
    };
  }

  return Result;
}

int WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, char* CommandLine, int CommandShow)
{
  // NOTE(robin): Find where our executable is so we can specify asset locations relative to the binary
  char EXEFileName[MAX_PATH] = {0};
  char EXEDirectory[MAX_PATH] = {0};
  GetModuleFileNameA(0, EXEFileName, sizeof(EXEFileName));
  Win64Dirname(EXEDirectory, EXEFileName);

  // NOTE(robin): Here we are responsible for setting up Memory.Platform. Everything else is handled by Init(&Memory)
  program_memory Memory = {0};
  Memory.Platform.AllocateMemory = Win64AllocateMemory;
  Memory.Platform.DeallocateMemory = Win64DeallocateMemory;
  Memory.Platform.ReadEntireFile = Win64ReadEntireFile;
  Memory.Platform.WriteEntireFile = Win64WriteEntireFile;
  Memory.Platform.ExecutableFilename = EXEFileName;
  Memory.Platform.ExecutableDirectory = EXEDirectory;

  Init(&Memory);
  InitGL(&Memory);

  WNDCLASS WindowClass = {0};
  WindowClass.style = CS_VREDRAW|CS_HREDRAW|CS_OWNDC;
  WindowClass.lpfnWndProc = MainWindowCallback;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "ChessWindowClass";

  RegisterClass(&WindowClass);

  HWND Window = CreateWindowEx(0, WindowClass.lpszClassName, "Chess", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      0, 0,
      Instance,
      &Memory);

  HDC WindowDC = GetDC(Window);

  Win64InitOpenGL(Window);

  for (u32 Glyph = 0; Glyph < 256; Glyph++)
  {
    glGenTextures(1, &GlyphTable[Glyph].TextureHandle);
    glBindTexture(GL_TEXTURE_2D, GlyphTable[Glyph].TextureHandle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GlyphTable[Glyph].Width, GlyphTable[Glyph].Height,
        0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, GlyphTable[Glyph].Pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  // NOTE(robin): Read our piece bitmaps and upload them to the GPU
  for (u32 Piece = NONE; Piece < CHESS_PIECE_COUNT; Piece++)
  {
    char BitmapPath[MAX_PATH] = {0};
    PieceBitmap[Piece] = ReadBitmap(CatStrings(BitmapPath, 3, EXEDirectory, "../", PieceAsset[Piece]));
    PieceBitmap[Piece].Transform = FLIP_VERTICAL;
    glGenTextures(1, &PieceBitmap[Piece].TextureHandle);
    glBindTexture(GL_TEXTURE_2D, PieceBitmap[Piece].TextureHandle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PieceBitmap[Piece].Width, PieceBitmap[Piece].Height,
        0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, PieceBitmap[Piece].Pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    VirtualFree(PieceBitmap[Piece].Pixels, 0, MEM_RELEASE);
  }

  while (Memory.State.Running)
  {
    Memory.Input.Mouse.RotatedUp = 0;
    Memory.Input.Mouse.RotatedDown = 0;
    Memory.LastInput = Memory.Input;

    MSG Message;
    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
      LPARAM LParam = Message.lParam;
      WPARAM WParam = Message.wParam;

      switch (Message.message)
      {
        case WM_MOUSEMOVE:
        {
          Memory.Input.Mouse.X = GET_X_LPARAM(LParam);
          Memory.Input.Mouse.Y = GET_Y_LPARAM(LParam);
        } break;
        
        case WM_MOUSEWHEEL:
        {
          s16 WheelDist = GetBits(WParam, 16, 31);
          Memory.Input.Mouse.RotatedUp = WheelDist > 0;
          Memory.Input.Mouse.RotatedDown = WheelDist < 0;
        } break;

        case WM_LBUTTONDOWN:
        {
          Memory.Input.Mouse.LButtonDown = 1;
        } break;

        case WM_LBUTTONUP:
        {
          Memory.Input.Mouse.LButtonDown = 0;
        } break;

        default:
        {
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        };
      }
    }

    recti ClientRect;
    GetClientRect(Window, (LPRECT)&ClientRect);

    Memory.RenderGroup->Used = 0;
    Update(&Memory);
    Render(&Memory, ClientRect);
    DrawRenderGroupOpenGL(Memory.RenderGroup);

    SwapBuffers(WindowDC);
  }

  return 0;
};
