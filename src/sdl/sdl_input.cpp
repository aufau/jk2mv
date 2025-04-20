#include <SDL.h>
#include "../qcommon/qcommon.h"
#include "../qcommon/q_shared.h"
#include "../client/client.h"
#include "../sys/sys_local.h"

struct in_gamepad_s {
	SDL_GameController *controller;
	qboolean buttonUINavigationActive;
	qboolean rtDown;
	qboolean ltDown;
	float residual_dx;
	float residual_dy;
	int lastLSEventTime;
	int nextRSXEventTime;
	int nextRSYEventTime;
};

struct in_gamepad_s in_pad;

static cvar_t *in_keyboardDebug     = NULL;

static SDL_Joystick *stick = NULL;

static qboolean mouseAvailable = qfalse;
static qboolean mouseActive = qfalse;

static cvar_t *in_mouse             = NULL;
static cvar_t *in_nograb;

static cvar_t *in_joystick			= NULL;
static cvar_t *in_joystickThreshold = NULL;
static cvar_t *in_joystickNo        = NULL;
static cvar_t *in_joystickUseAnalog = NULL;

static cvar_t *in_gamepad                   = NULL;
static cvar_t *in_gamepadNo                 = NULL;
static cvar_t *in_gamepadUIHack             = NULL;
static cvar_t *in_gamepadUISensitivity      = NULL;
static cvar_t *in_gamepadRSInvertX          = NULL;
static cvar_t *in_gamepadRSInvertY          = NULL;
static cvar_t *in_gamepadRSAccel            = NULL;
static cvar_t *in_gamepadRSAccelCurve       = NULL;
static cvar_t *in_gamepadRSSquareDeadzone   = NULL;
static cvar_t *in_gamepadRSInnerDeadzone    = NULL;
static cvar_t *in_gamepadRSOuterDeadzone    = NULL;
static cvar_t *in_gamepadLSSquareDeadzone   = NULL;
static cvar_t *in_gamepadLSInnerDeadzone    = NULL;
static cvar_t *in_gamepadLSOuterDeadzone    = NULL;
static cvar_t *in_gamepadLTInnerDeadzone    = NULL;
static cvar_t *in_gamepadLTOuterDeadzone    = NULL;
static cvar_t *in_gamepadRTInnerDeadzone    = NULL;
static cvar_t *in_gamepadRTOuterDeadzone    = NULL;
static cvar_t *in_gamepadTriggersAxis       = NULL;


static SDL_Window *SDL_window = NULL;

extern void GLimp_SaveWindowPosition( void );

/*
===============
IN_PrintKey
===============
*/
static void IN_PrintKey( const SDL_Keysym *keysym, fakeAscii_t key, qboolean down )
{
	if( down )
		Com_Printf( "+ " );
	else
		Com_Printf( "  " );

	Com_Printf( "Scancode: 0x%02x(%s) Sym: 0x%02x(%s)",
			keysym->scancode, SDL_GetScancodeName( keysym->scancode ),
			keysym->sym, SDL_GetKeyName( keysym->sym ) );

	if( keysym->mod & KMOD_LSHIFT )   Com_Printf( " KMOD_LSHIFT" );
	if( keysym->mod & KMOD_RSHIFT )   Com_Printf( " KMOD_RSHIFT" );
	if( keysym->mod & KMOD_LCTRL )    Com_Printf( " KMOD_LCTRL" );
	if( keysym->mod & KMOD_RCTRL )    Com_Printf( " KMOD_RCTRL" );
	if( keysym->mod & KMOD_LALT )     Com_Printf( " KMOD_LALT" );
	if( keysym->mod & KMOD_RALT )     Com_Printf( " KMOD_RALT" );
	if( keysym->mod & KMOD_LGUI )     Com_Printf( " KMOD_LGUI" );
	if( keysym->mod & KMOD_RGUI )     Com_Printf( " KMOD_RGUI" );
	if( keysym->mod & KMOD_NUM )      Com_Printf( " KMOD_NUM" );
	if( keysym->mod & KMOD_CAPS )     Com_Printf( " KMOD_CAPS" );
	if( keysym->mod & KMOD_MODE )     Com_Printf( " KMOD_MODE" );
	if( keysym->mod & KMOD_RESERVED ) Com_Printf( " KMOD_RESERVED" );

	Com_Printf( " Q:0x%02x(%s)\n", key, Key_KeynumToString( key ) );
}

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static bool IN_NumLockEnabled( void )
{
#if defined(_WIN32)
	return (GetKeyState( VK_NUMLOCK ) & 1) != 0;
#else
	// @fixme : doesn't give proper state if numlock is on before app startup
	return (SDL_GetModState() & KMOD_NUM) != 0;
#endif
}

static void IN_TranslateNumpad( SDL_Keysym *keysym, fakeAscii_t *key )
{
	if ( IN_NumLockEnabled() )
	{
		switch ( keysym->sym )
		{
		case SDLK_KP_0:
			keysym->scancode = SDL_SCANCODE_0;
			keysym->sym = SDLK_0;
			*key = A_0;
			break;
		case SDLK_KP_1:
			keysym->scancode = SDL_SCANCODE_1;
			keysym->sym = SDLK_1;
			*key = A_1;
			break;
		case SDLK_KP_2:
			keysym->scancode = SDL_SCANCODE_2;
			keysym->sym = SDLK_2;
			*key = A_2;
			break;
		case SDLK_KP_3:
			keysym->scancode = SDL_SCANCODE_3;
			keysym->sym = SDLK_3;
			*key = A_3;
			break;
		case SDLK_KP_4:
			keysym->scancode = SDL_SCANCODE_4;
			keysym->sym = SDLK_4;
			*key = A_4;
			break;
		case SDLK_KP_5:
			keysym->scancode = SDL_SCANCODE_5;
			keysym->sym = SDLK_5;
			*key = A_5;
			break;
		case SDLK_KP_6:
			keysym->scancode = SDL_SCANCODE_6;
			keysym->sym = SDLK_6;
			*key = A_6;
			break;
		case SDLK_KP_7:
			keysym->scancode = SDL_SCANCODE_7;
			keysym->sym = SDLK_7;
			*key = A_7;
			break;
		case SDLK_KP_8:
			keysym->scancode = SDL_SCANCODE_8;
			keysym->sym = SDLK_8;
			*key = A_8;
			break;
		case SDLK_KP_9:
			keysym->scancode = SDL_SCANCODE_9;
			keysym->sym = SDLK_9;
			*key = A_9;
			break;
		default:
			break;
		}
	}
}

/*
===============
IN_TranslateSDLToJKKey
===============
*/
static fakeAscii_t IN_TranslateSDLToJKKey( SDL_Keysym *keysym, qboolean down ) {
	fakeAscii_t key = A_NULL;

	if ( keysym->sym >= A_LOW_A && keysym->sym <= A_LOW_Z )
		key = (fakeAscii_t)(A_CAP_A + (keysym->sym - A_LOW_A));
	else if ( keysym->sym >= A_LOW_AGRAVE && keysym->sym <= A_LOW_THORN && keysym->sym != A_DIVIDE )
		key = (fakeAscii_t)(A_CAP_AGRAVE + (keysym->sym - A_LOW_AGRAVE));
	else if ( keysym->sym >= SDLK_SPACE && keysym->sym < SDLK_DELETE )
		key = (fakeAscii_t)keysym->sym;
	else
	{
		IN_TranslateNumpad( keysym, &key );

		switch( keysym->sym )
		{
			case SDLK_PAGEUP:       key = A_PAGE_UP;       break;
			case SDLK_KP_9:         key = A_KP_9;          break;
			case SDLK_PAGEDOWN:     key = A_PAGE_DOWN;     break;
			case SDLK_KP_3:         key = A_KP_3;          break;
			case SDLK_KP_7:         key = A_KP_7;          break;
			case SDLK_HOME:         key = A_HOME;          break;
			case SDLK_KP_1:         key = A_KP_1;          break;
			case SDLK_END:          key = A_END;           break;
			case SDLK_KP_4:         key = A_KP_4;          break;
			case SDLK_LEFT:         key = A_CURSOR_LEFT;   break;
			case SDLK_KP_6:         key = A_KP_6;          break;
			case SDLK_RIGHT:        key = A_CURSOR_RIGHT;  break;
			case SDLK_KP_2:         key = A_KP_2;          break;
			case SDLK_DOWN:         key = A_CURSOR_DOWN;   break;
			case SDLK_KP_8:         key = A_KP_8;          break;
			case SDLK_UP:           key = A_CURSOR_UP;     break;
			case SDLK_ESCAPE:       key = A_ESCAPE;        break;
			case SDLK_KP_ENTER:     key = A_KP_ENTER;      break;
			case SDLK_RETURN:       key = A_ENTER;         break;
			case SDLK_TAB:          key = A_TAB;           break;
			case SDLK_F1:           key = A_F1;            break;
			case SDLK_F2:           key = A_F2;            break;
			case SDLK_F3:           key = A_F3;            break;
			case SDLK_F4:           key = A_F4;            break;
			case SDLK_F5:           key = A_F5;            break;
			case SDLK_F6:           key = A_F6;            break;
			case SDLK_F7:           key = A_F7;            break;
			case SDLK_F8:           key = A_F8;            break;
			case SDLK_F9:           key = A_F9;            break;
			case SDLK_F10:          key = A_F10;           break;
			case SDLK_F11:          key = A_F11;           break;
			case SDLK_F12:          key = A_F12;           break;

			case SDLK_BACKSPACE:    key = A_BACKSPACE;     break;
			case SDLK_KP_PERIOD:    key = A_KP_PERIOD;     break;
			case SDLK_DELETE:       key = A_DELETE;        break;
			case SDLK_PAUSE:        key = A_PAUSE;         break;

			case SDLK_LSHIFT:
			case SDLK_RSHIFT:       key = A_SHIFT;         break;

			case SDLK_LCTRL:
			case SDLK_RCTRL:        key = A_CTRL;          break;

			case SDLK_RALT:			key = A_ALT2;          break;
			case SDLK_LALT:         key = A_ALT;           break;

			case SDLK_KP_5:         key = A_KP_5;          break;
			case SDLK_INSERT:       key = A_INSERT;        break;
			case SDLK_KP_0:         key = A_KP_0;          break;
			case SDLK_KP_MULTIPLY:  key = A_MULTIPLY;      break;
			case SDLK_KP_PLUS:      key = A_KP_PLUS;       break;
			case SDLK_KP_MINUS:     key = A_KP_MINUS;      break;
			case SDLK_KP_DIVIDE:    key = A_DIVIDE;        break;

			case SDLK_SCROLLLOCK:   key = A_SCROLLLOCK;    break;
			case SDLK_NUMLOCKCLEAR: key = A_NUMLOCK;       break;
			case SDLK_CAPSLOCK:     key = A_CAPSLOCK;      break;

			case L'\u00D7':			key = A_MULTIPLY;		break;
			case L'\u00E0':			key = A_LOW_AGRAVE;		break;
			case L'\u00E1':			key = A_LOW_AACUTE;		break;
			case L'\u00E2':			key = A_LOW_ACIRCUMFLEX; break;
			case L'\u00E3':			key = A_LOW_ATILDE;		break;
			case L'\u00E4':			key = A_LOW_ADIERESIS;	break;
			case L'\u00E5':			key = A_LOW_ARING;		break;
			case L'\u00E6':			key = A_LOW_AE;			break;
			case L'\u00E7':			key = A_LOW_CCEDILLA;	break;
			case L'\u00E8':			key = A_LOW_EGRAVE;		break;
			case L'\u00E9':			key = A_LOW_EACUTE;		break;
			case L'\u00EA':			key = A_LOW_ECIRCUMFLEX; break;
			case L'\u00EB':			key = A_LOW_EDIERESIS;	break;
			case L'\u00EC':			key = A_LOW_IGRAVE;		break;
			case L'\u00ED':			key = A_LOW_IACUTE;		break;
			case L'\u00EE':			key = A_LOW_ICIRCUMFLEX; break;
			case L'\u00EF':			key = A_LOW_IDIERESIS;	break;
			case L'\u00F0':			key = A_LOW_ETH;		break;
			case L'\u00F1':			key = A_LOW_NTILDE;		break;
			case L'\u00F2':			key = A_LOW_OGRAVE;		break;
			case L'\u00F3':			key = A_LOW_OACUTE;		break;
			case L'\u00F4':			key = A_LOW_OCIRCUMFLEX; break;
			case L'\u00F5':			key = A_LOW_OTILDE;		break;
			case L'\u00F6':			key = A_LOW_ODIERESIS;	break;
			case L'\u00F7':			key = A_DIVIDE;			break;
			case L'\u00F8':			key = A_LOW_OSLASH;		break;
			case L'\u00F9':			key = A_LOW_UGRAVE;		break;
			case L'\u00FA':			key = A_LOW_UACUTE;		break;
			case L'\u00FB':			key = A_LOW_UCIRCUMFLEX; break;
			case L'\u00FC':			key = A_LOW_UDIERESIS;	break;
			case L'\u00FD':			key = A_LOW_YACUTE;		break;
			case L'\u00FE':			key = A_LOW_THORN;		break;
			case L'\u00FF':			key = A_LOW_YDIERESIS;	break;

			default:
				break;
		}
	}

	if( in_keyboardDebug->integer )
		IN_PrintKey( keysym, key, down );

	return key;
}

/*
===============
IN_GobbleMotionEvents
===============
*/
static void IN_GobbleMotionEvents( void )
{
	SDL_Event dummy[ 1 ];
	int val = 0;

	// Gobble any mouse motion events
	SDL_PumpEvents( );
	while( ( val = SDL_PeepEvents( dummy, 1, SDL_GETEVENT,
		SDL_MOUSEMOTION, SDL_MOUSEMOTION ) ) > 0 ) { }

	if ( val < 0 )
		Com_Printf( "IN_GobbleMotionEvents failed: %s\n", SDL_GetError( ) );
}

/*
===============
IN_ActivateMouse
===============
*/
static void IN_ActivateMouse( void )
{
	if (!mouseAvailable || !SDL_WasInit( SDL_INIT_VIDEO ) )
		return;

	if( !mouseActive )
	{
		SDL_SetRelativeMouseMode( SDL_TRUE );
		SDL_SetWindowGrab( SDL_window, SDL_TRUE );

		IN_GobbleMotionEvents( );
	}

	// in_nograb makes no sense in fullscreen mode
	if( in_nograb->modified || !mouseActive )
	{
		if( !(SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_FULLSCREEN) )
		{
			if( in_nograb->integer )
			{
				SDL_SetRelativeMouseMode( SDL_FALSE );
				SDL_SetWindowGrab( SDL_window, SDL_FALSE );
			}
			else
			{
				SDL_SetRelativeMouseMode( SDL_TRUE );
				SDL_SetWindowGrab( SDL_window, SDL_TRUE );
			}
		}

		in_nograb->modified = qfalse;
	}

	mouseActive = qtrue;
}

/*
===============
IN_DeactivateMouse
===============
*/
static void IN_DeactivateMouse( void )
{
	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
		return;

	// Always show the cursor when the mouse is disabled,
	// but not when fullscreen
	if( !(SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_FULLSCREEN) )
		SDL_ShowCursor( 1 );

	if( !mouseAvailable )
		return;

	if( mouseActive )
	{
		IN_GobbleMotionEvents( );

		SDL_SetWindowGrab( SDL_window, SDL_FALSE );
		SDL_SetRelativeMouseMode( SDL_FALSE );

		// Don't warp the mouse unless the cursor is within the window
		if( SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_MOUSE_FOCUS )
			SDL_WarpMouseInWindow( SDL_window, cls.glconfig.winWidth / 2, cls.glconfig.winHeight / 2 );

		mouseActive = qfalse;
	}
}

// We translate axes movement into keypresses
static const int joy_keys[16] = {
	A_CURSOR_LEFT, A_CURSOR_RIGHT,
	A_CURSOR_UP, A_CURSOR_DOWN,
	A_JOY16, A_JOY17,
	A_JOY18, A_JOY19,
	A_JOY20, A_JOY21,
	A_JOY22, A_JOY23,
	A_JOY24, A_JOY25,
	A_JOY26, A_JOY27
};

// translate hat events into keypresses
// the 4 highest buttons are used for the first hat ...
static const int hat_keys[16] = {
	A_JOY28, A_JOY29,
	A_JOY30, A_JOY31,
	A_JOY24, A_JOY25,
	A_JOY26, A_JOY27,
	A_JOY20, A_JOY21,
	A_JOY22, A_JOY23,
	A_JOY16, A_JOY17,
	A_JOY18, A_JOY19
};


struct
{
	qboolean buttons[16];  // !!! FIXME: these might be too many.
	unsigned int oldaxes;
	int oldaaxes[MAX_JOYSTICK_AXIS];
	unsigned int oldhats;
} static stick_state;

/*
===============
IN_InitJoystick
===============
*/
static void IN_InitJoystick( void )
{
	int i = 0;
	int total = 0;
	char buf[16384] = "";

	if (stick != NULL)
		SDL_JoystickClose(stick);

	stick = NULL;
	memset(&stick_state, '\0', sizeof (stick_state));

	if (!SDL_WasInit(SDL_INIT_JOYSTICK))
	{
		Com_DPrintf("Calling SDL_Init(SDL_INIT_JOYSTICK)...\n");
		if (SDL_Init(SDL_INIT_JOYSTICK) == -1)
		{
			Com_DPrintf("SDL_Init(SDL_INIT_JOYSTICK) failed: %s\n", SDL_GetError());
			return;
		}
		Com_DPrintf("SDL_Init(SDL_INIT_JOYSTICK) passed.\n");
	}

	total = SDL_NumJoysticks();
	Com_DPrintf("%d possible joysticks\n", total);

	// Print list and build cvar to allow ui to select joystick.
	for (i = 0; i < total; i++)
	{
		Q_strcat(buf, sizeof(buf), SDL_JoystickNameForIndex(i));
		Q_strcat(buf, sizeof(buf), "\n");
	}

	Cvar_Get( "in_availableJoysticks", buf, CVAR_ROM );

	if( !in_joystick->integer ) {
		Com_DPrintf( "Joystick is not active.\n" );
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		return;
	}

	in_joystickNo = Cvar_Get( "in_joystickNo", "0", CVAR_ARCHIVE | CVAR_GLOBAL );
	if( in_joystickNo->integer < 0 || in_joystickNo->integer >= total )
		Cvar_Set( "in_joystickNo", "0" );

	in_joystickUseAnalog = Cvar_Get( "in_joystickUseAnalog", "0", CVAR_ARCHIVE | CVAR_GLOBAL);

	in_joystickThreshold = Cvar_Get( "joy_threshold", "0.15", CVAR_ARCHIVE | CVAR_GLOBAL);

	if ( !in_pad.controller )
		stick = SDL_JoystickOpen( in_joystickNo->integer );

	if (stick == NULL) {
		Com_DPrintf( "No joystick opened.\n" );
		return;
	}

	Com_DPrintf( "Joystick %d opened\n", in_joystickNo->integer );
	Com_DPrintf( "Name:       %s\n", SDL_JoystickNameForIndex(in_joystickNo->integer) );
	Com_DPrintf( "Axes:       %d\n", SDL_JoystickNumAxes(stick) );
	Com_DPrintf( "Hats:       %d\n", SDL_JoystickNumHats(stick) );
	Com_DPrintf( "Buttons:    %d\n", SDL_JoystickNumButtons(stick) );
	Com_DPrintf( "Balls:      %d\n", SDL_JoystickNumBalls(stick) );
	Com_DPrintf( "Use Analog: %s\n", in_joystickUseAnalog->integer ? "Yes" : "No" );
	Com_DPrintf( "Threshold: %f\n", in_joystickThreshold->value );

	SDL_JoystickEventState(SDL_QUERY);
}

#if 0
static const char * IN_PadButtonUIName(SDL_GameControllerType type, SDL_GameControllerButton button)
{
	// TODO: load mapping from file
	return SDL_GameControllerGetStringForButton(button);
}
#endif

#define GAMEPAD_DEF_INNER_DEADZONE 0.1
#define GAMEPAD_DEF_OUTER_DEADZONE 1.0

static void IN_OpenGameController( int index );
static void IN_CloseGameController( void );
static void IN_ShutdownGameController( void );

static void IN_InitGameController( void )
{
	int	index, padIndex;
	int	total;

	in_gamepadNo = Cvar_Get("in_gamepadNo", "0", CVAR_TEMP | CVAR_LATCH);
	in_gamepadUIHack = Cvar_Get("in_gamepadUIHack", "1", CVAR_ARCHIVE | CVAR_GLOBAL);
	in_gamepadUISensitivity = Cvar_Get("in_gamepadUISensitivity", "5", CVAR_ARCHIVE | CVAR_GLOBAL);
	// RS = Right Stick
	in_gamepadRSInvertX = Cvar_Get("in_gamepadRSInvertX", "0", CVAR_ARCHIVE | CVAR_GLOBAL);
	in_gamepadRSInvertY = Cvar_Get("in_gamepadRSInvertY", "0", CVAR_ARCHIVE | CVAR_GLOBAL);
	in_gamepadRSAccel = Cvar_Get("in_gamepadRSAccel", "1", CVAR_ARCHIVE | CVAR_GLOBAL);
	in_gamepadRSAccelCurve = Cvar_Get("in_gamepadRSAccelCurve", "0", CVAR_ARCHIVE | CVAR_GLOBAL);
	in_gamepadRSSquareDeadzone = Cvar_Get("in_gamepadRSSquareDeadzone", "0", CVAR_ARCHIVE | CVAR_GLOBAL);
	in_gamepadRSInnerDeadzone = Cvar_Get("in_gamepadRSInnerDeadzone", XSTR(GAMEPAD_DEF_INNER_DEADZONE), CVAR_ARCHIVE | CVAR_GLOBAL);
	in_gamepadRSInnerDeadzone->modified = qtrue; // validate next frame
	in_gamepadRSOuterDeadzone = Cvar_Get("in_gamepadRSOuterDeadzone", XSTR(GAMEPAD_DEF_OUTER_DEADZONE), CVAR_ARCHIVE | CVAR_GLOBAL);
	in_gamepadRSOuterDeadzone->modified = qtrue; // validate next frame
	// LS = Left Stick
	in_gamepadLSSquareDeadzone = Cvar_Get("in_gamepadLSSquareDeadzone", "0", CVAR_ARCHIVE | CVAR_GLOBAL);
	in_gamepadLSInnerDeadzone = Cvar_Get("in_gamepadLSInnerDeadzone", XSTR(GAMEPAD_DEF_INNER_DEADZONE), CVAR_ARCHIVE | CVAR_GLOBAL);
	in_gamepadLSInnerDeadzone->modified = qtrue; // validate next frame
	in_gamepadLSOuterDeadzone = Cvar_Get("in_gamepadLSOuterDeadzone", XSTR(GAMEPAD_DEF_OUTER_DEADZONE), CVAR_ARCHIVE | CVAR_GLOBAL);
	in_gamepadLSOuterDeadzone->modified = qtrue; // validate next frame
	// LT = Left Trigger
	in_gamepadLTInnerDeadzone = Cvar_Get("in_gamepadLTInnerDeadzone", XSTR(GAMEPAD_DEF_INNER_DEADZONE), CVAR_ARCHIVE | CVAR_GLOBAL);
	in_gamepadLTInnerDeadzone->modified = qtrue; // validate next frame
	in_gamepadLTOuterDeadzone = Cvar_Get("in_gamepadLTOuterDeadzone", XSTR(GAMEPAD_DEF_OUTER_DEADZONE), CVAR_ARCHIVE | CVAR_GLOBAL);
	in_gamepadLTOuterDeadzone->modified = qtrue; // validate next frame
	// RT = Right Trigger
	in_gamepadRTInnerDeadzone = Cvar_Get("in_gamepadRTInnerDeadzone", XSTR(GAMEPAD_DEF_INNER_DEADZONE), CVAR_ARCHIVE | CVAR_GLOBAL);
	in_gamepadRTInnerDeadzone->modified = qtrue; // validate next frame
	in_gamepadRTOuterDeadzone = Cvar_Get("in_gamepadRTOuterDeadzone", XSTR(GAMEPAD_DEF_OUTER_DEADZONE), CVAR_ARCHIVE | CVAR_GLOBAL);
	in_gamepadRTOuterDeadzone->modified = qtrue; // validate next frame
	in_gamepadTriggersAxis = Cvar_Get("in_gamepadTriggersAxis", "0", CVAR_ARCHIVE | CVAR_GLOBAL);

	if (!in_gamepad->integer) {
		IN_ShutdownGameController();
		return;
	}

	if (!SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
		Com_DPrintf("Calling SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)...\n");
		if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == -1) {
			Com_DPrintf("SDL_InitSubSystem(SDL_Init(SDL_INIT_GAMECONTROLLER) failed: %s\n", SDL_GetError());
			return;
		}
		Com_DPrintf("SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) passed.\n");
	} else {
		if (in_pad.controller) {
			IN_CloseGameController();
		}
	}

	Com_DPrintf("Loading gamescontrollerdb.txt...");
	fileHandle_t fh;
	int len = FS_SV_FOpenFileRead("gamecontrollerdb.txt", &fh, MODULE_SDL);
	if (len > 0) {
		void *buf = Z_Malloc(len, TAG_TEMP_WORKSPACE);
		FS_Read(buf, len, fh, MODULE_SDL);
		FS_FCloseFile(fh, MODULE_SDL);
		SDL_RWops *rwops = SDL_RWFromConstMem(buf, len);
		int num = SDL_GameControllerAddMappingsFromRW(rwops, 1);
		if (num >= 0) {
			Com_DPrintf(" added %d mappings\n", num);
		} else {
			Com_DPrintf(" failed!\n");
			Com_Printf(S_COLOR_RED "ERROR: %s\n", SDL_GetError());
		}
		Z_Free(buf);
	} else {
		Com_DPrintf(" file not found\n");
	}

	total = SDL_NumJoysticks();
	Com_DPrintf("%d possible gamepads\n", total);
	padIndex = 0;
	for (index = 0; index < total; index++) {
		if (SDL_IsGameController(index)) {
			if (padIndex == in_gamepadNo->integer) {
				IN_OpenGameController(index);
				break;
			}
			padIndex++;
		}
	}
	if (!in_pad.controller)
		Com_DPrintf("No gamepad opened.\n");
}

static void IN_CloseGameController( void )
{
	SDL_GameControllerClose(in_pad.controller);
	memset(&in_pad, 0, sizeof(in_pad));
}

static void IN_OpenGameController( int index )
{
	if ( !stick )
		in_pad.controller = SDL_GameControllerOpen(index);
	if (!in_pad.controller) {
		Com_Printf(S_COLOR_YELLOW "WARNING: Failed to open gamepad %d: %s\n", index, SDL_GetError());
		return;
	}

	SDL_GameController *controller = in_pad.controller;

	Com_Printf("Gamepad %d opened\n", index);
	Com_Printf("Name:             %s\n"  , SDL_GameControllerName(controller));

	if (com_developer->integer) {
		char	guid[128];

		SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(index), guid, sizeof(guid));
		Com_Printf("Player:           %d\n"  , SDL_GameControllerGetPlayerIndex(controller));
		Com_Printf("Vendor:           %.4x\n", SDL_GameControllerGetVendor(controller));
		Com_Printf("Product:          %.4x\n", SDL_GameControllerGetProduct(controller));
		Com_Printf("Product Version:  %.4x\n", SDL_GameControllerGetProductVersion(controller));
		Com_Printf("GUID:             %s\n"  , guid);
#if SDL_VERSION_ATLEAST(2, 0, 14)
		Com_Printf("Serial Number:    %s\n"  , SDL_GameControllerGetSerial(controller));
#endif
#if 0 // debug
		char *mapping = SDL_GameControllerMapping(controller);
		Com_Printf("Mapping:          %s\n", mapping);
		SDL_free(mapping);
#endif
	}
#if 0
	SDL_GameControllerType type = SDL_GameControllerGetType(controller);
	assert(SDL_CONTROLLER_BUTTON_MAX < 30); // two reserved for trigger buttons
	for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
		const char *uiName = IN_PadButtonUIName(type, (SDL_GameControllerButton)i);
		if (uiName) {
			keynames[A_JOY0 + i].uiName = uiName;
		}
	}
#endif
}

void IN_Init( void *windowData )
{
	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		Com_Error( ERR_FATAL, "IN_Init called before SDL_Init( SDL_INIT_VIDEO )" );
		return;
	}

	SDL_window = (SDL_Window *)windowData;

	Com_DPrintf( "\n------- Input Initialization -------\n" );

	// joystick variables
	in_keyboardDebug = Cvar_Get( "in_keyboardDebug", "0", CVAR_TEMP );

	in_joystick = Cvar_Get( "in_joystick", "0", CVAR_ARCHIVE | CVAR_GLOBAL | CVAR_LATCH );
	in_gamepad = Cvar_Get("in_gamepad", "1", CVAR_ARCHIVE | CVAR_GLOBAL | CVAR_LATCH);

	// mouse variables
	in_mouse = Cvar_Get( "in_mouse", "1", CVAR_ARCHIVE | CVAR_GLOBAL);
	in_nograb = Cvar_Get( "in_nograb", "0", CVAR_ARCHIVE | CVAR_GLOBAL);

	SDL_StartTextInput( );

	mouseAvailable = (qboolean)( in_mouse->value != 0 );

	if (in_mouse->integer == 0) {
		Com_DPrintf("IN_Init: Mouse input disabled\n");
	}

	if (in_mouse->integer == 1) {
		Com_DPrintf("IN_Init: Using raw mouse input\n");
	}

	if (in_mouse->integer == 2) {
		Com_DPrintf("IN_Init: Not using raw input\n");
		SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1");
	} else {
		SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "0");
	}

#if SDL_VERSION_ATLEAST(2, 26, 0)
	if (in_mouse->integer == 3) {
		Com_DPrintf("IN_Init: Using raw mouse input with system scaling\n");
		// low latency of raw mouse input with system mouse scaling
		SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, "1");
	} else {
		SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, "0");
	}
#endif

	IN_DeactivateMouse( );

	int appState = SDL_GetWindowFlags( SDL_window );
	Cvar_SetValue( "com_unfocused", ( appState & SDL_WINDOW_INPUT_FOCUS ) == 0 );
	Cvar_SetValue( "com_minimized", ( appState & SDL_WINDOW_MINIMIZED ) != 0 );

	IN_InitGameController( );
	IN_InitJoystick( );

	Com_DPrintf( "------------------------------------\n" );
}

/*
===============
Converts a UTF-8 character to UTF-32.
===============
*/
uint32_t ConvertUTF8ToUTF32(char *utf8CurrentChar, char **utf8NextChar) {
	uint32_t utf32 = 0;
	char *c = utf8CurrentChar;

	if ((*c & 0x80) == 0)
		utf32 = *c++;
	else if ((*c & 0xE0) == 0xC0) // 110x xxxx
	{
		utf32 |= (*c++ & 0x1F) << 6;
		utf32 |= (*c++ & 0x3F);
	} else if ((*c & 0xF0) == 0xE0) // 1110 xxxx
	{
		utf32 |= (*c++ & 0x0F) << 12;
		utf32 |= (*c++ & 0x3F) << 6;
		utf32 |= (*c++ & 0x3F);
	} else if ((*c & 0xF8) == 0xF0) // 1111 0xxx
	{
		utf32 |= (*c++ & 0x07) << 18;
		utf32 |= (*c++ & 0x3F) << 12;
		utf32 |= (*c++ & 0x3F) << 6;
		utf32 |= (*c++ & 0x3F);
	} else {
		Com_DPrintf("Unrecognised UTF-8 lead byte: 0x%x\n", (unsigned int)*c);
		c++;
	}

	*utf8NextChar = c;

	return utf32;
}

uint8_t ConvertUTF32ToExpectedCharset( uint32_t utf32 )
{
	switch ( utf32 )
	{
		// Cyrillic characters - mapped to Windows-1251 encoding
		case 0x0410: return 192;
		case 0x0411: return 193;
		case 0x0412: return 194;
		case 0x0413: return 195;
		case 0x0414: return 196;
		case 0x0415: return 197;
		case 0x0416: return 198;
		case 0x0417: return 199;
		case 0x0418: return 200;
		case 0x0419: return 201;
		case 0x041A: return 202;
		case 0x041B: return 203;
		case 0x041C: return 204;
		case 0x041D: return 205;
		case 0x041E: return 206;
		case 0x041F: return 207;
		case 0x0420: return 208;
		case 0x0421: return 209;
		case 0x0422: return 210;
		case 0x0423: return 211;
		case 0x0424: return 212;
		case 0x0425: return 213;
		case 0x0426: return 214;
		case 0x0427: return 215;
		case 0x0428: return 216;
		case 0x0429: return 217;
		case 0x042A: return 218;
		case 0x042B: return 219;
		case 0x042C: return 220;
		case 0x042D: return 221;
		case 0x042E: return 222;
		case 0x042F: return 223;
		case 0x0430: return 224;
		case 0x0431: return 225;
		case 0x0432: return 226;
		case 0x0433: return 227;
		case 0x0434: return 228;
		case 0x0435: return 229;
		case 0x0436: return 230;
		case 0x0437: return 231;
		case 0x0438: return 232;
		case 0x0439: return 233;
		case 0x043A: return 234;
		case 0x043B: return 235;
		case 0x043C: return 236;
		case 0x043D: return 237;
		case 0x043E: return 238;
		case 0x043F: return 239;
		case 0x0440: return 240;
		case 0x0441: return 241;
		case 0x0442: return 242;
		case 0x0443: return 243;
		case 0x0444: return 244;
		case 0x0445: return 245;
		case 0x0446: return 246;
		case 0x0447: return 247;
		case 0x0448: return 248;
		case 0x0449: return 249;
		case 0x044A: return 250;
		case 0x044B: return 251;
		case 0x044C: return 252;
		case 0x044D: return 253;
		case 0x044E: return 254;
		case 0x044F: return 255;

		// Eastern european characters - polish, czech, etc use Windows-1250 encoding
		case 0x0160: return 138;
		case 0x015A: return 140;
		case 0x0164: return 141;
		case 0x017D: return 142;
		case 0x0179: return 143;
		case 0x0161: return 154;
		case 0x015B: return 156;
		case 0x0165: return 157;
		case 0x017E: return 158;
		case 0x017A: return 159;
		case 0x0141: return 163;
		case 0x0104: return 165;
		case 0x015E: return 170;
		case 0x017B: return 175;
		case 0x0142: return 179;
		case 0x0105: return 185;
		case 0x015F: return 186;
		case 0x013D: return 188;
		case 0x013E: return 190;
		case 0x017C: return 191;
		case 0x0154: return 192;
		case 0x00C1: return 193;
		case 0x00C2: return 194;
		case 0x0102: return 195;
		case 0x00C4: return 196;
		case 0x0139: return 197;
		case 0x0106: return 198;
		case 0x00C7: return 199;
		case 0x010C: return 200;
		case 0x00C9: return 201;
		case 0x0118: return 202;
		case 0x00CB: return 203;
		case 0x011A: return 204;
		case 0x00CD: return 205;
		case 0x00CE: return 206;
		case 0x010E: return 207;
		case 0x0110: return 208;
		case 0x0143: return 209;
		case 0x0147: return 210;
		case 0x00D3: return 211;
		case 0x00D4: return 212;
		case 0x0150: return 213;
		case 0x00D6: return 214;
		case 0x0158: return 216;
		case 0x016E: return 217;
		case 0x00DA: return 218;
		case 0x0170: return 219;
		case 0x00DC: return 220;
		case 0x00DD: return 221;
		case 0x0162: return 222;
		case 0x00DF: return 223;
		case 0x0155: return 224;
		case 0x00E1: return 225;
		case 0x00E2: return 226;
		case 0x0103: return 227;
		case 0x00E4: return 228;
		case 0x013A: return 229;
		case 0x0107: return 230;
		case 0x00E7: return 231;
		case 0x010D: return 232;
		case 0x00E9: return 233;
		case 0x0119: return 234;
		case 0x00EB: return 235;
		case 0x011B: return 236;
		case 0x00ED: return 237;
		case 0x00EE: return 238;
		case 0x010F: return 239;
		case 0x0111: return 240;
		case 0x0144: return 241;
		case 0x0148: return 242;
		case 0x00F3: return 243;
		case 0x00F4: return 244;
		case 0x0151: return 245;
		case 0x00F6: return 246;
		case 0x0159: return 248;
		case 0x016F: return 249;
		case 0x00FA: return 250;
		case 0x0171: return 251;
		case 0x00FC: return 252;
		case 0x00FD: return 253;
		case 0x0163: return 254;
		case 0x02D9: return 255;

		default: return (uint8_t)utf32;
	}
}

/*
===============
IN_ModTogglesConsole
===============
*/
static qboolean IN_ModTogglesConsole( int mod ) {
	switch (mv_consoleShiftRequirement->integer) {
	case 0:
		return qtrue;
	case 2:
		return (qboolean)!!(mod & KMOD_SHIFT);
	case 1:
	default:
		return (qboolean)((mod & KMOD_SHIFT) || (Key_GetCatcher() & KEYCATCH_CONSOLE));
	}
}

/*
===============
IN_ProcessEvents
===============
*/

static void IN_ProcessEvents( int eventTime )
{
	SDL_Event e;
	fakeAscii_t key = A_NULL;
	int mx = 0, my = 0;
	 // not using SDL_StopTextInput for screen kbd and other
	 // considerations
	static qboolean textInput = qtrue;

	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
			return;

	while( SDL_PollEvent( &e ) )
	{
		switch( e.type )
		{
			case SDL_KEYDOWN:
				if ((e.key.keysym.mod & KMOD_LALT) && !(e.key.keysym.mod & KMOD_CTRL))
					textInput = qfalse;
				else
					textInput = qtrue;

				if ((e.key.keysym.mod & KMOD_CTRL) && (e.key.keysym.mod & KMOD_LALT))
					break;

				if (e.key.keysym.scancode == SDL_SCANCODE_GRAVE) {
					if (IN_ModTogglesConsole(e.key.keysym.mod)) {
						Sys_QueEvent(eventTime, SE_KEY, A_CONSOLE, qtrue, 0, NULL);
						textInput = qfalse;
					}
				} else {
					key = IN_TranslateSDLToJKKey(&e.key.keysym, qtrue);
					if (key != A_NULL)
						Sys_QueEvent(eventTime, SE_KEY, key, qtrue, 0, NULL);

					if (key == A_BACKSPACE && !(e.key.keysym.mod & KMOD_ALT))
						Sys_QueEvent(eventTime, SE_CHAR, CTRL('h'), qfalse, 0, NULL);
					else if ((e.key.keysym.mod & KMOD_CTRL) && key >= A_CAP_A && key <= A_CAP_Z)
						Sys_QueEvent(eventTime, SE_CHAR, CTRL(tolower(key)), qfalse, 0, NULL);
				}
				break;

			case SDL_KEYUP:
				if ((e.key.keysym.mod & KMOD_LALT) && !(e.key.keysym.mod & KMOD_CTRL))
					textInput = qfalse;
				else
					textInput = qtrue;

				key = IN_TranslateSDLToJKKey( &e.key.keysym, qfalse );
				if( key != A_NULL )
					Sys_QueEvent( eventTime, SE_KEY, key, qfalse, 0, NULL );

				if ((e.key.keysym.scancode == SDL_SCANCODE_LGUI || e.key.keysym.scancode == SDL_SCANCODE_RGUI) &&
					(SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_FULLSCREEN)) {
					GLimp_Minimize();
				}
				break;

			case SDL_TEXTINPUT:
				// relies on receiving no more than 1 character
				if( textInput )
				{
					char *c = e.text.text;

					// Quick and dirty UTF-8 to UTF-32 conversion
					while( *c )
					{
						uint32_t utf32 = ConvertUTF8ToUTF32( c, &c );
						if( utf32 != 0 )
						{
							uint8_t encoded = ConvertUTF32ToExpectedCharset( utf32 );
							Sys_QueEvent( eventTime, SE_CHAR, encoded, 0, 0, NULL );
						}
					}
				}
				break;

			case SDL_MOUSEMOTION:
				if ( mouseActive )
				{
					mx += e.motion.xrel;
					my += e.motion.yrel;
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				{
					unsigned short b;
					switch( e.button.button )
					{
						case SDL_BUTTON_LEFT:	b = A_MOUSE1;     break;
						case SDL_BUTTON_MIDDLE:	b = A_MOUSE3;     break;
						case SDL_BUTTON_RIGHT:	b = A_MOUSE2;     break;
						case SDL_BUTTON_X1:		b = A_MOUSE4;     break;
						case SDL_BUTTON_X2:		b = A_MOUSE5;     break;
						default: b = A_AUX0 + ( e.button.button - 6 ) % 32; break;
					}
					Sys_QueEvent( eventTime, SE_KEY, b,
						( e.type == SDL_MOUSEBUTTONDOWN ? qtrue : qfalse ), 0, NULL );
				}
				break;

			case SDL_MOUSEWHEEL:
				if( e.wheel.y > 0 )
				{
					Sys_QueEvent( eventTime, SE_KEY, A_MWHEELUP, qtrue, 0, NULL );
					Sys_QueEvent( eventTime, SE_KEY, A_MWHEELUP, qfalse, 0, NULL );
				}
				else if( e.wheel.y < 0 )
				{
					Sys_QueEvent( eventTime, SE_KEY, A_MWHEELDOWN, qtrue, 0, NULL );
					Sys_QueEvent( eventTime, SE_KEY, A_MWHEELDOWN, qfalse, 0, NULL );
				}
				break;

			case SDL_CONTROLLERAXISMOTION:
				break;

			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
			{
				qboolean down = (e.cbutton.state == SDL_PRESSED) ? qtrue : qfalse;

				// send A_JOY key first even when in_gamepadUIHack is
				// enabled. Otherwise intercepted JOY keys can't be
				// bound in menu
				if (e.cbutton.button > SDL_CONTROLLER_BUTTON_INVALID &&
					e.cbutton.button < SDL_CONTROLLER_BUTTON_MAX)
				{
					key = (fakeAscii_t)(A_JOY0 + e.cbutton.button);
					Sys_QueEvent(eventTime, SE_KEY, key, down, 0, NULL);
				}

				if (e.cbutton.button == SDL_CONTROLLER_BUTTON_START)
				{
					Sys_QueEvent(eventTime, SE_KEY, A_ESCAPE, down, 0, NULL);
				}

				if (in_gamepadUIHack->integer &&
					Key_GetCatcher() & (KEYCATCH_UI | KEYCATCH_CGAME))
				{
					qboolean nav = in_pad.buttonUINavigationActive;

					switch (e.cbutton.button) {
					case SDL_CONTROLLER_BUTTON_A         : key = nav ? A_ENTER : A_MOUSE1; break;
					case SDL_CONTROLLER_BUTTON_B         : key = A_MOUSE2; break; // force power UI widget uses mouse2 to unassign
#if SDL_VERSION_ATLEAST(2, 0, 14)
					case SDL_CONTROLLER_BUTTON_TOUCHPAD  : key = A_MOUSE1; break;
#endif
					case SDL_CONTROLLER_BUTTON_DPAD_UP   : key = A_CURSOR_UP; nav = qtrue; break;
					case SDL_CONTROLLER_BUTTON_DPAD_LEFT : key = A_CURSOR_LEFT; nav = qtrue; break;
					case SDL_CONTROLLER_BUTTON_DPAD_DOWN : key = A_CURSOR_DOWN; nav =  qtrue; break;
					case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: key = A_CURSOR_RIGHT; nav = qtrue; break;
					case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: key = A_CURSOR_UP; nav = qtrue; break;
					case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: key = A_CURSOR_DOWN; nav = qtrue; break;
					default: key = A_NULL; break;
					}

					in_pad.buttonUINavigationActive = nav;

					if (key != A_NULL) {
						Sys_QueEvent(eventTime, SE_KEY, key, down, 0, NULL);
					}
				}
			}
			break;

			case SDL_CONTROLLERDEVICEADDED:
				// SDL gamecontroller subsystem is certainly initialised
				if (in_pad.controller) {
					// open controller if current was disconnected
					if (!SDL_GameControllerGetAttached(in_pad.controller)) {
						IN_CloseGameController();
						IN_OpenGameController(e.cdevice.which);
					}
				} else {
					// open first connected controller
					IN_OpenGameController(e.cdevice.which);
				}
				// fallthrough
			case SDL_CONTROLLERDEVICEREMOVED:
			case SDL_CONTROLLERDEVICEREMAPPED:
			{
				for (int axis = 0; axis < MAX_JOYSTICK_AXIS; axis++) {
					Sys_QueEvent( eventTime, SE_JOYSTICK_AXIS, axis, 0, 0, NULL);
				}
			}
			break;

			case SDL_QUIT:
				Cbuf_ExecuteText(EXEC_NOW, "quit Closed window\n");
				break;

			case SDL_WINDOWEVENT:
				switch( e.window.event )
				{
					case SDL_WINDOWEVENT_MOVED:
					case SDL_WINDOWEVENT_SIZE_CHANGED:
					{
						if ( !(SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_FULLSCREEN) )
							GLimp_SaveWindowPosition();
						break;
					}

					case SDL_WINDOWEVENT_HIDDEN:
					case SDL_WINDOWEVENT_MINIMIZED:    Cvar_SetValue( "com_minimized", 1 ); break;
					case SDL_WINDOWEVENT_SHOWN:
					case SDL_WINDOWEVENT_RESTORED:
					case SDL_WINDOWEVENT_MAXIMIZED:    Cvar_SetValue( "com_minimized", 0 ); break;
					case SDL_WINDOWEVENT_FOCUS_LOST:   Cvar_SetValue( "com_unfocused", 1 ); break;
					case SDL_WINDOWEVENT_FOCUS_GAINED: Cvar_SetValue( "com_unfocused", 0 ); break;
				}
				break;

			default:
				break;
		}
	}

	if (mx || my) {
		Sys_QueEvent( eventTime, SE_MOUSE, mx, my, 0, NULL );
	}
}

/*
===============
IN_JoyMove
===============
*/
static void IN_JoyMove( int eventTime )
{
	unsigned int axes = 0;
	unsigned int hats = 0;
	int total = 0;
	int i = 0;

	if (!stick)
		return;

	SDL_JoystickUpdate();

	// update the ball state.
	total = SDL_JoystickNumBalls(stick);
	if (total > 0)
	{
		int balldx = 0;
		int balldy = 0;
		for (i = 0; i < total; i++)
		{
			int dx = 0;
			int dy = 0;
			SDL_JoystickGetBall(stick, i, &dx, &dy);
			balldx += dx;
			balldy += dy;
		}
		if (balldx || balldy)
		{
			// !!! FIXME: is this good for stick balls, or just mice?
			// Scale like the mouse input...
			if (abs(balldx) > 1)
				balldx *= 2;
			if (abs(balldy) > 1)
				balldy *= 2;
			Sys_QueEvent( eventTime, SE_MOUSE, balldx, balldy, 0, NULL );
		}
	}

	// now query the stick buttons...
	total = SDL_JoystickNumButtons(stick);
	if (total > 0)
	{
		if (total > (int)ARRAY_LEN(stick_state.buttons))
			total = ARRAY_LEN(stick_state.buttons);
		for (i = 0; i < total; i++)
		{
			qboolean pressed = (qboolean)(SDL_JoystickGetButton(stick, i) != 0);
			if (pressed != stick_state.buttons[i])
			{
				Sys_QueEvent( eventTime, SE_KEY, A_JOY1 + i, pressed, 0, NULL );
				stick_state.buttons[i] = pressed;
			}
		}
	}

	// look at the hats...
	total = SDL_JoystickNumHats(stick);
	if (total > 0)
	{
		if (total > 4) total = 4;
		for (i = 0; i < total; i++)
		{
			((Uint8 *)&hats)[i] = SDL_JoystickGetHat(stick, i);
		}
	}

	// update hat state
	if (hats != stick_state.oldhats)
	{
		for( i = 0; i < 4; i++ ) {
			if( ((Uint8 *)&hats)[i] != ((Uint8 *)&stick_state.oldhats)[i] ) {
				// release event
				switch( ((Uint8 *)&stick_state.oldhats)[i] ) {
					case SDL_HAT_UP:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 0], qfalse, 0, NULL );
						break;
					case SDL_HAT_RIGHT:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 1], qfalse, 0, NULL );
						break;
					case SDL_HAT_DOWN:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 2], qfalse, 0, NULL );
						break;
					case SDL_HAT_LEFT:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 3], qfalse, 0, NULL );
						break;
					case SDL_HAT_RIGHTUP:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 0], qfalse, 0, NULL );
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 1], qfalse, 0, NULL );
						break;
					case SDL_HAT_RIGHTDOWN:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 2], qfalse, 0, NULL );
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 1], qfalse, 0, NULL );
						break;
					case SDL_HAT_LEFTUP:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 0], qfalse, 0, NULL );
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 3], qfalse, 0, NULL );
						break;
					case SDL_HAT_LEFTDOWN:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 2], qfalse, 0, NULL );
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 3], qfalse, 0, NULL );
						break;
					default:
						break;
				}
				// press event
				switch( ((Uint8 *)&hats)[i] ) {
					case SDL_HAT_UP:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 0], qtrue, 0, NULL );
						break;
					case SDL_HAT_RIGHT:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 1], qtrue, 0, NULL );
						break;
					case SDL_HAT_DOWN:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 2], qtrue, 0, NULL );
						break;
					case SDL_HAT_LEFT:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 3], qtrue, 0, NULL );
						break;
					case SDL_HAT_RIGHTUP:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 0], qtrue, 0, NULL );
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 1], qtrue, 0, NULL );
						break;
					case SDL_HAT_RIGHTDOWN:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 2], qtrue, 0, NULL );
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 1], qtrue, 0, NULL );
						break;
					case SDL_HAT_LEFTUP:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 0], qtrue, 0, NULL );
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 3], qtrue, 0, NULL );
						break;
					case SDL_HAT_LEFTDOWN:
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 2], qtrue, 0, NULL );
						Sys_QueEvent( eventTime, SE_KEY, hat_keys[4*i + 3], qtrue, 0, NULL );
						break;
					default:
						break;
				}
			}
		}
	}

	// save hat state
	stick_state.oldhats = hats;

	// finally, look at the axes...
	total = SDL_JoystickNumAxes(stick);
	if (total > 0)
	{
		if (in_joystickUseAnalog->integer)
		{
			if (total > MAX_JOYSTICK_AXIS) total = MAX_JOYSTICK_AXIS;
			for (i = 0; i < total; i++)
			{
				Sint16 axis = SDL_JoystickGetAxis(stick, i);
				float f = ( (float) abs(axis) ) / 32767.0f;

				if( f < in_joystickThreshold->value ) axis = 0;

				if ( axis != stick_state.oldaaxes[i] )
				{
					Sys_QueEvent( eventTime, SE_JOYSTICK_AXIS, i, axis, 0, NULL );
					stick_state.oldaaxes[i] = axis;
				}
			}
		}
		else
		{
			if (total > 16) total = 16;
			for (i = 0; i < total; i++)
			{
				Sint16 axis = SDL_JoystickGetAxis(stick, i);
				float f = ( (float) axis ) / 32767.0f;
				if( f < -in_joystickThreshold->value ) {
					axes |= ( 1 << ( i * 2 ) );
				} else if( f > in_joystickThreshold->value ) {
					axes |= ( 1 << ( ( i * 2 ) + 1 ) );
				}
			}
		}
	}

	/* Time to update axes state based on old vs. new. */
	if (axes != stick_state.oldaxes)
	{
		for( i = 0; i < 16; i++ ) {
			if( ( axes & ( 1 << i ) ) && !( stick_state.oldaxes & ( 1 << i ) ) ) {
				Sys_QueEvent( eventTime, SE_KEY, joy_keys[i], qtrue, 0, NULL );
			}

			if( !( axes & ( 1 << i ) ) && ( stick_state.oldaxes & ( 1 << i ) ) ) {
				Sys_QueEvent( eventTime, SE_KEY, joy_keys[i], qfalse, 0, NULL );
			}
		}
	}

	/* Save for future generations. */
	stick_state.oldaxes = axes;
}

static void IN_PadDeadzoneAxis(float *inX, float inner, float outer)
{
	float x = *inX;

	// normalize position so that coordinates within deadzone are achievable

	if (x >= 0) {
		x = (x - inner) / (outer - inner);
		x = Com_Clamp(0.0f, 1.0f, x);
	} else {
		x = (x + inner) / (outer - inner);
		x = Com_Clamp(-1.0f, 0.0f, x);
	}

	*inX = x;
}

static void IN_PadDeadzoneCircular(float *inX, float *inY, float inner, float outer)
{
	// bg_pmove.c::PM_CmdScale() normalizes cmd->forwardmove and
	// cmd->rightmove such, that (127,127) is the same acceleration as
	// (127,0).
	// Controller Stick input space is [-1.0,1.0]x[-1.0,1.0] square.
	//
	// In modern controllers input space represents controller stick
	// tilt angles
	//
	// Corners of the input space square are often unreachable because
	// circular stick cutout in controller case limits its movement
	//
	// Circles in input space should map to squares in output space,
	// because then stick tilt translates to acceleration directly.
	//
	// circle of radius 1 should map to square of edge 2 so that max
	// diagonal values/acceleration can be achieved.
	//
	// deadzone shape must be circular in physical world (ergo in
	// input space)
	//
	// deadzone must be normalized so that all output coordinates are
	// achievable

	float x = *inX;
	float y = *inY;
	float r = sqrtf((double)x * x + (double)y * y);

	// normalize x,y to circle of radius 1
	r = (r - inner) / (outer - inner);
	// deadzone
	r = Com_Clamp(0.0f, 1.0f, r);

	if (x == 0.0f) {
		*inY = r;
		return;
	}

	if (y == 0.0f) {
		*inX = r;
		return;
	}

	// project x,y onto a square with edge length 2 * r
	qboolean tr_half = (qboolean)(x + y >= 0);
	qboolean tl_half = (qboolean)(y - x >= 0);
	if        ( tr_half && !tl_half) { // right square edge
		y = r * y / x;
		x = r;
	} else if (!tr_half && !tl_half) { // bottom square edge
		x = - r * x / y;
		y = - r;
	} else if (!tr_half &&  tl_half) { // left square edge
		y = - r * y / x;
		x = - r;
	} else if ( tr_half &&  tl_half) { // top square edge
		x = r * x / y;
		y = r;
	}

	// clamp any numerical errors
	*inX = Com_Clamp(-1.0f, 1.0f, x);
	*inY = Com_Clamp(-1.0f, 1.0f, y);
}

static void IN_PadDeadzoneStick(float *inX, float *inY, float inner, float outer, qboolean square)
{
	if (square) {
		// square deadzone, independent on each axis
		IN_PadDeadzoneAxis(inX, inner, outer);
		IN_PadDeadzoneAxis(inY, inner, outer);
	} else {
		IN_PadDeadzoneCircular(inX, inY, inner, outer);
	}
}

/*
===============
IN_SDLControllerGetAxis

Returns SDL_GameControllerAxis value in [-1.0,1.0] range
or [0.0,1.0] for trigger axes
===============
*/
static float IN_SDLControllerGetAxis(SDL_GameControllerAxis axis)
{
	float value = SDL_GameControllerGetAxis(in_pad.controller, axis);

	if (value >= 0) {
		return value / SDL_JOYSTICK_AXIS_MAX;
	} else {
		return - value / SDL_JOYSTICK_AXIS_MIN;
	}
}

static qboolean IN_PadValidDeadzone(float inner, float outer) {
	return (qboolean)((0.0f <= inner) && (inner + 0.05f < outer) && (outer <= 1.0f));
}

static void IN_PadGetLSDeadzone(float *innerp, float *outerp)
{
	float inner = in_gamepadLSInnerDeadzone->value;
	float outer = in_gamepadLSOuterDeadzone->value;

	if (!IN_PadValidDeadzone(inner, outer))
	{
		if (in_gamepadLSInnerDeadzone->modified || in_gamepadLSOuterDeadzone->modified) {
			in_gamepadLSInnerDeadzone->modified = qfalse;
			in_gamepadLSOuterDeadzone->modified = qfalse;
			Com_Printf(S_COLOR_YELLOW "WARNING: Incorrect Left Stick deadzones. Using default values\n");
		}
		inner = GAMEPAD_DEF_INNER_DEADZONE;
		outer = GAMEPAD_DEF_OUTER_DEADZONE;
	}

	*innerp = inner;
	*outerp = outer;
}

static void IN_PadGetRSDeadzone(float *innerp, float *outerp)
{
	float inner = in_gamepadRSInnerDeadzone->value;
	float outer = in_gamepadRSOuterDeadzone->value;

	if (!IN_PadValidDeadzone(inner, outer))
	{
		if (in_gamepadRSInnerDeadzone->modified || in_gamepadRSOuterDeadzone->modified) {
			in_gamepadRSInnerDeadzone->modified = qfalse;
			in_gamepadRSOuterDeadzone->modified = qfalse;
			Com_Printf(S_COLOR_YELLOW "WARNING: Incorrect Right Stick deadzones. Using default values\n");
		}
		inner = GAMEPAD_DEF_INNER_DEADZONE;
		outer = GAMEPAD_DEF_OUTER_DEADZONE;
	}

	*innerp = inner;
	*outerp = outer;
}

static void IN_PadGetLTDeadzone(float *innerp, float *outerp)
{
	float inner = in_gamepadLTInnerDeadzone->value;
	float outer = in_gamepadLTOuterDeadzone->value;

	if (!IN_PadValidDeadzone(inner, outer))
	{
		if (in_gamepadLTInnerDeadzone->modified || in_gamepadLTOuterDeadzone->modified) {
			in_gamepadLTInnerDeadzone->modified = qfalse;
			in_gamepadLTOuterDeadzone->modified = qfalse;
			Com_Printf(S_COLOR_YELLOW "WARNING: Incorrect Left Trigger deadzones. Using default values\n");
		}
		inner = GAMEPAD_DEF_INNER_DEADZONE;
		outer = GAMEPAD_DEF_OUTER_DEADZONE;
	}

	*innerp = inner;
	*outerp = outer;
}

static void IN_PadGetRTDeadzone(float *innerp, float *outerp)
{
	float inner = in_gamepadRTInnerDeadzone->value;
	float outer = in_gamepadRTOuterDeadzone->value;

	if (!IN_PadValidDeadzone(inner, outer))
	{
		if (in_gamepadRTInnerDeadzone->modified || in_gamepadRTOuterDeadzone->modified) {
			in_gamepadRTInnerDeadzone->modified = qfalse;
			in_gamepadRTOuterDeadzone->modified = qfalse;
			Com_Printf(S_COLOR_YELLOW "WARNING: Incorrect Right Trigger deadzones. Using default values\n");
		}
		inner = GAMEPAD_DEF_INNER_DEADZONE;
		outer = GAMEPAD_DEF_OUTER_DEADZONE;
	}

	*innerp = inner;
	*outerp = outer;
}

static void IN_PadShapeStick(float *inX, float *inY, int curve, float accel)
{
	// Apply acceleration curve to axis tilt value. This must be called
	// on the square input, after deadzonning.

	if (accel == 1.0f)
		return;

	float x = *inX;
	float y = *inY;

	float tilt = MAX(fabsf(x), fabsf(y)); // in [0,1] range
	if (tilt == 0)
		return;

	// each curve must normalize acceleration value so that for any of
	// them [1, 10] range is reasonable because there is only one cvar
	// This is for the sake of unified GUI controls
	float t = tilt;
	float A, newTilt;

	// normalization process:
	// a) when accel == 1 it is hardly noticeable (gui should not go below 1)
	// b) for accel = 10 max deviation t - f(t) is less than 0.6
	// c) accel 1, 2, 3 ... are distinct and usable
	switch (curve) {
	case 1: // polynomial curve
		// f(t) = t^A; f(0) = 0; f(1) = 1
		A = Com_Clamp(-10, 10, accel);
		A = 1.0f + 0.6f * A; // A in [1,7] range
		newTilt = powf(t, A);
		break;
	case 2: // exponential curve
		// f(x) = (A^t - 1)/(A - 1); f(0) = 0; f(1) = 1
		A = Com_Clamp(0.1f, 10.0f, accel);
		A = expf(0.7f * A); // A in [1.07,1097] range
		newTilt = (powf(A, t) - 1) / (A - 1);
		break;
	default:
		newTilt = tilt;
		break;
	}

	x = newTilt * x / tilt;
	if (!isnormal(x))
		x = 0.0f;

	y = newTilt * y / tilt;
	if (!isnormal(y))
		y = 0.0f;

	// clamp any numerical errors
	*inX = Com_Clamp(-1.0f, 1.0f, x);
	*inY = Com_Clamp(-1.0f, 1.0f, y);
}

static void IN_PadMoveUILS(int eventTime)
{
	int deltaTime = eventTime - in_pad.lastLSEventTime;
	in_pad.lastLSEventTime = eventTime;
	if (deltaTime < 0 || deltaTime > 1000) {
		return;
	}

	float x = IN_SDLControllerGetAxis(SDL_CONTROLLER_AXIS_LEFTX);
	float y = IN_SDLControllerGetAxis(SDL_CONTROLLER_AXIS_LEFTY);
	float inner, outer;

	IN_PadGetLSDeadzone(&inner, &outer);
	IN_PadDeadzoneStick(&x, &y, inner, outer, qtrue);

	if (x != 0.0f || y != 0.0f)
		in_pad.buttonUINavigationActive = qfalse;

	in_pad.residual_dx += 0.2f * x * deltaTime * in_gamepadUISensitivity->value;
	in_pad.residual_dy += 0.2f * y * deltaTime * in_gamepadUISensitivity->value;

	int dx = (int)in_pad.residual_dx;
	int dy = (int)in_pad.residual_dy;

	if (dx || dy)
	{
		Sys_QueEvent(eventTime, SE_MOUSE, dx, dy, 0, NULL);
		in_pad.residual_dx -= dx;
		in_pad.residual_dy -= dy;
	}
}

static void IN_PadMoveUIRS(int eventTime)
{
	// Left Stick sends mouse wheel events that repeat every 50ms with
	// 200ms initial delay

	// 0 is special value meaning RS was released since last event

	if (in_pad.nextRSXEventTime > eventTime + 200)
		in_pad.nextRSXEventTime = 0;

	qboolean xEventAllowed = (qboolean)(eventTime > in_pad.nextRSXEventTime);

	if (in_pad.nextRSYEventTime > eventTime + 200)
		in_pad.nextRSYEventTime = 0;

	qboolean yEventAllowed = (qboolean)(eventTime > in_pad.nextRSYEventTime);

	float x = IN_SDLControllerGetAxis(SDL_CONTROLLER_AXIS_RIGHTX);
	float y = IN_SDLControllerGetAxis(SDL_CONTROLLER_AXIS_RIGHTY);
	float inner, outer;

	IN_PadGetRSDeadzone(&inner, &outer);
	IN_PadDeadzoneStick(&x, &y, inner, outer, qtrue);

	if (x != 0.0f || y != 0.0f)
		in_pad.buttonUINavigationActive = qfalse;

	const float activeThreshold = 0.7f;
	const float passiveThreshold = 0.4f;

	if (xEventAllowed && fabsf(y) < passiveThreshold) {
		qboolean xEvent = qfalse;
		if (x < -activeThreshold) {
			xEvent = qtrue;
			Sys_QueEvent(eventTime, SE_KEY, A_MWHEELUP, qtrue, 0, NULL);
		}
		if (x >  activeThreshold) {
			xEvent = qtrue;
			Sys_QueEvent(eventTime, SE_KEY, A_MWHEELDOWN, qtrue, 0, NULL);
		}
		if (xEvent) {
			in_pad.nextRSXEventTime = in_pad.nextRSXEventTime ? eventTime + 50 : eventTime + 200;
		} else {
			in_pad.nextRSXEventTime = 0;
		}
	}

	if (yEventAllowed && fabsf(x) < passiveThreshold) {
		qboolean yEvent = qfalse;
		if (y < -activeThreshold) {
			yEvent = qtrue;
			Sys_QueEvent(eventTime, SE_KEY, A_MWHEELUP, qtrue, 0, NULL);
		}
		if (y >  activeThreshold) {
			yEvent = qtrue;
			Sys_QueEvent(eventTime, SE_KEY, A_MWHEELDOWN, qtrue, 0, NULL);
		}
		if (yEvent) {
			in_pad.nextRSYEventTime = in_pad.nextRSYEventTime ? eventTime + 50 : eventTime + 200;
		} else {
			in_pad.nextRSYEventTime = 0;
		}
	}
}

static void IN_PadMoveUI(int eventTime)
{
	IN_PadMoveUILS(eventTime);
	IN_PadMoveUIRS(eventTime);
}

static void IN_PadMoveSticks(int eventTime)
{
	// Process gamepad analogue inputs:
	// 1. Filtering - not implemented
	// 2. Deadzoning - inner/outer square/circle
	// 3. Bias - deadzone is sufficient nowadays
	// 4. Shaping - more sensitivity at high values

	float x, y;
	float inner, outer;

	x = IN_SDLControllerGetAxis(SDL_CONTROLLER_AXIS_LEFTX);
	y = - IN_SDLControllerGetAxis(SDL_CONTROLLER_AXIS_LEFTY);
	IN_PadGetLSDeadzone(&inner, &outer);
	IN_PadDeadzoneStick(&x, &y, inner, outer, (qboolean)!!in_gamepadLSSquareDeadzone->integer);

	Sys_QueEvent(eventTime, SE_JOYSTICK_AXIS, AXIS_SIDE   , roundf(127 * x), 0, NULL);
	Sys_QueEvent(eventTime, SE_JOYSTICK_AXIS, AXIS_FORWARD, roundf(127 * y), 0, NULL);

	x = - IN_SDLControllerGetAxis(SDL_CONTROLLER_AXIS_RIGHTX);
	y = IN_SDLControllerGetAxis(SDL_CONTROLLER_AXIS_RIGHTY);
	if (in_gamepadRSInvertX->integer)
		x = -x;
	if (in_gamepadRSInvertY->integer)
		y = -y;
	IN_PadGetRSDeadzone(&inner, &outer);
	IN_PadDeadzoneStick(&x, &y, inner, outer, (qboolean)!!in_gamepadRSSquareDeadzone->integer);
	IN_PadShapeStick(&x, &y, in_gamepadRSAccelCurve->integer, in_gamepadRSAccel->value);

	Sys_QueEvent(eventTime, SE_JOYSTICK_AXIS, AXIS_YAW  , roundf(127 * x), 0, NULL);
	Sys_QueEvent(eventTime, SE_JOYSTICK_AXIS, AXIS_PITCH, roundf(127 * y), 0, NULL);
}

static void IN_PadMoveTriggers(int eventTime)
{
	float inner, outer;
	float rt, lt;

	rt = IN_SDLControllerGetAxis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
	IN_PadGetRTDeadzone(&inner, &outer);
	IN_PadDeadzoneAxis(&rt, inner, outer);
	lt = IN_SDLControllerGetAxis(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
	IN_PadGetLTDeadzone(&inner, &outer);
	IN_PadDeadzoneAxis(&lt, inner, outer);

	switch (in_gamepadTriggersAxis->integer) {
	case 0: // triggers act as A_JOY30 and A_JOY31 buttons
		if (rt > 0.0f) {
			if (!in_pad.rtDown) {
				in_pad.rtDown = qtrue;
				Sys_QueEvent(eventTime, SE_KEY, A_JOY30, qtrue, 0, NULL);
			}
		} else {
			if (in_pad.rtDown) {
				in_pad.rtDown = qfalse;
				Sys_QueEvent(eventTime, SE_KEY, A_JOY30, qfalse, 0, NULL);
			}
		}
		if (lt > 0.0f) {
			if (!in_pad.ltDown) {
				in_pad.ltDown = qtrue;
				Sys_QueEvent(eventTime, SE_KEY, A_JOY31, qtrue, 0, NULL);
			}
		} else {
			if (in_pad.ltDown) {
				in_pad.ltDown = qfalse;
				Sys_QueEvent(eventTime, SE_KEY, A_JOY31, qfalse, 0, NULL);
			}
		}
		break;
	case  1:
		Sys_QueEvent(eventTime, SE_JOYSTICK_AXIS, AXIS_UP, roundf(127 * (rt - lt)), 0, NULL);
		break;
	case -1:
		Sys_QueEvent(eventTime, SE_JOYSTICK_AXIS, AXIS_UP, roundf(127 * (lt - rt)), 0, NULL);
		break;
	case 2:
		Sys_QueEvent(eventTime, SE_JOYSTICK_AXIS, AXIS_ROLL, roundf(127 * (rt - lt)), 0, NULL);
		break;
	case -2:
		Sys_QueEvent(eventTime, SE_JOYSTICK_AXIS, AXIS_ROLL, roundf(127 * (lt - rt)), 0, NULL);
		break;
	}
}

static void IN_PadMove(int eventTime)
{
	if (!in_pad.controller)
		return;
	if (!SDL_GameControllerGetAttached(in_pad.controller))
		return;

	SDL_GameControllerUpdate();

	if (Key_GetCatcher() & (KEYCATCH_UI | KEYCATCH_CGAME | KEYCATCH_CONSOLE)) {
		IN_PadMoveUI(eventTime);
		if (in_gamepadTriggersAxis->integer == 0) {
			// send SE_KEY events for triggers so that binding works
			IN_PadMoveTriggers(eventTime);
		}
	} else {
		IN_PadMoveSticks(eventTime);
		IN_PadMoveTriggers(eventTime);
	}
}

void IN_Frame (void) {
	static int	eventTime;
	qboolean	loading;
	Uint32		flags;

	IN_JoyMove( eventTime );
	IN_PadMove( eventTime );

	// If not DISCONNECTED (main menu) or ACTIVE (in game), we're loading
	loading = (qboolean)( cls.state != CA_DISCONNECTED && cls.state != CA_ACTIVE && !(Key_GetCatcher() & KEYCATCH_UI));
	flags = SDL_GetWindowFlags( SDL_window );

	if( !(flags & SDL_WINDOW_FULLSCREEN) && ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) )
	{
		// Console is down in windowed mode
		IN_DeactivateMouse( );
	}
	else if( !(flags & SDL_WINDOW_FULLSCREEN) && loading )
	{
		// Loading in windowed mode
		IN_DeactivateMouse( );
	}
	else if( !(flags & SDL_WINDOW_INPUT_FOCUS ) )
	{
		// Window not got focus
		IN_DeactivateMouse( );
	}
	else
		IN_ActivateMouse( );

	IN_ProcessEvents( eventTime );

	// Set event time for next frame to earliest possible time an event could happen
	eventTime = Sys_Milliseconds( );
}

/*
===============
IN_ShutdownJoystick
===============
*/
static void IN_ShutdownJoystick( void )
{
	if ( !SDL_WasInit( SDL_INIT_JOYSTICK ) )
		return;

	if (stick)
	{
		SDL_JoystickClose(stick);
		stick = NULL;
	}

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

/*
===============
IN_ShutdownGameController
===============
*/
static void IN_ShutdownGameController( void )
{
#if 0
	for (int i = 0; i < 32; i++) {
		keynames[A_JOY0 + i].uiName = NULL;
	}
#endif
	if (!SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
		return;
	}

	if (in_pad.controller) {
		IN_CloseGameController();
	}

	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

void IN_Shutdown( void ) {
	SDL_StopTextInput( );

	IN_DeactivateMouse( );
	mouseAvailable = qfalse;

	IN_ShutdownGameController();
	IN_ShutdownJoystick( );

	SDL_window = NULL;
}

/*
===============
IN_Restart
===============
*/
void IN_Restart( void )
{
	IN_ShutdownGameController();
	IN_ShutdownJoystick( );
	IN_Init( SDL_window );
}
