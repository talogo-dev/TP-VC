#include <iostream>
#include <string>
#include <chrono>
#include <vector>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

extern "C" {
#include "vc.h"
}

IVC* cvMatToIVC(const cv::Mat& mat) {
    if (mat.empty() || (mat.type() != CV_8UC1 && mat.type() != CV_8UC3)) {
        std::cerr << "Formato de imagem inválido para conversão." << std::endl;
        return NULL;
    }

    IVC* image = new IVC;
    image->width = mat.cols;
    image->height = mat.rows;
    image->channels = (mat.type() == CV_8UC1) ? 1 : 3;
    image->levels = 255;
    image->bytesperline = image->width * image->channels;
    image->data = new unsigned char[image->bytesperline * image->height];

    for (int y = 0; y < image->height; y++) {
        for (int x = 0; x < image->width; x++) {
            if (image->channels == 1) {
                image->data[y * image->bytesperline + x] = mat.at<uchar>(y, x);
            }
            else {
                cv::Vec3b bgr = mat.at<cv::Vec3b>(y, x);
                int pos = (y * image->width + x) * 3;
                image->data[pos] = bgr[2];
                image->data[pos + 1] = bgr[1];
                image->data[pos + 2] = bgr[0];
            }
        }
    }

    return image;
}

cv::Mat ivcToCvMat(const IVC* image) {
    if (!image || !image->data || image->channels != 1 && image->channels != 3) {
        std::cerr << "Imagem IVC inválida." << std::endl;
        return cv::Mat();
    }

    int type = (image->channels == 1) ? CV_8UC1 : CV_8UC3;
    cv::Mat mat(image->height, image->width, type);

    for (int y = 0; y < image->height; y++) {
        for (int x = 0; x < image->width; x++) {
            if (image->channels == 1) {
                mat.at<uchar>(y, x) = image->data[y * image->bytesperline + x];
            }
            else {
                int pos = (y * image->width + x) * 3;
                uchar r = image->data[pos];
                uchar g = image->data[pos + 1];
                uchar b = image->data[pos + 2];
                mat.at<cv::Vec3b>(y, x) = cv::Vec3b(b, g, r);
            }
        }
    }

    return mat;
}

#pragma endregion

#pragma region Timer
void vc_timer(void) {
    static bool running = false;
    static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();

    if (!running) {
        running = true;
    }
    else {
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        std::chrono::steady_clock::duration elapsedTime = currentTime - previousTime;

        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(elapsedTime);
        double nseconds = time_span.count();

        std::cout << "Tempo decorrido: " << nseconds << " segundos" << std::endl;
        std::cout << "Pressione qualquer tecla para continuar...\n";
        std::cin.get();
    }
}
#pragma endregion

int main(void) {
#pragma region Variaveis
    char videofile[256] = "./video1.mp4";

    cv::VideoCapture capture;
    struct {
        int width, height;
        int ntotalframes;
        int fps;
        int nframe;
    } video;

    std::string str;
    int key = 0;

    capture.open(videofile);
    if (!capture.isOpened()) {
        std::cerr << "Erro ao abrir o ficheiro de v deo: " << videofile << std::endl;
        return 1;
    }
    else {
        std::cout << "V deo aberto com sucesso!" << std::endl;
    }

    video.ntotalframes = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_COUNT));
    video.fps = static_cast<int>(capture.get(cv::CAP_PROP_FPS));
    video.width = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
    video.height = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));

    cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);
    vc_timer();
#pragma endregion

#pragma region Inicializacao de variaveis

    int colors[8][3] = {
    {255, 0, 0},
    {255, 128, 0},
    {255, 255, 0},
    {0, 255, 0},
    {0, 255, 255},
    {0, 0, 255},
    {128, 0, 255},
    {255, 0, 255}
    };

    int minArea_1c = 9750, minArea_2c = 11250, minArea_5c = 16000, minArea_10c = 15300, minArea_20c = 19000, minArea_50c = 22000, minArea_1e = 19842, minArea_2e = 25000;
    int maxArea_1c = 10850, maxArea_2c = 12750, maxArea_5c = 19000, maxArea_10c = 15630, maxArea_20c = 21000, maxArea_50c = 25000, maxArea_1e = 20936, maxArea_2e = 28000;

    int total_1c = 0, total_2c = 0, total_5c = 0, total_10c = 0, total_20c = 0, total_50c = 0, total_1e = 0, total_2e = 0;
    int total_moedas = 0;
    float total_dinheiro = 0.0f;

    IVC* image_rgb = vc_image_new(video.width, video.height, 3, 255);
    IVC* image_hsv = vc_image_new(video.width, video.height, 3, 255);

    IVC* mask_1c = vc_image_new(video.width, video.height, 1, 255);
    IVC* mask_2c = vc_image_new(video.width, video.height, 1, 255);
    IVC* mask_5c = vc_image_new(video.width, video.height, 1, 255);

    IVC* mask_10c = vc_image_new(video.width, video.height, 1, 255);
    IVC* mask_20c = vc_image_new(video.width, video.height, 1, 255);
    IVC* mask_50c = vc_image_new(video.width, video.height, 1, 255);

    IVC* mask_1eDentro = vc_image_new(video.width, video.height, 1, 255);
    IVC* mask_1eFora = vc_image_new(video.width, video.height, 1, 255);
    IVC* mask_1e = vc_image_new(video.width, video.height, 1, 255);

    IVC* mask_2eDentro = vc_image_new(video.width, video.height, 1, 255);
    IVC* mask_2eFora = vc_image_new(video.width, video.height, 1, 255);
    IVC* mask_2e = vc_image_new(video.width, video.height, 1, 255);

    BlobTrack tracked_1c[MAX_TRACKED] = { 0 };
    BlobTrack tracked_2c[MAX_TRACKED] = { 0 };
    BlobTrack tracked_5c[MAX_TRACKED] = { 0 };
    BlobTrack tracked_10c[MAX_TRACKED] = { 0 };
    BlobTrack tracked_20c[MAX_TRACKED] = { 0 };
    BlobTrack tracked_50c[MAX_TRACKED] = { 0 };
    BlobTrack tracked_1e[MAX_TRACKED] = { 0 };
    BlobTrack tracked_2e[MAX_TRACKED] = { 0 };

#pragma endregion
    cv::Mat frame;
    while (true) {
#pragma region Leitura e conversão de frames
        capture.read(frame);
        if (frame.empty()) break;
        video.nframe = static_cast<int>(capture.get(cv::CAP_PROP_POS_FRAMES));

        vc_bgr_to_rgb(frame.data, frame.step, video.width, video.height, image_rgb);

        vc_rgb_to_hsv(image_rgb, image_hsv);
#pragma endregion
#pragma region Segmentação HSV e Morfologia

        vc_hsv_segmentation(image_hsv, mask_1c, 20, 40, 30, 80, 20, 60);
        vc_close_binary(mask_1c, mask_1c, 19);
        vc_hsv_segmentation(image_hsv, mask_2c, 30, 40, 50, 70, 25, 35);
        vc_close_binary(mask_2c, mask_2c, 7);
        vc_hsv_segmentation(image_hsv, mask_5c, 25, 35, 50, 70, 20, 30);
        vc_close_binary(mask_5c, mask_5c, 7);

        vc_hsv_segmentation(image_hsv, mask_10c, 40, 60, 35, 75, 20, 45);
        vc_close_binary(mask_10c, mask_10c, 7);
        vc_hsv_segmentation(image_hsv, mask_20c, 40, 70, 40, 90, 15, 40);
        vc_close_binary(mask_20c, mask_20c, 9);
        vc_hsv_segmentation(image_hsv, mask_50c, 45, 55, 30, 55, 35, 50);
        vc_close_binary(mask_50c, mask_50c, 11);

        vc_hsv_segmentation(image_hsv, mask_1eDentro, 50, 95, 10, 25, 14, 25);
        vc_close_binary(mask_1eDentro, mask_1eDentro, 19);

        vc_hsv_segmentation(image_hsv, mask_1eFora, 45, 55, 30, 45, 25, 40);
        vc_close_binary(mask_1eFora, mask_1eFora, 19);

        vc_hsv_segmentation(image_hsv, mask_2eDentro, 40, 60, 5, 40, 30, 40);
        vc_close_binary(mask_2eDentro, mask_2eDentro, 9);

        vc_hsv_segmentation(image_hsv, mask_2eFora, 35, 140, 5, 25, 20, 40);
        vc_close_binary(mask_2eFora, mask_2eFora, 15);

        vc_binary_mask_or(mask_1eDentro, mask_1eFora, mask_1e);
        vc_close_binary(mask_1e, mask_1e, 13);
        vc_binary_mask_or(mask_2eDentro, mask_2eFora, mask_2e);
        vc_close_binary(mask_2e, mask_2e, 11);

#pragma endregion
#pragma region Mostragem das segmentacões
        // scv::Mat mask1c(video.height, video.width, CV_8UC1, mask_1c->data);
        //cv::Mat mask2c(video.height, video.width, CV_8UC1, mask_2c->data);
        // cv::Mat mask5c(video.height, video.width, CV_8UC1, mask_5c->data);

        //cv::Mat mask10cMat(video.height, video.width, CV_8UC1, mask_10c->data);
        //cv::Mat mask20cMat(video.height, video.width, CV_8UC1, mask_20c->data);
        //cv::Mat mask50cMat(video.height, video.width, CV_8UC1, mask_50c->data);

        //cv::Mat mask1eMat(video.height, video.width, CV_8UC1, mask_1e->data);
         //cv::Mat mask2eMat(video.height, video.width, CV_8UC1, mask_2e->data);

        //cv::imshow("Segmentacao 1c", mask1c);
        //cv::imshow("Segmentacao 2c", mask2c);
        // cv::imshow("Segmentacao 5c", mask5c);

        //cv::imshow("Segmentacao 10c", mask10cMat);
        //cv::imshow("Segmentacao 20c", mask20cMat);
        //cv::imshow("Segmentacao 50c", mask50cMat);

        //cv::imshow("Segmentacao 1e", mask1eMat);
         //cv::imshow("Segmentacao 2e", mask2eMat);

#pragma endregion
#pragma region Rotulagem e Análise
        int nlabels_1c = 0, nlabels_2c = 0, nlabels_5c = 0;
        int nlabels_10c = 0, nlabels_20c = 0, nlabels_50c = 0;
        int nlabels_1e = 0, nlabels_2e = 0;

        IVC* labels_1c = vc_image_new(video.width, video.height, 1, 255);
        IVC* labels_2c = vc_image_new(video.width, video.height, 1, 255);
        IVC* labels_5c = vc_image_new(video.width, video.height, 1, 255);

        IVC* labels_10c = vc_image_new(video.width, video.height, 1, 255);
        IVC* labels_20c = vc_image_new(video.width, video.height, 1, 255);
        IVC* labels_50c = vc_image_new(video.width, video.height, 1, 255);

        IVC* labels_1e = vc_image_new(video.width, video.height, 1, 255);
        IVC* labels_2e = vc_image_new(video.width, video.height, 1, 255);

        OVC* blobs_1c = vc_binary_blob_labelling(mask_1c, labels_1c, &nlabels_1c);
        if (blobs_1c != NULL) {
            vc_binary_blob_info(labels_1c, blobs_1c, nlabels_1c);
        }

        OVC* blobs_2c = vc_binary_blob_labelling(mask_2c, labels_2c, &nlabels_2c);
        if (blobs_2c != NULL) {
            vc_binary_blob_info(labels_2c, blobs_2c, nlabels_2c);
        }

        OVC* blobs_5c = vc_binary_blob_labelling(mask_5c, labels_5c, &nlabels_5c);
        if (blobs_5c != NULL) {
            vc_binary_blob_info(labels_5c, blobs_5c, nlabels_5c);
        }

        OVC* blobs_10c = vc_binary_blob_labelling(mask_10c, labels_10c, &nlabels_10c);
        if (blobs_10c != NULL) {
            vc_binary_blob_info(labels_10c, blobs_10c, nlabels_10c);
        }

        OVC* blobs_20c = vc_binary_blob_labelling(mask_20c, labels_20c, &nlabels_20c);
        if (blobs_20c != NULL) {
            vc_binary_blob_info(labels_20c, blobs_20c, nlabels_20c);
        }

        OVC* blobs_50c = vc_binary_blob_labelling(mask_50c, labels_50c, &nlabels_50c);
        if (blobs_50c != NULL) {
            vc_binary_blob_info(labels_50c, blobs_50c, nlabels_50c);
        }

        OVC* blobs_1e = vc_binary_blob_labelling(mask_1e, labels_1e, &nlabels_1e);
        if (blobs_1e != NULL) {
            vc_binary_blob_info(labels_1e, blobs_1e, nlabels_1e);
        }

        OVC* blobs_2e = vc_binary_blob_labelling(mask_2e, labels_2e, &nlabels_2e);
        if (blobs_2e != NULL) {
            vc_binary_blob_info(labels_2e, blobs_2e, nlabels_2e);
        }
#pragma endregion

#pragma region Validacao e Contagem 
        IVC* rectangles = cvMatToIVC(frame);

        total_1c += trackAndDraw(blobs_1c, nlabels_1c, tracked_1c, rectangles, colors[0][0], colors[0][1], colors[0][2], "1c", minArea_1c, maxArea_1c);
        total_2c += trackAndDraw(blobs_2c, nlabels_2c, tracked_2c, rectangles, colors[1][0], colors[1][1], colors[1][2], "2c", minArea_2c, maxArea_2c);
        total_5c += trackAndDraw(blobs_5c, nlabels_5c, tracked_5c, rectangles, colors[2][0], colors[2][1], colors[2][2], "5c", minArea_5c, maxArea_5c);

        total_10c += trackAndDraw(blobs_10c, nlabels_10c, tracked_10c, rectangles, colors[3][0], colors[3][1], colors[3][2], "10c", minArea_10c, maxArea_10c);
        total_20c += trackAndDraw(blobs_20c, nlabels_20c, tracked_20c, rectangles, colors[4][0], colors[4][1], colors[4][2], "20c", minArea_20c, maxArea_20c);
        total_50c += trackAndDraw(blobs_50c, nlabels_50c, tracked_50c, rectangles, colors[5][0], colors[5][1], colors[5][2], "50c", minArea_50c, maxArea_50c);

        total_1e += trackAndDraw(blobs_1e, nlabels_1e, tracked_1e, rectangles, colors[6][0], colors[6][1], colors[6][2], "1e", minArea_1e, maxArea_1e);
        total_2e += trackAndDraw(blobs_2e, nlabels_2e, tracked_2e, rectangles, colors[7][0], colors[7][1], colors[7][2], "2e", minArea_2e, maxArea_2e);

        total_dinheiro = (total_1c * 0.01) + (total_2c * 0.02) + (total_5c * 0.05) + (total_10c * 0.10) + (total_20c * 0.20) + (total_50c * 0.50) + (total_1e * 1.00) + (total_2e * 2.00);
        total_moedas = total_1c + total_2c + total_5c + total_10c + total_20c + total_50c + total_1e + total_2e;

        frame = ivcToCvMat(rectangles);
#pragma endregion
#pragma region Anotacoes e Display
        str = "RESOLUCAO: " + std::to_string(video.width) + "x" + std::to_string(video.height);
        cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        str = "TOTAL DE FRAMES: " + std::to_string(video.ntotalframes);
        cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        str = "FRAME RATE: " + std::to_string(video.fps);
        cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        str = "N. DA FRAME: " + std::to_string(video.nframe);
        cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

        str = "Moedas 2 euros: " + std::to_string(total_2e);
        cv::putText(frame, str, cv::Point(20, 130), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        str = "Moedas 1 euro: " + std::to_string(total_1e);
        cv::putText(frame, str, cv::Point(20, 160), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

        str = "Moedas 50 centimos: " + std::to_string(total_50c);
        cv::putText(frame, str, cv::Point(20, 190), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        str = "Moedas 20 centimos: " + std::to_string(total_20c);
        cv::putText(frame, str, cv::Point(20, 220), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        str = "Moedas 10 centimos: " + std::to_string(total_10c);
        cv::putText(frame, str, cv::Point(20, 250), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

        str = "Moedas 5 centimos: " + std::to_string(total_5c);
        cv::putText(frame, str, cv::Point(20, 280), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        str = "Moedas 2 centimos: " + std::to_string(total_2c);
        cv::putText(frame, str, cv::Point(20, 310), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        str = "Moedas 1 centimos: " + std::to_string(total_1c);
        cv::putText(frame, str, cv::Point(20, 340), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

        str = "Dinheiro Total: " + std::to_string(total_dinheiro);
        cv::putText(frame, str, cv::Point(20, 370), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        str = "Total Moedas: " + std::to_string(total_moedas);
        cv::putText(frame, str, cv::Point(20, 400), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

        cv::imshow("VC - VIDEO", frame);
        key = cv::waitKey(1);
        if (key == 'q' || key == 27) break;

#pragma endregion 
#pragma region Limpeza de variaveis
        if (blobs_1c != NULL)
        {
            free(blobs_1c);
            blobs_1c = NULL;
        }
        if (blobs_2c != NULL)
        {
            free(blobs_2c);
            blobs_2c = NULL;
        }
        if (blobs_5c != NULL)
        {
            free(blobs_5c);
            blobs_5c = NULL;
        }
        if (blobs_10c != NULL)
        {
            free(blobs_10c);
            blobs_10c = NULL;
        }
        if (blobs_20c != NULL)
        {
            free(blobs_20c);
            blobs_20c = NULL;
        }
        if (blobs_50c != NULL)
        {
            free(blobs_50c);
            blobs_50c = NULL;
        }
        if (blobs_1e != NULL)
        {
            free(blobs_1e);
            blobs_1e = NULL;
        }
        if (blobs_2e != NULL)
        {
            free(blobs_2e);
            blobs_2e = NULL;
        }

        if (labels_1c) vc_image_free(labels_1c);
        if (labels_2c) vc_image_free(labels_2c);
        if (labels_5c) vc_image_free(labels_5c);
        if (labels_10c) vc_image_free(labels_10c);
        if (labels_20c) vc_image_free(labels_20c);
        if (labels_50c) vc_image_free(labels_50c);
        if (labels_1e) vc_image_free(labels_1e);
        if (labels_2e) vc_image_free(labels_2e);
        vc_image_free(rectangles);
#pragma endregion
    }
    vc_timer();

#pragma region Limpeza Final
    vc_image_free(image_rgb);
    vc_image_free(image_hsv);

    vc_image_free(mask_20c);
    vc_image_free(mask_10c);
    vc_image_free(mask_50c);
    vc_image_free(mask_1c);
    vc_image_free(mask_2c);
    vc_image_free(mask_5c);
    vc_image_free(mask_1eDentro);
    vc_image_free(mask_1eFora);
    vc_image_free(mask_1e);
    vc_image_free(mask_2eDentro);
    vc_image_free(mask_2eFora);
    vc_image_free(mask_2e);
#pragma endregion

    capture.release();
    cv::destroyAllWindows();

    return 0;
}