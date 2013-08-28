

#include "controller.h"
#include <gl/glut.h>


// 10ボタン ジョイスティック
#define GLUT_JOYSTICK_BUTTON_E  0x10    // 5
#define GLUT_JOYSTICK_BUTTON_F  0x20    // 6
#define GLUT_JOYSTICK_BUTTON_G  0x40    // 7
#define GLUT_JOYSTICK_BUTTON_H  0x80    // 8
#define GLUT_JOYSTICK_BUTTON_I  0x100   // 9b
#define GLUT_JOYSTICK_BUTTON_J  0x200   // 10

static CONTROLLER_DATA	g_cntl = { KEY_UP, KEY_UP, KEY_UP, KEY_UP, KEY_UP, KEY_UP, KEY_UP };


//===========================================================================
// コントローラの状態を取得する関数
//===========================================================================
void GetControllerData( CONTROLLER_DATA *cntl )
{
	*cntl = g_cntl;
}


//===========================================================================
// キーが押された場合に呼ばれる関数
//===========================================================================
void KeyDown( unsigned char key, int x, int y )
{
	switch ( key ) {
	case KEY_ESC:	exit(0);				break;	// ESC
	case KEY_A:		g_cntl.A = KEY_DOWN;	break;	// A
	case KEY_B:		g_cntl.B = KEY_DOWN;	break;	// B
	case KEY_C:		g_cntl.C = KEY_DOWN;	break;	// C
	default:	break;
	}
}


//===========================================================================
// キーが離された場合に呼ばれる関数
//===========================================================================
void KeyUp( unsigned char key, int x, int y )
{
	switch ( key ) {
	case KEY_A:		g_cntl.A = KEY_UP;		break;	// A
	case KEY_B:		g_cntl.B = KEY_UP;		break;	// B
	case KEY_C:		g_cntl.C = KEY_UP;		break;	// C
	default:	break;
	}
}


//===========================================================================
// 特殊キーが押された場合に呼ばれる関数
//===========================================================================
void SpecialKeyDown(int key, int x, int y)
{
	switch ( key ) {
	case GLUT_KEY_UP:		g_cntl.up	 = KEY_DOWN;	break;
	case GLUT_KEY_DOWN:		g_cntl.down  = KEY_DOWN;	break;
	case GLUT_KEY_LEFT:		g_cntl.left	 = KEY_DOWN;	break;
	case GLUT_KEY_RIGHT:	g_cntl.right = KEY_DOWN;	break;

	default:	break;
	}
}


//===========================================================================
// 特殊キーが離された場合に呼ばれる関数
//===========================================================================
void SpecialKeyUp(int key, int x, int y)
{
	switch ( key ) {
	case GLUT_KEY_UP:		g_cntl.up	 = KEY_UP;	break;
	case GLUT_KEY_DOWN:		g_cntl.down  = KEY_UP;	break;
	case GLUT_KEY_LEFT:		g_cntl.left	 = KEY_UP;	break;
	case GLUT_KEY_RIGHT:	g_cntl.right = KEY_UP;	break;

	default:	break;
	}
}


void joystick(unsigned int buttonMask, int x, int y, int z)
{
		if(300<x){
			g_cntl.left		 = KEY_UP;
			g_cntl.right	 = KEY_DOWN;
		}else if(x<-300){
			g_cntl.left		 = KEY_DOWN;
			g_cntl.right	 = KEY_UP;
		}else{
			g_cntl.left		 = KEY_UP;
			g_cntl.right	 = KEY_UP;
		}
		
		if(300<y){
			g_cntl.up		= KEY_UP;
			g_cntl.down		= KEY_DOWN;
		}else if(y<-300){
			g_cntl.up		= KEY_DOWN;
			g_cntl.down		= KEY_UP;
		}else{
			g_cntl.up		= KEY_UP;
			g_cntl.down		= KEY_UP;
		}

        switch (buttonMask) {
            case GLUT_JOYSTICK_BUTTON_A:
                break;

            case GLUT_JOYSTICK_BUTTON_B:
                break;

            case GLUT_JOYSTICK_BUTTON_C:
                break;

            case GLUT_JOYSTICK_BUTTON_D:
                break;

            case GLUT_JOYSTICK_BUTTON_E:
                break;

            case GLUT_JOYSTICK_BUTTON_F:
                break;

            case GLUT_JOYSTICK_BUTTON_G:
				break;

            case GLUT_JOYSTICK_BUTTON_H:
                break;

			case GLUT_JOYSTICK_BUTTON_I:
                break;

			case GLUT_JOYSTICK_BUTTON_J:
                break;
		}
}