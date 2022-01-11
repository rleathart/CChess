#include "program.c"

// NOTE(robin): Make sure to include and platform specific headers afer program code
// so as to cause compile errors if platform code is used in the program layer.

// TODO(robin): Replace windows and gl headers
#include <windows.h>
#include <windowsx.h>
#include <gl/gl.h>

void* Win64AllocateMemory(umm Bytes)
{
  void* Result = VirtualAlloc(0, Bytes, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  return Result;
}

void Win64WriteToStdOut(char* Buffer)
{
  u32 BytesWritten = 0;
  u32 BytesToWrite = StringLength(Buffer);
  WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), Buffer, BytesToWrite, &BytesWritten, 0);
  Assert(BytesWritten == BytesToWrite);
}

file_read_result Win64ReadEntireFile(char* Filename)
{
  file_read_result Result = {0};
  HANDLE FileHandle;
  if ((FileHandle =
        CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0)) != INVALID_HANDLE_VALUE)
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
  }
  else
  {
    Assert(!"Failed to open file");
  }

  CloseHandle(FileHandle);

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

  switch (Message)
  {
    case WM_DESTROY:
    case WM_QUIT:
    {
      Running = 0;
    } break;

    case WM_SIZE:
    {
      RECT ClientRect;
      GetClientRect(Window, &ClientRect);

      s32 Width = ClientRect.right - ClientRect.left;
      s32 Height = ClientRect.bottom - ClientRect.top;
      Win64ResizeWindow(Width, Height);
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

  Platform.ReadEntireFile = Win64ReadEntireFile;
  Platform.StdOut = Win64WriteToStdOut;
  Platform.AllocateMemory = Win64AllocateMemory;

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
      0);

  HDC WindowDC = GetDC(Window);

  Win64InitOpenGL(Window);

  RenderGroup = AllocateRenderGroup(Megabytes(8));

  // NOTE(robin): Read our piece bitmaps and upload them to the GPU
  for (u32 Piece = NONE; Piece < CHESS_PIECE_COUNT; Piece++)
  {
    char BitmapPath[MAX_PATH] = {0};
    PieceBitmap[Piece] = ReadBitmap((char*)CatStrings(BitmapPath, 3, EXEDirectory, "../", PieceAsset[Piece]));
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

  CopyArray(CurrentBoard, DefaultBoard);
  CopyArray(LastBoard, DefaultBoard);

  while (Running)
  {
    RenderGroup->Used = 0;
    Update();

    MSG Message;

    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
      LPARAM LParam = Message.lParam;

      switch (Message.message)
      {
        case WM_MOUSEMOVE:
        {
          Input.Mouse.X = GET_X_LPARAM(LParam);
          Input.Mouse.Y = GET_Y_LPARAM(LParam);
        } break;

        case WM_LBUTTONDOWN:
        {
          Input.Mouse.LButtonDown = 1;
        } break;

        case WM_LBUTTONUP:
        {
          Input.Mouse.LButtonDown = 0;
        } break;

        default:
        {
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        };
      }
    }

    Render(RenderGroup);

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

    SwapBuffers(WindowDC);
  }

  return 0;
};
