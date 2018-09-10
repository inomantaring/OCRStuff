//
//  main.cpp
//  OCR
//
//  Created by Rafael Lino Mantaring on 15/03/2018.
//  Copyright © 2018 Rafael Lino Mantaring. All rights reserved.
//

#include <iostream>
#include <unistd.h>

//tesseract imports
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

//open cv imports
#include <opencv2/opencv.hpp>

#define INPUT_FILE "IMG_6800.JPG"

std::vector<cv::Rect> detectLetters(cv::Mat img)
{
    std::vector<cv::Rect> boundRect;
    cv::Mat img_gray, img_sobel, img_threshold, element;
    cvtColor(img, img_gray, CV_BGR2GRAY);
    cv::Sobel(img_gray, img_sobel, CV_8U, 1, 0, 3, 1, 0, cv::BORDER_DEFAULT);
    cv::threshold(img_sobel, img_threshold, 0, 255, CV_THRESH_OTSU+CV_THRESH_BINARY);
    element = getStructuringElement(cv::MORPH_RECT, cv::Size(40,10));
    cv::morphologyEx(img_threshold, img_threshold, CV_MOP_CLOSE, element); //Does the trick
    std::vector< std::vector< cv::Point> > contours;
    cv::findContours(img_threshold, contours, 0, 1);
    std::vector<std::vector<cv::Point> > contours_poly( contours.size() );
    for( int i = 0; i < contours.size(); i++ )
        if (contours[i].size()>100)
        {
            cv::approxPolyDP( cv::Mat(contours[i]), contours_poly[i], 3, true );
            cv::Rect appRect( boundingRect( cv::Mat(contours_poly[i]) ));
            if (appRect.width>appRect.height)
                boundRect.push_back(appRect);
        }
    return boundRect;
}

cv::Mat preProcessImage(cv::Mat img)
{
    cv::Mat img_gray, img_morphed, img_threshold, element;
    cvtColor(img, img_gray, CV_BGR2GRAY);
    
    element = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3,3));
    cv::morphologyEx(img_gray, img_morphed, CV_MOP_OPEN, element);
    cv::threshold(img_morphed, img_threshold, 0, 255, CV_THRESH_OTSU+CV_THRESH_BINARY);
    
    return img_threshold;
}

int main(int argc,char** argv)
{
    std::string filePath = argv[1];
    
    //Read
    cv::Mat img1=cv::imread(filePath);

    //initialize tesseract API
    tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
    if(api->Init(NULL, "eng")){
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }
    
    Pix *image = pixRead((filePath).c_str());
    api->Init(NULL, "eng");
    api->SetImage(image);
    
    std::vector<std::string> results;
    
    //Detect
    std::vector<cv::Rect> letterBBoxes1=detectLetters(img1);
    
    double confidence = 0.0;
    printf("Found %lu textline image components.\n", letterBBoxes1.size());
    for (int i = letterBBoxes1.size()-1; i >= 0; i--) {
        cv::Rect box = letterBBoxes1[i];
        api->SetRectangle(box.x, box.y, box.width, box.height);
        char* ocrResult = api->GetUTF8Text();
        int conf = api->MeanTextConf();
        confidence += conf;
        if(conf < 60) continue;
        results.push_back(ocrResult);
    }
    
    fprintf(stdout, "confidence: %f\n", confidence/letterBBoxes1.size());
    
    api->End();
    pixDestroy(&image);
    return 0;
}
