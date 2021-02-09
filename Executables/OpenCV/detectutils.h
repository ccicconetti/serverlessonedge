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

#pragma once

#include "opencv2/opencv.hpp"

#include <string>
#include <utility>

namespace uiiit {
namespace edge {
class EdgeClientInterface;
}
} // namespace uiiit

namespace cv {

/**
 * Perform face (and optionally eyes) detection through an edge computing
 * domain.
 *
 * \param aClient The edge client to use.
 * \param aLambdaFaces The lambda function name for face recognition.
 * \param aLambdaEyes The lambda function name for eyes recognition.
 * \param aImg The original image.
 * \param[out] aFaces The faces found, relative to the image size. Cleared upon
 *             call of this method.
 * \param aDetectEyes If true perform eye detection, too.
 * \param[out] aEyes The eyes found, relative to the image size. Cleared upon
 *             call of this method.
 * \param aNormal If true return only faces with two eyes, if aDetectEyes.
 *
 * \return a pair or: the overall time elapsed, in s (always non-negative);
 *         the average last 1-second load reported by the computer.
 *
 * \throw std::runtime_error if anything fails
 */
std::pair<double, unsigned short>
detect(uiiit::edge::EdgeClientInterface& aClient,
       const std::string&                aLambdaFaces,
       const std::string&                aLambdaEyes,
       Mat&                              aImg,
       std::vector<Rect>&                aFaces,
       const bool                        aDetectEyes,
       std::vector<Rect>&                aEyes,
       const bool                        aNormal);

} // namespace cv
