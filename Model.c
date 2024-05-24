#define UNICODE
#define _UNICODE
#define _WIN32_WINNT_WIN10

#include <gsl/gsl_blas.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_vector.h>
#include <math.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <windowsx.h>

#include "Model.h"
#include "MatrixTransformations.h"
#include "Parser.h"
#include "ThreadPool.h"

#include "DoubleBuffering.h"
#include "FpsUtility.h"

#include <jpeglib.h>
#include <png.h>

#define APPLICATION_NAME    _TEXT("3D Model")

#define PTH_OBJ_FILE        "C:\\Users\\Aleksej\\Downloads\\model\\model.obj"
#define PTH_FLOOR_FILE		"C:\\Users\\Aleksej\\Downloads\\model\\floor.obj"
#define PTH_FLOOR_TEXTURE 	"C:\\Users\\Aleksej\\Downloads\\model\\textures\\floor.jpg"
#define PTH_ALBEDO_TEXTURE "C:\\Users\\Aleksej\\Downloads\\model\\textures\\M_Cat_Statue_albedo.jpg"
#define PTH_DIFFUSE_TEXTURE PTH_ALBEDO_TEXTURE

/*An albedo map, also known as a diffuse map, represents the base color or reflectance of an object's surface. It defines the color and appearance of the object under diffuse lighting conditions.*/

#define PTH_NORMALS_TEXTURE "C:\\Users\\Aleksej\\Downloads\\model\\textures\\M_Cat_Statue_normal.png"
/*map of normals*/

#define PTH_METAL_TEXTURE "C:\\Users\\Aleksej\\Downloads\\model\\textures\\M_Cat_Statue_metallic.jpg"

#define PTH_SPECULAR_TEXTURE "C:\\Users\\Aleksej\\Downloads\\model\\textures\\M_Cat_Statue_roughness.jpg" //<!-- not specular actually
#define PTH_ROUGH_TEXTURE PTH_SPECULAR_TEXTURE

#define PTH_AMBIENT_TEXTURE "C:\\Users\\Aleksej\\Downloads\\model\\textures\\M_Cat_Statue_AO.jpg"
#define PTH_AO_TEXTURE PTH_AMBIENT_TEXTURE
/*an ambient map, also known as an ambient occlusion map, is used to simulate the occlusion, or shadowing, of ambient light in a scene. It enhances the perception of depth and realism by darkening areas where objects are close together or where light is blocked by nearby surfaces. */

#define SWAP(a, b)          {typeof(a) temp = a; a = b; b = temp;}
#define COLOR_BCKGRD		RGB(255, 0, 0)
#define COLOR_IMAGE         RGB(255, 255, 255)

#define TIMER_REPAINT_ID 1
#define TIMER_REPAINT_MS 15
#define WHEEL_DELTA_SCALE 0.1
#define MOVEMENT_SPEED 3

/* Perspective projection */
DOUBLE CAMERA_VIEW_WIDTH = 2;
DOUBLE CAMERA_VIEW_HEIGHT = 2;
#define Z_NEAR              1
#define Z_FAR               1000

/* Graphic window */
HWND hwndMainWindow;
RECT rcClient;
WINDOWPLACEMENT g_wpPrev = {sizeof(g_wpPrev)};

/* Back buffer */
HDC hdcBack;
HBITMAP hbmBack;

/* Screen bitmap */
BITMAPINFO bmi;
HBRUSH hbrBackground;
BITMAP bmp;
byte *pBytes;

/* Model info */
RECT rcModelInfo;
TCHAR *tcsFpsInfo;

/* .OBJ file */
FILE *objFile;
ObjFile *pObjFile;

/* Textures */
byte *floorBuffer;
byte *albedoBuffer;
byte *normalsBuffer;

byte *metallicBuffer;
byte *specularBuffer; byte *roughnessBuffer;
byte *ambientBuffer;

/* Model temp vertices */
gsl_vector **gvWorldVertices;
gsl_vector **gvPaintVertices;
gsl_block *gbWorldVertices;
gsl_block *gbPaintVertices;

/* Mouse tracking */
POINT ptMousePrev;
BOOL isActivated;

/* Camera coordinates */
gsl_vector *eye;
gsl_vector *target;
gsl_vector *up;
gsl_vector *xAxis;
gsl_vector *yAxis;
gsl_vector *zAxis;
DOUBLE yOffset = 		0;
DOUBLE destR =          2;
DOUBLE angleThetha =    M_PI_2;
DOUBLE anglePhi =       M_PI_2;

/* Light */
gsl_vector *light;
#define GAMMA 2.2
double exposure = 1.0;

/* Z buffer */
double *zBuffer;
CRITICAL_SECTION *zBufferCS;
byte *pIsDrawable;

/*length of 3 filled with 1*/
gsl_vector* g_light_color;
gsl_vector* g_one;
gsl_vector* g_zero;
gsl_vector* g_F0;

/* Model transformation matrices */
gsl_matrix *matrixTransformation;
gsl_matrix matrixViewPort;
gsl_matrix matrixProjection;
gsl_matrix matrixView;

/* ACES INPUT */
gsl_matrix ACESInputMatMx;
gsl_matrix ACESOutputMatMx;
double ACESInputMat[] =
{
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777
};
double ACESOutputMat[] =
{
     1.60475, -0.53108, -0.07367,
    -0.10208,  1.10813, -0.00605,
    -0.00327, -0.07276,  1.07602
};


/* Just a temp value for single-threaded computing */
gsl_vector *pResult;

/* Keyboard */
byte keys[255];

/* Multithreading */
#define N_QUEUE_MAX_SIZE 10000
#define N_THREADS 16
#define N_THREADS 16
#define N_PARAMS 24
HTHDPOOL hPool;
#define GGX_VEC_SIZE 3
typedef struct {
	HDC dc;
	int from;
	int to;
	gsl_vector *pV0;
	gsl_vector *pV1;
	gsl_vector *pV2;
	gsl_vector *pVs0;
	gsl_vector *pVs1;
	gsl_vector *pVs2;

	gsl_vector *pAT;
	gsl_vector *pBT;
	gsl_vector *pPT;
	gsl_vector *pVT0;
	gsl_vector *pVT1;
	gsl_vector *pVT2;
	gsl_vector *pTangent;
	gsl_vector *pBTangent;
	gsl_vector *pNormal;
	gsl_matrix *pTBN;
	gsl_vector *pE1;
	gsl_vector *pE2;
	gsl_vector *pUV1;
	gsl_vector *pUV2;
	gsl_vector *pResult;

	gsl_vector *pA;
	gsl_vector *pB;
	gsl_vector *pP;
	gsl_vector *pAs;
	gsl_vector *pBs;
	gsl_vector *pPs;
	gsl_vector *norm;
	gsl_vector *pVN0;
	gsl_vector *pVN1;
	gsl_vector *pVN2;
	gsl_vector *pAN;
	gsl_vector *pBN;
	gsl_vector *pPN;
	gsl_vector *L;

	gsl_vector* G;//<!-- PartialGeometry, length<GGX_VEC_SIZE>
	gsl_vector* D;//<!-- Distribution, length<GGX_VEC_SIZE>
	gsl_vector* F;//<!-- FresnelSchlick, length<GGX_VEC_SIZE>
	gsl_vector* h;//<!-- micro normal, length<GGX_VEC_SIZE>
	gsl_vector* tmp1;//<!-- temporary storage, length<GGX_VEC_SIZE>
	gsl_vector* radiance;
	gsl_vector* albedo;
	gsl_vector* F0;
} PARAMS;
PARAMS params[N_PARAMS]; // <- There can be any size you want
HANDLE hTaskExecutedEvent;

PTP_POOL pThreadPool;
TP_CALLBACK_ENVIRON tpCBEnvironment;
PTP_CLEANUP_GROUP tpCUGroup;
volatile LONG count;

/* Arrays for wrapper matrices */
double translViewPort[] =
	{
		R, 0, 0, R,
		0, R, 0, R,
		0, 0, 1, 0,
		0, 0, 0, 1};

double translProjection[] =
	{
		2.0 * Z_NEAR / 2.0, 0, 0, 0,
		0, 2.0 * Z_NEAR / 2.0, 0, 0,
		0, 0, (double)Z_FAR / (Z_NEAR - Z_FAR), (double)Z_NEAR *Z_FAR / (Z_NEAR - Z_FAR),
		0, 0, -1, 0};

double translView[] =
	{
		R, R, R, R,
		R, R, R, R,
		R, R, R, R,
		0, 0, 0, 1};

double shininessVal = 80.0;

	/* Debug variables */
int floorFaceIndex;

inline void vector_cross_product3(gsl_vector *v1, gsl_vector *v2, gsl_vector *result)
{
  result->data[0] = v1->data[1] * v2->data[2] - v1->data[2] * v2->data[1];
  result->data[1] = v1->data[2] * v2->data[0] - v1->data[0] * v2->data[2];
  result->data[2] = v1->data[0] * v2->data[1] - v1->data[1] * v2->data[0];
}

inline int isInside(gsl_vector *pPoint)
{
  return ((pPoint->data[0] > -pPoint->data[3] && pPoint->data[0] < pPoint->data[3]) &&
		  (pPoint->data[1] > -pPoint->data[3] && pPoint->data[1] < pPoint->data[3]) &&
		  (pPoint->data[2] > 0 && pPoint->data[2] < pPoint->data[3]));
}

void LoadJpg(char *textureFileName, byte **colorBuffer)
{
	FILE *textureFile;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	unsigned long buffer_size;

	// Open the JPEG file
	textureFile = fopen(textureFileName, "rb");
	if (!textureFile) {
		fprintf(stderr, "Failed to open image file\n");
		return;
	}

	// Initialize the JPEG decompression object
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	// Specify the source file
	jpeg_stdio_src(&cinfo, textureFile);

	// Read the JPEG header
	(void)jpeg_read_header(&cinfo, TRUE);

	// Start the decompression process
	(void)jpeg_start_decompress(&cinfo);

	// Calculate buffer size
	buffer_size = cinfo.output_width * cinfo.output_height * cinfo.output_components;

	// Allocate memory for the buffer
	*colorBuffer = (byte *)malloc(buffer_size);
	byte *bufarray = *colorBuffer;

	// Read scanlines and copy to buffer
	JSAMPROW row_pointer[1];
	unsigned long offset = 0;
	while (cinfo.output_scanline < cinfo.output_height) {
		row_pointer[0] = &bufarray[offset];
		(void)jpeg_read_scanlines(&cinfo, row_pointer, 1);
		offset += cinfo.output_width * cinfo.output_components;
	}

	// Finish the decompression process
	(void)jpeg_finish_decompress(&cinfo);

	// Clean up the JPEG decompression object
	jpeg_destroy_decompress(&cinfo);

	// Close the file
	fclose(textureFile);
}

void LoadPng(char* filename, byte** buffer) {
	FILE* fp = fopen(filename, "rb");
	int height;
	int width;

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);

	png_init_io(png_ptr, fp);
	png_read_info(png_ptr, info_ptr);

	png_byte color_type = png_get_color_type(png_ptr, info_ptr);
	png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	// Allocate memory for the buffer
	size_t row_bytes = png_get_rowbytes(png_ptr, info_ptr);
	width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	*buffer = (unsigned char*)malloc(width * height * 3);

	png_bytepp row_pointers = (png_bytepp)malloc(width * sizeof(png_bytep));
	for (int y = 0; y < height; y++) {
		row_pointers[y] = (*buffer) + (y * row_bytes);
	}

	png_read_image(png_ptr, row_pointers);

	// Clean up
	fclose(fp);
	free(row_pointers);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
}

void ApplyTransformations()
{
  memcpy(gbPaintVertices->data, gbWorldVertices->data, sizeof(double) * gbPaintVertices->size);

  // View matrix creation
  gsl_vector_memcpy(zAxis, eye);
  gsl_vector_sub(zAxis, target);
  gsl_vector_scale(zAxis, 1.0 / gsl_blas_dnrm2(zAxis));

  vector_cross_product3(up, zAxis, xAxis);
  gsl_vector_scale(xAxis, 1.0 / gsl_blas_dnrm2(xAxis));

  vector_cross_product3(zAxis, xAxis, yAxis);
  gsl_vector_scale(yAxis, 1.0 / gsl_blas_dnrm2(yAxis));

  for (int i = 0; i < 3; i++)
  {
	translView[i] = xAxis->data[i];
	translView[4 + i] = yAxis->data[i];
	translView[8 + i] = zAxis->data[i];
  }

  gsl_blas_ddot(xAxis, eye, translView + 3);
  gsl_blas_ddot(yAxis, eye, translView + 7);
  gsl_blas_ddot(zAxis, eye, translView + 11);
  for (int i = 0; i < 3; i++)
  {
	translView[4 * i + 3] *= -1;
  }
  ///////////////////////////////////////////////

  // Creation global transformation matrix
  // gsl_matrix_memcpy(matrixTransformation, &matrixViewPort);
  gsl_matrix_memcpy(matrixTransformation, &matrixProjection);

  // MatrixMult(matrixTransformation, &matrixProjection);
  MatrixMult(matrixTransformation, &matrixView);
  // Also, there may be multiplication by the tranformation matix
  ////////

  for (int i = 1; i < pObjFile->v->nCurSize; i++)
  {
	gsl_vector *pVector = gvPaintVertices[i];
	double w;
	gsl_blas_dgemv(CblasNoTrans, 1.0, matrixTransformation, pVector, 0, pResult);

	if (pIsDrawable[i] = isInside(pResult))
	{
		gsl_vector_memcpy(pVector, pResult);
		gsl_blas_dgemv(CblasNoTrans, 1.0, &matrixViewPort, pVector, 0, pResult);
		gsl_vector_memcpy(pVector, pResult);
		gsl_vector_scale(pVector, 1.0 / (w = gsl_vector_get(pVector, 3)));
		// pVector->data[3] = w;
	}
  }
}

void InitializeResources()
{
  InitializeMatrixTrans();
  objFile = fopen(PTH_OBJ_FILE, "r");
  pObjFile = parseOBJ(objFile);

  matrixViewPort = gsl_matrix_view_array(translViewPort, 4, 4).matrix;
  matrixProjection = gsl_matrix_view_array(translProjection, 4, 4).matrix;
  matrixView = gsl_matrix_view_array(translView, 4, 4).matrix;
  matrixTransformation = gsl_matrix_alloc(4, 4);

  /*ACES */
  ACESInputMatMx = gsl_matrix_view_array(ACESInputMat, 3, 3).matrix;
  ACESOutputMatMx = gsl_matrix_view_array(ACESOutputMat, 3, 3).matrix;

  pResult = gsl_vector_calloc(4);
  xAxis = gsl_vector_calloc(4);
  yAxis = gsl_vector_calloc(4);
  zAxis = gsl_vector_calloc(4);

  eye = gsl_vector_calloc(4);
  gsl_vector_set(eye, 2, destR);
  gsl_vector_set(eye, 3, 1);

  up = gsl_vector_calloc(4);
  gsl_vector_set_basis(up, 1);
  target = gsl_vector_calloc(4);

	// floor
	FILE *pFloor;
	ObjFile *pObjFloor;
	pFloor = fopen(PTH_FLOOR_FILE, "r");
	pObjFloor = parseOBJ(pFloor);
	floorFaceIndex = pObjFile->fv->nCurSize;

	for (int i = 1; i < pObjFloor->v->nCurSize; i++)
	{
		Add(pObjFile->v, pObjFloor->v->data[i]);
	}
	for (int i = 1; i < pObjFloor->vn->nCurSize; i++)
	{
		Add(pObjFile->vn, pObjFloor->vn->data[i]);
	}
	for (int i = 1; i < pObjFloor->vt->nCurSize; i++)
	{
		Add(pObjFile->vt, pObjFloor->vt->data[i]);
	}
	for (int i = 1; i < 3; i++)
	{
		gsl_vector_int *pVector = gsl_vector_int_calloc(3);
		gsl_vector_int_set(pVector, 0, pObjFile->v->nCurSize - 4);
		gsl_vector_int_set(pVector, 1, pObjFile->v->nCurSize - (4 - i));
		gsl_vector_int_set(pVector, 2, pObjFile->v->nCurSize - (3 - i));

		Add(pObjFile->fv, pVector);
	}
	for (int i = 1; i < 3; i++)
	{
		gsl_vector_int *pVector = gsl_vector_int_calloc(3);
		gsl_vector_int_set_all(pVector, pObjFile->vn->nCurSize - 1);

		Add(pObjFile->fvn, pVector);
	}
	for (int i = 1; i < 3; i++)
	{
		gsl_vector_int *pVector = gsl_vector_int_calloc(3);
		gsl_vector_int_set(pVector, 0, pObjFile->vt->nCurSize - 4);
		gsl_vector_int_set(pVector, 1, pObjFile->vt->nCurSize - (4 - i));
		gsl_vector_int_set(pVector, 2, pObjFile->vt->nCurSize - (3 - i));

		Add(pObjFile->fvt, pVector);
	}
	LoadJpg(PTH_FLOOR_TEXTURE, &floorBuffer);
	TrimObjFileArrays(pObjFile);
	fclose(pFloor);

  gbWorldVertices = gsl_block_alloc(pObjFile->v->nCurSize * 4);
  gbPaintVertices = gsl_block_alloc(pObjFile->v->nCurSize * 4);

  gvWorldVertices = calloc(pObjFile->v->nCurSize, sizeof(gsl_vector *));
  gvPaintVertices = calloc(pObjFile->v->nCurSize, sizeof(gsl_vector *));

  for (int i = 1; i < pObjFile->v->nCurSize; i++)
  {
	gvPaintVertices[i] = gsl_vector_alloc_from_block(gbPaintVertices, i * 4, 4, 1);
	gvWorldVertices[i] = gsl_vector_alloc_from_block(gbWorldVertices, i * 4, 4, 1);
	gsl_vector_memcpy(gvWorldVertices[i], pObjFile->v->data[i]);
  }

  pIsDrawable = calloc(pObjFile->v->nCurSize + 1, sizeof(byte));

	translProjection[0] = 2.0 * Z_NEAR / CAMERA_VIEW_WIDTH;
	translProjection[5] = 2.0 * Z_NEAR / CAMERA_VIEW_HEIGHT;

  ApplyTransformations();

  tcsFpsInfo = calloc(FPS_OUT_MAX_LENGTH, sizeof(TCHAR));

  pThreadPool = CreateThreadpool(NULL);
  SetThreadpoolThreadMaximum(pThreadPool, N_THREADS);
  SetThreadpoolThreadMinimum(pThreadPool, N_THREADS);
  InitializeThreadpoolEnvironment(&tpCBEnvironment);
  tpCUGroup = CreateThreadpoolCleanupGroup();
  SetThreadpoolCallbackCleanupGroup(&tpCBEnvironment, tpCUGroup, NULL);

  light = gsl_vector_calloc(4);

	for (int i = 0; i < N_PARAMS; i++)
	{
		params[i].pV0 = gsl_vector_calloc(4);
		params[i].pV1 = gsl_vector_calloc(4); 
		params[i].pV2 = gsl_vector_calloc(4);
		params[i].pVs0 = gsl_vector_calloc(4);
		params[i].pVs1 = gsl_vector_calloc(4);
		params[i].pVs2 = gsl_vector_calloc(4);
		params[i].pA = gsl_vector_calloc(4);
		params[i].pB = gsl_vector_calloc(4);
		params[i].pP = gsl_vector_calloc(4);
		params[i].pAs = gsl_vector_calloc(4);
		params[i].pBs = gsl_vector_calloc(4);
		params[i].pPs = gsl_vector_calloc(4);
		params[i].norm = gsl_vector_calloc(4);
		params[i].pVN0 = gsl_vector_calloc(4);
		params[i].pVN1 = gsl_vector_calloc(4); 
		params[i].pVN2 = gsl_vector_calloc(4);
		params[i].pAN = gsl_vector_alloc(4);
		params[i].pBN = gsl_vector_alloc(4);
		params[i].pPN = gsl_vector_alloc(4);
		params[i].L = gsl_vector_alloc(4);

		params[i].pAT = gsl_vector_alloc(4);
		params[i].pBT = gsl_vector_alloc(4);
		params[i].pPT = gsl_vector_alloc(4);
		params[i].pVT0 = gsl_vector_alloc(4);
		params[i].pVT1 = gsl_vector_alloc(4);
		params[i].pVT2 = gsl_vector_alloc(4);
		params[i].pTBN = gsl_matrix_calloc(4,4);
		params[i].pTangent = gsl_vector_alloc_row_from_matrix(params[i].pTBN, 0);
		params[i].pBTangent = gsl_vector_alloc_row_from_matrix(params[i].pTBN, 1);
		params[i].pNormal = gsl_vector_alloc_row_from_matrix(params[i].pTBN, 2);
		params[i].pResult = gsl_vector_alloc(4);
		params[i].pE1 = gsl_vector_alloc(4);
		params[i].pE2 = gsl_vector_alloc(4);
		params[i].pUV1 = gsl_vector_alloc(4);
		params[i].pUV2 = gsl_vector_alloc(4);

		params[i].G 		= gsl_vector_alloc(GGX_VEC_SIZE);
		params[i].D 		= gsl_vector_alloc(GGX_VEC_SIZE);
		params[i].F 		= gsl_vector_alloc(GGX_VEC_SIZE);
		params[i].h 		= gsl_vector_alloc(GGX_VEC_SIZE);
		params[i].tmp1 		= gsl_vector_alloc(GGX_VEC_SIZE);
		params[i].radiance 	= gsl_vector_alloc(GGX_VEC_SIZE);
		params[i].albedo 	= gsl_vector_alloc(GGX_VEC_SIZE);
		params[i].F0 		= gsl_vector_alloc(GGX_VEC_SIZE);
	}

	LoadJpg(PTH_ALBEDO_TEXTURE, &albedoBuffer);
	LoadPng(PTH_NORMALS_TEXTURE, &normalsBuffer);
	LoadJpg(PTH_METAL_TEXTURE, &metallicBuffer);
	LoadJpg(PTH_ROUGH_TEXTURE, &roughnessBuffer);
	LoadJpg(PTH_AMBIENT_TEXTURE, &ambientBuffer);
	
	specularBuffer = roughnessBuffer;

	hbrBackground = CreateSolidBrush(COLOR_BCKGRD);

	//#light
	g_light_color = gsl_vector_alloc(3);
	gsl_vector_set_all(g_light_color, 1);

	g_one = gsl_vector_alloc(3);
	gsl_vector_set_all(g_one, 1);

	g_zero = gsl_vector_alloc(3);
	gsl_vector_set_all(g_zero, 0.0);

	g_F0 = gsl_vector_alloc(3);

	gsl_vector_set_all(g_F0, 0.04);
}

void FreeAllResources()
{
  CloseHandle(hTaskExecutedEvent);
  free(tcsFpsInfo);
  free(gvPaintVertices);
  free(gvWorldVertices);

  gsl_block_free(gbPaintVertices);
  gsl_block_free(gbWorldVertices);
  gsl_matrix_free(matrixTransformation);
  gsl_vector_free(light);

  for (int i = 0; i < N_PARAMS; i++)
  {
	gsl_vector_free(params[i].pA);
	gsl_vector_free(params[i].pB);
	gsl_vector_free(params[i].pV0);
	gsl_vector_free(params[i].pV1);
	gsl_vector_free(params[i].pV2);
	gsl_vector_free(params[i].pP);
	gsl_vector_free(params[i].norm);
  }

  free(pResult);
  free(eye);
  free(target);
  free(up);
  free(xAxis);
  free(yAxis);
  free(zAxis);

  free(pBytes);
  free(zBuffer);
  free(pIsDrawable);

  DestroyObjFileInfo(pObjFile);
  fclose(objFile);
  FinalizeMatrixTrans();
  KillTimer(hwndMainWindow, TIMER_REPAINT_ID);

  DeleteObject(hbrBackground);
}

inline void DrawLine(HDC dc, int x0, int y0, int x1, int y1, byte r, byte g, byte b)
{
  BOOL isTranspose = FALSE;

  if (x0 < 0 || x1 < 0 || x0 >= bmp.bmWidth || x1 >= bmp.bmWidth || y0 < 0 || y1 < 0 || y0 >= bmp.bmHeight || y1 >= bmp.bmHeight)
	return;

  if (abs(x1 - x0) < abs(y1 - y0))
  {
	SWAP(x0, y0);
	SWAP(x1, y1);
	isTranspose = TRUE;
  }

  if (x0 > x1)
  {
	SWAP(x0, x1);
	SWAP(y0, y1);
  }

  int dx = x1 - x0;
  int dy = y1 - y0;
  int signY = y1 >= y0 ? 1 : -1;
  int dxDouble = dx << 1;
  int error = 0;
  int offset;
  int bmWidth = bmp.bmWidth;
  int derror = abs(dy) << 1;

  for (int x = x0, y = y0; x <= x1; x++)
  {
	offset = (y * bmWidth + x) << 2;
	if (isTranspose)
	{
	  offset = (x * bmWidth + y) << 2;
	}
	pBytes[offset + 0] = r;
	pBytes[offset + 1] = g;
	pBytes[offset + 2] = b;
	// InterlockedOr((long *)(pBytes + offset), 0x00FFFFFF);

	error += derror;

	if (error > dx)
	{
	  y += signY;
	  error -= dxDouble;
	}
  }
}

double _PartialGeometry(double cosThetaN, double alpha)
{
	double cosTheta_sqr = cosThetaN*cosThetaN;
    double tan2 = ( 1 - cosTheta_sqr ) / cosTheta_sqr;
    double GP = 2 / ( 1 + sqrt( 1 + alpha * alpha * tan2 ) );
    return GP;
}

double _SchlickGGX(double NdotV, double roughness) 
{
	double r = roughness + 1.0;
	double k = (r*r) / 8.0;

	return NdotV / NdotV * (1 - k) + k;
}

double _Distribution(double cosThetaNH, double alpha)
{
 	double alpha2 = alpha * alpha;
    double NH_sqr = cosThetaNH * cosThetaNH;
    double den = NH_sqr * (alpha2 - 1.0) + 1.0;
    return alpha2 / (M_PI * den * den );
}

void RRTAndODTFit(gsl_vector* result, gsl_vector* v)
{
	gsl_vector_set_zero(result);
	gsl_vector_memcpy(result, v);
	gsl_vector_add_constant(result, 0.0245786f);
	for (int i = 0; i < 3; i++) {
		double v_value = gsl_vector_get(v, i);
		gsl_vector_set(result, i, ((v_value * gsl_vector_get(result, i)) - 0.000090537f) / (v_value * (0.983729f * v_value + 0.4329510f) + 0.238081f));
	}
}

/**
 * @brief Fully rewrite @paramref result vector 
 * 
 * @param result allocated with length<3>, noninit, PARAMS.F
 * @param F0 fresnel cooficient
 * @param cosTheta cos of angle between h and l
 */
void _FresnelSchlick(gsl_vector* result, gsl_vector* F0, double cosTheta) {

	gsl_vector_memcpy(result, g_one);
	gsl_vector_sub(result, F0);
	if (cosTheta >= 1.0)
		gsl_vector_scale(result, 0);
	gsl_vector_scale(result, pow(1.0 - cosTheta, 5.0));
	gsl_vector_add(result, F0);
}

void _CookTorrance_GGX(gsl_vector* result, PARAMS *pThreadParams, gsl_vector* n, gsl_vector* l, gsl_vector* v, gsl_vector* F0, 
	gsl_vector* albedo, double roughness, double metal, double ambient) {

	double distance = gsl_blas_dnrm2(l);
  	double attenuation = 1 / (distance * distance);
	gsl_vector* radiance = pThreadParams->radiance;
	gsl_vector_memcpy(radiance, g_light_color);
	gsl_vector_scale(radiance, attenuation);
	
	gsl_vector* h = pThreadParams->h;

	gsl_vector_scale(n, 1.0 / gsl_blas_dnrm2(n));
	gsl_vector_scale(l, 1.0 / gsl_blas_dnrm2(l));//<!-- l = v so normalize v too
	
	gsl_vector_memcpy(h, v);
	//gsl_vector_add(h, l);
	//gsl_vector_scale(h, 1.0 / gsl_blas_dnrm2(h));
	
	//precompute dots
	double NL;
	gsl_blas_ddot(n, l, &NL);
	if (NL <= 0.0) {
		gsl_vector_memcpy(result, g_zero);
		return;
	}

	double NV;
	gsl_blas_ddot(n, v, &NV);
	if (NV <= 0.0) {
		gsl_vector_memcpy(result, g_zero);
		return;
	}

	double NH;
	gsl_blas_ddot(n, h, &NH);
	NH = max(NH, 0.0);

	double HV;
	gsl_blas_ddot(h, v, &HV);
	//HV = max(HV, 0.0);
	HV = 1.0;

 	//precompute roughness square
 	double roug_sqr = pow(roughness, 2);
	// double G = _PartialGeometry(NV, roug_sqr) * _PartialGeometry(NL, roug_sqr);
	double G = _SchlickGGX(NV, roughness) * _SchlickGGX(NL, roughness);
	double D = _Distribution(NH, roug_sqr);
	gsl_vector* F = pThreadParams->F;
	_FresnelSchlick(F, F0, HV);

	gsl_vector_memcpy(result, F);
	gsl_vector_scale(result, G*D*0.25/NV);
	
	gsl_vector* diffK = pThreadParams->D;
	gsl_vector_memcpy(diffK, g_one);
	gsl_vector_sub(diffK, F);

	for (size_t i = 0; i < GGX_VEC_SIZE; i++)
	{
		gsl_vector_set(pThreadParams->G, i, gsl_vector_get(diffK, i) * gsl_vector_get(albedo, i));
	}
	gsl_vector_scale(pThreadParams->G, NL / M_PI);
	gsl_vector_scale(pThreadParams->G, 1.0 - metal);
	gsl_vector_add(result, pThreadParams->G);
	
	for (size_t i = 0; i < GGX_VEC_SIZE; i++)
	{
		gsl_vector_set(result, i, gsl_vector_get(result, i) * gsl_vector_get(radiance, i));
	}

	gsl_vector_scale(albedo, 0.05f * ambient);
	gsl_vector_add(result, albedo);

	for (size_t i = 0; i < 3; i++)
	{
		if (gsl_vector_get(result, i) < 0) {

			gsl_vector_memcpy(result, g_zero);
			return;
		}
	}
}


/**
 * @brief
 *
 * @param pThreadParams
 * @param dc
 * @param pTriangleVertices Sequence numbers of vertexes in vertexes coordinates array from .obj @see gvPaintVertices
 * @param r
 * @param g
 * @param b
 */
inline void DrawTriangle(PARAMS *pThreadParams, HDC dc, gsl_vector_int *pTriangleVertices, byte r, byte g, byte b)
{
  if (!(pIsDrawable[pTriangleVertices->data[0]] && pIsDrawable[pTriangleVertices->data[1]] && pIsDrawable[pTriangleVertices->data[2]]))
	return;

  gsl_vector_memcpy(pThreadParams->pV0, gvPaintVertices[pTriangleVertices->data[0]]);
  gsl_vector_memcpy(pThreadParams->pV1, gvPaintVertices[pTriangleVertices->data[1]]);
  gsl_vector_memcpy(pThreadParams->pV2, gvPaintVertices[pTriangleVertices->data[2]]);

  if (pThreadParams->pV0->data[1] > pThreadParams->pV1->data[1])
	gsl_vector_swap(pThreadParams->pV0, pThreadParams->pV1); //<!-- gsl_vactor_swap exists
  if (pThreadParams->pV0->data[1] > pThreadParams->pV2->data[1])
	gsl_vector_swap(pThreadParams->pV0, pThreadParams->pV2);
  if (pThreadParams->pV1->data[1] > pThreadParams->pV2->data[1])
	gsl_vector_swap(pThreadParams->pV1, pThreadParams->pV2);

  for (int i = 0; i < 2; i++)
  {
	pThreadParams->pV0->data[i] = round(pThreadParams->pV0->data[i]);
	pThreadParams->pV1->data[i] = round(pThreadParams->pV1->data[i]);
	pThreadParams->pV2->data[i] = round(pThreadParams->pV2->data[i]);
  }
  if (abs(pThreadParams->pV0->data[1] - pThreadParams->pV1->data[1]) < 1 && abs(pThreadParams->pV0->data[1] - pThreadParams->pV2->data[1]) < 1)
	return; //<!-- realy need this? it is validation of .obj file actually

  int total_height = pThreadParams->pV2->data[1] - pThreadParams->pV0->data[1];

  for (int i = 0; i < total_height; i++)
  {
	BOOL second_half = i > pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1] || abs(pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1]) < 1;
	int segment_height = (second_half ? (pThreadParams->pV2->data[1] - pThreadParams->pV1->data[1]) : (pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1]));
	double alpha = (double)i / total_height;
	double beta = (double)(i - (second_half ? pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1] : 0)) / segment_height;

	gsl_vector_memcpy(pThreadParams->pA, pThreadParams->pV2);
	gsl_vector_sub(pThreadParams->pA, pThreadParams->pV0);
	gsl_vector_scale(pThreadParams->pA, alpha);
	gsl_vector_add(pThreadParams->pA, pThreadParams->pV0);

	if (second_half)
	{
	  gsl_vector_memcpy(pThreadParams->pB, pThreadParams->pV2);
	  gsl_vector_sub(pThreadParams->pB, pThreadParams->pV1);
	  gsl_vector_scale(pThreadParams->pB, beta);
	  gsl_vector_add(pThreadParams->pB, pThreadParams->pV1);
	}
	else
	{
	  gsl_vector_memcpy(pThreadParams->pB, pThreadParams->pV1);
	  gsl_vector_sub(pThreadParams->pB, pThreadParams->pV0);
	  gsl_vector_scale(pThreadParams->pB, beta);
	  gsl_vector_add(pThreadParams->pB, pThreadParams->pV0);
	}

	pThreadParams->pA->data[0] = round(pThreadParams->pA->data[0]);
	pThreadParams->pA->data[1] = round(pThreadParams->pA->data[1]);
	pThreadParams->pB->data[0] = round(pThreadParams->pB->data[0]);
	pThreadParams->pB->data[1] = round(pThreadParams->pB->data[1]);

	int offset;
	if (pThreadParams->pA->data[0] > pThreadParams->pB->data[0])
	  gsl_vector_swap(pThreadParams->pA, pThreadParams->pB);
	// pThreadParams->pA->data[0] += 1;
	for (int j = pThreadParams->pA->data[0] + 1; j <= pThreadParams->pB->data[0]; j++)
	{
	  if (j < 0 || j >= bmp.bmWidth || ((int)pThreadParams->pV0->data[1] + i) < 0 || ((int)pThreadParams->pV0->data[1] + i) >= bmp.bmHeight)
		continue;
	  double phi = abs(pThreadParams->pB->data[0] - pThreadParams->pA->data[0]) < 1 ? 1.0 : (double)(j - pThreadParams->pA->data[0]) / (double)(pThreadParams->pB->data[0] - pThreadParams->pA->data[0] + 1);
	  gsl_vector_memcpy(pThreadParams->pP, pThreadParams->pB);
	  gsl_vector_sub(pThreadParams->pP, pThreadParams->pA);
	  gsl_vector_scale(pThreadParams->pP, phi);
	  gsl_vector_add(pThreadParams->pP, pThreadParams->pA);

	  offset = (((int)pThreadParams->pV0->data[1] + i) * bmp.bmWidth + j) << 2;
	  int idx = j + (pThreadParams->pV0->data[1] + i) * bmp.bmWidth;

	  EnterCriticalSection(zBufferCS + idx);
	  if (zBuffer[idx] > pThreadParams->pP->data[2])
	  {
		zBuffer[idx] = pThreadParams->pP->data[2];

		pBytes[offset + 0] = r;
		pBytes[offset + 1] = g;
		pBytes[offset + 2] = b;
	  }
	  LeaveCriticalSection(zBufferCS + idx);
	}
  }
}

/**
 * @brief Interpolate third vector between two which align between on
 *
 * @param firstVector
 * @param secondVector
 * @param rotationPercent
 * @param interpolatedVector
 */
void interpolateVector(gsl_vector *firstVector, gsl_vector *secondVector, double rotationPercent, gsl_vector *interpolatedVector, BOOL isNormalized)
{
  gsl_vector_memcpy(interpolatedVector, secondVector);

  gsl_vector_sub(interpolatedVector, firstVector);
  gsl_vector_scale(interpolatedVector, rotationPercent);
  gsl_vector_add(interpolatedVector, firstVector);

  if (isNormalized)
	gsl_vector_scale(interpolatedVector, 1.0 / gsl_blas_dnrm2(interpolatedVector));
}

void interpolateTexture(PARAMS *pThreadParams, gsl_vector* firstVector, gsl_vector* secondVector, double rotationPercent, gsl_vector* interpolatedVector, int number)
{
	double z0, z1;
	switch (number)
	{
		case 1:
			z0 = pThreadParams->pV0->data[3];
			z1 = pThreadParams->pV1->data[3];
			break;
		case 2: 
			z0 = pThreadParams->pV0->data[3];
			z1 = pThreadParams->pV2->data[3];
			break;
		case 12:
			z0 = pThreadParams->pV1->data[3];
			z1 = pThreadParams->pV2->data[3];
			break;
		case 22:
			z0 = pThreadParams->pA->data[2];
			z1 = pThreadParams->pB->data[2];
			break;
	}

	gsl_vector_memcpy(interpolatedVector, secondVector);
	gsl_vector_memcpy(pThreadParams->pResult, firstVector);
	gsl_vector_scale(interpolatedVector, 1.0 / z1);
	gsl_vector_scale(pThreadParams->pResult, 1.0 / z0);

	gsl_vector_sub(interpolatedVector, pThreadParams->pResult);
	gsl_vector_scale(interpolatedVector, rotationPercent);
	gsl_vector_add(interpolatedVector, pThreadParams->pResult);
	gsl_vector_scale(interpolatedVector, 1.0 / ((1.0 - rotationPercent) / z0 + rotationPercent / z1));
}

void calculateSideScanlinePoints(PARAMS *p, double alpha, BOOL second_half, double beta) {

	interpolateVector(p->pV0, p->pV2, alpha, p->pA, FALSE);
	interpolateVector(p->pVs0, p->pVs2, alpha, p->pAs, FALSE);
	interpolateTexture(p, p->pVT0, p->pVT2, alpha, p->pAT, 2);
	interpolateVector(p->pVN0, p->pVN2, alpha, p->pAN, TRUE);

	if (second_half)
	{
		interpolateVector(p->pV1, p->pV2, beta, p->pB, FALSE);
		interpolateVector(p->pVs1, p->pVs2, beta, p->pBs, FALSE);
		interpolateTexture(p, p->pVT1, p->pVT2, beta, p->pBT, 12);
		interpolateVector(p->pVN1, p->pVN2, beta, p->pBN, TRUE);
	}
	else
	{
		interpolateVector(p->pV0, p->pV1, beta, p->pB, FALSE);
		interpolateVector(p->pVs0, p->pVs1, beta, p->pBs, FALSE);
		interpolateTexture(p, p->pVT0, p->pVT1, beta, p->pBT, 1);
		interpolateVector(p->pVN0, p->pVN1, beta, p->pBN, TRUE);
	}
}

void DrawTriangleIl(PARAMS *pThreadParams, int index)
{
	gsl_vector_int *pTriangleVertices = pObjFile->fv->data[index];
	gsl_vector_int *pnTriangleVertices = pObjFile->fvn->data[index];
	gsl_vector_int *ptTriangleVertices = pObjFile->fvt->data[index];

	if (!(pIsDrawable[pTriangleVertices->data[0]] && pIsDrawable[pTriangleVertices->data[1]] && pIsDrawable[pTriangleVertices->data[2]])) return;

	gsl_vector_memcpy(pThreadParams->pVN0, pObjFile->vn->data[pnTriangleVertices->data[0]]);
	gsl_vector_memcpy(pThreadParams->pVN1, pObjFile->vn->data[pnTriangleVertices->data[1]]); 
	gsl_vector_memcpy(pThreadParams->pVN2, pObjFile->vn->data[pnTriangleVertices->data[2]]); 

	gsl_vector_memcpy(pThreadParams->pVs0, pObjFile->v->data[pTriangleVertices->data[0]]); 
	gsl_vector_memcpy(pThreadParams->pVs1, pObjFile->v->data[pTriangleVertices->data[1]]); 
	gsl_vector_memcpy(pThreadParams->pVs2, pObjFile->v->data[pTriangleVertices->data[2]]); 
	
	gsl_vector_memcpy(pThreadParams->pVT0, pObjFile->vt->data[ptTriangleVertices->data[0]]); 
	gsl_vector_memcpy(pThreadParams->pVT1, pObjFile->vt->data[ptTriangleVertices->data[1]]); 
	gsl_vector_memcpy(pThreadParams->pVT2, pObjFile->vt->data[ptTriangleVertices->data[2]]); 

	gsl_vector_memcpy(pThreadParams->pV0, gvPaintVertices[pTriangleVertices->data[0]]);
	gsl_vector_memcpy(pThreadParams->pV1, gvPaintVertices[pTriangleVertices->data[1]]);
	gsl_vector_memcpy(pThreadParams->pV2, gvPaintVertices[pTriangleVertices->data[2]]);


	gsl_vector_memcpy(pThreadParams->pE1, pThreadParams->pVs1);
	gsl_vector_sub(pThreadParams->pE1, pThreadParams->pVs0);
	gsl_vector_memcpy(pThreadParams->pE2, pThreadParams->pVs2);
	gsl_vector_sub(pThreadParams->pE2, pThreadParams->pVs0);
	gsl_vector_memcpy(pThreadParams->pUV1, pThreadParams->pVT1);
	gsl_vector_sub(pThreadParams->pUV1, pThreadParams->pVT0);
	gsl_vector_memcpy(pThreadParams->pUV2, pThreadParams->pVT2);
	gsl_vector_sub(pThreadParams->pUV2, pThreadParams->pVT0);

	vector_cross_product3(pThreadParams->pE1, pThreadParams->pE2, pThreadParams->pNormal);
	gsl_vector_scale(pThreadParams->pNormal, 1.0 / gsl_blas_dnrm2(pThreadParams->pNormal));

	double f = 1.0 / (pThreadParams->pUV1->data[0] * pThreadParams->pUV2->data[1] - pThreadParams->pUV2->data[0] * pThreadParams->pUV1->data[1]);
	pThreadParams->pTangent->data[0] = f * (pThreadParams->pUV2->data[1] * pThreadParams->pE1->data[0] - pThreadParams->pUV1->data[1] * pThreadParams->pE2->data[0]);
	pThreadParams->pTangent->data[1] = f * (pThreadParams->pUV2->data[1] * pThreadParams->pE1->data[1] - pThreadParams->pUV1->data[1] * pThreadParams->pE2->data[1]);
	pThreadParams->pTangent->data[2] = f * (pThreadParams->pUV2->data[1] * pThreadParams->pE1->data[2] - pThreadParams->pUV1->data[1] * pThreadParams->pE2->data[2]);
	gsl_vector_scale(pThreadParams->pTangent, 1.0 / gsl_blas_dnrm2(pThreadParams->pTangent));

	if (pThreadParams->pV0->data[1] > pThreadParams->pV1->data[1]) {
		gsl_vector_swap(pThreadParams->pV0, pThreadParams->pV1); 
		gsl_vector_swap(pThreadParams->pVN0, pThreadParams->pVN1);
		gsl_vector_swap(pThreadParams->pVs0, pThreadParams->pVs1);
		gsl_vector_swap(pThreadParams->pVT0, pThreadParams->pVT1);
	}
	if (pThreadParams->pV0->data[1] > pThreadParams->pV2->data[1]) {
	  gsl_vector_swap(pThreadParams->pV0, pThreadParams->pV2);
	  gsl_vector_swap(pThreadParams->pVN0, pThreadParams->pVN2);
	  gsl_vector_swap(pThreadParams->pVs0, pThreadParams->pVs2);
	  gsl_vector_swap(pThreadParams->pVT0, pThreadParams->pVT2);
	}
	if (pThreadParams->pV1->data[1] > pThreadParams->pV2->data[1]) {
	  gsl_vector_swap(pThreadParams->pV1, pThreadParams->pV2);
	  gsl_vector_swap(pThreadParams->pVN1, pThreadParams->pVN2);
	  gsl_vector_swap(pThreadParams->pVs1, pThreadParams->pVs2);
	  gsl_vector_swap(pThreadParams->pVT1, pThreadParams->pVT2);
	}

	for (int i = 0; i < 2; i++)
	{
		pThreadParams->pV0->data[i] = round(pThreadParams->pV0->data[i]);
		pThreadParams->pV1->data[i] = round(pThreadParams->pV1->data[i]);
		pThreadParams->pV2->data[i] = round(pThreadParams->pV2->data[i]);
	}

	if (abs(pThreadParams->pV0->data[1] - pThreadParams->pV1->data[1]) < 1 && abs(pThreadParams->pV0->data[1] - pThreadParams->pV2->data[1]) < 1) return; //<!-- realy need this? it is validation of .obj file actually

	int total_height = pThreadParams->pV2->data[1] - pThreadParams->pV0->data[1];

	vector_cross_product3(pThreadParams->pNormal, pThreadParams->pTangent, pThreadParams->pBTangent);
	gsl_vector_scale(pThreadParams->pBTangent, 1.0 / gsl_blas_dnrm2(pThreadParams->pBTangent)); 
	
	for (int i=0; i<total_height; i++) {
		BOOL second_half = i > pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1] || abs(pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1]) < 1;
		int segment_height = (second_half ? (pThreadParams->pV2->data[1] - pThreadParams->pV1->data[1]) : (pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1]));
		double alpha = (double) i / total_height;
		double beta  = (double) (i - (second_half ? pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1] : 0)) / segment_height;
		
		calculateSideScanlinePoints(pThreadParams, alpha, second_half, beta);


		for (int i = 0; i < 2; i++)
		{
		pThreadParams->pA->data[i] = round(pThreadParams->pA->data[i]);
		pThreadParams->pB->data[i] = round(pThreadParams->pB->data[i]);
		}

		if (pThreadParams->pA->data[0] > pThreadParams->pB->data[0]) {
		   gsl_vector_swap(pThreadParams->pA, pThreadParams->pB);
		   gsl_vector_swap(pThreadParams->pAs, pThreadParams->pBs);
		   gsl_vector_swap(pThreadParams->pAT, pThreadParams->pBT);
		   gsl_vector_swap(pThreadParams->pAN, pThreadParams->pBN);
		}

		int offset;

		for (int j = pThreadParams->pA->data[0] + 1; j <= pThreadParams->pB->data[0]; j++)
		{

			if (j < 0 || j >= bmp.bmWidth || ((int)pThreadParams->pV0->data[1] + i) < 0 || ((int)pThreadParams->pV0->data[1] + i) >= bmp.bmHeight)
			continue;

			double phi = abs(pThreadParams->pB->data[0] - pThreadParams->pA->data[0]) < 1 ? 1.0 : (double)(j - pThreadParams->pA->data[0]) / (double)(pThreadParams->pB->data[0] - pThreadParams->pA->data[0] + 1);

			interpolateVector(pThreadParams->pA, pThreadParams->pB, phi, pThreadParams->pP, FALSE);
			interpolateVector(pThreadParams->pAs, pThreadParams->pBs, phi, pThreadParams->pPs, FALSE);
			interpolateTexture(pThreadParams, pThreadParams->pAT, pThreadParams->pBT, phi, pThreadParams->pPT, 22);
			interpolateVector(pThreadParams->pAN, pThreadParams->pBN, phi, pThreadParams->pPN, TRUE);

			offset = (((int) pThreadParams->pV0->data[1] + i) * bmp.bmWidth + j) << 2;
			int idx = j+(pThreadParams->pV0->data[1] + i)*bmp.bmWidth;

			EnterCriticalSection(zBufferCS + idx);
			if (zBuffer[idx] > pThreadParams->pP->data[2]) {
				zBuffer[idx] = pThreadParams->pP->data[2];

				int globalTextureIDX = (4096 * (int)(4096 - 4096 * pThreadParams->pPT->data[1]) + (int)(pThreadParams->pPT->data[0] * 4096));
				int normalIDX = globalTextureIDX * 3;
				int colorIDX = globalTextureIDX * 3;
				double lambertian = 0.0;
				double shininessVal = 20;
				double specular = 0.0;
				double temp = 0;
				
				if (index < floorFaceIndex)
				{
					gsl_vector_memcpy(pThreadParams->pNormal, pThreadParams->pPN);
					gsl_vector_memcpy(pThreadParams->pResult, pThreadParams->pTangent);
					gsl_blas_ddot(pThreadParams->pResult, pThreadParams->pNormal, &temp);
					gsl_vector_scale(pThreadParams->pPN, temp);
					gsl_vector_sub(pThreadParams->pResult, pThreadParams->pPN);
					gsl_vector_scale(pThreadParams->pTangent, 1.0 / gsl_blas_dnrm2(pThreadParams->pTangent));
					vector_cross_product3(pThreadParams->pNormal, pThreadParams->pTangent, pThreadParams->pBTangent);
					gsl_vector_scale(pThreadParams->pBTangent, 1.0 / gsl_blas_dnrm2(pThreadParams->pBTangent)); 
					
					gsl_vector_set(pThreadParams->pPN, 0, (double) normalsBuffer[normalIDX + 0]/255.0);
					gsl_vector_set(pThreadParams->pPN, 1, (double) normalsBuffer[normalIDX + 1]/255.0);
					gsl_vector_set(pThreadParams->pPN, 2, (double) normalsBuffer[normalIDX + 2]/255.0);

					gsl_vector_scale(pThreadParams->pPN, 2.0);
					gsl_vector_add_constant(pThreadParams->pPN, -1.0);
					gsl_vector_scale(pThreadParams->pPN, 1.0 / gsl_blas_dnrm2(pThreadParams->pPN));

					gsl_vector* L = pThreadParams->L;
					gsl_vector_memcpy(L, eye);

					gsl_blas_dgemv(CblasTrans, 1.0, pThreadParams->pTBN, pThreadParams->pPN, 0, pThreadParams->pResult);
					gsl_vector_memcpy(pThreadParams->pPN, pThreadParams->pResult);
					gsl_vector_set(pThreadParams->pPN, 3, 0);

					gsl_vector_sub(L, pThreadParams->pPs);

					gsl_vector_view n = gsl_vector_subvector(pThreadParams->pPN, 0, 3);
					gsl_vector_view l = gsl_vector_subvector(L, 0, 3);
					gsl_vector* col = pThreadParams->tmp1;
					gsl_vector* albedo = pThreadParams->albedo;
					
					gsl_vector_set(pThreadParams->albedo, 0, (double)albedoBuffer[normalIDX + 0]/255.0);
					gsl_vector_set(pThreadParams->albedo, 1, (double)albedoBuffer[normalIDX + 1]/255.0);
					gsl_vector_set(pThreadParams->albedo, 2, (double)albedoBuffer[normalIDX + 2]/255.0);

					gsl_vector_memcpy(pThreadParams->F0, albedo);
					gsl_vector_sub(pThreadParams->F0, g_F0);
					gsl_vector_scale(pThreadParams->F0, metallicBuffer[colorIDX] / 255.0);
					gsl_vector_add(pThreadParams->F0, g_F0);
					
					for (size_t i = 0; i < 3; i++)
					{
						gsl_vector_set(albedo, i, pow(albedoBuffer[colorIDX + i] / 255.0, GAMMA));
					}

					//#tag
					_CookTorrance_GGX(col, pThreadParams, &n.vector, &l.vector, &l.vector, pThreadParams->F0, albedo, 
														roughnessBuffer[colorIDX] / 255.0, metallicBuffer[colorIDX] / 255.0, pow(ambientBuffer[colorIDX] / 255.0, GAMMA));
					
					// double c1 = 1.0 - exp(-gsl_vector_get(col, 0) * exposure);
					// double c2 = 1.0 - exp(-gsl_vector_get(col, 1) * exposure);
					// double c3 = 1.0 - exp(-gsl_vector_get(col, 2) * exposure);		
					gsl_blas_dgemv(CblasNoTrans, 1.0, &ACESInputMatMx, col, 0, pThreadParams->D);
					RRTAndODTFit(pThreadParams->G, pThreadParams->D);
					gsl_blas_dgemv(CblasNoTrans, 1.0, &ACESOutputMatMx, pThreadParams->G, 0, col);
					double c1 = 1.0 - exp(-gsl_vector_get(col, 0) * exposure);
					double c2 = 1.0 - exp(-gsl_vector_get(col, 1) * exposure);
					double c3 = 1.0 - exp(-gsl_vector_get(col, 2) * exposure);

					//#color with gamma correction
					pBytes[offset + 0] = min(pow(c3, 1 / GAMMA) * 255, 255);
					pBytes[offset + 1] = min(pow(c2, 1 / GAMMA) * 255, 255); 
					pBytes[offset + 2] = min(pow(c1, 1 / GAMMA) * 255, 255);  
				} 
			}
			LeaveCriticalSection(zBufferCS+idx);
		}
	}
}

void CALLBACK task(PTP_CALLBACK_INSTANCE Instance, PVOID params, PTP_WORK Work)
{
  PARAMS *param = (PARAMS *)params;
  for (int i = param->from; i < param->to; i++)
  {
	// if (!PrepareTriangleFillData(param, i))
	  DrawTriangleIl(param, i);
  }

  InterlockedAdd(&count, 1);
}

void SetFullScreen(HWND hwnd)
{
  DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);

  if (dwStyle & WS_OVERLAPPEDWINDOW)
  {
	MONITORINFO mi = {sizeof(mi)};

	if (GetWindowPlacement(hwnd, &g_wpPrev) && GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi))
	{
	  SetWindowLong(hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
	  SetWindowPos(hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
				   SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
  }
  else
  {
	SetWindowLong(hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
	SetWindowPlacement(hwnd, &g_wpPrev);
	SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
				 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
					 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  }
}

void DrawProc(HDC hdc)
{
  SaveDC(hdc);

  FillRect(hdc, &rcClient, hbrBackground);

	GetDIBits(hdc, hbmBack, 0, bmp.bmHeight, pBytes, &bmi, DIB_RGB_COLORS);
	
	for (int i = 0; i < bmp.bmWidth * bmp.bmHeight; i++)
	{
		zBuffer[i] = 1000;
	}
	gsl_vector_memcpy(light, target);
	gsl_vector_sub(light, eye);
	gsl_vector_scale(light, 1.0 / gsl_blas_dnrm2(light));

  PTP_WORK work;
  int delta = pObjFile->fv->nCurSize / N_PARAMS;
  params[0].from = 1;
  params[0].to = delta;
  params[0].dc = hdc;

  // SubmitTask(hPool, task, params);
  work = CreateThreadpoolWork(task, params, &tpCBEnvironment);
  SubmitThreadpoolWork(work);
  for (int i = 1; i < N_PARAMS - 1; i++)
  {
	params[i].from = params[i - 1].to;
	params[i].to = params[i].from + delta;
	params[i].dc = hdc;
	work = CreateThreadpoolWork(task, params + i, &tpCBEnvironment);
	SubmitThreadpoolWork(work);
	// SubmitTask(hPool, task, params + i);
  }
  params[N_PARAMS - 1].from = params[N_PARAMS - 2].to;
  params[N_PARAMS - 1].to = pObjFile->fv->nCurSize;
  params[N_PARAMS - 1].dc = hdc;
  work = CreateThreadpoolWork(task, params + N_PARAMS - 1, &tpCBEnvironment);
  SubmitThreadpoolWork(work);
  // SubmitTask(hPool, task, params + N_PARAMS - 1);

  // waiting for all threads finish drawing
  while (count < N_PARAMS)
  {
  }
  count = 0;
  CloseThreadpoolCleanupGroupMembers(tpCUGroup,
									 FALSE,
									 &tpCBEnvironment);

  SetDIBits(hdc, hbmBack, 0, bmp.bmHeight, pBytes, &bmi, DIB_RGB_COLORS);

  // FPS counter //////////////////////////////////////
  SetTextColor(hdc, COLOR_IMAGE);
  SetBkColor(hdc, TRANSPARENT);
  DrawText(hdc, tcsFpsInfo, -1, &rcModelInfo, DT_CENTER);
  /////////////////////////////////////////////////////

  RestoreDC(hdc, -1);
}

void MoveProc()
{
	static long lastTime = 0;
	double cameraSpeed = (GetTickCount() - lastTime) / 1000.0 * MOVEMENT_SPEED;

	if (keys[0x57])
	{
		gsl_vector_memcpy(pResult, target);
		gsl_vector_sub(pResult, eye);
		gsl_vector_scale(pResult, 1 / gsl_blas_dnrm2(pResult) * cameraSpeed);
		gsl_vector_add(eye, pResult);
	}

	if (keys[0x53])
	{
		gsl_vector_memcpy(pResult, target);
		gsl_vector_sub(pResult, eye);
		gsl_vector_scale(pResult, 1 / gsl_blas_dnrm2(pResult) * cameraSpeed);
		gsl_vector_sub(eye, pResult);
	}

	if (keys[0x41])
	{
		gsl_vector_memcpy(pResult, target);
		gsl_vector_sub(pResult, eye);
		vector_cross_product3(pResult, up, xAxis);
		gsl_vector_scale(xAxis, 1.0 / gsl_blas_dnrm2(xAxis) * (-cameraSpeed));
		gsl_vector_add(eye, xAxis);
		gsl_vector_add(target, xAxis);
	}

	if (keys[0x51]) 
	{
		exposure -= 0.1;
		exposure = max(exposure, 0.0);
	}
	if (keys[0x45]) 
	{
		exposure += 0.1;
		exposure = min(exposure, 10.0);
	}

	if (keys[0x44])
	{
		gsl_vector_memcpy(pResult, target);
		gsl_vector_sub(pResult, eye);
		vector_cross_product3(pResult, up, xAxis);
		gsl_vector_scale(xAxis, 1.0 / gsl_blas_dnrm2(xAxis) * (cameraSpeed));
		gsl_vector_add(eye, xAxis);
		gsl_vector_add(target, xAxis);
	}

	if (keys[VK_UP])
	{
		gsl_vector_set_basis(pResult, 1);
		gsl_vector_scale(pResult, cameraSpeed);
		gsl_vector_add(target, pResult);
	}

	if (keys[VK_DOWN])
	{
		gsl_vector_set_basis(pResult, 1);
		gsl_vector_scale(pResult, -cameraSpeed);
		gsl_vector_add(target, pResult);
	}

  lastTime = GetTickCount();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  /* Section for window messages */
  case WM_CREATE:
	InitializeResources();
	SetTimer(hWnd, TIMER_REPAINT_ID, TIMER_REPAINT_MS, NULL);
	break;
  case WM_TIMER:
	MoveProc();
	ApplyTransformations();
	// InvalidateRect(hWnd, NULL, FALSE);
	break;
  case WM_PAINT:
	HDC dc;
	PAINTSTRUCT ps;

	dc = BeginPaint(hWnd, &ps);

	DrawProc(hdcBack);

	BitBlt(dc, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, hdcBack, 0, 0, SRCCOPY);

	EndPaint(hWnd, &ps);

	fps.frames++;
	break;
  case WM_SIZE:
	FinalizeBuffer(&hdcBack, &hbmBack);
	InitializeBuffer(hWnd, &hdcBack, &hbmBack, &rcClient);

	// for fps label
	rcModelInfo.left = rcClient.left;
	rcModelInfo.top = rcClient.bottom - 30;
	rcModelInfo.bottom = rcClient.bottom;
	rcModelInfo.right = rcClient.left + 80;

	// reinitializing BITMAPINFO structure
	GetObject(hbmBack, sizeof(BITMAP), &bmp);

	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	GetDIBits(hdcBack, hbmBack, 0, bmp.bmHeight, NULL, &bmi, DIB_RGB_COLORS);
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biHeight = -bmp.bmHeight;

	double scaleX = bmp.bmWidth / 2.0;
	double scaleY = bmp.bmHeight / 2.0;

	translViewPort[0] = scaleY;
	translViewPort[3] = scaleX;
	translViewPort[5] = -scaleY;
	translViewPort[7] = scaleY;

	pBytes = realloc(pBytes, bmi.bmiHeader.biSizeImage);
	zBuffer = realloc(zBuffer, bmp.bmHeight * bmp.bmWidth * sizeof(double));
	zBufferCS = realloc(zBufferCS, bmp.bmHeight * bmp.bmWidth * sizeof(CRITICAL_SECTION));
	for (int i = 0; i < bmp.bmHeight * bmp.bmWidth; i++)
	{
	  InitializeCriticalSectionAndSpinCount(zBufferCS + i, 2000);
	}
	break;
  case WM_LBUTTONDOWN:
	ptMousePrev.x = GET_X_LPARAM(lParam);
	ptMousePrev.y = GET_Y_LPARAM(lParam);
	isActivated = TRUE;
	break;
  case WM_LBUTTONUP:
	isActivated = FALSE;
	break;
  case WM_MOUSEMOVE:
	if (isActivated)
	{
	  int xMousePos = GET_X_LPARAM(lParam);
	  int yMousePos = GET_Y_LPARAM(lParam);

	  angleThetha -= (yMousePos - ptMousePrev.y) / 720.0 * 2 * M_PI;
	  anglePhi += (xMousePos - ptMousePrev.x) / 1280.0 * 2 * M_PI;

	  gsl_vector_set_zero(target);
	  eye->data[0] = destR * sin(angleThetha) * cos(anglePhi);
	  eye->data[1] = destR * cos(angleThetha);
	  eye->data[2] = destR * sin(angleThetha) * sin(anglePhi);

	  ptMousePrev.x = xMousePos;
	  ptMousePrev.y = yMousePos;
	}
	break;
  case WM_MOUSEWHEEL:
	byte isScalePositive = GET_WHEEL_DELTA_WPARAM(wParam) < 0;

	CAMERA_VIEW_HEIGHT += isScalePositive ? WHEEL_DELTA_SCALE : -WHEEL_DELTA_SCALE;
	CAMERA_VIEW_WIDTH += isScalePositive ? WHEEL_DELTA_SCALE : -WHEEL_DELTA_SCALE;

	translProjection[0] = 2.0 * Z_NEAR / CAMERA_VIEW_WIDTH;
	translProjection[5] = 2.0 * Z_NEAR / CAMERA_VIEW_HEIGHT;

	break;
  case WM_KEYDOWN:
	keys[wParam] = 1;
	break;
  case WM_KEYUP:
	keys[wParam] = 0;
	break;
  case WM_SYSKEYDOWN:
	if (wParam == VK_F12 && (lParam & (1 << 29)))
	{
	  SetFullScreen(hWnd);
	}
	break;
  case WM_DESTROY:
	ExitProcess(0); // need some fix
	FreeAllResources();
	FinalizeBuffer(&hdcBack, &hbmBack);
	PostQuitMessage(0);
	break;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI ModelStart(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  WNDCLASSEX wcex;
  MSG msg;

  memset(&wcex, 0, sizeof(wcex));
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.lpfnWndProc = WndProc;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszClassName = _TEXT("AppClass");
  wcex.hIconSm = wcex.hIcon;

  RegisterClassExW(&wcex);
  hwndMainWindow = CreateWindow(_TEXT("AppClass"), APPLICATION_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 800, 600, NULL, NULL, hInstance, NULL);

  ShowWindow(hwndMainWindow, nShowCmd);
  UpdateWindow(hwndMainWindow);

  fps.out = tcsFpsInfo;
  fps.lastTime = GetTickCount();

  for (;;)
  {
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
	  DispatchMessage(&msg);
	}
	fpsLOG();
	InvalidateRect(hwndMainWindow, NULL, FALSE);
  }

  return (int)msg.wParam;
}