//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT CNICO DO C VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM TICOS
//                    VIS O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de fun  es n o seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "vc.h"

#define MAX(a,b) ((a > b) ? a : b)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN  ES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Alocar mem ria para uma imagem
IVC* vc_image_new(int width, int height, int channels, int levels)
{
	// Alocar memoria para a estrutura IVC
	IVC* image = (IVC*)malloc(sizeof(IVC));

	if (image == NULL) return NULL;
	if ((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char*)malloc(image->width * image->height * image->channels * sizeof(char));

	if (image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


// Libertar mem ria de uma imagem
IVC* vc_image_free(IVC* image)
{
	if (image != NULL)
	{
		if (image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUN  ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char* netpbm_get_token(FILE* file, char* tok, int len)
{
	char* t;
	int c;

	for (;;)
	{
		while (isspace(c = getc(file)));
		if (c != '#') break;
		do c = getc(file);
		while ((c != '\n') && (c != EOF));
		if (c == EOF) break;
	}

	t = tok;

	if (c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while ((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if (c == '#') ungetc(c, file);
	}

	*t = 0;

	return tok;
}


long int unsigned_char_to_bit(unsigned char* datauchar, unsigned char* databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char* p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}


void bit_to_unsigned_char(unsigned char* databit, unsigned char* datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char* p = databit;

	countbits = 1;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}


IVC* vc_read_image(char* filename)
{
	FILE* file = NULL;
	IVC* image = NULL;
	unsigned char* tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;

	// Abre o ficheiro
	if ((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if (strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if (strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if (strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
#endif

			fclose(file);
			return NULL;
		}

		if (levels == 1) // PBM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			size = image->width * image->height * image->channels;

			if ((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}

		fclose(file);
	}
	else
	{
#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
#endif
	}

	return image;
}


int vc_write_image(char* filename, IVC* image)
{
	FILE* file = NULL;
	unsigned char* tmp;
	long int totalbytes, sizeofbinarydata;

	if (image == NULL) return 0;

	if ((file = fopen(filename, "wb")) != NULL)
	{
		if (image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);

			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);

			if (fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				return 0;
			}
		}

		fclose(file);

		return 1;
	}

	return 0;
}

int vc_gray_negative(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verifica  o de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Inverte a imagem Gray
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			data[pos] = srcdst->levels - data[pos];
		}
	}

	return 1;
}

int vc_rgb_negative(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verifica  o de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 3) return 0;

	// Inverte a imagem RGB
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = 255 - data[pos];       // Canal Vermelho (R)
			data[pos + 1] = 255 - data[pos + 1]; // Canal Verde (G)
			data[pos + 2] = 255 - data[pos + 2]; // Canal Azul (B)
		}
	}

	return 1;
}

// Converter de RGB para Gray
int vc_rgb_to_gray(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;

	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	float rf, gf, bf;

	// Verifica  o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 3) || (dst->channels != 1)) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			// Red
			rf = (float)datasrc[pos_src];
			// Green
			gf = (float)datasrc[pos_src + 1];
			// Blue
			bf = (float)datasrc[pos_src + 2];

			datadst[pos_dst] = (unsigned char)((rf * 0.299) + (gf * 0.587) + (bf * 0.114));
		}
	}

	return 1;
}

// Aula 24/02/2025
int vc_rgb_to_hsv(IVC* src, IVC* dst)
{
	// Imagem Origem (RGB)
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;

	// Imagem Destino (HSV)
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;

	// Resolu  o da imagem, para percorrer todos os pixeis
	int width = src->width;
	int height = src->height;

	// Posicoes para percorrer todos os pixeis
	long int pos_src, pos_dst;

	// RGB frequ ncias
	unsigned char rf, gf, bf;

	// HSV componentes
	unsigned char h, s, v;
	unsigned char max, min;

	// hf   a tonalidade (hue) em graus ( ), variando entre 0  e 360 .
	float hf;

	// Verifica  o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 3) || (dst->channels != 3)) return 0; // A imagem HSV precisa ter 3 canais

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			// Captura os valores RGB
			rf = datasrc[pos_src];
			gf = datasrc[pos_src + 1];
			bf = datasrc[pos_src + 2];

			// Calcula max e min dos canais RGB
			max = (rf > gf) ? ((rf > bf) ? rf : bf) : ((gf > bf) ? gf : bf);
			min = (rf < gf) ? ((rf < bf) ? rf : bf) : ((gf < bf) ? gf : bf);

			// Calcula Value (Brilho)
			v = max;

			// Calcula Satura  o (verifica tamb m se o max   zero)
			s = (max == 0) ? 0 : ((max - min) * 255) / max;

			// Calcula Hue
			if (max == min)
			{
				hf = 0;
			}
			else if (max == rf)
			{
				// tem os dois casos do (max == R)
				hf = (gf < bf ? 360 : 0) + (60.0 * (gf - bf) / (max - min));
			}
			else if (max == gf)
			{
				// caso do (max == G)
				hf = 120 + (60.0 * (bf - rf) / (max - min));
			}
			else
			{	// caso do (max == B)
				hf = 240 + (60.0 * (rf - gf) / (max - min));
			}

			// Normaliza  o Hue para 0-255
			h = (unsigned char)((hf / 360.0) * 255.0);

			// Atribui valores   imagem de destino
			datadst[pos_dst] = h;
			datadst[pos_dst + 1] = s;
			datadst[pos_dst + 2] = v;

		}
	}

	return 1; // Sucesso
}

int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax)
{
	// Imagem Origem (HSV)
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;

	// Imagem Destino (Bin ria)
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;

	// Resolu  o da imagem
	int width = src->width;
	int height = src->height;

	// Posicoes para percorrer todos os pixeis
	long int pos_src, pos_dst;

	// Componentes HSV
	float h, s, v;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			// Percorre os pixeis da imagem
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			// Divide-se por 255 para obter valores entre 0 e 1 p.e. 0.34
			// Multiplica-se por 360 para o HUE
			// Multiplica-se por 100 para a SATURATION
			// Multiplica-se por 100 para o VALUE
			h = (float)((datasrc[pos_src] / 255.0f) * 360.0f);
			s = (float)((datasrc[pos_src + 1] / 255.0f) * 100.0f);
			v = (float)((datasrc[pos_src + 2] / 255.0f) * 100.0f);

			// Verifica se o pixel est  dentro do intervalo
			if ((h >= hmin) && (h <= hmax) &&
				(s >= smin) && (s <= smax) &&
				(v >= vmin) && (v <= vmax))
			{
				// Pixel dentro do intervalo - Branco (255)
				datadst[pos_dst] = 255;
			}
			else
			{
				// Pixel fora do intervalo - Preto (0)
				datadst[pos_dst] = 0;
			}
		}
	}

	return 1; // Sucesso
}

int vc_scale_gray_to_color_palette(IVC* src, IVC* dst)
{
	// Verifica se as imagens t m o mesmo tamanho
	if (src->width != dst->width || src->height != dst->height || src->channels != 1 || dst->channels != 3)
	{
		return 0;
	}

	// Apontadors para os dados de imagem
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	// Dimens es
	int width = src->width;
	int height = src->height;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;

	// Posicoes para percorrer todos os pixels
	long int pos_src, pos_dst;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			// Posi  o dos pixels na mem ria
			pos_src = y * bytesperline_src + x * src->channels;
			pos_dst = y * bytesperline_dst + x * dst->channels;

			// Red
			if (datasrc[pos_src] < 128) // 0 a 127
			{
				datadst[pos_dst] = 0;
			}
			else if (datasrc[pos_src] >= 192) // 192 a 255
			{
				datadst[pos_dst] = 255;
			}
			else // 128 a 191
			{
				// tom interm dio
				datadst[pos_dst] = (datasrc[pos_src] - 128) * 4;
			}

			// Green
			if (datasrc[pos_src] < 192 && datasrc[pos_src] >= 64) // 64 a 191
			{
				datadst[pos_dst + 1] = 255;
			}
			else if (datasrc[pos_src] < 64) // 0 a 63
			{
				datadst[pos_dst + 1] = datasrc[pos_src] * 4;
			}
			else // 192 a 255
			{
				// tom interm dio
				datadst[pos_dst + 1] = 255 - ((datasrc[pos_src] - 192) * 4);
			}

			// Blue
			if (datasrc[pos_src] >= 128) // 128 a 191
			{
				datadst[pos_dst + 2] = 0;
			}
			else if (datasrc[pos_src] < 64) // 0 a 63
			{
				datadst[pos_dst + 2] = 255;
			}
			else // 191 a 255
			{
				// tom interm dio
				datadst[pos_dst + 2] = 255 - ((datasrc[pos_src] - 64) * 4);
			}
		}
	}

	return 1;
}

int vc_count_white_pixels(IVC* src)
{
	// Imagem Origem (escala de cinzentos)
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;

	// Resolu  o
	int width = src->width;
	int height = src->height;

	long int pos_src;

	int count = 0;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * src->channels;

			if (datasrc[pos_src] == 255)
			{
				count += 1;
			}
		}
	}

	return count;

}

int vc_exercicio1(IVC* imageArea, IVC* imageBlue, IVC* imageGreen, IVC* imageYellow, IVC* imageRed1, IVC* imageRed2)
{
	int area = vc_count_white_pixels(imageArea);

	float countBlueP = 0, countGreenP = 0, countRedP = 0, countYellowP = 0;

	countBlueP = vc_count_white_pixels(imageBlue);
	countGreenP = vc_count_white_pixels(imageGreen);
	countYellowP = vc_count_white_pixels(imageYellow);
	countRedP = vc_count_white_pixels(imageRed1) + vc_count_white_pixels(imageRed2);

	printf("Area da imagem: %d\n", imageArea->height * imageArea->width);
	printf("Area funcional do cerebro: %d\n", area);

	printf("---------------------------\n");
	printf("Pixeis:\n");
	printf("Pixeis de cor: %.f\n", countBlueP + countGreenP + countRedP + countYellowP);
	printf("Pixeis vermelhos: %.f\n", countRedP);
	printf("Pixeis verdes: %.f\n", countGreenP);
	printf("Pixeis azuis: %.f\n", countBlueP);
	printf("Pixeis amarelos: %.f\n", countYellowP);

	printf("---------------------------\n");
	printf("Percentagens:\n");
	printf("Azul (0 a 25): %.1f\n", (countBlueP / area) * 100);
	printf("Verde (26 a 50): %.1f\n", (countGreenP / area) * 100);
	printf("Amarelo (51 a 75): %.1f\n", (countYellowP / area) * 100);
	printf("Vermelho (76 a 100): %.1f\n", (countRedP / area) * 100);

	return area;
}

int vc_gray_to_binary(IVC* src, IVC* dst, int threshold)
{
	// Verifica se as imagens t m o mesmo tamanho
	if (src->width != dst->width || src->height != dst->height || src->channels != 1 || dst->channels != 1)
	{
		return 0;
	}

	// Apontadors para os dados de imagem
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	// Dimens es
	int width = src->width;
	int height = src->height;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;

	// Posicoes para percorrer todos os pixels
	long int pos_src, pos_dst;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * src->channels;
			pos_dst = y * bytesperline_dst + x * dst->channels;

			// Codigo original (da na imagem dos avioes (FLIR/flir-01))
			if (datasrc[pos_src] > threshold)
			{
				datadst[pos_dst] = 255;
			}
			else if (datasrc[pos_src] <= threshold)
			{
				datadst[pos_dst] = 0;
			}
		}
	}
}

int vc_calc_threshold(IVC* src)
{

	// Apontadors para os dados de imagem
	unsigned char* datasrc = (unsigned char*)src->data;

	// Dimens es
	int width = src->width;
	int height = src->height;
	int bytesperline_src = src->width * src->channels;

	// Posicoes para percorrer todos os pixels
	long int pos_src;

	int sum = 0;
	int resolution = width * height;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * src->channels;

			sum += datasrc[pos_src];
		}
	}

	return (sum / resolution);
}

int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel)
{
	// Kernel: vizinhan a do pixel

	int x, y, i, j;
	long int pos, posk;
	int half_kernel = kernel / 2;
	int min, max, midpoint;

	// Verifica  o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 1)) return 0;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			min = 255;
			max = 0;

			for (j = -half_kernel; j <= half_kernel; j++)
			{
				for (i = -half_kernel; i <= half_kernel; i++)
				{
					if ((y + j >= 0) && (y + j < src->height) && (x + i >= 0) && (x + i < src->width))
					{
						posk = (y + j) * src->bytesperline + (x + i) * src->channels;
						if (src->data[posk] < min) min = src->data[posk];
						if (src->data[posk] > max) max = src->data[posk];
					}
				}
			}

			midpoint = (min + max) / 2;
			pos = y * src->bytesperline + x * src->channels;
			dst->data[pos] = (src->data[pos] > midpoint) ? 255 : 0;
		}
	}

	return 1;
}

// Explicar este c digo 17/03/2025
int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int kernel, float k)
{

	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;

	long int pos_src, pos_dst;

	int x, y, i, j;
	int half_kernel = kernel / 2;
	float media, desvioPadrao;
	int threshold;

	// Verifica  o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 1)) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos_src = y * bytesperline_src + x;
			pos_dst = y * bytesperline_dst + x;

			media = 0;
			desvioPadrao = 0;

			// Calcula a m dia na vizinhan a
			for (j = -half_kernel; j <= half_kernel; j++) {
				for (i = -half_kernel; i <= half_kernel; i++) {
					int neighbor_x = x + i;
					int neighbor_y = y + j;

					if (neighbor_x >= 0 && neighbor_x < width && neighbor_y >= 0 && neighbor_y < height) {
						int neighbor_pos = neighbor_y * bytesperline_src + neighbor_x;
						media += datasrc[neighbor_pos];
					}
				}
			}

			media /= kernel * kernel;

			// Calcula o desvio padr o na vizinhan a
			for (j = -half_kernel; j <= half_kernel; j++) {
				for (i = -half_kernel; i <= half_kernel; i++) {
					int neighbor_x = x + i;
					int neighbor_y = y + j;

					if (neighbor_x >= 0 && neighbor_x < width && neighbor_y >= 0 && neighbor_y < height) {
						int neighbor_pos = neighbor_y * bytesperline_src + neighbor_x;
						desvioPadrao += (datasrc[neighbor_pos] - media) * (datasrc[neighbor_pos] - media);
					}
				}
			}

			desvioPadrao = sqrt(desvioPadrao / (kernel * kernel));

			// Calcula o limiar de Niblack
			threshold = media + k * desvioPadrao;

			// Aplica  o do limiar de Niblack
			if (datasrc[pos_src] < threshold) {
				datadst[pos_dst] = 0; // Preto
			}
			else {
				datadst[pos_dst] = 255; // Branco
			}
		}
	}

	return 1;
}

// Aula 19/03/2025
int vc_binary_dilate(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i, j;
	int half_kernel = kernel / 2;
	long int pos_src, pos_dst;

	// Verifica  o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 1)) return 0;

	// Inicializa a imagem de destino com preto (0)
	memset(datadst, 0, width * height * sizeof(unsigned char));

	// Aplica a dilata  o bin ria
	for (y = half_kernel; y < height - half_kernel; y++)
	{
		for (x = half_kernel; x < width - half_kernel; x++)
		{
			pos_src = y * bytesperline + x * channels;

			if (datasrc[pos_src] == 255) // Se o pixel atual for branco
			{
				for (j = -half_kernel; j <= half_kernel; j++)
				{
					for (i = -half_kernel; i <= half_kernel; i++)
					{
						int neighbor_x = x + i;
						int neighbor_y = y + j;
						int pos_neigh = neighbor_y * bytesperline + neighbor_x * channels;

						if (neighbor_x >= 0 && neighbor_x < width && neighbor_y >= 0 && neighbor_y < height)
						{
							datadst[pos_neigh] = 255; // Expande a  rea branca
						}
					}
				}
			}
		}
	}

	return 1;
}

int vc_binary_erode(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i, j;
	int half_kernel = kernel / 2;
	long int pos_src, pos_dst;

	// Verifica  o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 1)) return 0;

	// Inicializa a imagem de destino com preto (0)
	memset(datadst, 0, width * height * sizeof(unsigned char));

	// Aplica a eros o bin ria
	for (y = half_kernel; y < height - half_kernel; y++)
	{
		for (x = half_kernel; x < width - half_kernel; x++)
		{
			pos_src = y * bytesperline + x * channels;

			if (datasrc[pos_src] == 255) // Apenas verifica se   branco
			{
				int erode = 1; // Assume que vai manter o branco

				for (j = -half_kernel; j <= half_kernel; j++)
				{
					for (i = -half_kernel; i <= half_kernel; i++)
					{
						int neighbor_x = x + i;
						int neighbor_y = y + j;
						int pos_neigh = neighbor_y * bytesperline + neighbor_x * channels;

						if (datasrc[pos_neigh] == 0) // Se houver um pixel preto na vizinhan a, apaga o pixel
						{
							erode = 0;
							break;
						}
					}
					if (!erode) break;
				}

				if (erode) datadst[pos_src] = 255; // Mant m branco apenas se todos os vizinhos s o brancos
			}
		}
	}

	return 1;
}

int vc_open_binary(IVC* src, IVC* dst, int kernel)
{
	if (src == NULL || dst == NULL) return 0;

	IVC* temp = vc_image_new(src->width, src->height, src->channels, src->levels);
	if (temp == NULL) return 0;

	if (!vc_binary_erode(src, temp, kernel)) {
		vc_image_free(temp);
		return 0;
	}

	if (!vc_binary_dilate(temp, dst, kernel)) {
		vc_image_free(temp);
		return 0;
	}

	vc_image_free(temp);
	return 1;
}

int vc_close_binary(IVC* src, IVC* dst, int kernel)
{
	if (src == NULL || dst == NULL) return 0;

	IVC* temp = vc_image_new(src->width, src->height, src->channels, src->levels);
	if (temp == NULL) return 0;

	if (!vc_binary_dilate(src, temp, kernel)) {
		vc_image_free(temp);
		return 0;
	}

	if (!vc_binary_erode(temp, dst, kernel)) {
		vc_image_free(temp);
		return 0;
	}

	vc_image_free(temp);

	return 1;
}

// 27/03/2025
int vc_binary_blob_labellingah(IVC* src, IVC* dst)
{
	if (!src || !dst || !src->data || !dst->data) return -1;

	// Copiar os dados da imagem fonte para a imagem destino (considerando bytesperline)
	memcpy(dst->data, src->data, src->bytesperline * src->height);

	int label = 1;

	for (int y = 1; y < dst->height - 1; y++)  // Evita acessar fora da mem ria
	{
		for (int x = 1; x < dst->width - 1; x++)
		{

			int posX = y * dst->bytesperline + x;
			int posA = (y - 1) * dst->bytesperline + (x - 1);
			int posB = (y - 1) * dst->bytesperline + x;
			int posC = (y - 1) * dst->bytesperline + (x + 1);
			int posD = y * dst->bytesperline + (x - 1);

			if (dst->data[posX] == 255)
			{
				int min_label = 255;

				// Encontrar o menor r tulo vizinho
				if (dst->data[posA] > 0 && dst->data[posA] < min_label) min_label = dst->data[posA];
				if (dst->data[posB] > 0 && dst->data[posB] < min_label) min_label = dst->data[posB];
				if (dst->data[posC] > 0 && dst->data[posC] < min_label) min_label = dst->data[posC];
				if (dst->data[posD] > 0 && dst->data[posD] < min_label) min_label = dst->data[posD];

				if (min_label == 255) // Nenhum vizinho rotulado
				{
					dst->data[posX] = label;
					label++;
					if (label > 254) label = 254; // Evita estouro
				}
				else
				{
					dst->data[posX] = min_label;
				}
			}
			else
			{
				dst->data[posX] = 0;
			}
		}
	}

	return 1; // Sucesso
}

// Etiquetagem de blobs
// src		: Imagem bin ria de entrada
// dst		: Imagem grayscale (ir  conter as etiquetas)
// nlabels	: Endere o de mem ria de uma vari vel, onde ser  armazenado o n mero de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas.   necess rio libertar posteriormente esta mem ria.
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	int labeltable[256] = { 0 };
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC* blobs; // Apontador para array de blobs (objectos) que ser  retornado desta fun  o.

	// Verifica  o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	// Copia dados da imagem bin ria para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pix is de plano de fundo devem obrigat riamente ter valor 0
	// Todos os pix is de primeiro plano devem obrigat riamente ter valor 255
	// Ser o atribu das etiquetas no intervalo [1,254]
	// Este algoritmo est  assim limitado a 254 labels
	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0) datadst[i] = 255;
	}

	// Limpa os rebordos da imagem bin ria
	for (y = 0; y < height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}
	for (x = 0; x < width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels; // B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels; // D
			posX = y * bytesperline + x * channels; // X

			// Se o pixel foi marcado
			if (datadst[posX] != 0)
			{
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
				{
					datadst[posX] = label;
					labeltable[label] = label;
					label++;

					if (label == 255)
					{
						return NULL;
					}
				}
				else
				{
					num = 255;

					// Se A est  marcado
					if (datadst[posA] != 0) num = labeltable[datadst[posA]];
					// Se B est  marcado, e   menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
					// Se C est  marcado, e   menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
					// Se D est  marcado, e   menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posB] != 0)
					{
						if (labeltable[datadst[posB]] != num)
						{
							for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posC] != 0)
					{
						if (labeltable[datadst[posC]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posD] != 0)
					{
						if (labeltable[datadst[posD]] != num)
						{
							for (tmplabel = labeltable[datadst[posD]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	// Contagem do n mero de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que n o hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++; // Conta etiquetas
		}
	}

	// Se n o h  blobs
	if (*nlabels == 0) return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC*)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a < (*nlabels); a++) blobs[a].label = labeltable[a];
	}
	else return NULL;

	return blobs;
}


int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verifica  o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Conta  rea de cada blob
	for (i = 0; i < nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y < height - 1; y++)
		{
			for (x = 1; x < width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					//  rea
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x) xmin = x;
					if (ymin > y) ymin = y;
					if (xmax < x) xmax = x;
					if (ymax < y) ymax = y;

					// Per metro
					// Se pelo menos um dos quatro vizinhos n o pertence ao mesmo label, ent o   um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		// Bounding Box
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		// Centro de Gravidade
		blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}

	return 1;
}

// Est  bem?
//void draw_rectangles(IVC* image, OVC* blobs, int nblobs) {
//	for (int i = 0; i < nblobs; i++) {
//		int x1 = blobs[i].x;
//		int y1 = blobs[i].y;
//		int x2 = x1 + blobs[i].width;
//		int y2 = y1 + blobs[i].height;
//
//		// Desenha ret ngulo branco (intensidade 255)
//		vc_draw_rectangle(image, x1, y1, x2, y2, 255,0,0);
//	}
//}
//
//// Fun  o para definir a cor do pixel
//void set_pixel_color(IVC* image, int x, int y, unsigned char r, unsigned char g, unsigned char b) {
//	if (image == NULL) return;
//
//	int width = image->width;
//	int channels = image->channels;
//	unsigned char* data = image->data;
//
//	// Limitar coordenadas dentro dos limites da imagem
//	if (x < 0 || x >= width || y < 0 || y >= image->height) {
//		return;
//	}
//
//	int idx = y * width * channels + x * channels;
//
//	if (channels == 1) {
//		data[idx] = r;  // Em imagens em escala de cinza, usa 'r' como intensidade
//	}
//	else if (channels == 3) {
//		data[idx] = r;       // Canal Red
//		data[idx + 1] = g;   // Canal Green
//		data[idx + 2] = b;   // Canal Blue
//	}
//}
//
//void vc_draw_rectangle(IVC* image, int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b) {
//	// Verificar se a imagem   v lida
//	if (image == NULL) return;
//
//	int width = image->width;
//	int height = image->height;
//	int channels = image->channels;
//
//	// Garantir que x1 <= x2 e y1 <= y2
//	if (x1 > x2) {
//		int tmp = x1;
//		x1 = x2;
//		x2 = tmp;
//	}
//	if (y1 > y2) {
//		int tmp = y1;
//		y1 = y2;
//		y2 = tmp;
//	}
//
//	// Limitar coordenadas aos limites da imagem
//	x1 = max(0, min(x1, width - 1));
//	x2 = max(0, min(x2, width - 1));
//	y1 = max(0, min(y1, height - 1));
//	y2 = max(0, min(y2, height - 1));
//
//	// Desenhar as linhas horizontais (topo e base)
//	for (int x = x1; x <= x2; x++) {
//		set_pixel_color(image, x, y1, r, g, b);  // Linha superior
//		set_pixel_color(image, x, y2, r, g, b);  // Linha inferior
//	}
//
//	// Desenhar as linhas verticais (esquerda e direita)
//	for (int y = y1; y <= y2; y++) {
//		set_pixel_color(image, x1, y, r, g, b);  // Linha esquerda
//		set_pixel_color(image, x2, y, r, g, b);  // Linha direita
//	}
//}

// 2/4/2025
// ainda nao testado 2/4/2025 11:16
int vc_gray_histogram_show(IVC* src, IVC* dst) {
	if (src == NULL || dst == NULL || src->channels != 1) {
		printf("Erro: Imagem de entrada inv lida ou n o est  em tons de cinza.\n");
		return 0;
	}

	int ni[256] = { 0 };
	int totalPixels = src->width * src->height;

	// Contagem do n mero de pixels para cada n vel de brilho
	for (int i = 0; i < totalPixels; ni[src->data[i++]]++);

	// C lculo da PDF (probabilidade de cada n vel de cinza)
	float pdf[256];
	for (int i = 0; i < 256; i++) {
		pdf[i] = (float)ni[i] / totalPixels;
	}

	// Encontrar o valor m ximo da PDF
	float pdfmax = 0;
	for (int i = 0; i < 256; i++) {
		if (pdf[i] > pdfmax) pdfmax = pdf[i];
	}

	// Normaliza  o da PDF
	float pdfnorm[256];
	for (int i = 0; i < 256; i++) {
		pdfnorm[i] = pdf[i] / pdfmax;
	}

	// Gera  o da imagem do histograma (assume-se 256x256)
	memset(dst->data, 0, 256 * 256);
	for (int x = 0; x < 256; x++) {
		for (int y = 255; y >= 255 - (pdfnorm[x] * 255); y--) {
			dst->data[y * 256 + x] = 255;
		}
	}

	return 1;
}

int vc_gray_histogram_equalization(IVC* src, IVC* dst) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels = src->channels;
	int bytesperline = src->bytesperline;
	int x, y;
	long int pos;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 1)) return 0;

	// contar pixeis para cada valor de brilho
	int ni[256] = { 0 };
	for (int i = 0; i < width * height; ni[datasrc[i++]]++);

	// Step 2: Calculate PDF (Probability Density Function)
	float pdf[256];
	int n = width * height;
	for (int i = 0; i < 256; i++) {
		pdf[i] = (float)ni[i] / (float)n;
	}

	// Step 3: Calculate CDF (Cumulative Distribution Function)
	float cdf[256];
	cdf[0] = pdf[0];
	for (int i = 1; i < 256; i++) {
		cdf[i] = cdf[i - 1] + pdf[i];
	}

	// Find cdfmin (first non-zero value)
	float cdfmin = 0.0f;
	for (int i = 0; i < 256; i++) {
		if (cdf[i] > 0.0f) {
			cdfmin = cdf[i];
			break;
		}
	}

	// Step 4: Apply histogram equalization
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;

			// Apply the equalization formula
			float new_value = (cdf[datasrc[pos]] - cdfmin) / (1.0f - cdfmin) * 255.0f;

			// Ensure the value is within valid range [0,255]
			datadst[pos] = (unsigned char)(new_value + 0.5f);
		}
	}

	return 1;
}

// 07/04/2025
// N o sei se est  certo mas d  alguma coisa
int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th)
{
	// Verificar se as imagens s o v lidas
	if (src == NULL || dst == NULL) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;
	if (src->channels != 1 || dst->channels != 1) return 0;

	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	unsigned char* src_data = src->data;
	unsigned char* dst_data = dst->data;

	// Inicializar a imagem de destino com 0
	memset(dst_data, 0, bytesperline * height);

	// Percorrer cada pixel (excluindo bordas)
	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			// Calcular gradiente horizontal (Gx)
			int sum_left = src_data[(y - 1) * bytesperline + (x - 1)] +
				src_data[y * bytesperline + (x - 1)] +
				src_data[(y + 1) * bytesperline + (x - 1)];
			int sum_right = src_data[(y - 1) * bytesperline + (x + 1)] +
				src_data[y * bytesperline + (x + 1)] +
				src_data[(y + 1) * bytesperline + (x + 1)];
			int Gx = (sum_right - sum_left) / 3;

			// Calcular gradiente vertical (Gy)
			int sum_top = src_data[(y - 1) * bytesperline + (x - 1)] +
				src_data[(y - 1) * bytesperline + x] +
				src_data[(y - 1) * bytesperline + (x + 1)];
			int sum_bottom = src_data[(y + 1) * bytesperline + (x - 1)] +
				src_data[(y + 1) * bytesperline + x] +
				src_data[(y + 1) * bytesperline + (x + 1)];
			int Gy = (sum_bottom - sum_top) / 3;

			// Calcular a magnitude do gradiente
			float magnitude = sqrtf(Gx * Gx + Gy * Gy);

			// Aplicar o limiar
			dst_data[y * bytesperline + x] = (magnitude >= th) ? 255 : 0;
		}
	}

	return 1;
}

int vc_gray_edge_sobel(IVC* src, IVC* dst, float th) {
	// Verify if images are valid
	if (src == NULL || dst == NULL) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;
	if (src->channels != 1 || dst->channels != 1) return 0;

	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	unsigned char* src_data = src->data;
	unsigned char* dst_data = dst->data;

	// Initialize destination image with 0
	memset(dst_data, 0, bytesperline * height);

	// Sobel kernels
	const int sobel_x[3][3] = {
		{-1, 0, 1},
		{-2, 0, 2},
		{-1, 0, 1}
	};

	const int sobel_y[3][3] = {
		{-1, -2, -1},
		{ 0,  0,  0},
		{ 1,  2,  1}
	};

	// Process each pixel (excluding borders)
	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			int gx = 0, gy = 0;

			// Apply Sobel kernels
			for (int ky = -1; ky <= 1; ky++) {
				for (int kx = -1; kx <= 1; kx++) {
					int pixel = src_data[(y + ky) * bytesperline + (x + kx)];
					gx += pixel * sobel_x[ky + 1][kx + 1];
					gy += pixel * sobel_y[ky + 1][kx + 1];
				}
			}

			// Calculate gradient magnitude
			float magnitude = sqrtf(gx * gx + gy * gy);

			// Apply threshold
			dst_data[y * bytesperline + x] = (magnitude >= th) ? 255 : 0;
		}
	}

	return 1;
}

int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernelsize)
{
	int x, y, i, j;
	int sum;
	int offset = kernelsize / 2;

	// Verifica  es b sicas
	if (!src || !dst || src->data == NULL || dst->data == NULL) return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != dst->channels) return 0;
	if (src->channels != 1) return 0; // Apenas imagens em escala de cinzentos

	int width = src->width;
	int height = src->height;
	int normalization = kernelsize * kernelsize;

	for (y = offset; y < height - offset; y++) {
		for (x = offset; x < width - offset; x++) {
			sum = 0;

			for (j = -offset; j <= offset; j++) {
				for (i = -offset; i <= offset; i++) {
					int px = src->data[(y + j) * width + (x + i)];
					sum += px;
				}
			}

			dst->data[y * width + x] = sum / normalization;
		}
	}

	return 1;
}
#pragma region Projeto

void vc_bgr_to_rgb(const unsigned char* frame_data, int frame_step, int width, int height, IVC* image_rgb)
{
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int pos_frame = y * frame_step + x * 3;

			int pos_image = y * image_rgb->bytesperline + x * 3;

			image_rgb->data[pos_image] = frame_data[pos_frame + 2];
			image_rgb->data[pos_image + 1] = frame_data[pos_frame + 1];
			image_rgb->data[pos_image + 2] = frame_data[pos_frame];
		}
	}
}

int vc_binary_mask_or(IVC* mascara1, IVC* mascara2, IVC* resultado)
{
	if (mascara1 == NULL || mascara2 == NULL || resultado == NULL) return 0;
	if (mascara1->width != mascara2->width || mascara1->height != mascara2->height) return 0;
	if (mascara1->width != resultado->width || mascara1->height != resultado->height) return 0;
	if (mascara1->channels != 1 || mascara2->channels != 1 || resultado->channels != 1) return 0;

	unsigned char* data1 = (unsigned char*)mascara1->data;
	unsigned char* data2 = (unsigned char*)mascara2->data;
	unsigned char* datares = (unsigned char*)resultado->data;

	int width = mascara1->width;
	int height = mascara1->height;
	int bytesperline = mascara1->bytesperline;
	long int pos;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * mascara1->channels;

			datares[pos] = (data1[pos] == 255 || data2[pos] == 255) ? 255 : 0;
		}
	}

	return 1;
}


const unsigned char font_5x7[][7] = {
	{0b01110,0b10001,0b10011,0b10101,0b11001,0b10001,0b01110},
	{0b00100,0b01100,0b00100,0b00100,0b00100,0b00100,0b01110},
	{0b01110,0b10001,0b00001,0b00110,0b01000,0b10000,0b11111},
	{0b01110,0b10001,0b00001,0b00110,0b00001,0b10001,0b01110},
	{0b00010,0b00110,0b01010,0b10010,0b11111,0b00010,0b00010},
	{0b11111,0b10000,0b11110,0b00001,0b00001,0b10001,0b01110},
	{0b00110,0b01000,0b10000,0b11110,0b10001,0b10001,0b01110},
	{0b11111,0b00001,0b00010,0b00100,0b01000,0b01000,0b01000},
	{0b01110,0b10001,0b10001,0b01110,0b10001,0b10001,0b01110},
	{0b01110,0b10001,0b10001,0b01111,0b00001,0b00010,0b01100},
	{0b01110,0b10001,0b10000,0b10000,0b10000,0b10001,0b01110},
	{0b01110,0b10001,0b10000,0b11110,0b10000,0b10001,0b01110}
};

void drawChar(IVC* image, int x, int y, const unsigned char* character, int r, int g, int b)
{
	if (image == NULL || image->channels != 3) return;

	unsigned char* data = (unsigned char*)image->data;
	int width = image->width;
	int height = image->height;
	int bytesperline = width * image->channels;

	for (int row = 0; row < 7; row++) {
		for (int col = 0; col < 5; col++) {
			if (character[row] & (1 << (4 - col))) {
				int px = x + col;
				int py = y + row;
				if (px >= 0 && px < width && py >= 0 && py < height) {
					long pos = py * bytesperline + px * 3;
					data[pos] = b;
					data[pos + 1] = g;
					data[pos + 2] = r;
				}
			}
		}
	}
}

int drawRectangle(IVC* srcdst, int xtl, int ytl, int xbr, int ybr, int r, int g, int b, const char* label)
{
	if (srcdst == NULL || srcdst->data == NULL || srcdst->channels != 3)
		return 0;

	unsigned char* datasrc = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int channels = srcdst->channels;
	int bytesperline = width * channels;

	if (width <= 0 || height <= 0) return 0;
	if (xtl < 0 || xtl >= width || ytl < 0 || ytl >= height) return 0;
	if (xbr < 0 || xbr >= width || ybr < 0 || ybr >= height) return 0;

	if (xtl > xbr) { int tmp = xtl; xtl = xbr; xbr = tmp; }
	if (ytl > ybr) { int tmp = ytl; ytl = ybr; ybr = tmp; }

	for (int x = xtl; x <= xbr; x++) {
		long int pos_top = ytl * bytesperline + x * channels;
		long int pos_bottom = ybr * bytesperline + x * channels;

		datasrc[pos_top] = b;
		datasrc[pos_top + 1] = g;
		datasrc[pos_top + 2] = r;

		datasrc[pos_bottom] = b;
		datasrc[pos_bottom + 1] = g;
		datasrc[pos_bottom + 2] = r;
	}

	for (int y = ytl; y <= ybr; y++) {
		long int pos_left = y * bytesperline + xtl * channels;
		long int pos_right = y * bytesperline + xbr * channels;

		datasrc[pos_left] = b;
		datasrc[pos_left + 1] = g;
		datasrc[pos_left + 2] = r;

		datasrc[pos_right] = b;
		datasrc[pos_right + 1] = g;
		datasrc[pos_right + 2] = r;
	}

	int textX = xtl;
	int textY = ytl - 10;
	if (textY < 0) textY = ytl + 2;

	int cursorX = textX;

	for (int i = 0; label[i] != '\0'; i++) {
		char ch = label[i];
		int fontIndex = -1;

		if (ch >= '0' && ch <= '5') fontIndex = ch - '0';
		else if (ch == 'c') fontIndex = 10;
		else if (ch == 'e') fontIndex = 11;

		if (fontIndex >= 0) {
			drawChar(srcdst, cursorX, textY, font_5x7[fontIndex], r, g, b);
			cursorX += 6;
		}
	}

	int cgx = (xtl + xbr) / 2;
	int cgy = (ytl + ybr) / 2;

	for (int i = -5; i <= 5; i++) {
		int x1 = cgx + i;
		int y1 = cgy + i;
		if (x1 >= 0 && x1 < width && y1 >= 0 && y1 < height) {
			long int pos = y1 * bytesperline + x1 * channels;
			datasrc[pos] = b;
			datasrc[pos + 1] = g;
			datasrc[pos + 2] = r;
		}

		int x2 = cgx + i;
		int y2 = cgy - i;
		if (x2 >= 0 && x2 < width && y2 >= 0 && y2 < height) {
			long int pos = y2 * bytesperline + x2 * channels;
			datasrc[pos] = b;
			datasrc[pos + 1] = g;
			datasrc[pos + 2] = r;
		}
	}

	return 1;
}

bool isCircular(const OVC* blob, double threshold) {
	double perimeter = 2.0 * (blob->width + blob->height);

	if (perimeter == 0) return false;

	double circularity = (4.0 * M_PI * blob->area) / (perimeter * perimeter);

	return circularity >= threshold;
}

bool overlapAABB(const OVC* a, const OVC* b)
{
	return (a->x < b->x + b->width) &&
		(b->x < a->x + a->width) &&
		(a->y < b->y + b->height) &&
		(b->y < a->y + a->height);
}

bool matchBlobAABB(OVC* blob, BlobTrack* track)
{
	OVC trackBlob;

	trackBlob.x = track->x;
	trackBlob.y = track->y;
	trackBlob.width = track->width;
	trackBlob.height = track->height;

	return overlapAABB(blob, &trackBlob);
}

int trackAndDraw(OVC* blobs, int nlabels, BlobTrack* tracks, IVC* image, int r, int g, int b, const char* label, int minArea, int maxArea)
{
	int total = 0;

	for (int i = 0; i < nlabels; i++) {
		if (blobs[i].area < minArea || blobs[i].area > maxArea)
			continue;

		if (!isCircular(&blobs[i], 0.5))
			continue;

		bool matched = false;

		for (int t = 0; t < MAX_TRACKED; t++) {
			if (!tracks[t].active) continue;
			
			if (matchBlobAABB(&blobs[i], &tracks[t])) {
				tracks[t].frames_detected++;

				tracks[t].x = blobs[i].x;
				tracks[t].y = blobs[i].y;
				tracks[t].width = blobs[i].width;
				tracks[t].height = blobs[i].height;

				if (tracks[t].frames_detected == 4 && !tracks[t].counted) {
					total++;
					tracks[t].counted = true;
				}

				matched = true;
				break;
			}
		}

		if (!matched) {
			for (int t = 0; t < MAX_TRACKED; t++) {
				if (!tracks[t].active) {
					tracks[t].x = blobs[i].x;
					tracks[t].y = blobs[i].y;
					tracks[t].width = blobs[i].width;
					tracks[t].height = blobs[i].height;
					tracks[t].frames_detected = 1;
					tracks[t].counted = false;
					tracks[t].active = true;

					break;
				}
			}
		}

		printf("=======================================================\n");
		printf("Perimetro da moeda %s: %d pixeis.\n", label, blobs[i].perimeter);
		printf("Area da moeda %s: %d pixeis.\n", label, blobs[i].area);
		printf("=======================================================\n");

		drawRectangle(image, blobs[i].x, blobs[i].y,
			blobs[i].x + blobs[i].width, blobs[i].y + blobs[i].height,
			r, g, b, label);
	}

	return total;
}

#pragma endregion