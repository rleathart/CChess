#include <windows.h>
#include <windowsx.h>
#include <gl/gl.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef s32 b32;

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

b32 Running = 1;
u32 MouseX = 0;
u32 MouseY = 0;

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
        // Assert(bytes_read = Result.Size);
      }
      else // {{{ Error handling
      {
        // Failed to read Result
      }
    }
    else
    {
      // Failed to get Result Size
    }
    }
    else
    {
      // Failed to open Result...
    }

  CloseHandle(FileHandle);

  return Result;
}

loaded_bitmap ReadBitmap(char* Filename)
{
  loaded_bitmap Result = {0};

  file_read_result File = Win64ReadEntireFile(Filename);

  bmp_header* Header = (bmp_header*)File.Data;

  if (Header->HeaderSize == 124) // NOTE(robin): BitmapV5Header
  {
    bmp_header_v5* Header5 = (bmp_header_v5*)(Header + 1);
    Result.Width = Header5->Width;
    Result.Height = Header5->Height;
  }

  Result.Pixels = File.Data + Header->DataOffset;

  return Result;
}

void Win64ResizeWindow(s32 Width, s32 Height)
{
  glViewport(0, 0, Width, Height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, Width, Height, 0, 0, -1);
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

    case WM_MOUSEMOVE:
    {
      MouseX = GET_X_LPARAM(LParam);
      MouseY = GET_Y_LPARAM(LParam);
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

  loaded_bitmap TestBitmap = ReadBitmap("assets/piece_white_bishop.bmp");

  glGenTextures(1, &TestBitmap.TextureHandle);
  glBindTexture(GL_TEXTURE_2D, TestBitmap.TextureHandle);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TestBitmap.Width, TestBitmap.Height,
      0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, TestBitmap.Pixels);

  glBindTexture(GL_TEXTURE_2D, 0);

  while (Running)
  {
    MSG Message;

    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&Message);
      DispatchMessage(&Message);
    }

    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    s32 XOffset = 100;
    s32 YOffset = 100;
    for (s32 Y = 0; Y < 8; Y++)
    {
      for (s32 X = 0; X < 8; X++)
      {
        // TODO(robin): FIX THIS FFS!
        if (X & 1)
        {
          if (Y & 1)
          {
            glColor3f(1, 1, 1);
          }
          else
          {
            glColor3f(0.2,0.2,0.2);
          }
        }
        else
        {
          if (Y & 1)
          {
            glColor3f(0.2,0.2,0.2);
          }
          else
          {
            glColor3f(1, 1, 1);
          }
        }

        glRectf(XOffset + 80 * X, YOffset + 80 * Y, XOffset + 80 * (X + 1), YOffset + 80 * (Y + 1));
      }
    }

    glBindTexture(GL_TEXTURE_2D, TestBitmap.TextureHandle);

    glEnable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);

    glTexCoord2i(1, 1); glVertex2i(MouseX, MouseY);
    glTexCoord2i(1, 0); glVertex2i(MouseX, MouseY + 100);
    glTexCoord2i(0, 0); glVertex2i(MouseX + 100, MouseY + 100);
    glTexCoord2i(0, 1); glVertex2i(MouseX + 100, MouseY);

    glEnd();

    glDisable(GL_TEXTURE_2D);

    SwapBuffers(WindowDC);
  }

  return 0;
};
