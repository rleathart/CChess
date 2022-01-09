#include "program.c"

// NOTE(robin): Make sure to include and platform specific headers afer program code
// so as to cause compile errors if platform code is used in the program layer.

// TODO(robin): Replace windows and gl headers
#include <windows.h>
#include <windowsx.h>
#include <gl/gl.h>

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
  Platform.ReadEntireFile = Win64ReadEntireFile;
  Platform.StdOut = Win64WriteToStdOut;

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

  // NOTE(robin): Read our piece bitmaps and upload them to the GPU
  for (u32 Piece = NONE; Piece < CHESS_PIECE_COUNT; Piece++)
  {
    PieceBitmap[Piece] = ReadBitmap((char*)PieceAsset[Piece]);
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

    // TODO(robin): Create a proper pushbuffer renderer in the program layer

    colour ClearColour = {0xFF312EBB};
    glClearColor(ClearColour.R/255.0, ClearColour.G/255.0, ClearColour.B/255.0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

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
        (TileIndex + (Y & 1)) & 1 ? glColor3f(0.2, 0.2, 0.2) : glColor3f(1, 1, 1);

        recti Tile = {
          XOffset + TileSize * X,
          YOffset + TileSize * Y,
          XOffset + TileSize * (X + 1),
          YOffset + TileSize * (Y + 1),
        };

        glRectiv(&Tile.E[0], &Tile.E[2]);

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

          glBindTexture(GL_TEXTURE_2D, PieceBitmap[Piece].TextureHandle);
          glEnable(GL_TEXTURE_2D);
          glBegin(GL_QUADS);

          glColor3f(1, 1, 1);
          glTexCoord2i(1, 1); glVertex2i(TileRect.Left, TileRect.Top);
          glTexCoord2i(1, 0); glVertex2i(TileRect.Left, TileRect.Bottom);
          glTexCoord2i(0, 0); glVertex2i(TileRect.Right, TileRect.Bottom);
          glTexCoord2i(0, 1); glVertex2i(TileRect.Right, TileRect.Top);

          glEnd();
          glDisable(GL_TEXTURE_2D);
          glBindTexture(GL_TEXTURE_2D, 0);
        }
      }
    }

    // NOTE(robin): The texture lags the cursor a bit because the window callback is not called every frame.

    if (MousePiece) // NOTE(robin): We're holding a piece with the mouse
    {
      // NOTE(robin): Draw the piece bitmap over the cursor
      s32 Left = Input.Mouse.X - TileSize/2 + TileMargin;
      s32 Top = Input.Mouse.Y - TileSize/2 + TileMargin;
      s32 Right = Input.Mouse.X + TileSize/2 - TileMargin;
      s32 Bottom = Input.Mouse.Y + TileSize/2 - TileMargin;

      glBindTexture(GL_TEXTURE_2D, PieceBitmap[MousePiece].TextureHandle);

      glEnable(GL_TEXTURE_2D);

      glBegin(GL_QUADS);

      glColor3f(1, 1, 1);
      glTexCoord2i(1, 1); glVertex2i(Left, Top);
      glTexCoord2i(1, 0); glVertex2i(Left, Bottom);
      glTexCoord2i(0, 0); glVertex2i(Right, Bottom);
      glTexCoord2i(0, 1); glVertex2i(Right, Top);

      glEnd();

      glDisable(GL_TEXTURE_2D);

      // NOTE(robin): Also highlight the possible moves in red
      move_array PossibleMoves = GetMoves(LastMouseTileIndex);
      glColor4f(0.8, 0, 0, 0.5);
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
        glRectiv(&Tile.E[0], &Tile.E[2]);
      }
    }

    SwapBuffers(WindowDC);
  }

  return 0;
};
