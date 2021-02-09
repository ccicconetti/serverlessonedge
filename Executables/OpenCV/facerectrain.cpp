/*
              __ __ __
             |__|__|  | __
             |  |  |  ||__|
  ___ ___ __ |  |  |  |
 |   |   |  ||  |  |  |    Ubiquitous Internet @ IIT-CNR
 |   |   |  ||  |  |  |    C++ edge computing libraries and tools
 |_______|__||__|__|__|    https://github.com/ccicconetti/serverlessonedge

Licensed under the MIT License <http://opensource.org/licenses/MIT>
Copyright (c) 2021 C. Cicconetti <https://ccicconetti.github.io/>

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
 * Copyright (c) 2011. Philipp Wagner <bytefish[at]gmx[dot]de>.
 * Released to public domain under terms of the BSD Simplified license.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the organization nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 *   See <http://www.opensource.org/licenses/bsd-license>
 */

#include "opencv2/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/face.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using namespace cv;
using namespace cv::face;
using namespace std;

static void read_csv(const string&          filename,
                     vector<Mat>&           images,
                     vector<int>&           labels,
                     std::map<int, string>& labelsInfo,
                     char                   separator = ';') {
  ifstream csv(filename.c_str());
  if (!csv)
    CV_Error(Error::StsBadArg,
             "No valid input file was given, please check the given filename.");
  string line, path, classlabel, info;
  while (getline(csv, line)) {
    stringstream liness(line);
    path.clear();
    classlabel.clear();
    info.clear();
    getline(liness, path, separator);
    getline(liness, classlabel, separator);
    getline(liness, info, separator);
    if (!path.empty() && !classlabel.empty()) {
      cout << "Processing " << path << endl;
      int label = atoi(classlabel.c_str());
      if (!info.empty())
        labelsInfo.insert(std::make_pair(label, info));
      // 'path' can be file, dir or wildcard path
      String         root(path.c_str());
      vector<String> files;
      glob(root, files, true);
      for (vector<String>::const_iterator f = files.begin(); f != files.end();
           ++f) {
        cout << "\t" << *f << endl;
        Mat         img                  = imread(*f, IMREAD_GRAYSCALE);
        static bool showSmallSizeWarning = true;
        if (showSmallSizeWarning && (img.cols < 50 || img.rows < 50)) {
          cout << "* Warning: for better results images should be not smaller "
                  "than 50x50!"
               << endl;
          showSmallSizeWarning = false;
        }
        images.push_back(img);
        labels.push_back(label);
      }
    }
  }
}

int main(int argc, const char* argv[]) {
  // Check for valid command line arguments, print usage
  // if no arguments were given.
  if (argc != 2) {
    cout << "Usage: " << argv[0] << " <csv>\n"
         << "\t<csv> - path to config file in CSV format\n"
         << "The CSV config file consists of the following lines:\n"
         << "<path>;<label>[;<comment>]\n"
         << "\t<path> - file, dir or wildcard path\n"
         << "\t<label> - non-negative integer person label\n"
         << "\t<comment> - optional comment string (e.g. person name)" << endl;
    exit(1);
  }
  // Get the path to your CSV.
  string fn_csv = string(argv[1]);
  // These vectors hold the images and corresponding labels.
  vector<Mat>           images;
  vector<int>           labels;
  std::map<int, string> labelsInfo;
  // Read in the data. This can fail if no valid
  // input filename is given.
  try {
    read_csv(fn_csv, images, labels, labelsInfo);
  } catch (cv::Exception& e) {
    cerr << "Error opening file \"" << fn_csv << "\". Reason: " << e.msg
         << endl;
    // nothing more we can do
    exit(1);
  }

  // Quit if there are not enough images for this demo.
  if (images.empty()) {
    string error_message = "No images in the training dataset";
    CV_Error(Error::StsError, error_message);
  }

  Ptr<EigenFaceRecognizer> model = EigenFaceRecognizer::create(10);
  for (size_t i = 0; i < labels.size(); i++) {
    model->setLabelInfo(i, labelsInfo[i]);
  }
  model->train(images, labels);
  string saveModelPath = "face-rec-model.txt";
  cout << "Saving the trained model to " << saveModelPath << endl;
  model->save(saveModelPath);

  return 0;
}
