//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <stdbool.h>
#define VC_DEBUG
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


typedef struct {
	unsigned char* data;    // Guarda os valores dos pixeis da imagem
	int width, height;		// Resolução da imagem
	int channels;			// Binário/Cinzentos=1; RGB=3
	int levels;				// Binário=1; Cinzentos [1,255]; RGB [1,255]
	int bytesperline;		// width * channels
} IVC;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROTÓTIPOS DE FUNÇÕES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
IVC* vc_image_new(int width, int height, int channels, int levels);
IVC* vc_image_free(IVC* image);

// FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC* vc_read_image(char* filename);
int vc_write_image(char* filename, IVC* image);

// FUNÇÕES: ESPAÇOS DE COR
int vc_gray_negative(IVC* srcdst);
int vc_rgb_negative(IVC* srcdst);

int vc_rgb_to_gray(IVC* src, IVC* dst);
int vc_rgb_to_hsv(IVC* src, IVC* dst);

int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax);
int vc_count_white_pixels(IVC* src);

int vc_scale_gray_to_color_palette(IVC* src, IVC* dst);

int vc_exercicio1(IVC* imageArea, IVC* imageBlue, IVC* imageGreen, IVC* imageYellow, IVC* imageRed1, IVC* imageRed2);

int vc_gray_to_binary(IVC* src, IVC* dst, int threshold);
int vc_calc_threshold(IVC* src);
int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel);
int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int kernel, float k);

int vc_binary_dilate(IVC* src, IVC* dst, int kernel);
int vc_binary_erode(IVC* src, IVC* dst, int kernel);

int vc_open_binary(IVC* src, IVC* dst, int kernel);
int vc_close_binary(IVC* src, IVC* dst, int kernel);

// Etiquetagem
//int vc_binary_blob_labellingah(IVC* src, IVC* dst);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UM BLOB (OBJECTO)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
	int x, y, width, height;	// Caixa Delimitadora (Bounding Box)
	int area;					// Área
	int xc, yc;					// Centro-de-massa
	int perimeter;				// Perímetro
	int label;					// Etiqueta
} OVC;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROTÓTIPOS DE FUNÇÕES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels);
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);

// Histograma
int vc_gray_histogram_show(IVC* src, IVC* dst);
int vc_gray_histogram_equalization(IVC* src, IVC* dst);

int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th);
int vc_gray_edge_sobel(IVC* src, IVC* dst, float th);

int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernelsize);

// Cabecalhos VC-TP - Criados
#define MAX_TRACKED 100

typedef struct {
	int x, y;
	int width, height;
	int frames_detected;
	bool active;
	bool counted;
} BlobTrack;

void vc_bgr_to_rgb(const unsigned char* frame_data, int frame_step, int width, int height, IVC* image_rgb);

int vc_binary_mask_or(IVC* mascara1, IVC* mascara2, IVC* resultado);

int drawRectangle(IVC* srcdst, int xtl, int ytl, int xbr, int ybr, int r, int g, int b, const char* label);

bool isCircular(const OVC* blob, double threshold);

bool overlapAABB(const OVC* a, const OVC* b);

bool matchBlobAABB(OVC* blob, BlobTrack* track);

int trackAndDraw(OVC* blobs, int nlabels, BlobTrack* tracks, IVC* image, int r, int g, int b, const char* label, int minArea, int maxArea);