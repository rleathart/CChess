#define GL_SILENCE_DEPRECATION

#include <AppKit/AppKit.h>
#include <OpenGL/gl.h>

#include "program.c"

NSOpenGLContext* GLContext;

void DrawRenderGroupOpenGL(render_group* RenderGroup)
{
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
}

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

global NSOpenGLContext* GlobalGLContext;

@interface main_application_delegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end

@implementation main_application_delegate

- (void) windowDidResize:(NSNotification *)notification
{
  MacResizeWindow([notification object]);
}

- (void)windowWillClose:(id)sender
{
  Running = false;
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

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  Platform.AllocateMemory = (platform_allocate_memory*)malloc;
  Platform.ReadEntireFile = MacReadEntireFile;
  RenderGroup = AllocateRenderGroup(Megabytes(8));

  CopyArray(CurrentBoard, DefaultBoard);
  CopyArray(LastBoard, DefaultBoard);

  for (u32 Piece = NONE; Piece < CHESS_PIECE_COUNT; Piece++)
  {
    PieceBitmap[Piece] = ReadBitmap((char*)PieceAsset[Piece]);
    PieceBitmap[Piece].Transform = FLIP_VERTICAL;
    glGenTextures(1, &PieceBitmap[Piece].TextureHandle);
    glBindTexture(GL_TEXTURE_2D, PieceBitmap[Piece].TextureHandle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PieceBitmap[Piece].Width, PieceBitmap[Piece].Height,
        0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, PieceBitmap[Piece].Pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  char FontPath[260] = {0};
  GlyphTableP = DeserialiseGlyphTable(CatStrings(FontPath, 2,
        "/Users/Robin/src/clones/CChess", "/assets/font.bin"));

  for (u32 Glyph = 0; Glyph < ArrayCount(GlyphTable); Glyph++)
  {
    glGenTextures(1, &GlyphTableP[Glyph].TextureHandle);
    glBindTexture(GL_TEXTURE_2D, GlyphTableP[Glyph].TextureHandle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GlyphTableP[Glyph].Width, GlyphTableP[Glyph].Height,
        0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, GlyphTableP[Glyph].Pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  MacResizeWindow(Window);

  while (Running)
  {
    LastInput = Input;

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
              NSRect RectInWindow =
                [Window convertRectFromScreen:NSMakeRect(Mouse.x, Mouse.y, 1, 1)];
              NSPoint PointInWindow = RectInWindow.origin;
              Mouse = [[Window contentView] convertPoint:PointInWindow fromView:nil];

              Input.Mouse.X = Mouse.x;
              Input.Mouse.Y = Window.contentView.bounds.size.height - Mouse.y;
            }

            [NSApp sendEvent: Event];
          } break;

        case NSEventTypeLeftMouseUp:
          {
            Input.Mouse.LButtonDown = 0;
            [NSApp sendEvent: Event];
          } break;

        case NSEventTypeLeftMouseDown:
          {
            Input.Mouse.LButtonDown = 1;
            [NSApp sendEvent: Event];
          } break;

        default:
          [NSApp sendEvent: Event];
      }

    } while (Event);

    s32 Width = Window.contentView.frame.size.width;
    s32 Height = Window.contentView.frame.size.height;

    recti ClipRect = {0, 0, Width, Height};

    Update();
    RenderGroup->Used = 0;
    Render(RenderGroup, ClipRect);
    DrawRenderGroupOpenGL(RenderGroup);

    [GLContext flushBuffer];
  }

  return 0;
}
