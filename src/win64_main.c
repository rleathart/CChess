#include <windows.h>
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

b32 Running = 1;

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

  ReleaseDC(Window, WindowDC);
}

void Win64ResizeWindow(s32 Width, s32 Height)
{
  glViewport(0, 0, Width, Height);
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

  while (Running)
  {
    MSG Message;

    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&Message);
      DispatchMessage(&Message);
    }

    glBegin(GL_QUADS);

    glColor3f(0, 1, 0);
    glVertex2f(0, 0);
    glVertex2f(0, 1);
    glVertex2f(1, 1);
    glVertex2f(1, 0);

    glEnd();

    SwapBuffers(WindowDC);
  }

  return 0;
};
