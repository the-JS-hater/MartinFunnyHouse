#ifndef _MICROGLUT_
#define _MICROGLUT_

#ifdef __cplusplus
extern "C" {
#endif

// Same or similar to old GLUT calls
void glutMainLoop();
void glutCheckLoop();
void glutInit(int *argcp, char **argv);
void glutPostRedisplay();

void glutReshapeFunc(void (*func)(int width, int height));
void glutDisplayFunc(void (*func)(void));
void glutKeyboardFunc(void (*func)(unsigned char key, int x, int y));
void glutKeyboardUpFunc(void (*func)(unsigned char key, int x, int y));
void glutSpecialFunc(void (*func)(unsigned char key, int x, int y));
void glutSpecialUpFunc(void (*func)(unsigned char key, int x, int y));

void glutMouseFunc(void (*func)(int button, int state, int x, int y));
void glutPassiveMotionFunc(void (*func)(int x, int y));
void glutMotionFunc(void (*func)(int x, int y));

void glutInitWindowPosition (int x, int y);
void glutInitWindowSize (int width, int height);
void glutCreateWindow (const char *windowTitle);

void glutSwapBuffers();

#define GLUT_ELAPSED_TIME		(700)
#define GLUT_WINDOW_WIDTH		(102)
#define GLUT_WINDOW_HEIGHT		(103)
#define GLUT_MOUSE_POSITION_X		(802)
#define GLUT_MOUSE_POSITION_Y		(803)
#define GLUT_QUIT_FLAG			(801)
int glutGet(int type);

void glutInitDisplayMode(unsigned int mode);
void glutIdleFunc(void (*func)(void));
char glutKeyIsDown(unsigned char c);
char glutMouseIsDown(unsigned int c);

// Standard GLUT timer
void glutTimerFunc(int millis, void (*func)(int arg), int arg);
// Ingemar's version
void glutRepeatingTimer(int millis);

void glutInitContextVersion(int major, int minor);

void glutHideCursor();
void glutShowCursor();
void glutWarpPointer(int x, int y);
void glutFullScreen();
void glutExitFullScreen();
void glutToggleFullScreen();
void glutPositionWindow(int x, int y);
void glutReshapeWindow(int width, int height);

/* Mouse buttons. */
#define GLUT_LEFT_BUTTON		0
// No support for middle yet
//#define GLUT_MIDDLE_BUTTON		1
#define GLUT_RIGHT_BUTTON		2

/* Mouse button  state. */
#define GLUT_DOWN			0
#define GLUT_UP				1

// Only some modes supported
#define GLUT_STENCIL			32
//#define GLUT_MULTISAMPLE		128
//#define GLUT_STEREO			256
#define GLUT_RGB			0
#define GLUT_RGBA			GLUT_RGB
#define GLUT_SINGLE			0
#define GLUT_DOUBLE			2
#define GLUT_DEPTH			16



// Special keys.
#define GLUT_KEY_F1			1
#define GLUT_KEY_F2			2
#define GLUT_KEY_F3			3
#define GLUT_KEY_F4			4
#define GLUT_KEY_F5			5
#define GLUT_KEY_F6			6
#define GLUT_KEY_F7			7
// F8 and up ignored since they are not possible on some keyboards - like mine
#define GLUT_KEY_LEFT			28
#define GLUT_KEY_UP				29
#define GLUT_KEY_RIGHT			30
#define GLUT_KEY_DOWN			31
#define GLUT_KEY_PAGE_UP		22
#define GLUT_KEY_PAGE_DOWN		23
#define GLUT_KEY_HOME			24
#define GLUT_KEY_END			25
#define GLUT_KEY_INSERT			26

#define GLUT_KEY_ESC			27
#define GLUT_KEY_TAB			9
#define GLUT_KEY_RETURN			13
#define GLUT_KEY_SPACE			' '

// Modifiers, only supported for glutKeyIsDown
#define GLUT_KEY_SHIFT			19
#define GLUT_KEY_LEFT_SHIFT GLUT_KEY_SHIFT
#define GLUT_KEY_CTRL			20
#define GLUT_KEY_ALT			21


#ifdef __cplusplus
}
#endif


#endif
