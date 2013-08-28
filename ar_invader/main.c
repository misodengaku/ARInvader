#include <windows.h>
#include <stdio.h>
#include <string.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <AR/ar.h>
#include <AR/param.h>
#include <AR/video.h>
#include <AR/gsub.h>

#include "controller.h"	// キーボードによる制御
#include "GLMetaseq.h"	// モデルローダ


// ライブラリの設定
#ifdef _DEBUG
	#pragma comment(lib,"libARd.lib")
	#pragma comment(lib,"libARgsubd.lib")
	#pragma comment(lib,"libARvideod.lib")
	#pragma comment(linker,"/NODEFAULTLIB:libcmtd.lib")
#else
	#pragma comment(lib,"libAR.lib")
	#pragma comment(lib,"libARgsub.lib")
	#pragma comment(lib,"libARvideo.lib")
	#pragma comment(linker,"/NODEFAULTLIB:libcmt.lib")
#endif


// キャラクタデータ構造体
typedef struct {
	MQO_MODEL	model;
	GLfloat		rot;
	GLfloat		x;
	GLfloat		y;
	GLfloat		z;
} CHARACTER_DATA;


// インベーダーの数
#define INVADER_COUNT 20

// スコア表示に使うフォント名とサイズ
// 上手いやり方がわからん
#define FONT_NAME GLUT_BITMAP_HELVETICA_18
#define FONT_SIZE 18
#define FONT_R 1
#define FONT_G 0
#define FONT_B 0

// グローバル変数
static char	*g_vconf_name	= "Data/WDM_camera_flipV.xml";	// ビデオデバイスの設定ファイル
static char	*g_cparam_name  = "Data/camera_para.dat";		// カメラパラメータファイル
static char	*g_patt_name    = "Data/patt.sample1";		// パターンファイル

static int		g_patt_id;							// パターンのID
static double	g_patt_trans[3][4];					// 座標変換行列
static double	g_patt_center[2]	= { 0.0, 0.0 };	// パターンの中心座標
static double	g_patt_width		= 80.0;			// パターンのサイズ（単位：mm）
static int		g_thresh			= 100;			// 2値化の閾値




static CHARACTER_DATA g_airplane = { NULL, 0.0, 0.0, 0.0, 0.0 };	// キャラクタのデータ
static char	*g_mqo_airplane_name	= "Data/mqo/airplane.mqo";			// MQOファイル

static CHARACTER_DATA g_invader[INVADER_COUNT] = { NULL, 0.0, 0.0, 0.0, 0.0 };		// インベーダー
static char	*g_mqo_invader_name = "Data/mqo/invader_small.mqo";
static int invader_show[INVADER_COUNT];
static int invader_x[INVADER_COUNT];
static int invader_y[INVADER_COUNT];
static int invader_z[INVADER_COUNT];

static CHARACTER_DATA g_bullet = { NULL, 0.0, 0.0, 0.0, 0.0 };	// 弾丸
static char	*g_mqo_bullet_name = "Data/mqo/bullet.mqo";


static int				g_light_on  = TRUE;							// シェーディングのOn/Off
static int				g_optcmf_on = FALSE;						// 光学迷彩のOn/Off
static int				score = 0;									// スコア


// プロトタイプ宣言
static void MainLoop(void);
static void Cleanup(void);
static void DrawObject( ARUint8 *image );
static void CalcState(void);
static void SetLight( GLenum light );
static void OpticalCamouflage( unsigned char *image );
static void GetObjectDepth( GLfloat *depth, int width, int height, GLfloat *depth_min, GLfloat *depth_max );
static void DrawImage( unsigned char *color_buf, int width, int height );
static void DrawString(float x, float y, char* _string);


// ===========================================================================
// main関数
// ===========================================================================
int main( int argc, char **argv )
{
	ARParam	cparam;			// カメラパラメータ
	ARParam	wparam;			// カメラパラメータ（作業用変数）
	int		xsize, ysize;	// 画像サイズ
	int i;

	// GLUTの初期化
	glutInit( &argc, argv );

	// ビデオデバイスの設定
	if ( arVideoOpen( g_vconf_name ) < 0 ) {
		printf("ビデオデバイスのエラー");
		return -1;
	}

	// カメラパラメータの設定
	if ( arVideoInqSize( &xsize, &ysize ) < 0 ) {
		printf("画像サイズを取得できませんでした\n");
		return -1;
	}
	if ( arParamLoad( g_cparam_name, 1, &wparam ) < 0 ) {
		printf("カメラパラメータの読み込みに失敗しました\n");
		return -1;
	}
	arParamChangeSize( &wparam, xsize, ysize, &cparam );
	arInitCparam( &cparam );

	// パターンファイルのロード
	if ( (g_patt_id = arLoadPatt(g_patt_name)) < 0 ) {
		printf("パターンファイルの読み込みに失敗しました\n");
		return -1;
	}

	// ウィンドウの設定
	argInit( &cparam, 1.0, 0, 0, 0, 0 );
	glutSetWindowTitle("ARインベーダー");

	// GLMetaseqの初期化
	mqoInit();

	// モデルの読み込み
	if ( ( g_airplane.model = mqoCreateModel( g_mqo_airplane_name, 1.0 )) == NULL ) {
		printf("モデルの読み込みに失敗しました\n");
		return -1;
	}

	for (i = 0; i < INVADER_COUNT; i++){
		if ( ( g_invader[i].model = mqoCreateModel( g_mqo_invader_name, 1.0 )) == NULL ) {
			printf("モデルの読み込みに失敗しました\n");
			return -1;
		}
		invader_show[i] = 1;
	}

	if ( ( g_bullet.model = mqoCreateModel( g_mqo_bullet_name, 1.0 )) == NULL ) {
		printf("モデルの読み込みに失敗しました\n");
		return -1;
	}

	// ビデオキャプチャの開始
	arVideoCapStart();

	// 終了時に呼ぶ関数を指定
	atexit( Cleanup );

	// キーボード関数の登録
	glutKeyboardUpFunc( KeyUp );
	glutSpecialFunc( SpecialKeyDown );
	glutSpecialUpFunc( SpecialKeyUp );
	glutJoystickFunc( joystick, 100 ); // ジョイスティック


	printf("<<操作方法>>\n");
	printf("方向キー : 移動\n");
	printf("   %c     : 初期位置にリセット\n", KEY_B);
	printf("   %c     : シェーディングOn/Off\n", KEY_A);
	printf("   %c     : 光学迷彩On/Off \n", KEY_C);
	printf("  ESC    : 終了\n");

	// メインループの開始
	argMainLoop( NULL, KeyDown, MainLoop );

	return 0;
}


// ===========================================================================
// メインループ関数
// ===========================================================================
static void MainLoop(void)
{
	ARUint8			*image;
	ARMarkerInfo	*marker_info;
	int				marker_num;
	int				j, k;
	char			score_str[64];

	// カメラ画像の取得
	if ( (image = arVideoGetImage()) == NULL ) {
		arUtilSleep( 2 );
		return;
	}

	// カメラ画像の描画
	argDrawMode2D();
	argDispImage( image, 0, 0 );

	// マーカの検出と認識
	if ( arDetectMarker( image, g_thresh, &marker_info, &marker_num ) < 0 ) {
		exit(0);
	}

	// 次の画像のキャプチャ指示
	arVideoCapNext();

	// マーカの信頼度の比較
	k = -1;
	for( j = 0; j < marker_num; j++ ) {
		if ( g_patt_id == marker_info[j].id ) {
			if ( k == -1 ) k = j;
			else if ( marker_info[k].cf < marker_info[j].cf ) k = j;
		}
	}

	if ( k != -1 ) {
		// マーカの位置・姿勢（座標変換行列）の計算
		arGetTransMat( &marker_info[k], g_patt_center, g_patt_width, g_patt_trans );

		// 3Dオブジェクトの描画
		DrawObject( image );
	}

	// スコア表示に使う文字列処理
	sprintf_s(score_str, 64, "score: %d", score);

	// スコア表示
	DrawString(0.0f, 0.0f, score_str);

	// バッファの内容を画面に表示
	argSwapBuffers();

}


// ===========================================================================
// 終了処理関数
// ===========================================================================
static void Cleanup(void)
{
    int i;

	arVideoCapStop();	// ビデオキャプチャの停止
    arVideoClose();		// ビデオデバイスの終了
    argCleanup();		// グラフィック処理の終了

	// モデルの削除
	mqoDeleteModel( g_airplane.model );
	mqoDeleteModel( g_bullet.model );
	
	for (i = 0; i < INVADER_COUNT; i++){
		mqoDeleteModel( g_invader[i].model );
	}
	
	mqoCleanup();							// GLMetaseqの終了処理
}


// ===========================================================================
// 3Dオブジェクトの描画を行う関数
// ===========================================================================
static void DrawObject( ARUint8 *image )
{
	double	gl_para[16];
	int i;
	int invader_row;
	int invader_col;

	// オブジェクトの状態を計算
	CalcState();

	// 3Dオブジェクトを描画するための準備
	argDrawMode3D();
	argDraw3dCamera( 0, 0 );

	// 光源の設定
	glPushMatrix();
		SetLight(GL_LIGHT0);
	glPopMatrix();

	// 座標変換行列の適用
	argConvGlpara( g_patt_trans, gl_para );
	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixd( gl_para );

	// 3Dオブジェクトの描画
	glClear( GL_DEPTH_BUFFER_BIT );		// Zバッファの初期化
	glEnable( GL_DEPTH_TEST );			// 隠面処理の適用

	// シェーディング
	if ( g_light_on ) {
		glEnable( GL_LIGHTING );	// ライトOn
	} else {
		glDisable( GL_LIGHTING );	// ライトOff
	}

	// モデルの描画
	glPushMatrix();
		glTranslatef( g_airplane.x, g_airplane.y, g_airplane.z ) ;	// モデルの位置
		glRotatef( g_airplane.rot, 0.0, 0.0, 1.0);						// モデルの向き
		glRotatef( 90.0, 1.0, 0.0, 0.0 );								// モデルを立たせる
		mqoCallModel( g_airplane.model );								// モデルの描画
	glPopMatrix();

	for (i = 0; i < INVADER_COUNT; i++){
		if (invader_show[i] == 1){
			// インベーダーの行と列を計算
			invader_col = i % 5;
			invader_row = i / 5;

			// インベーダーの座標を計算
			invader_x[i] = invader_col * 80 - 100;
			invader_y[i] = invader_col * 50 - 100;
			invader_z[i] = 300 - invader_row * 60;

			// 描画
			glPushMatrix();			
				glTranslatef( invader_x[i], invader_y[i], invader_z[i] );
				glRotatef( invader_col * 20, 0.0, 0.0, 1.0);
				glRotatef( 90.0, 1.0, 0.0, 0.0 );
				mqoCallModel( g_invader[i].model );
			glPopMatrix();
		}
	}

	glPushMatrix();
		glTranslatef( g_bullet.x, g_bullet.y, g_bullet.z );
		glRotatef( 0.0, 0.0, 0.0, 1.0);
		glRotatef( 90.0, 1.0, 0.0, 0.0 );
		mqoCallModel( g_bullet.model );
	glPopMatrix();


	g_bullet.z += 20;
	for (i = 0; i < INVADER_COUNT; i++){

		// インベーダーとの衝突判定
		if (invader_show[i] == 1
			&& abs(g_bullet.x - invader_x[i]) < 40
			&& abs(g_bullet.y - invader_y[i]) < 20
			&& abs(g_bullet.z - invader_z[i]) < 20){
				// 衝突したら
				invader_show[i] = 0;
				g_bullet.x = g_airplane.x;
				g_bullet.y = g_airplane.y;
				g_bullet.z = 40;
				score += 10;
		}
	}

	if (g_bullet.z > 400){
		g_bullet.x = g_airplane.x;
		g_bullet.y = g_airplane.y;
		g_bullet.z = 40;
	}


	glDisable( GL_LIGHTING );
	glDisable( GL_DEPTH_TEST );

	// 光学迷彩
	if ( g_optcmf_on ) {
		OpticalCamouflage( image );
	}
}


// ===========================================================================
// オブジェクトの状態を計算する関数
// ===========================================================================
static void CalcState(void)
{
	CONTROLLER_DATA	control;	// コントローラ（キーボード）
	static GLfloat v = 0;		// 進行方向の速度

	// 強制的にジョイスティックを読みに行く
	glutForceJoystickFunc();

	// コントローラのデータを取得
	GetControllerData( &control );

	if (control.up == KEY_DOWN) {
		g_airplane.y += 10;
	}
	if (control.down == KEY_DOWN) {
		g_airplane.y -= 10;
	}
	if (control.right == KEY_DOWN) {
		g_airplane.x += 10;
	}
	if (control.left == KEY_DOWN) {
		g_airplane.x -= 10;
	}

	/*
	// 加速と減速
	if ( control.up == KEY_DOWN ) {
		// 正方向に加速
		v = 20;
	} else if ( control.down  == KEY_DOWN ) { 
		// 逆方向に加速
		v = -20;
	} else {
		// 減速
		if (v>0) { v--; }
		else if (v<0) { v++; }
	}

	// 回転
	if ( control.right == KEY_DOWN ) { g_airplane.rot -= 20.0; }
	if ( control.left  == KEY_DOWN ) { g_airplane.rot += 20.0; }

	// 移動
	g_airplane.x +=  v * sin(g_airplane.rot*3.14/180);
	g_airplane.y += -v * cos(g_airplane.rot*3.14/180);
	*/

	// シェーディングのOn/Off
	if ( control.A == KEY_DOWN ) g_light_on = !g_light_on;

	// 初期位置にリセット
	if ( control.B == KEY_DOWN ) {
		g_airplane.rot = 0;
		g_airplane.x = 0;
		g_airplane.y = 0;
		g_airplane.z = 0;
	}

	// 光学迷彩のOn/Off
	if ( control.C == KEY_DOWN ) g_optcmf_on = !g_optcmf_on;

}


// ===========================================================================
// 光源の設定を行う関数
// ===========================================================================
static void SetLight( GLenum light )
{
	GLfloat light_diffuse[]  = { 0.9, 0.9, 0.9, 1.0 };	// 拡散反射光
	GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };	// 鏡面反射光
	GLfloat light_ambient[]  = { 0.3, 0.3, 0.3, 0.1 };	// 環境光
	GLfloat light_position[] = { 0.0, 0.0, 0.0, 1.0 };	// 位置と種類

	// 光源の設定
	glLightfv( light, GL_DIFFUSE,  light_diffuse );	 // 拡散反射光の設定
	glLightfv( light, GL_SPECULAR, light_specular ); // 鏡面反射光の設定
	glLightfv( light, GL_AMBIENT,  light_ambient );	 // 環境光の設定
	glLightfv( light, GL_POSITION, light_position ); // 位置と種類の設定

	glEnable( light );	// 光源の有効化
}


// ===========================================================================
// 光学迷彩処理を行う関数
// ===========================================================================
static void OpticalCamouflage( unsigned char *image )
{
	GLfloat			*depth_buf, *pDepth;			// デプスバッファ
	unsigned char	*color_buf, *pColor;			// カラーバッファ
	GLfloat			depth_obj_min, depth_obj_max;	// 物体のデプス値の最小値・最大値
	GLfloat			depth, depth_obj_n;				// デプス値、物体の正規化デプス値
	unsigned char	cmfR, cmfG, cmfB;				// 光学迷彩の色
	int				width, height;					// 画像サイズ
	int				i, x, y, dx, dy;

	// 迷彩の色（迷彩の領域に薄っすら色をつけるための設定。それぞれ0～255）
	cmfR = 255;
	cmfG = 255;
	cmfB = 255;

	// 画像サイズの取得（ARToolKitのグローバル変数から取得）
	width = arImXsize;
	height = arImYsize;

	// デプスバッファの取得
	depth_buf = (GLfloat*)malloc( width * height * sizeof(GLfloat) );
	glReadPixels( 0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_buf);

	// 描画用のバッファを作成
	color_buf = (unsigned char*)malloc( width * height * 4 );

	if ( color_buf == NULL || depth_buf == NULL ) return;

	// オブジェクトのデプス値の最大最小を求める
	GetObjectDepth( depth_buf, width, height, &depth_obj_min, &depth_obj_max);

	// ラスタスキャン用のポインタの準備
	pDepth = depth_buf;
	pColor = color_buf;

	for (y=0; y<height; y++) {
		for (x=0; x<width; x++) {

			// デプス値
			depth = *pDepth++;

			if ( depth < 1.0 ) {
				// オブジェクト領域の場合

				// デプス値を正規化
				depth_obj_n = ( depth - depth_obj_min )/( depth_obj_max - depth_obj_min );

				// デプス値を元に読み取り座標をずらす（＝歪ませる）
				dx = (int)( x -0.05 * width * depth_obj_n * ((x-width/2.0)*0.01) );
				dy = (int)( y -0.05 * height * depth_obj_n * ((y-height/2.0)*0.01) );
				if ( dx < 0 ) dx = x;
				if ( dy < 0 ) dy = y;

				// インデックスを求める
				i = 4 * ( (height-1-dy) * width + dx );

				// 画素値の書き込み
				*pColor++ = (unsigned char)( image[i+2] * 0.9 + cmfR * 0.1 );	// R
				*pColor++ = (unsigned char)( image[i+1] * 0.9 + cmfG * 0.1 );	// G
				*pColor++ = (unsigned char)( image[i]   * 0.9 + cmfB * 0.1 );	// B
				*pColor++ = 255;												// A

			} else {
				// 背景の場合
				pColor += 3;
				*pColor++ = 0;
			}
		}
	}

	// 画像の描画
	DrawImage( color_buf, width ,height );

	// バッファの開放
	free( depth_buf );
	free( color_buf );
}


// ===========================================================================
// オブジェクトのデプス値の最小値、最大値を求める関数
// depth         : デプスバッファ
// width, height : 画像サイズ
// depth_min     : デプス値の最小値
// depth_max     : デプス値の最大値（背景=1.0を除く）
// ===========================================================================
static void GetObjectDepth( GLfloat *depth, int width, int height,
							GLfloat *depth_min, GLfloat *depth_max )
{
	GLfloat min = 0;
	GLfloat max = 0;
	GLfloat d;
	int i;

	for ( i=0; i<width*height; i++ ) {

		d = *depth++;

		if ( i==0 ) {
			min = d;
			max = 0;
		} else {
			if ( d > max && d <1.0 ) max = d;
			if ( d < min ) min = d;
		}
	}

	*depth_min = min;
	*depth_max = max;
}


// ===========================================================================
// 画像の描画を行う関数
// color_buf     : カラーバッファ
// width, height : 画像サイズ
// ===========================================================================
void DrawImage( unsigned char *color_buf, int width, int height )
{
	// 射影行列を保存
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();

		// 射影行列を変更
		glLoadIdentity();
		gluOrtho2D( 0, width, 0, height );

		// モデルビュー行列を保存
		glMatrixMode( GL_MODELVIEW );
		glPushMatrix();

			// 画像の描画
			glLoadIdentity();
			glRasterPos2i( 0, 0 );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			glEnable( GL_BLEND);
			glDrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, color_buf );
			glDisable( GL_BLEND );

		// モデルビュー行列を復元
		glPopMatrix();

	// 射影行列を復元
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
}

/*
	文字描画関数
	http://wiki.livedoor.jp/eruvasu/d/glutSting
*/
static void DrawString(float x, float y, char* _string)
{
	glDisable(GL_LIGHTING);
    // 平行投影にする
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

	glColor3f(FONT_R, FONT_G, FONT_B);
	glRasterPos2f(x, y + FONT_SIZE);
	//char* p = (char*)_string;
	while (*_string != '\0') glutBitmapCharacter(FONT_NAME, *_string++);
	
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}