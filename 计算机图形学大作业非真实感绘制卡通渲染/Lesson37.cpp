#include <windows.h>											// Header File For Windows
#include <gl\gl.h>												// Header File For The OpenGL32 Library
#include <gl\glu.h>		
#include <gl\glut.h>
//#include <glaux.h>											// Header File For The GLaux Library

#include <math.h>												// Header File For The Math Library  
#include <stdio.h>												// Header File For The Standard I/O Library  

#include "NeHeGL.h"												// Header File For NeHeGL

#pragma comment( lib, "opengl32.lib" )							// Search For OpenGL32.lib While Linking
#pragma comment( lib, "glu32.lib" )								// Search For GLu32.lib While Linking
#pragma comment( lib, "glaux.lib" )								// Search For GLaux.lib While Linking

#ifndef CDS_FULLSCREEN											// CDS_FULLSCREEN Is Not Defined By Some
#define CDS_FULLSCREEN 4										// Compilers. By Defining It This Way,
#endif	
														// We Can Avoid Errors

#define ALPHA_W_LIMIT 0.0
#define BETA_W_LIMIT 15.0
#define DARKEN 20.0
#define WEAKEN 2.0

GL_Window*	g_window;
Keys*		g_keys;
GLfloat moveX = 0.0,moveY = -2.0, moveZ = -20.0;
char *shaders[2] = {"Data\\shader.txt","Data\\shader1.txt"};
int shaderIndex = 0;

//enum FILE_CHOICE { flyhorse, castle, teapot, kitten };
//FILE_CHOICE MODEL_ID = flyhorse;
#define flyhorse 0
#define castle 1
//#define teapot 2
#define kitten 2
int MODEL_ID = flyhorse;
int MODEL_NUM = 3;



float MOVE_SPEED = 0.3;
// User Defined Structures
typedef struct tagMATRIX										// A Structure To Hold An OpenGL Matrix  
{
	float Data[16];												// We Use [16] Due To OpenGL's Matrix Format  
}
MATRIX;

typedef struct tagVECTOR										// A Structure To Hold A Single Vector  
{
	float X, Y, Z;												// The Components Of The Vector  
	tagVECTOR(){
		X = Y = Z = 0;
	}
	tagVECTOR(float v){
		X = Y = Z = v;
	}
}
VECTOR;

typedef struct tagVERTEX										// A Structure To Hold A Single Vertex  
{
	int num;
	VECTOR Nor;													// Vertex Normal  
	VECTOR Pos;													// Vertex Position  
	tagVERTEX(){
		num = 0;
	}
}
VERTEX;

typedef struct tagPOLYGON										// A Structure To Hold A Single Polygon  
{
	int Verts[3];											// Array Of 3 VERTEX Structures  
}
POLYGON;

// User Defined Variables
bool	ChangeMode = false;
bool	ChangeMode2 = false;
bool GaoGuang = false;
bool		outlineDraw		= true;								// Flag To Draw The Outline  
bool		outlineSmooth	= false;							// Flag To Anti-Alias The Lines  
float		outlineColor[3]	= { 0.0f, 0.0f, 0.0f };				// Color Of The Lines  
float		outlineWidth	= 3.0f;								// Width Of The Lines  

VECTOR		lightAngle;											// The Direction Of The Light  
bool		lightRotate		= false;							// Flag To See If We Rotate The Light  

float		modelAngle		= 0.0f;								// Y-Axis Angle Of The Model  
float		modelangle = 0.0f;
bool        modelRotate		= false;							// Flag To Rotate The Model  

POLYGON		*polyData		= NULL;								// Polygon Data  
int			polyNum			= 0;								// Number Of Polygons  

GLuint		shaderTexture[1];									// Storage For One Texture  

VECTOR CrossVertex(VECTOR v1,VECTOR v2){
	VECTOR ret;
	ret.X = v1.Y*v2.Z - v1.Z*v2.Y;
	ret.Y = v1.Z*v2.X - v1.X*v2.Z;
	ret.Z = v1.X*v2.Y - v1.Y*v2.X;
	return ret;
}

VECTOR NormalizeVertex(VECTOR v){
	float len = sqrtf(v.X*v.X + v.Y*v.Y + v.Z*v.Z);
	VECTOR ret;
	ret.X = v.X/len;
	ret.Y = v.Y/len;
	ret.Z = v.Z/len;
	return ret;
}

VECTOR VectorAdd(VECTOR v1,VECTOR v2){
	VECTOR ret;
	ret.X = v1.X + v2.X;
	ret.X = v1.Y + v2.Y;
	ret.Z = v1.Z + v2.Z;
	return ret;
}

VECTOR VectorMinus(VECTOR v1,VECTOR v2){
	VECTOR ret;
	ret.X = v1.X - v2.X;
	ret.Y = v1.Y - v2.Y;
	ret.Z = v1.Z - v2.Z;
	return ret;
}


// Math Functions
inline float DotProduct (VECTOR &V1, VECTOR &V2)				// Calculate The Angle Between The 2 Vectors  
{
	return V1.X * V2.X + V1.Y * V2.Y + V1.Z * V2.Z;				// Return The Angle  
}

inline float Magnitude (VECTOR &V)								// Calculate The Length Of The Vector  
{
	return sqrtf (V.X * V.X + V.Y * V.Y + V.Z * V.Z);			// Return The Length Of The Vector  
}

void chu(VECTOR &v,int num){
	if(num!=0) {
		v.X/=(float)num;
		v.Y/=(float)num;
		v.Z/=(float)num;
	}
	return;
}
void Normalize (VECTOR &V)										// Creates A Vector With A Unit Length Of 1  
{
	float M = Magnitude (V);									// Calculate The Length Of The Vector   

	if (M != 0.0f)												// Make Sure We Don't Divide By 0   
	{
		V.X /= M;												// Normalize The 3 Components   
		V.Y /= M;
		V.Z /= M;
	}
}

void RotateVector (MATRIX &M, VECTOR &V, VECTOR &D)				// Rotate A Vector Using The Supplied Matrix  
{
	D.X = (M.Data[0] * V.X) + (M.Data[4] * V.Y) + (M.Data[8]  * V.Z);	// Rotate Around The X Axis  
	D.Y = (M.Data[1] * V.X) + (M.Data[5] * V.Y) + (M.Data[9]  * V.Z);	// Rotate Around The Y Axis  
	D.Z = (M.Data[2] * V.X) + (M.Data[6] * V.Y) + (M.Data[10] * V.Z);	// Rotate Around The Z Axis  
}

VERTEX vArray[100010];
VECTOR nArray[100010];
char buf[1000024];

//FILE* foutt = fopen("Nor.txt","w");

GLvoid NorCal(int fcnt){
	VECTOR v1 = VectorMinus(vArray[polyData[fcnt].Verts[0]].Pos,vArray[polyData[fcnt].Verts[1]].Pos),
			v2 = VectorMinus(vArray[polyData[fcnt].Verts[1]].Pos,vArray[polyData[fcnt].Verts[2]].Pos);
	VECTOR vn = CrossVertex(v1,v2);
	for(int i = 0;i<3;i++){vArray[polyData[fcnt].Verts[i]].Nor.X += vn.X;
		vArray[polyData[fcnt].Verts[i]].Nor.Y += vn.Y;
		vArray[polyData[fcnt].Verts[i]].Nor.Z += vn.Z;vArray[polyData[fcnt].Verts[i]].num++;
	}
	return;
}

float HighLight(float t, float threshold)
{
	return 0.99f * (t - threshold) / (1.0f - threshold) * (t - threshold) / (1.0f - threshold) ;
}

int vcnt = 0,fcnt = 0,ncnt = 0;
char* buf2 = new char [1000024];
class Matieral
{
public:
	char Name[100];
	float color[3];
};
Matieral mtl[1000];
int mtl_c = 0;
GLvoid readMtlFile(FILE* file)
{
	while(fscanf(file, "%s", buf2) != EOF)
	{
		switch(buf2[0]) 
		{
		case 'n':
//			while(buf2[0] != ' ')
//				buf2 = buf2 + 1;
			fscanf(file, "%s", &mtl[mtl_c].Name);
			mtl_c = mtl_c + 1;
			break;
		case 'K':
			if(buf2[1] == 'd')
			{
				fscanf(file, "%f %f %f", &(mtl[mtl_c - 1]).color[0],&(mtl[mtl_c - 1]).color[1],&(mtl[mtl_c - 1]).color[2] );
			}
		}
	}
}
class mylog
{
public:
	int num;
	double color[3];
};
mylog Log[1000000];
int log_c = 0;
/*******************************/
/*远处会消失，能不能把可见深度调的大一点*/
/*城堡的效果比较明显*/
char* buf3 = new char [1000024];
bool Change[8] = {false, false, false, false, false, false, false, false};
/*bool Change1 = false; //外凸锥面映射
bool Change2 = false; //内凹锥面映射
bool Change3 = false; //内凹柱面映射
bool Change4 = false; //内凹球面映射
bool Change5 = false; //正弦波浪映射
bool Change6 = false; //马鞍面映射
bool Change7 = false; //模型扭转映射
bool Change8 = false; //任意方向映射*/

double Pos[3] = {1, 1, 1}; //映射方向
void Exchange()
{
	if(Change[0])  //外凸锥面映射 
	{
		vArray[vcnt].Pos.Z -= sqrt(vArray[vcnt].Pos.X * vArray[vcnt].Pos.X + vArray[vcnt].Pos.Y * vArray[vcnt].Pos.Y);
	}
	if(Change[1])  //内凹锥面映射
	{
		vArray[vcnt].Pos.Z += sqrt(vArray[vcnt].Pos.X * vArray[vcnt].Pos.X + vArray[vcnt].Pos.Y * vArray[vcnt].Pos.Y) / 2;
	}
	if(Change[2])  //内凹柱面映射
	{
		vArray[vcnt].Pos.Z -= 10 - vArray[vcnt].Pos.X * vArray[vcnt].Pos.X / 20;
	}
	if(Change[3])  //内凹球面映射   
	{
		vArray[vcnt].Pos.Z += sqrt(35 * 35 - vArray[vcnt].Pos.X * vArray[vcnt].Pos.X - vArray[vcnt].Pos.Y * vArray[vcnt].Pos.Y);
	}
	if(Change[4])  //正弦波浪映射
	{
		vArray[vcnt].Pos.Z += 2 * sin(vArray[vcnt].Pos.X / 2);
	}
	if(Change[5])  //马鞍面映射
	{
		vArray[vcnt].Pos.Z += - vArray[vcnt].Pos.X * vArray[vcnt].Pos.X / 20 + vArray[vcnt].Pos.Y * vArray[vcnt].Pos.Y / 20;
	}
	if(Change[6]) // 模型扭转映射
	{
		double step = vArray[vcnt].Pos.Y * 2 / 20;
		vArray[vcnt].Pos.Z += vArray[vcnt].Pos.Y / 5 + (vArray[vcnt].Pos.X + 10 ) * step;
	}
	if(Change[7])  //任意方向映射
	{
		double s1 = Pos[0] / sqrt(Pos[0] * Pos[0] + Pos[2] * Pos[2]);
		double c1 = Pos[2] / sqrt(Pos[0] * Pos[0] + Pos[2] * Pos[2]);
		double s2 = Pos[1] / sqrt(Pos[0] * Pos[0] + Pos[1] * Pos[1] + Pos[2] * Pos[2]);
		double c2 = sqrt(Pos[0] * Pos[0] + Pos[2] * Pos[2]) / sqrt(Pos[0] * Pos[0] + Pos[1] * Pos[1] + Pos[2] * Pos[2]);
		double x0 = vArray[vcnt].Pos.X;
		double y0 = vArray[vcnt].Pos.Y;
		double z0 = vArray[vcnt].Pos.Z; 
		double x1 = c1 * x0 + s1 * z0;
		double y1 = y0;
		double z1 = -s1 * x0 + c1 * z0;
		double x2 = x1;
		double y2 = c2 * y1 - s2 * z1;
		double z2 = s2 * y1 + c2 * z1;
		vArray[vcnt].Pos.X = x2;
		vArray[vcnt].Pos.Y = y2;
		vArray[vcnt].Pos.Z = z2;
	}
}
GLvoid readTxtFile(FILE* file)
{
	int t;
	for(int i = 0; i < 7; ++i)
	{
		fscanf(file, "%d", &t);
		if(t == 1)
			Change[i] = true;
		else Change[i] = false;
	}
	fscanf(file, "%lf %lf %lf", &Pos[0], &Pos[1], &Pos[2]);
}
/**********************************/
//int vcnt = 0,fcnt = 0,ncnt = 0;
GLvoid readObjFile(FILE* file){

	vcnt = 0;
	fcnt = 0;
	ncnt = 0;
	while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
			case 'u':
	//		while(buf[0] != ' ')
	//			buf = buf + 1;
			fscanf(file, "%s", buf);
			for(int i = 0; i < mtl_c; ++i)
			{
				if(strcmp(buf, mtl[i].Name) == 0)
				{
					Log[log_c].num = fcnt;
					Log[log_c].color[0] = mtl[i].color[0];
					Log[log_c].color[1] = mtl[i].color[1];
					Log[log_c].color[2] = mtl[i].color[2];
					log_c++;
					break;
				}
			}
			break;
        case '#':               /* comment */
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
            break;
        case 'v':               /* v, vn, vt */
            switch(buf[1]) {
				case '\0':          /* vertex */
					fscanf(file, "%f %f %f", &vArray[vcnt].Pos.X,&vArray[vcnt].Pos.Y,&vArray[vcnt].Pos.Z);
					//vArray[vcnt] = NormalizeVertex(vArray[vcnt]);
					Exchange();
					vcnt++;
					break;
				case 'n':           
					fscanf(file, "%f %f %f",&nArray[ncnt].X,&nArray[ncnt].Y,&nArray[ncnt].Z);
					//fprintf(foutt,"");
					//fprintf(foutt,"%f %f %f\n",nArray[ncnt].X,nArray[ncnt].Y,nArray[ncnt].Z);
					ncnt++;
					break;
				default:
					fgets(buf, sizeof(buf), file);
					break;
				}
			break;
        case 'f':               /* face */
			int v,n,t;
			v = n = t = -1;
                fscanf(file, "%s", buf);
				//fprintf(foutt,"%s\n",buf);
                /* can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d */
                if (strstr(buf, "//")) {
					sscanf(buf, "%d//%d", &v, &n);
					v = v < 0 ? v + vcnt : v;
					v--;
					n = n < 0 ? n + ncnt : n;
					n--;
					polyData[fcnt].Verts[0] = v;
					vArray[polyData[fcnt].Verts[0]].Nor = nArray[n];
                    fscanf(file,  "%d//%d", &v, &n);
					v = v < 0 ? v + vcnt : v;
                    v--;
					n = n < 0 ? n + ncnt : n;
					n--;
					polyData[fcnt].Verts[1] = v;
					vArray[polyData[fcnt].Verts[1]].Nor = nArray[n];
                    fscanf(file, "%d//%d", &v, &n);
					v = v < 0 ? v + vcnt : v;
                    v--;
					n = n < 0 ? n + ncnt : n;
					n--;
					polyData[fcnt].Verts[2] = v;
					vArray[polyData[fcnt].Verts[2]].Nor = nArray[n];
					//NorCal(fcnt);
					fcnt++;
                    while(fscanf(file, "%d//%d", &v, &n) > 0) {
						v = v < 0 ? v + vcnt : v;
						v--;
						n = n < 0 ? n + ncnt : n;
						n--;
						polyData[fcnt].Verts[0] = polyData[fcnt-1].Verts[0];
                        polyData[fcnt].Verts[1] = polyData[fcnt-1].Verts[2];
                        polyData[fcnt].Verts[2] = v;
						vArray[polyData[fcnt].Verts[2]].Nor = nArray[n];
						fcnt++;
                    }
                } else if (sscanf(buf, "%d/%d/%d", &v, &t, &n) == 3) {
					v = v < 0 ? v + vcnt : v;
					v--;
					n = n < 0 ? n + ncnt : n;
					n--;
					polyData[fcnt].Verts[0] = v;
					vArray[polyData[fcnt].Verts[0]].Nor = nArray[n];
                    fscanf(file,  "%d/%d/%d", &v, &t, &n);
					v = v < 0 ? v + vcnt : v;
                    v--;
					n = n < 0 ? n + ncnt : n;
					n--;
					polyData[fcnt].Verts[1] = v;
					vArray[polyData[fcnt].Verts[1]].Nor = nArray[n];
                    fscanf(file,  "%d/%d/%d", &v, &t, &n);
					v = v < 0 ? v + vcnt : v;
                    v--;
					n = n < 0 ? n + ncnt : n;
					n--;
					polyData[fcnt].Verts[2] = v;
					vArray[polyData[fcnt].Verts[2]].Nor = nArray[n];
					fcnt++;
                    while(fscanf(file, "%d/%d/%d", &v, &t, &n) > 0) {
						v < 0 ? v + vcnt : v;
						v--;
						n = n < 0 ? n + ncnt : n;
						n--;
						polyData[fcnt].Verts[0] = polyData[fcnt-1].Verts[0];
                        polyData[fcnt].Verts[1] = polyData[fcnt-1].Verts[2];
                        polyData[fcnt].Verts[2]= v;
						vArray[polyData[fcnt].Verts[2]].Nor = nArray[n];
						fcnt++;
                    }
                } else if (sscanf(buf, "%d/%d", &v, &t) == 2) {
                    /* v/t */
					v = v < 0 ? v + vcnt : v;
					v--;
					polyData[fcnt].Verts[0]= v;
                    fscanf(file, "%d/%d", &v,&t);
					v = v < 0 ? v + vcnt : v;
                    v--;
					polyData[fcnt].Verts[1]= v;
                    fscanf(file, "%d/%d", &v,&t);
					v = v < 0 ? v + vcnt : v;
                    v--;
					polyData[fcnt].Verts[2]= v;
					NorCal(fcnt);
					fcnt++;
                    while(fscanf(file, "%d/%d", &v,&t) > 0) {
						v < 0 ? v + vcnt : v;
						v--;
						polyData[fcnt].Verts[0] = polyData[fcnt-1].Verts[0];
                        polyData[fcnt].Verts[1] = polyData[fcnt-1].Verts[2];
                        polyData[fcnt].Verts[2] = v;
						NorCal(fcnt);
                        fcnt++;
                    }
                } else {
                    /* v */
                    sscanf(buf, "%d", &v);
					v = v < 0 ? v + vcnt : v;
					v--;
					polyData[fcnt].Verts[0] = v;
                    fscanf(file, "%d", &v);
					v = v < 0 ? v + vcnt : v;
                    v--;
					polyData[fcnt].Verts[1] = v;
                    fscanf(file, "%d", &v);
					v = v < 0 ? v + vcnt : v;
                    v--;
					polyData[fcnt].Verts[2] = v;
					NorCal(fcnt);
					fcnt++;
                    while(fscanf(file, "%d", &v) > 0) {
						v < 0 ? v + vcnt : v;
						v--;
						polyData[fcnt].Verts[0] = polyData[fcnt-1].Verts[0];
                        polyData[fcnt].Verts[1]= polyData[fcnt-1].Verts[2];
                        polyData[fcnt].Verts[2]= v;
						NorCal(fcnt);
                        fcnt++;
                    }
                }
			/*	if(Exchange2)
				{
					polyData[fcnt].Verts[2] -= sqrt((double)polyData[fcnt].Verts[0] * polyData[fcnt].Verts[0] + polyData[fcnt].Verts[1] * polyData[fcnt].Verts[1]) ;
				}*/
                break;
                
            default:
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
		}
	}
	polyNum = fcnt;
	//fprintf(foutt,"%d\n",polyNum);
}


// File Functions
BOOL ReadMesh ()												// Reads The Contents Of The "model.txt" File  
{
	// Open The File  
	FILE * In = NULL;
	FILE * In2 = NULL; 
	/*****************************/
	FILE * In3 = NULL;
	if(MODEL_ID == flyhorse) {
		In2 = fopen ("flyhorse.mtl", "rb");
		In = fopen("flyhorse.obj", "rb");       // */
		In3 = fopen("flyhorse.txt", "rb");
	}
	else if(MODEL_ID == castle) {
		In2 = fopen ("castle.mtl", "rb");
		In = fopen("castle.obj", "rb");			// */
		In3 = fopen("castle.txt", "rb");
	}
/*	else if(MODEL_ID == teapot) {
		In2 = fopen ("teapot.mtl", "rb");
		In = fopen("teapot.obj", "rb");			
	}											// */
	else if(MODEL_ID == kitten) {
		In = fopen("kitten.obj", "rb");     
		In2 = fopen ("kitten.mtl", "rb");		// */
		In3 = fopen("kitten.txt", "rb");
	}

/*	FILE * In = fopen("Simba.obj","rb");      
	FILE * In2 = fopen ("Simba.mtl", "rb");		// */
//	FILE * In = fopen("pit.obj", "rb");         // direction
//	FILE * In = fopen("kitten.obj", "rb");         //ok
//	FILE * In = fopen("dragon-2.obj", "rb");    // direction
//	FILE * In = fopen("blaze.obj", "rb");   // direction
//	FILE * In = fopen("arcticcondor.obj", "rb"); // wrong

	if (!In){
		return FALSE;                                                                                                                                                                                                                                                                                                                                                                                                                                                
		//fprintf(foutt,"LLKJHLJKHLJK");
	}// Return FALSE If File Not Opened  
	log_c = 0;
	memset(Log, 0, sizeof(Log));
	memset(mtl, 0, sizeof(mtl));
	//fread (&polyNum, sizeof (int), 1, In);						// Read The Header (i.e. Number Of Polygons)  
	if(In2) readMtlFile(In2);
	polyData = new POLYGON [80010];							// Allocate The Memory  
	if(In3) readTxtFile(In3);
	readObjFile(In);
	//fread (&polyData[0], sizeof (POLYGON) * polyNum, 1, In);	// Read In All Polygon Data  
	
	if(In)
		fclose (In);												// Close The File  
	if(In2)
		fclose (In2);
	if(In3)
		fclose (In3);
	return TRUE;												// It Worked  
}

void colorFunct2 ( float rangeX, float rangeY, float rangeZ, float tmp, float goal[3])
{
	float Red, Green, Blue;
	float BW;				// black || white (linear)
	float a, b, c;	// f(tmp) = c * (tmp - a) * (tmp - b)
	float k;
	// BW
	k = (rangeZ - 0.0) / (rangeY - rangeX);
	BW = k * (tmp - rangeX);
	// B
	if(tmp >= (rangeX + rangeY) / 2)
		Blue = 0;
	else
	{
		a = rangeY;
		b = 2 * rangeX - rangeY;
		c = (-1) * (4 * rangeZ) / ((a - b) * (a - b));
		Blue = c * (tmp - a) * (tmp - b);
	}
	// R
	if(tmp <= (rangeX + rangeY) / 2)
		Red = 0;
	else
	{
		a = rangeX;
		b = 2 * rangeY - rangeX;
		c = (-1) * (4 * rangeZ) / ((a - b) * (a - b));
		Red = c * (tmp - a) * (tmp - b);
	}
	// G
	Green = Red + Blue;
	goal[0] = Red / DARKEN + BW / WEAKEN;
	goal[1] = Green / DARKEN + BW / WEAKEN;
	goal[2] = Blue / DARKEN + BW / WEAKEN;
}

void colorFunct ( float rangeX, float rangeY, float rangeZ, float tmp, float goal[3])
{
	float Red, Green, Blue;
	float a, b, c;	// f(tmp) = c * (tmp - a) * (tmp - b)
	/*
	// R
	if(tmp >= (rangeX + rangeY) / 2)
		Red = 0;
	else
	{
		a = (rangeX + rangeY) / 2;
		b = ( 3 * rangeX - rangeY) / 2;
		c = (-1) * (4 * rangeZ) / ((a - b) * (a - b));
		Red = c * (tmp - a) * (tmp - b);
	}
	// G
	a = rangeX;
	b = rangeY;
	c = (-1) * (4 * rangeZ) / ((a - b) * (a - b));
	Green = c * (tmp - a) * (tmp - b);
	// B
	if(tmp <= (rangeX + rangeY) / 2)
		Blue = 0;
	else
	{
		a = (rangeX + rangeY) / 2;
		b = ( 3 * rangeY - rangeX) / 2;
		c = (-1) * (4 * rangeZ) / ((a - b) * (a - b));
		Blue = c * (tmp - a) * (tmp - b);
	}
	*/
	// B
	if(tmp >= (rangeX + rangeY) / 2)
		Blue = 0;
	else
	{
		a = (rangeX + rangeY) / 2;
		b = ( 3 * rangeX - rangeY) / 2;
		c = (-1) * (4 * rangeZ) / ((a - b) * (a - b));
		Blue = c * (tmp - a) * (tmp - b);
	}
	// G
	a = rangeX;
	b = rangeY;
	c = (-1) * (4 * rangeZ) / ((a - b) * (a - b));
	Green = c * (tmp - a) * (tmp - b);
	// R
	if(tmp <= (rangeX + rangeY) / 2)
		Red = 0;
	else
	{
		a = (rangeX + rangeY) / 2;
		b = ( 3 * rangeY - rangeX) / 2;
		c = (-1) * (4 * rangeZ) / ((a - b) * (a - b));
		Red = c * (tmp - a) * (tmp - b);
	}
	goal[0] = Red;
	goal[1] = Green;
	goal[2] = Blue;
}
	float shaderData[32][3];									// Storate For The 96 Shader Values  
	float shaderData2[32][3];
// Engine Functions
BOOL Initialize (GL_Window* window, Keys* keys)					// Any GL Init Code & User Initialiazation Goes Here
{
	int i;														// Looping Variable  
	char Line[255];												// Storage For 255 Characters  

	FILE *In	= NULL;											// File Pointer  

	g_window	= window;
	g_keys		= keys;

	// Start Of User Initialization
	glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);			// Realy Nice perspective calculations
	
	glClearColor (0.7f, 0.7f, 0.7f, 0.0f);						// Light Grey Background
	glClearDepth (1.0f);										// Depth Buffer Setup

	glEnable (GL_DEPTH_TEST);									// Enable Depth Testing
	glDepthFunc (GL_LESS);										// The Type Of Depth Test To Do

	glShadeModel (GL_SMOOTH);									// Enables Smooth Color Shading  
	glDisable (GL_LINE_SMOOTH);									// Initially Disable Line Smoothing  

	glEnable (GL_CULL_FACE);									// Enable OpenGL Face Culling  

	glDisable (GL_LIGHTING);									// Disable OpenGL Lighting  

	In = fopen ("Data\\shader.txt", "r");						// Open The Shader File  

	if (In)														// Check To See If The File Opened  
	{
		for (i = 0; i < 32; i++)								// Loop Though The 32 Greyscale Values  
		{
			if (feof (In))										// Check For The End Of The File  
				break;

			fgets (Line, 255, In);								// Get The Current Line  

			shaderData[i][0] = shaderData[i][1] = shaderData[i][2] =  shaderData2[i][0] = shaderData2[i][1] = shaderData2[i][2] = float(atof (Line)); // Copy Over The Value  
			//if(ChangeMode) colorFunct ( 0.5, 1.0, 2.0, shaderData2[i][0], shaderData2[i]);
		}

		fclose (In);											// Close The File  
	}

	else
		return FALSE;											// It Went Horribly Horribly Wrong  

	glGenTextures (1, &shaderTexture[0]);						// Get A Free Texture ID  

	glBindTexture (GL_TEXTURE_1D, shaderTexture[0]);			// Bind This Texture. From Now On It Will Be 1D  

	// For Crying Out Loud Don't Let OpenGL Use Bi/Trilinear Filtering!  
	glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);	
	glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage1D (GL_TEXTURE_1D, 0, GL_RGB, 32, 0, GL_RGB , GL_FLOAT, shaderData);	// Upload  

	lightAngle.X = 0.0f;										// Set The X Direction  
	lightAngle.Y = 0.0f;										// Set The Y Direction  
	lightAngle.Z = 1.0f;										// Set The Z Direction  

	Normalize (lightAngle);										// Normalize The Light Direction  


	return ReadMesh ();											// Return The Value Of ReadMesh  
}

void Deinitialize (void)										// Any User DeInitialization Goes Here
{
	glDeleteTextures (1, &shaderTexture[0]);					// Delete The Shader Texture  

	delete [] polyData;											// Delete The Polygon Data  
}

void keyboard(unsigned char key,int x,int y){
	switch(key){
		case 'x':
			moveX+=5;
			glutPostRedisplay();
			break;
		case 'X':
			moveX-=5;
			glutPostRedisplay();
			break;
		case 'z':
			moveZ+=5;
			glutPostRedisplay();
			break;
		case 'Z':
			moveZ-=5;
			glutPostRedisplay();
			break;
		case 'y':
			moveY+=5;
			glutPostRedisplay();
			break;
		case 'Y':
			moveY-=5;
			glutPostRedisplay();
			break;
		default:
			break;
	}
}

void Update (DWORD milliseconds)								// Perform Motion Updates Here
{
	if (g_keys->keyDown [VK_ESCAPE] == TRUE)					// Is ESC Being Pressed?
	{
		TerminateApplication (g_window);						// Terminate The Program
	}

	if (g_keys->keyDown [VK_F1] == TRUE)						// Is F1 Being Pressed?
	{
		ToggleFullscreen (g_window);							// Toggle Fullscreen Mode
	}

	if (g_keys->keyDown [' '] == TRUE)							// Is the Space Bar Being Pressed?  
	{
		modelRotate = !modelRotate;								// Toggle Model Rotation On/Off  

		g_keys->keyDown [' '] = FALSE;
	}

	if (g_keys->keyDown ['1'] == TRUE)							// Is The Number 1 Being Pressed?  
	{
		outlineDraw = !outlineDraw;								// Toggle Outline Drawing On/Off  

		g_keys->keyDown ['1'] = FALSE;
	}

	if (g_keys->keyDown ['2'] == TRUE)							// Is The Number 2 Being Pressed?  
	{
		outlineSmooth = !outlineSmooth;							// Toggle Anti-Aliasing On/Off  

		g_keys->keyDown ['2'] = FALSE;
	}
	if(g_keys->keyDown ['3'] == TRUE)
	{
		ChangeMode = !ChangeMode;
		g_keys->keyDown['3'] = FALSE;
	}
	if(g_keys->keyDown ['4'] == TRUE)
	{
		GaoGuang = !GaoGuang;
		g_keys->keyDown ['4'] = FALSE;
	}
	if(g_keys->keyDown ['5'] == TRUE)
	{
		ChangeMode2 = !ChangeMode2;
		g_keys->keyDown['5'] = FALSE;
	}

	/* 权宜之计 */
	if(g_keys->keyDown ['6']){
		moveX-= MOVE_SPEED;					// 1.05
		g_keys->keyDown['6'] = FALSE;
	}
	if(g_keys->keyDown ['7']){
		moveX+= MOVE_SPEED;
		g_keys->keyDown['7'] = FALSE;
	}
	if(g_keys->keyDown ['8']){
		moveY-= MOVE_SPEED;
		g_keys->keyDown['8'] = FALSE;
	}
	if(g_keys->keyDown ['9']){
		moveY+= MOVE_SPEED;
		g_keys->keyDown['9'] = FALSE;
	}
	if(g_keys->keyDown ['0']){
		MODEL_ID++;
		MODEL_ID %= MODEL_NUM;
		ReadMesh();
		g_keys->keyDown['0'] = FALSE;
	}
	/* 权宜之计 */

	if (g_keys->keyDown [VK_LEFT] == TRUE)						// Is The Left Arrow Being Pressed?  
	{
		if(outlineWidth <= BETA_W_LIMIT)
			outlineWidth++;											// Increase Line Width  

		g_keys->keyDown [VK_LEFT] = FALSE;
	}

	if (g_keys->keyDown [VK_RIGHT] == TRUE)						// Is The Right Arrow Being Pressed?  
	{
		if(outlineWidth >= ALPHA_W_LIMIT)
			outlineWidth--;											// Decrease Line Width  

		g_keys->keyDown [VK_RIGHT] = FALSE;
	}
	if(g_keys->keyDown [VK_UP]){
		moveZ-=1.05;
		g_keys->keyDown[VK_UP] = FALSE;
	}
	if(g_keys->keyDown [VK_DOWN]){
		moveZ+=1.05;
		g_keys->keyDown[VK_DOWN] = FALSE;
	}

	if(g_keys->keyDown ['A']){
		moveX-= MOVE_SPEED;					// 1.05
		g_keys->keyDown['A'] = FALSE;
	}
	if(g_keys->keyDown ['D']){
		moveX+= MOVE_SPEED;
		g_keys->keyDown['D'] = FALSE;
	}
	if(g_keys->keyDown ['S']){
		moveY-= MOVE_SPEED;
		g_keys->keyDown['S'] = FALSE;
	}
	if(g_keys->keyDown ['W']){
		moveY+= MOVE_SPEED;
		g_keys->keyDown['W'] = FALSE;
	}
	if(g_keys->keyDown[VK_SHIFT]){
		modelangle += 0.5;
		g_keys->keyDown[VK_SHIFT] = FALSE;
	}
	if(ChangeMode)
	{
		
		for(int i = 0; i < 32; ++i)
		{
		 shaderData2[i][0] = shaderData2[i][1] = shaderData2[i][2] = shaderData[i][0]; // Copy Over The Value  
		 colorFunct ( 0.5, 1.0, 2.0, shaderData2[i][0], shaderData2[i]);
		}
		 glBindTexture (GL_TEXTURE_1D, shaderTexture[0]);			// Bind This Texture. From Now On It Will Be 1D  
	     glTexImage1D (GL_TEXTURE_1D, 0, GL_RGB, 32, 0, GL_RGB , GL_FLOAT, shaderData2);	// Upload  
	}
	else if(ChangeMode2)
	{
		
		for(int i = 0; i < 32; ++i)
		{
		 shaderData2[i][0] = shaderData2[i][1] = shaderData2[i][2] = shaderData[i][0]; // Copy Over The Value  
		 colorFunct2 ( 0.5, 1.0, 2.0, shaderData2[i][0], shaderData2[i]);
		}
		 glBindTexture (GL_TEXTURE_1D, shaderTexture[0]);			// Bind This Texture. From Now On It Will Be 1D  
	     glTexImage1D (GL_TEXTURE_1D, 0, GL_RGB, 32, 0, GL_RGB , GL_FLOAT, shaderData2);	// Upload  
	}
	else
	{
		 glBindTexture (GL_TEXTURE_1D, shaderTexture[0]);			// Bind This Texture. From Now On It Will Be 1D  
	     glTexImage1D (GL_TEXTURE_1D, 0, GL_RGB, 32, 0, GL_RGB , GL_FLOAT, shaderData);	// Upload  
	}
	if(g_keys->keyDown ['X'])
	{
		shaderIndex = 1-shaderIndex;
		int i;														// Looping Variable  
		char Line[255];												// Storage For 255 Characters  
		float shaderData[32][3];									// Storate For The 96 Shader Values  
		float shaderData2[32][3];
		FILE *In	= NULL;											// File Pointer  

		glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);			// Realy Nice perspective calculations
	
	glClearColor (0.7f, 0.7f, 0.7f, 0.0f);						// Light Grey Background
	glClearDepth (1.0f);										// Depth Buffer Setup

	glEnable (GL_DEPTH_TEST);									// Enable Depth Testing
	glDepthFunc (GL_LESS);										// The Type Of Depth Test To Do

	glShadeModel (GL_SMOOTH);									// Enables Smooth Color Shading  
	glDisable (GL_LINE_SMOOTH);									// Initially Disable Line Smoothing  

	glEnable (GL_CULL_FACE);									// Enable OpenGL Face Culling  

	glDisable (GL_LIGHTING);									// Disable OpenGL Lighting  
		In = fopen (shaders[shaderIndex], "r");						// Open The Shader File  
		
		if (In)														// Check To See If The File Opened  
		{
			for (i = 0; i < 32; i++)								// Loop Though The 32 Greyscale Values  
			{
				if (feof (In))										// Check For The End Of The File  
					break;
					fgets (Line, 255, In);								// Get The Current Line  

				
			}

			fclose (In);											// Close The File  
		}								// It Went Horribly Horribly Wrong  
		else {
			shaderIndex = 0;
			return;
			
		}
		glBindTexture (GL_TEXTURE_1D, shaderTexture[0]);			// Bind This Texture. From Now On It Will Be 1D  
		if(!ChangeMode)glTexImage1D (GL_TEXTURE_1D, 0, GL_RGB, 32, 0, GL_RGB , GL_FLOAT, shaderData);	// Upload  
		else glTexImage1D (GL_TEXTURE_1D, 0, GL_RGB, 32, 0, GL_RGB , GL_FLOAT, shaderData2);	// Upload  
		g_keys->keyDown['X'] = false;
	}
	if (modelRotate)											// Check To See If Rotation Is Enabled  
		modelAngle += (float) (milliseconds) / 60.0f;			// Update Angle Based On The Clock
}




void Draw (void)
{
	int i, j;													// Looping Variables  

	float TmpShade;												// Temporary Shader Value  
	//FILE* fout = fopen("txt.txt","w");
	MATRIX TmpMatrix;											// Temporary MATRIX Structure  
	VECTOR TmpVector, TmpNormal;								// Temporary VECTOR Structures  
	
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clear The Buffers
	glLoadIdentity ();											// Reset The Matrix

	if (outlineSmooth)											// Check To See If We Want Anti-Aliased Lines  
	{
		glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);				// Use The Good Calculations  
		glEnable (GL_LINE_SMOOTH);								// Enable Anti-Aliasing  
	}

	else														// We Don't Want Smooth Lines  
		glDisable (GL_LINE_SMOOTH);								// Disable Anti-Aliasing  
	glTranslatef (0.0f+moveX, 0.0f+moveY, -2.0f+moveZ);							// Move 2 Units Away From The Screen  
	glRotatef (modelAngle, 0.0f, 1.0f, 0.0f);
	glRotatef(modelangle,1.0f,0.0f,0.0f);// Rotate The Model On It's Y-Axis  
	glGetFloatv (GL_MODELVIEW_MATRIX, TmpMatrix.Data);			// Get The Generated Matrix  

	// Cel-Shading Code //
	glEnable (GL_TEXTURE_1D);									// Enable 1D Texturing  
	glBindTexture (GL_TEXTURE_1D, shaderTexture[0]);			// Bind Our Texture  

	glColor3f (1.0f, 1.0f, 1.0f);								// Set The Color Of The Model  
	for(int i = 0;i<vcnt;i++){
		//chu(vArray[i].Nor,vArray[i].num);
	}
	glBegin (GL_TRIANGLES);										// Tell OpenGL That We're Drawing Triangles
		int logcnt = 0;
		for (i = 0; i < polyNum; i++)							// Loop Through Each Polygon  
		{
			if(log_c > 0 && i == Log[logcnt].num)
				{
					glColor3f(Log[logcnt].color[0],  Log[logcnt].color[1],  Log[logcnt].color[2]);
					logcnt++;
				}
			//fprintf(fout,"%d\n",polyNum);
			for (j = 0; j < 3; j++)								// Loop Through Each Vertex  
			{

				TmpNormal.X = vArray[polyData[i].Verts[j]].Nor.X;		// Fill Up The TmpNormal Structure With
				TmpNormal.Y = vArray[polyData[i].Verts[j]].Nor.Y;		// The Current Vertices' Normal Values  
				TmpNormal.Z = vArray[polyData[i].Verts[j]].Nor.Z;
				//if(TmpNormal.Z != 0) fprintf(fout,"%f %f %f\n",TmpNormal.X,TmpNormal.Y,TmpNormal.Z);
				RotateVector (TmpMatrix, TmpNormal, TmpVector);	// Rotate This By The Matrix  
				//fprintf(fout,"%f %f %f\n",TmpVector.X,TmpVector.Y,TmpVector.Z);
				Normalize (TmpVector);							// Normalize The New Normal  

				TmpShade = DotProduct (TmpVector, lightAngle);	// Calculate The Shade Value  
				//fprintf(fout,"%f\n",TmpShade);
				if(GaoGuang)
				{
					if(TmpShade > 0.75f && TmpShade < 1.0f)
						TmpShade = HighLight(TmpShade, 0.75);
					else
						TmpShade = 0;
				}
				if (TmpShade < 0.0f) TmpShade = 0.0f;							// Clamp The Value to 0 If Negative  

				glTexCoord1f (TmpShade);						// Set The Texture Co-ordinate As The Shade Value  
				glVertex3fv (&vArray[polyData[i].Verts[j]].Pos.X);		// Send The Vertex Position  
		    }
		}

    glEnd ();													// Tell OpenGL To Finish Drawing

	glDisable (GL_TEXTURE_1D);									// Disable 1D Textures  

	// Outline Code //
	if (outlineDraw)											// Check To See If We Want To Draw The Outline  
	{
		glEnable (GL_BLEND);									// Enable Blending  
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);		// Set The Blend Mode  

		glPolygonMode (GL_BACK, GL_LINE);						// Draw Backfacing Polygons As Wireframes  
		glLineWidth (outlineWidth);								// Set The Line Width  

		glCullFace (GL_FRONT);									// Don't Draw Any Front-Facing Polygons  

		glDepthFunc (GL_LEQUAL);								// Change The Depth Mode  

		glColor3fv (&outlineColor[0]);							// Set The Outline Color  

		glBegin (GL_TRIANGLES);									// Tell OpenGL What We Want To Draw

			for (i = 0; i < polyNum; i++)						// Loop Through Each Polygon  
			{
				//fprintf(fout,"%d\n",polyNum);
				for (j = 0; j < 3; j++)							// Loop Through Each Vertex  
				{
					glVertex3fv (&vArray[polyData[i].Verts[j]].Pos.X);	// Send The Vertex Position  
				}
			}

		glEnd ();												// Tell OpenGL We've Finished

		glDepthFunc (GL_LESS);									// Reset The Depth-Testing Mode  

		glCullFace (GL_BACK);									// Reset The Face To Be Culled  

		glPolygonMode (GL_BACK, GL_FILL);						// Reset Back-Facing Polygon Drawing Mode  

		glDisable (GL_BLEND);									// Disable Blending  
	}
}