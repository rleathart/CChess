#define GL_SILENCE_DEPRECATION

#include <AppKit/AppKit.h>
#include <OpenGL/gl.h>

#include "program.c"
#include "gl.c"

NSOpenGLContext* GLContext;

entire_file MacReadEntireFile(char* Filename)
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

void MacResizeWindow(NSWindow* Window)
{
  s32 Width = Window.contentView.frame.size.width;
  s32 Height = Window.contentView.frame.size.height;

  glViewport(0, 0, 2*Width, 2*Height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, Width, Height, 0, 0, -1);

  [GLContext update];
}

char* MacDirname(char* OutPath, char* Path)
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

global program_memory Memory;

@interface main_application_delegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end

@implementation main_application_delegate

- (void) windowDidResize:(NSNotification *)notification
{
  MacResizeWindow([notification object]);
}

- (void)windowWillClose:(id)sender
{
  Memory.State.Running = false;
}

@end

int main(int argc, char** argv)
{
  NSApplication* App = [NSApplication sharedApplication];
  [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];

  main_application_delegate* MainAppDelegate = [[main_application_delegate alloc] init];
  [App setDelegate: MainAppDelegate];
  [NSApp finishLaunching];

  NSRect ScreenRect = [[NSScreen mainScreen] frame];
  NSRect Frame = NSMakeRect(0, 0, 1280, 720);
  NSWindow* Window = [[NSWindow alloc] initWithContentRect:Frame
                                                 styleMask:NSWindowStyleMaskTitled
                                                 | NSWindowStyleMaskClosable
                                                 | NSWindowStyleMaskMiniaturizable
                                                 | NSWindowStyleMaskResizable
                                                 backing:NSBackingStoreBuffered
                                                   defer:NO];

  [Window makeKeyAndOrderFront:nil];
  [Window setDelegate:MainAppDelegate];
  [Window center];

  NSOpenGLPixelFormatAttribute Attributes[] =
  {
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADoubleBuffer,
    0
  };

  NSOpenGLPixelFormat* Format = [[NSOpenGLPixelFormat alloc] initWithAttributes:Attributes];
  GLContext = [[NSOpenGLContext alloc] initWithFormat:Format shareContext:NULL];
  [Format release];

  int SwapMode = 1;
  [GLContext setValues:&SwapMode forParameter:NSOpenGLContextParameterSwapInterval];
  [GLContext setView:[Window contentView]];
  [GLContext makeCurrentContext];

  Memory = (program_memory){0};
  Memory.Platform.AllocateMemory = (platform_allocate_memory*)malloc;
  Memory.Platform.ReadEntireFile = MacReadEntireFile;
  char EXEDirectory[2048];
  Memory.Platform.ExecutableFilename = (char*)[NSRunningApplication.currentApplication.executableURL fileSystemRepresentation];
  Memory.Platform.ExecutableDirectory = MacDirname(EXEDirectory, Memory.Platform.ExecutableFilename);

  Init(&Memory);
  InitGL(&Memory);

  MacResizeWindow(Window);

  while (Memory.State.Running)
  {
    Memory.LastInput = Memory.Input;

    NSEvent* Event;
    do
    {
      Event = [NSApp nextEventMatchingMask: NSEventMaskAny
                                 untilDate: nil
                                    inMode: NSDefaultRunLoopMode
                                   dequeue: true];

      switch ([Event type])
      {
        case NSEventTypeLeftMouseDragged:
        case NSEventTypeRightMouseDragged:
        case NSEventTypeMouseMoved:
          {
            NSPoint Mouse = [NSEvent mouseLocation];

            if (NSPointInRect(Mouse, Window.frame))
            {
              NSRect WindowRect =
                [Window convertRectFromScreen:NSMakeRect(Mouse.x, Mouse.y, 1, 1)];
              Mouse = [[Window contentView] convertPoint:WindowRect.origin fromView:nil];

              Memory.Input.Mouse.X = Mouse.x;
              Memory.Input.Mouse.Y = Window.contentView.bounds.size.height - Mouse.y;
            }

            [NSApp sendEvent: Event];
          } break;

        case NSEventTypeLeftMouseUp:
          {
            Memory.Input.Mouse.LButtonDown = 0;
            [NSApp sendEvent: Event];
          } break;

        case NSEventTypeLeftMouseDown:
          {
            Memory.Input.Mouse.LButtonDown = 1;
            [NSApp sendEvent: Event];
          } break;

        default:
          [NSApp sendEvent: Event];
      }

    } while (Event);

    s32 Width = Window.contentView.frame.size.width;
    s32 Height = Window.contentView.frame.size.height;

    recti ClipRect = {0, 0, Width, Height};

    Update(&Memory);
    Memory.RenderGroup->Used = 0;
    Render(&Memory, ClipRect);
    DrawRenderGroupOpenGL(Memory.RenderGroup);

    [GLContext flushBuffer];
  }

  return 0;
}
