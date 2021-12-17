#ifndef _H_INPUT
#define _H_INPUT

// these both could probably use some tweaking, deadzone affects the joysticks, max cross talk affects the face
// buttons and triggers

#define XENGINE_INPUT_DEADZONE					0.25f//0.4f
#define XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK		XINPUT_GAMEPAD_MAX_CROSSTALK

// some enums and defines concerning getting input from the xengine

#define XENGINE_INPUT_ALL_PORTS	    -1

enum AXIS_TYPE
{
	XENGINE_INPUT_AXIS_LEFT_JOYSTICK = 0,
	XENGINE_INPUT_AXIS_RIGHT_JOYSTICK,
};

enum BUTTON_TYPE
{
	XENGINE_INPUT_BUTTON_A = 0,
	XENGINE_INPUT_BUTTON_B,
	XENGINE_INPUT_BUTTON_X,
	XENGINE_INPUT_BUTTON_Y,
	XENGINE_INPUT_BUTTON_BLACK,
	XENGINE_INPUT_BUTTON_WHITE,
	XENGINE_INPUT_BUTTON_START,
	XENGINE_INPUT_BUTTON_BACK,
	XENGINE_INPUT_BUTTON_LTRIGGER,
	XENGINE_INPUT_BUTTON_RTRIGGER,
	XENGINE_INPUT_BUTTON_DPAD_UP,
	XENGINE_INPUT_BUTTON_DPAD_DOWN,
	XENGINE_INPUT_BUTTON_DPAD_LEFT,
	XENGINE_INPUT_BUTTON_DPAD_RIGHT,
	XENGINE_INPUT_BUTTON_LEFT_JOYSTICK,
	XENGINE_INPUT_BUTTON_RIGHT_JOYSTICK,
};
	
// structure that completely keeps track of 1 controller's input

struct XBGAMEPAD
{
    // the following members were inherited from XINPUT_GAMEPAD:

	WORD    buttons;
	BYTE    analogButtons[8];

    SHORT   thumbLX;
    SHORT   thumbLY;
    SHORT   thumbRX;
    SHORT   thumbRY;

    // thumb stick values converted to range [-1,+1]

    FLOAT      x1;
    FLOAT      y1;
    FLOAT      x2;
    FLOAT      y2;
    
    // state of buttons tracked since last poll

    WORD       lastButtons;
    BOOL       lastAnalogButtons[8];
    WORD       pressedButtons;
    BOOL       pressedAnalogButtons[8];

    // rumble properties

    XINPUT_RUMBLE   rumble;
    XINPUT_FEEDBACK feedback;

    // device properties

    XINPUT_CAPABILITIES caps;
    HANDLE			    device;

    // flags for whether game pad was just inserted or removed

    BOOL       inserted;
    BOOL       removed;
};

#endif