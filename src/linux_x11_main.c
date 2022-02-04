#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "program.c"
#include "gl.c"

entire_file LinuxReadEntireFile(char* Filename)
{
  entire_file Result = {0};

  FILE* File = fopen(Filename, "rb");
  fseek(File, 0, SEEK_END);
  s64 FileSize = ftell(File);
  fseek(File, 0, SEEK_SET);

  Result.Size = FileSize;
  Result.Data = Platform.AllocateMemory(FileSize);
  fread(Result.Data, FileSize, 1, File);
  fclose(File);

  return Result;
}

char* Dirname(char* OutPath, char* Path)
{
  char* OnePastLastSlash = Path;
  for (char* Scan = Path; *Scan; ++Scan)
  {
    if (*Scan == '/')
    {
      OnePastLastSlash = Scan + 1;
    }
  }
  CopyMemory(OutPath, Path, OnePastLastSlash - Path);

  OutPath[OnePastLastSlash - Path] = 0;

  return OutPath;
}

int main(int argc, char** argv)
{
  Display* XDisplay = XOpenDisplay(NULL);
  int Screen = DefaultScreen(XDisplay);
  Window XWindow = XCreateSimpleWindow(XDisplay,
      RootWindow(XDisplay, Screen), 10, 10,
      1024, 768,
      0, 0, 0);
  XSelectInput(XDisplay, XWindow, ExposureMask | KeyPressMask | KeyReleaseMask |
      ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
  XMapWindow(XDisplay, XWindow);

  int Attributes[] =
  {
    GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, 0,
  };

  XVisualInfo* VisualInfo = glXChooseVisual(XDisplay, 0, Attributes);
  GLXContext GLContext = glXCreateContext(XDisplay, VisualInfo, 0, 1);
  glXMakeCurrent(XDisplay, XWindow, GLContext);

  char EXEFilename[1024] = {0};
  char EXEDirectory[1024] = {0};
  int _ = readlink("/proc/self/exe", EXEFilename, sizeof(EXEFilename));

  program_memory Memory = {0};
  Memory.Platform.AllocateMemory = (platform_allocate_memory*)malloc;
  Memory.Platform.ReadEntireFile = (platform_read_entire_file*)LinuxReadEntireFile;
  Memory.Platform.ExecutableFilename = EXEFilename;
  Memory.Platform.ExecutableDirectory = Dirname(EXEDirectory, EXEFilename);

  Init(&Memory);
  InitGL(&Memory);

  while (Memory.State.Running)
  {
    Memory.LastInput = Memory.Input;

    XEvent MouseEvent;
    XQueryPointer(XDisplay, XWindow,
        &MouseEvent.xbutton.root,
        &MouseEvent.xbutton.window,
        &MouseEvent.xbutton.x_root,
        &MouseEvent.xbutton.y_root,
        &MouseEvent.xbutton.x,
        &MouseEvent.xbutton.y,
        &MouseEvent.xbutton.state);

    Memory.Input.Mouse.RotatedDown = 0;
    Memory.Input.Mouse.RotatedUp = 0;
    Memory.Input.Mouse.LButtonDown = (MouseEvent.xbutton.state & Button1Mask) ? 1 : 0;
    Memory.Input.Mouse.X = MouseEvent.xbutton.x;
    Memory.Input.Mouse.Y = MouseEvent.xbutton.y;

    while (XEventsQueued(XDisplay, QueuedAfterReading))
    {
      XEvent Event;
      XNextEvent(XDisplay, &Event);

      switch (Event.type)
      {
        case ButtonPress:
        {
          if (Event.xbutton.button == Button4)
            Memory.Input.Mouse.RotatedUp = 1;
          if (Event.xbutton.button == Button5)
            Memory.Input.Mouse.RotatedDown = 1;
        } break;
      }
    }

    XWindowAttributes WindowAttributes = {0};
    XGetWindowAttributes(XDisplay, XWindow, &WindowAttributes);
    glViewport(0, 0, WindowAttributes.width, WindowAttributes.height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WindowAttributes.width, WindowAttributes.height, 0, 0, -1);

    recti ClipRect = {0, 0, WindowAttributes.width, WindowAttributes.height};

    Update(&Memory);
    Memory.RenderGroup->Used = 0;
    Render(&Memory, ClipRect);
    DrawRenderGroupOpenGL(Memory.RenderGroup);

    glXSwapBuffers(XDisplay, XWindow);
  }

  XCloseDisplay(XDisplay);
  return 0;
}
