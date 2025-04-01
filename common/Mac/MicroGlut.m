// Rebuilt MicroGlut for Mac 2023
// Major point: Fixes an elusive bug.
// Other: Skipped some junk that we don't need, like special function key callback
// (which is integrated with the standard one anyway) and some more.
// Pre-3.2 not supported any more. Easy to fix, or just use the old version.

// No context settings. Everything is always included (Z-buffer, stencil, multisampling).

// Based on Cocoa-GL-Tutorial

#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>

#include <sys/time.h>
//#include "GL_utilities.h"
//#include "LittleOBJLoader.h"
//#include "LoadTGA.h"
//#include "VectorUtils3.h"
#include "MicroGlut.h"

#define GL_SILENCE_DEPRECATION

void (*gDisplay)(void);
void (*gReshape)(int width, int height);
void (*gKey)(unsigned char key, int x, int y);
//void (*gSpecialKey)(unsigned char key, int x, int y);
void (*gKeyUp)(unsigned char key, int x, int y);
//void (*gSpecialKeyUp)(unsigned char key, int x, int y);
void (*gMouseMoved)(int x, int y);
void (*gMouseDragged)(int x, int y);
void (*gMouseFunc)(int button, int state, int x, int y);

int gWindowInitWidth = 400;
int gWindowInitHeight = 400;
int gWindowInitPositionX = 30;
int gWindowInitPositionY = 60;
char gKeymap[256];
char gButtonPressed[10] = {0,0,0,0,0,0,0,0,0,0};
char gTitle[255] = "MicroGlut window"; // Saved title, so we can restore after full screen!
char gRunning = 1;

static struct timeval timeStart;

void glutReshapeFunc(void (*func)(int width, int height))
{
	gReshape = func;
}
void glutDisplayFunc(void (*func)(void))
{
	gDisplay = func;
}
void glutKeyboardFunc(void (*func)(unsigned char key, int x, int y))
{
	gKey = func;
}
void glutMouseFunc(void (*func)(int button, int state, int x, int y))
{
	gMouseFunc = func;
}

void glutPassiveMotionFunc(void (*func)(int x, int y))
{
	gMouseMoved = func;
}

void glutMotionFunc(void (*func)(int x, int y))
{
	gMouseDragged = func;
}

void glutInitWindowSize (int width, int height)
{
	gWindowInitWidth = width;
	gWindowInitHeight = height;
}

void glutInitWindowPosition (int x, int y)
{
	gWindowInitPositionX = x;
	gWindowInitPositionY = y;
}

static void doKeyboardEvent(NSEvent *theEvent, void (*func)(unsigned char key, int x, int y),
		//void (*specialfunc)(unsigned char key, int x, int y),
		int keyMapValue)
{
	unsigned char *chars;
	
	chars = (unsigned char *)[[theEvent characters] cStringUsingEncoding: NSMacOSRomanStringEncoding];
	NSPoint mouseDownPos = [theEvent locationInWindow];

	if (chars != NULL)
	{
		if (func != NULL) // Change 120913
			func(chars[0], mouseDownPos.x, mouseDownPos.y); // TO DO: x and y
		
		gKeymap[chars[0]] = keyMapValue;
	}
	else
	{
		char code;
		switch( [theEvent keyCode] )
		{
			case 0x35: code = GLUT_KEY_ESC; break;
			case 0x30: code = GLUT_KEY_TAB; break;
			case 0x24: code = GLUT_KEY_RETURN; break;
			case 0x31: code = GLUT_KEY_SPACE; break;
			case 0x29: code = GLUT_KEY_SEMICOLON; break;
			case 0x2B: code = GLUT_KEY_COMMA; break;
			case 0x2F: code = GLUT_KEY_DECIMAL; break;
			case 0x32: code = GLUT_KEY_GRAVE; break;
			case 0x27: code = GLUT_KEY_QUOTE; break;
			case 0x21: code = GLUT_KEY_LBRACKET; break;
			case 0x1E: code = GLUT_KEY_RBRACKET; break;
			case 0x2A: code = GLUT_KEY_BACKSLASH; break;
			case 0x2C: code = GLUT_KEY_SLASH; break;
			case 0x18:
			case 0x51: code = GLUT_KEY_EQUAL; break;

			case 126: code = GLUT_KEY_UP; break;
			case 125: code = GLUT_KEY_DOWN; break;
			case 124: code = GLUT_KEY_RIGHT; break;
			case 123: code = GLUT_KEY_LEFT; break;
			case 122: code = GLUT_KEY_F1; break;
			case 120: code = GLUT_KEY_F2; break;
			case 99: code = GLUT_KEY_F3; break;
			case 118: code = GLUT_KEY_F4; break;
			case 96: code = GLUT_KEY_F5; break;
			case 97: code = GLUT_KEY_F6; break;
			case 98: code = GLUT_KEY_F7; break;
			case 115: code = GLUT_KEY_HOME; break; // ?
			case 116: code = GLUT_KEY_PAGE_UP; break;
			case 119: code = GLUT_KEY_END; break; // ?
			case 121: code = GLUT_KEY_PAGE_DOWN; break;
			case 117: code = GLUT_KEY_INSERT; break; // Looks more like DEL?
			default: code = [theEvent keyCode];
		}
//		if (specialfunc != NULL) // Change 130114
//			specialfunc(code, mouseDownPos.x, mouseDownPos.y); // TO DO: x and y
//		else // If no special, send to normal (future preferred way)
		if (func != NULL) // Change 150114
			func(code, mouseDownPos.x, mouseDownPos.y); // TO DO: x and y
// NOTE: This was a bug until I modified the special key constants! We can now check gKeymap with normal ASCII and special codes with the same table!
		gKeymap[(int)code] = keyMapValue;
	}
}




@interface NanoView : NSView <NSWindowDelegate> 
{
	NSOpenGLContext * m_context;
@public
	int id;
}
-(void)drawRect:(NSRect)rect;
-(void)keyDown:(NSEvent *)theEvent;
-(void)keyUp:(NSEvent *)theEvent;
-(void)mouseMoved:(NSEvent *)theEvent;
-(void)mouseDragged:(NSEvent *)theEvent;
-(void)mouseDown:(NSEvent *)theEvent;
-(void)mouseUp:(NSEvent *)theEvent;
-(id)initWithFrame:(NSRect)aRect;
-(void)windowDidResize:(NSNotification *)note;
-(BOOL)isFlipped;
@end

NSAutoreleasePool *pool = NULL;
NSApplication *NSApp = NULL;
int windowCount = 0;
NanoView *theView = NULL;
NSWindow *window;
float lastWidth, lastHeight;
NSPoint gMousePosition;


@implementation NanoView

-(void)drawRect:(NSRect)rect
{
    [m_context makeCurrentContext];
	[m_context clearDrawable];
	[m_context setView:self];
	
	if (gDisplay)
		gDisplay();
	
    [m_context flushBuffer];
	[NSOpenGLContext clearCurrentContext];
}

-(void)windowWillClose:(NSNotification *)note
{
    [[NSApplication sharedApplication] terminate:self];
}

-(id)initWithFrame:(NSRect)aRect
{
	NSOpenGLPixelFormatAttribute pixelFormatAttributes[] =
	{
		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
		NSOpenGLPFAColorSize    , 24,
		NSOpenGLPFAAlphaSize    , 8,
		NSOpenGLPFADoubleBuffer ,
		NSOpenGLPFAAccelerated  ,
		NSOpenGLPFANoRecovery   ,
		NSOpenGLPFADepthSize, 32,
		NSOpenGLPFAMultisample,
		NSOpenGLPFAStencilSize, 8,
		NSOpenGLPFASampleBuffers, (NSOpenGLPixelFormatAttribute)1,
		NSOpenGLPFASamples, (NSOpenGLPixelFormatAttribute)4,
		0
	};
    NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes];
    m_context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
    self = [super initWithFrame:aRect];
    [m_context makeCurrentContext];
    return self;
}

-(void)keyDown:(NSEvent *)theEvent
{
	doKeyboardEvent(theEvent, gKey, 1);
}

-(void)keyUp:(NSEvent *)theEvent
{
	doKeyboardEvent(theEvent, gKeyUp, 0);
}

-(void) mouseDown:(NSEvent *)theEvent
{
	NSPoint p;
	[m_context makeCurrentContext];
	
	gButtonPressed[0] = 1;

	// Convert location in window to location in view
	p = [theEvent locationInWindow];
	p = [self convertPoint: p fromView: nil];
	if (gMouseFunc)
		gMouseFunc(0, GLUT_DOWN, p.x, p.y);
}

-(void) mouseUp:(NSEvent *)theEvent
{
	NSPoint p;
	[m_context makeCurrentContext];
	
	gButtonPressed[0] = 0;

	// Convert location in window to location in view
	p = [theEvent locationInWindow];
	p = [self convertPoint: p fromView: nil];
	if (gMouseFunc)
		gMouseFunc(0, GLUT_UP, p.x, p.y);
}

-(void)mouseDragged:(NSEvent *)theEvent
{
	NSPoint p;
	[m_context makeCurrentContext];
	
	// Convert location in window to location in view
	p = [theEvent locationInWindow];
	p = [self convertPoint: p fromView: nil];
		if (gMouseDragged)
			gMouseDragged(p.x, p.y);
}

-(void)mouseMoved:(NSEvent *)theEvent
{
	NSPoint p;
	[m_context makeCurrentContext];
	
	// Convert location in window to location in view
	gMousePosition = [theEvent locationInWindow];

	p = [theEvent locationInWindow];
	p = [self convertPoint: p fromView: nil];
	if (gMouseMoved)
		gMouseMoved(p.x, p.y);
}

- (BOOL)acceptsFirstResponder	{ return YES; }
- (BOOL)becomeFirstResponder	{ return YES; }
- (BOOL)resignFirstResponder	{ return YES; }

-(void)windowDidResize:(NSNotification *)note
{
	[m_context setView: theView]; // Make the view current in case reshape calls OpenGL
	[m_context makeCurrentContext]; // 131011
	glViewport(0, 0, [theView frame].size.width, [theView frame].size.height); // I am not 100% sure this is needed

	lastWidth = [theView frame].size.width;
	lastHeight = [theView frame].size.height;

	printf("Resize to %f %f\n", lastWidth, lastHeight);

	if (gReshape)
		gReshape([theView frame].size.width, [theView frame].size.height);
}

-(BOOL)isFlipped
{
	return YES;
}

@end

// -------------------- Timer ------------------------

// Mini-mini class for the timer
@interface TimerController : NSObject { }
-(void)timerFireMethod:(NSTimer *)t;
@end

NSTimer	*gTimer;
TimerController	*myTimerController;

// Timer!
@implementation TimerController
-(void)timerFireMethod:(NSTimer *)t;
{
	[theView setNeedsDisplay: YES];
}
@end

void glutRepeatingTimer(int millis)
{
	if (gTimer)
	{
		[gTimer invalidate];
//		[gTimer release];
		gTimer = NULL;
	}
	if (millis > 0)
	{
		gTimer = [NSTimer
			scheduledTimerWithTimeInterval: millis/1000.0
			target: myTimerController
			selector: @selector(timerFireMethod:)
			userInfo: nil
			repeats: YES];
	}
}

int glutCreateWindow(const char *windowTitle) // and window and view
{
    if (!pool)
    	pool = [NSAutoreleasePool new];
    if (!NSApp)
    	NSApp = [NSApplication sharedApplication];
    [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];
//    NSRect frame = NSMakeRect( gWindowInitPositionX, gWindowInitPositionY, gWindowInitWidth, gWindowInitHeight );
	NSRect frame = NSMakeRect(gWindowInitPositionX, NSScreen.mainScreen.frame.size.height - gWindowInitPositionY-gWindowInitHeight, gWindowInitWidth, gWindowInitHeight);
    
    window = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                      defer:false];
    
    [window setTitle:@"Testing"];
   	[window setTitle: [[NSString alloc] initWithCString:windowTitle
				encoding:NSMacOSRomanStringEncoding]];

    
    theView = [[[NanoView alloc] initWithFrame:frame] autorelease];
    theView->id = windowCount++;
    
    [window setAcceptsMouseMovedEvents:YES];
    
   	[NSApp activateIgnoringOtherApps:YES];
   	[window setDelegate: (NanoView*)theView];
	[window setContentView: theView];
	[window makeKeyAndOrderFront: nil];
	[window makeFirstResponder: theView];
	
	lastWidth = gWindowInitWidth;
	lastHeight = gWindowInitHeight;
	
	return 0; // Should be window index if I get around to support multi-window.
}


@interface MGApplication : NSApplication
@end

@implementation MGApplication
/* Invoked from the Quit menu item */
- (void)terminate:(id)sender
{
	gRunning = 0;
}
@end

void CreateMenu()
{
	NSMenu *mainMenu, *theMiniMenu;
	NSMenuItem *menuItem2, *dummyItem;

	// Create main menu = menu bar
	mainMenu = NSMenu.alloc;
	[mainMenu initWithTitle: @""];
	[NSApp setMainMenu: mainMenu];

	// Create the custom menu
	theMiniMenu = NSMenu.alloc;
	[theMiniMenu initWithTitle: @"The MiniMenu"];
	
	// Create menu items with standard messages
	menuItem2 = NSMenuItem.alloc;
	[menuItem2 initWithTitle: @"Hide" action: @selector(hide:) keyEquivalent: @"h"];
	[menuItem2 setKeyEquivalentModifierMask: NSEventModifierFlagCommand];
	[theMiniMenu addItem: menuItem2];
	
	menuItem2 = NSMenuItem.alloc;
	[menuItem2 initWithTitle: @"Hide others" action: @selector(hideOtherApplications:) keyEquivalent: @"h"];
	[menuItem2 setKeyEquivalentModifierMask: NSEventModifierFlagCommand | NSEventModifierFlagOption];
	[theMiniMenu addItem: menuItem2];
	
	menuItem2 = NSMenuItem.alloc;
	[menuItem2 initWithTitle: @"Show all" action: @selector(unhideAllApplications:) keyEquivalent: @"h"];
	[menuItem2 setKeyEquivalentModifierMask: NSEventModifierFlagCommand | NSEventModifierFlagControl];
	[theMiniMenu addItem: menuItem2];
	
	menuItem2 = NSMenuItem.alloc;
	[menuItem2 initWithTitle: @"Quit" action: @selector(terminate:) keyEquivalent: @"q"];
	[menuItem2 setKeyEquivalentModifierMask: NSEventModifierFlagCommand];
	[theMiniMenu addItem: menuItem2];
	
	// Adding a menu is done with a dummy item to connect the menu to its parent
	dummyItem = NSMenuItem.alloc;
	[dummyItem initWithTitle: @"" action: nil keyEquivalent: @""];
	[dummyItem setSubmenu: theMiniMenu];

	[mainMenu addItem: dummyItem];
}

void home()
{
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
	char path[PATH_MAX];
	if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
	{
		// error!
		return;
	}
	CFRelease(resourcesURL);

	chdir(path);
//	printf("Current Path: %s\n", path);
}

void glutInit(int *argcp, char **argv)
{
	NSApp = [MGApplication sharedApplication];

	[NSApp setActivationPolicy: NSApplicationActivationPolicyRegular]; // Thanks to Marcus Stenbäck

	home();
	gettimeofday(&timeStart, NULL);
	CreateMenu();
	gRunning = 1;

   	myTimerController = [TimerController alloc];
}

void glutMainLoop()
{
	NSEvent *event;
	
	[NSApp finishLaunching];
	
	// Added 2023-09-14
	if (gReshape)
		gReshape([theView frame].size.width, [theView frame].size.height);

 	while (gRunning)
	{
		event = [NSApp nextEventMatchingMask: NSEventMaskAny
						untilDate: [NSDate distantFuture]
						inMode: NSDefaultRunLoopMode
						dequeue: true
						];
		[NSApp sendEvent: event];
	}

    [pool release];
}

void glutCheckEvents()
{
	NSEvent *event;
	
	event = [NSApp nextEventMatchingMask: NSEventMaskAny
					untilDate: [NSDate distantFuture]
					inMode: NSDefaultRunLoopMode
					dequeue: true
					];
	[NSApp sendEvent: event];
}

void glutPostRedisplay()
{
	[theView setNeedsDisplay: YES];
}

void glutSwapBuffers()
{
}

// Placeholders
void glutInitDisplayMode(unsigned int mode)
{
}
void glutInitContextVersion(int mainVersion, int minorVersion)
{
}

void glutSetWindowTitle(const char *title)
{
	strcpy(gTitle, title);
	[window setTitle: [NSString stringWithUTF8String: title]];
}

char glutKeyIsDown(unsigned char c)
{
	if (c == GLUT_KEY_SHIFT)
	{
		return (([NSEvent modifierFlags] & NSEventModifierFlagShift) != 0);
	}
	if (c == GLUT_KEY_CTRL)
	{
		return (([NSEvent modifierFlags] & NSEventModifierFlagControl) != 0);
	}
	if (c == GLUT_KEY_ALT)
	{
		return (([NSEvent modifierFlags] & NSEventModifierFlagOption) != 0);
	}
	return gKeymap[(unsigned int)c];
}

char glutMouseIsDown(unsigned char c)
{
	return gButtonPressed[(unsigned int)c];
}



void glutWarpPointer(int x, int y)
{
	NSPoint mp;
	CGPoint pt;
	NSRect r;
	
	mp.x = 0; mp.y = 0;

	NSRect mpmp;
	mpmp.size.height = 0;
	mpmp.size.width = 0;
	mpmp.origin = mp;
	mpmp = [window convertRectToScreen: mpmp];
	mp = mpmp.origin;

// Flip to downwards Y
	mp.y = NSScreen.mainScreen.frame.size.height - mp.y;

// Get the view size
	r = [[window contentView] frame];

// Add the x, y offset, minus the view frame height to move to upper left.
	pt.x = x + mp.x;
	pt.y = y - r.size.height + mp.y;

	CGWarpMouseCursorPosition(pt);
	CGAssociateMouseAndMouseCursorPosition(true);
}

// Should be used for auto-show-hide on activate/deactivate
char hidden = 0;
NSRect gSavedWindowPosition;
char gFullScreen = 0;

void glutShowCursor()
{
	if (hidden)
	{
		[NSCursor unhide];
		hidden = 0;
	}
}
void glutHideCursor()
{
	if (!hidden)
	{
		[NSCursor hide];
		hidden = 1;
	}
}

void glutFullScreen()
{
	if (gFullScreen == 0)
	{
		gFullScreen = 1;
		
		gSavedWindowPosition = [window frame];
		[NSMenu setMenuBarVisible: FALSE];
		[window setStyleMask: NSWindowStyleMaskBorderless];
		[window setFrame: NSScreen.mainScreen.frame display: true];

		[window makeKeyAndOrderFront: nil];
		[window makeFirstResponder: theView];
	}
}

// Any reason why we should wait with resizing?
// This page says we should: https://www.opengl.org/documentation/specs/glut/spec3/node24.html

// This call is not in the original GLUT but IMHO better than relying on glutReshapeWindow or glutPositionWindow to exit!
// (These calls also exit full screen mode though.)
void glutExitFullScreen()
{
	if (gFullScreen != 0)
	{
		gFullScreen = 0;
		[NSMenu setMenuBarVisible: TRUE];
//		[window setStyleMask: NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask]; // Back to normal
		[window setStyleMask: NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable]; // Back to normal
		[window setFrame: gSavedWindowPosition display: true];

		[window makeKeyAndOrderFront: nil];
		[window makeFirstResponder: theView];

		[window setTitle: [NSString stringWithUTF8String: gTitle]];
	}
}

// When I am at it... this is what I really want!
void glutToggleFullScreen()
{
	if (gFullScreen)
		glutExitFullScreen();
	else
		glutFullScreen();
}

void glutExit()
{
	gRunning = 0;
}

int glutGet(int type)
{
	struct timeval tv;
	
	switch (type)
	{
	case GLUT_QUIT_FLAG:
		return !gRunning;
		break;
	case GLUT_WINDOW_WIDTH:
		return lastWidth;
		break;
	case GLUT_WINDOW_HEIGHT:
		return lastHeight;
		break;
	case GLUT_ELAPSED_TIME:
		gettimeofday(&tv, NULL);
		return (tv.tv_usec - timeStart.tv_usec) / 1000 + (tv.tv_sec - timeStart.tv_sec)*1000;
		break;
	case GLUT_MOUSE_POSITION_X:
		return gMousePosition.x;
		break;
	case GLUT_MOUSE_POSITION_Y:
		return gMousePosition.y;
		break;
	}
	return 0;
}
