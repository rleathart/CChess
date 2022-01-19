#define GL_SILENCE_DEPRECATION

#include <AppKit/AppKit.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>

#include "program.c"

const u32 BytesPerPixel = 4;
const b32 UseOpenGL = false;

u32* GraphicsBuffer;
u32 Width = 1024;
u32 Height = 720;

u32 WindowsPixelToMac(u32 Value)
{
  u8 A = 0xFF & (Value >> 24);
  u8 R = 0xFF & (Value >> 16);
  u8 G = 0xFF & (Value >> 8);
  u8 B = 0xFF & (Value >> 0);
  u32 Result = A << 24 | B << 16 | G << 8 | R;
  return Result;
}

void RenderWeirdGradient()
{
  byte* Row = (byte*)GraphicsBuffer;

  for (u32 Y = 0; Y < Height; Y++)
  {
    u32* Pixel = (u32*)Row;
    for (u32 X = 0; X < Width; X++)
    {
      u8 Red = 0;
      u8 Green = Y;
      u8 Blue = X;

      *Pixel++ = 0xFF << 24 | Blue << 16 | Green << 8 | Red;
    }

    Row += BytesPerPixel * Width;
  }
}

void MacResizeGraphicsBuffer(NSWindow* Window)
{
  if (GraphicsBuffer)
    free(GraphicsBuffer);

  Width = Window.contentView.bounds.size.width; 
  Height = Window.contentView.bounds.size.height; 
  GraphicsBuffer = malloc(Width * Height * BytesPerPixel);
}

void MacRedrawWindow(NSWindow* Window)
{
  @autoreleasepool
  {
    NSBitmapImageRep* Rep = [[[NSBitmapImageRep alloc]
      initWithBitmapDataPlanes: (byte**)&GraphicsBuffer
                    pixelsWide: Width
                    pixelsHigh: Height
                 bitsPerSample: 8
               samplesPerPixel: 4
                      hasAlpha: true
                      isPlanar: false
                colorSpaceName: NSDeviceRGBColorSpace
                   bytesPerRow: BytesPerPixel * Width
                  bitsPerPixel: BytesPerPixel * 8
    ] autorelease];

    NSSize ImageSize = NSMakeSize(Width, Height);
    NSImage* Image = [[[NSImage alloc] initWithSize: ImageSize] autorelease];
    [Image addRepresentation: Rep];
    Window.contentView.layer.contents = Image;
  }
}

file_read_result MacReadEntireFile(char* Filename)
{
  file_read_result Result = {0};

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

global NSOpenGLContext* GlobalGLContext;

@interface main_view : NSOpenGLView
{

}
@end

@implementation main_view
- (id) init
{
  self = [super init];
  return self;
}

- (void) prepareOpenGL
{
  [super prepareOpenGL];
  [[self openGLContext] makeCurrentContext];
}

- (void) reshape
{
  [super reshape];
  NSRect Bounds = [self bounds];
  [GlobalGLContext makeCurrentContext];
  [GlobalGLContext update];
  glViewport(0, 0, Bounds.size.width, Bounds.size.height);
}
@end

void MacInitOpenGL(NSWindow* Window)
{
  NSOpenGLView* OpenGLView;
}

@interface main_window_delegate : NSObject <NSWindowDelegate>
@end

@implementation main_window_delegate

- (void)windowWillClose:(id)sender
{
  Running = false;
}

@end

int main(int argc, char** argv)
{
  main_window_delegate* MainWindowDelegate =
    [[main_window_delegate alloc] init];
  NSRect ScreenRect = [[NSScreen mainScreen] frame];
  NSRect InitialFrame =
    NSMakeRect(ScreenRect.size.width - Width, ScreenRect.size.height - Height,
        Width, Height);

  NSOpenGLPixelFormatAttribute OpenGLAttributes[] = {
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFADepthSize, 24,
    0,
  };

  NSOpenGLPixelFormat* PixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:OpenGLAttributes];
  GlobalGLContext = [[NSOpenGLContext alloc] initWithFormat:PixelFormat shareContext:NULL];


  NSWindow* Window = [[NSWindow alloc]
    initWithContentRect:InitialFrame
              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
              NSWindowStyleMaskMiniaturizable |
              NSWindowStyleMaskResizable
              backing:NSBackingStoreBuffered
                defer:NO];

  [Window setBackgroundColor: NSColor.blackColor];
  [Window setTitle: @"Chess"];
  [Window makeKeyAndOrderFront: nil];
  [Window setDelegate: MainWindowDelegate];
  Window.contentView.wantsLayer = true;

  main_view* GLView = [[main_view alloc] init];
  [GLView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  [GLView setPixelFormat:PixelFormat];
  [GLView setOpenGLContext:GlobalGLContext];
  [GLView setFrame:Window.contentView.bounds];
  [[GLView openGLContext] makeCurrentContext];

  [Window.contentView addSubview:GLView];

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  MacResizeGraphicsBuffer(Window);

  Platform.AllocateMemory = (platform_allocate_memory*)malloc;
  Platform.ReadEntireFile = MacReadEntireFile;
  RenderGroup = AllocateRenderGroup(Megabytes(8));

  CopyArray(CurrentBoard, DefaultBoard);
  CopyArray(LastBoard, DefaultBoard);

  for (u32 Piece = NONE; Piece < CHESS_PIECE_COUNT; Piece++)
  {
    PieceBitmap[Piece] = ReadBitmap((char*)PieceAsset[Piece]);
  }

  while (Running)
  {
    MacRedrawWindow(Window);

    NSEvent* Event;

    do
    {
      Event = [NSApp nextEventMatchingMask: NSEventMaskAny
                                 untilDate: nil
                                    inMode: NSDefaultRunLoopMode
                                   dequeue: true];

      switch ([Event type])
      {
        default:
          [NSApp sendEvent: Event];
      }

    } while (Event);

    RenderGroup->Used = 0;

    Render(RenderGroup);

    for (umm BaseAddress = 0; BaseAddress < RenderGroup->Used;)
    {
      render_entry_header* Header = (render_entry_header*)(RenderGroup->Buffer + BaseAddress);
      BaseAddress += sizeof(*Header);
      void* Data = (byte*)Header + sizeof(*Header);

      if (UseOpenGL)
      {
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
      else
      {
        switch (Header->Type)
        {
          case RENDER_RECT:
            {
              render_entry_rect* Entry = (render_entry_rect*)Data;
              colour Colour = Entry->Colour;

              for (u32 Y = Entry->Top; Y < Entry->Bottom; Y++)
              {
                for (u32 X = Entry->Left; X < Entry->Right; X++)
                {
                  GraphicsBuffer[Y * Width + X] = Colour.A << 24 | Colour.B << 16 | Colour.G << 8 | Colour.R;
                }
              }

              BaseAddress += sizeof(*Entry);
            } break;

          case RENDER_BITMAP:
            {
              render_entry_bitmap* Entry = (render_entry_bitmap*)Data;
              colour Colour = Entry->Colour;

              // TODO(robin): Currently fucked

              for (u32 Y = 0; Y < Entry->Bitmap->Height; Y++)
              {
                for (u32 X = 0; X < Entry->Bitmap->Width; X++)
                {
                  u32 BufX = Entry->Left + X;
                  u32 BufY = Entry->Top + Y;
                  u32 BitmapWidth = Entry->Bitmap->Width;
                  u32 Pixel = Entry->Bitmap->Pixels[Y * BitmapWidth + X];
                  GraphicsBuffer[BufX * BitmapWidth + BufX] =
                    WindowsPixelToMac(Pixel);
                }
              }

#if 0

              float ScaleFactorX = Entry->Bitmap->Width / (float)(Entry->Right - Entry->Left);
              float ScaleFactorY = Entry->Bitmap->Height / (float)(Entry->Bottom - Entry->Top);

              for (u32 Y = Entry->Top; Y < Entry->Bottom; Y++)
              {
                for (u32 X = Entry->Left; X < Entry->Right; X++)
                {
                  u32 BitmapY = (Y - Entry->Top) * ScaleFactorX;
                  u32 BitmapX = (X - Entry->Left) * ScaleFactorY;
                  u32 BitmapWidth = Entry->Bitmap->Width;
                  u32 Pixel = Entry->Bitmap->Pixels[BitmapY * BitmapWidth + BitmapX];
                  u8 A = Pixel >> 24;
                  u8 R = Pixel >> 16;
                  u8 G = Pixel >> 8;
                  u8 B = Pixel >> 0;
                  GraphicsBuffer[Y * Width + X] = 0xFF << 24 | B << 16 | G << 8 | R << 0;
                }
              }

#endif

              BaseAddress += sizeof(*Entry);
            } break;

          case RENDER_CLEAR:
            {
              render_entry_clear* Entry = (render_entry_clear*)Data;
              colour Colour = Entry->Colour;

              for (u32 BufferIndex = 0; BufferIndex < Width * Height; BufferIndex++)
              {
                GraphicsBuffer[BufferIndex] = Colour.A << 24 | Colour.B << 16 | Colour.G << 8 | Colour.R;
              }

              BaseAddress += sizeof(*Entry);
            } break;
        }
      }
    }

  }

  return 0;
}
