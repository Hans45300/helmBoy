/*
  ==============================================================================

    HelmBoyGraphics.cpp
    Created: 1 Nov 2025 1:43:59pm
    Author:  marcs

  ==============================================================================
*/

#include "HelmBoyGraphics.h"
#include "../look_and_feel/colors.h"
#include <BinaryData.h>

HelmBoyGraphics::HelmBoyGraphics(){
    activeContourColour = Colors::helm_tertiary_accent;
    inactiveContourColour = toGrayScale(activeContourColour);
    setIdentity(HelmBoyGraphics::Identity::logo);
    setActivated(true);
    setSelected(true);

    /*pathData obtenu à partir d'un graphique SVG créé sous inkScape.
    - Exporter au format SVG simple.
    - Récupérer le contenu du fichier à partir de "style" inclu, et la définition qui suit. Exemple :

    style="fill:none;fill-opacity:0;stroke:#000000;stroke-width:0.265;stroke-dasharray:none;stroke-opacity:1"
       d="m 78.131148,83.221277 v 50.665143 c 1.795836,-0.96746 4.017229,-2.2275 3.478352,-4.67265 0.08823,-7.37563 0.176464,-14.75125 0.264696,-22.12688 5.243985,-1.24091 11.547078,-1.90222 15.991565,1.8 2.839069,2.44844 4.397259,6.17874 5.020039,9.76255 -3.746803,9.18852 -7.493603,18.37703 -11.240404,27.56555 -0.321245,0.7878 0.08343,1.24101 0.885497,0.95761 8.527657,-3.01313 15.061157,-10.42454 17.984257,-18.89345 4.6449,-12.20009 0.76168,-26.73742 -8.67974,-35.570732 -6.302568,-6.056872 -14.960489,-9.530205 -23.704262,-9.487141 z"

    - Ouvrir le projucer, et choisir "Tools > SVG Path Converter"
    - Coller dans la partie supérieure le code récupéré du fichier SVG
    - Cliquer sur "Copy" pour récupérer le path défini par l'outil (on a un aperçu)
    - le coller dans un static const unsigned char [], comme ci-dessous.
    */

    static const unsigned char logoPathData[] = {110, 109, 203, 81, 129, 67, 12, 162, 6, 67, 98, 203, 81, 129, 67, 75, 247, 228, 66, 150, 99, 118, 67, 102, 166, 195, 66, 31, 165, 102, 67, 112, 253, 190, 66, 98, 142, 23, 100, 67, 194, 53, 185, 66, 25, 228, 96, 67, 165, 155, 180, 66, 221, 68, 93, 67, 83, 163, 177, 66, 108, 221, 68, 93, 67, 175, 114, 175, 66, 98, 221, 68,
                                             93, 67, 194, 181, 131, 66, 244, 189, 84, 67, 151, 25, 47, 66, 190, 223, 69, 67, 242, 40, 217, 65, 98, 23, 57, 54, 67, 113, 61, 26, 65, 119, 158, 33, 67, 0, 0, 0, 0, 100, 219, 11, 67, 0, 0, 0, 0, 98, 162, 48, 236, 66, 0, 0, 0, 0, 231, 251, 194, 66, 113, 61, 26, 65, 158, 175, 163, 66, 246, 40, 217, 65, 98, 52, 243, 133, 66, 154,
                                             25, 47, 66, 193, 202, 105, 66, 194, 181, 131, 66, 193, 202, 105, 66, 176, 114, 175, 66, 108, 193, 202, 105, 66, 215, 163, 177, 66, 98, 218, 78, 91, 66, 166, 155, 180, 66, 7, 129, 78, 66, 70, 54, 185, 66, 193, 74, 68, 66, 113, 253, 190, 66, 98, 230, 80, 5, 66, 228, 165, 195, 66, 204, 161, 168, 65, 201, 246, 228, 66, 204,
                                             161, 168, 65, 78, 162, 6, 67, 98, 204, 161, 168, 65, 121, 201, 26, 67, 230, 80, 5, 66, 45, 114, 43, 67, 200, 75, 68, 66, 102, 198, 45, 67, 98, 98, 101, 80, 66, 241, 50, 49, 67, 204, 33, 96, 66, 79, 205, 51, 67, 251, 254, 113, 66, 104, 49, 53, 67, 108, 251, 254, 113, 66, 206, 87, 62, 67, 98, 251, 254, 113, 66, 91, 207, 73,
                                             67, 130, 21, 138, 66, 104, 145, 85, 67, 222, 228, 159, 66, 40, 28, 89, 67, 108, 80, 141, 246, 66, 26, 47, 103, 67, 98, 109, 7, 0, 67, 28, 186, 104, 67, 166, 91, 4, 67, 157, 175, 110, 67, 166, 91, 4, 67, 26, 175, 115, 67, 108, 166, 91, 4, 67, 198, 27, 136, 67, 98, 166, 91, 4, 67, 243, 45, 138, 67, 10, 183, 7, 67, 198, 219, 139,
                                             67, 166, 219, 11, 67, 198, 219, 139, 67, 98, 66, 0, 16, 67, 198, 219, 139, 67, 166, 91, 19, 67, 243, 45, 138, 67, 166, 91, 19, 67, 198, 27, 136, 67, 108, 166, 91, 19, 67, 25, 175, 115, 67, 98, 166, 91, 19, 67, 156, 175, 110, 67, 33, 176, 23, 67, 93, 186, 104, 67, 105, 113, 28, 67, 25, 47, 103, 67, 108, 156, 196, 71, 67, 39,
                                             28, 89, 67, 98, 205, 172, 82, 67, 102, 145, 85, 67, 207, 55, 91, 67, 90, 207, 73, 67, 207, 55, 91, 67, 205, 87, 62, 67, 108, 207, 55, 91, 67, 103, 49, 53, 67, 98, 27, 175, 95, 67, 78, 205, 51, 67, 53, 158, 99, 67, 175, 50, 49, 67, 31, 165, 102, 67, 101, 198, 45, 67, 98, 150, 99, 118, 67, 104, 113, 43, 67, 203, 81, 129, 67,
                                             246, 200, 26, 67, 203, 81, 129, 67, 12, 162, 6, 67, 99, 109, 16, 184, 83, 67, 248, 83, 39, 67, 98, 92, 15, 78, 67, 248, 83, 39, 67, 122, 116, 73, 67, 88, 185, 34, 67, 122, 116, 73, 67, 230, 16, 29, 67, 108, 122, 116, 73, 67, 102, 102, 224, 66, 98, 122, 116, 73, 67, 129, 21, 213, 66, 91, 15, 78, 67, 65, 224, 203, 66, 16, 184,
                                             83, 67, 65, 224, 203, 66, 98, 131, 96, 89, 67, 65, 224, 203, 66, 34, 251, 93, 67, 128, 21, 213, 66, 34, 251, 93, 67, 102, 102, 224, 66, 108, 34, 251, 93, 67, 229, 16, 29, 67, 98, 225, 250, 93, 67, 154, 185, 34, 67, 131, 96, 89, 67, 248, 83, 39, 67, 16, 184, 83, 67, 248, 83, 39, 67, 99, 109, 100, 219, 11, 67, 0, 0, 112, 65,
                                             98, 164, 144, 29, 67, 0, 0, 112, 65, 217, 46, 46, 67, 184, 30, 188, 65, 53, 94, 58, 67, 18, 131, 19, 66, 98, 170, 209, 43, 67, 3, 86, 227, 65, 184, 222, 26, 67, 190, 159, 184, 65, 100, 219, 11, 67, 190, 159, 184, 65, 98, 32, 176, 249, 66, 190, 159, 184, 65, 192, 202, 215, 66, 247, 83, 227, 65, 170, 177, 186, 66, 18, 131,
                                             19, 66, 98, 223, 15, 211, 66, 184, 30, 188, 65, 205, 76, 244, 66, 0, 0, 112, 65, 100, 219, 11, 67, 0, 0, 112, 65, 99, 109, 250, 254, 135, 66, 248, 83, 39, 67, 98, 41, 92, 121, 66, 248, 83, 39, 67, 170, 241, 102, 66, 88, 185, 34, 67, 170, 241, 102, 66, 230, 16, 29, 67, 108, 170, 241, 102, 66, 102, 102, 224, 66, 98, 170, 241,
                                             102, 66, 129, 21, 213, 66, 41, 92, 121, 66, 65, 224, 203, 66, 250, 254, 135, 66, 65, 224, 203, 66, 98, 98, 80, 147, 66, 65, 224, 203, 66, 37, 134, 156, 66, 128, 21, 213, 66, 37, 134, 156, 66, 102, 102, 224, 66, 108, 37, 134, 156, 66, 229, 16, 29, 67, 98, 37, 134, 156, 66, 154, 185, 34, 67, 98, 80, 147, 66, 248, 83, 39, 67,
                                             250, 254, 135, 66, 248, 83, 39, 67, 99, 109, 170, 241, 42, 66, 184, 158, 230, 66, 108, 170, 241, 42, 66, 63, 245, 25, 67, 98, 66, 224, 26, 66, 110, 114, 21, 67, 223, 79, 16, 66, 30, 101, 14, 67, 223, 79, 16, 66, 12, 162, 6, 67, 98, 223, 79, 16, 66, 244, 189, 253, 66, 72, 225, 26, 66, 215, 163, 239, 66, 170, 241, 42, 66, 184,
                                             158, 230, 66, 99, 109, 78, 34, 67, 67, 141, 215, 74, 67, 108, 27, 207, 23, 67, 127, 234, 88, 67, 98, 209, 66, 19, 67, 156, 100, 90, 67, 19, 35, 15, 67, 158, 79, 93, 67, 232, 219, 11, 67, 43, 7, 97, 67, 98, 189, 148, 8, 67, 158, 79, 93, 67, 64, 117, 4, 67, 156, 100, 90, 67, 237, 209, 255, 66, 127, 234, 88, 67, 108, 254, 41,
                                             169, 66, 141, 215, 74, 67, 98, 242, 167, 159, 66, 73, 76, 73, 67, 127, 255, 150, 66, 10, 87, 67, 67, 127, 255, 150, 66, 75, 87, 62, 67, 108, 127, 255, 150, 66, 229, 48, 53, 67, 98, 219, 142, 171, 66, 47, 253, 49, 67, 170, 134, 186, 66, 143, 98, 40, 67, 170, 134, 186, 66, 229, 16, 29, 67, 108, 170, 134, 186, 66, 102, 102,
                                             224, 66, 98, 170, 134, 186, 66, 55, 73, 200, 66, 64, 138, 169, 66, 116, 19, 180, 66, 242, 231, 146, 66, 116, 19, 175, 66, 98, 54, 243, 146, 66, 188, 244, 156, 66, 4, 107, 150, 66, 71, 97, 139, 66, 101, 144, 156, 66, 189, 31, 118, 66, 98, 95, 15, 185, 66, 126, 234, 53, 66, 62, 159, 237, 66, 222, 79, 4, 66, 167, 219, 11, 67,
                                             222, 79, 4, 66, 98, 241, 231, 32, 67, 222, 79, 4, 66, 224, 47, 59, 67, 126, 234, 53, 66, 28, 111, 73, 67, 189, 31, 118, 66, 98, 14, 130, 76, 67, 71, 97, 139, 66, 180, 61, 78, 67, 188, 244, 156, 66, 151, 67, 78, 67, 116, 19, 175, 66, 98, 178, 242, 66, 67, 247, 19, 180, 66, 124, 116, 58, 67, 186, 73, 200, 66, 124, 116, 58, 67,
                                             102, 102, 224, 66, 108, 124, 116, 58, 67, 229, 16, 29, 67, 98, 124, 116, 58, 67, 143, 98, 40, 67, 34, 240, 65, 67, 47, 253, 49, 67, 208, 55, 76, 67, 229, 48, 53, 67, 108, 208, 55, 76, 67, 75, 87, 62, 67, 98, 207, 55, 76, 67, 10, 87, 67, 67, 150, 227, 71, 67, 74, 76, 73, 67, 78, 34, 67, 67, 141, 215, 74, 67, 99, 109, 225, 250,
                                             108, 67, 254, 244, 25, 67, 108, 225, 250, 108, 67, 184, 158, 230, 66, 98, 59, 255, 112, 67, 215, 163, 239, 66, 84, 163, 115, 67, 118, 190, 253, 66, 84, 163, 115, 67, 203, 161, 6, 67, 98, 150, 163, 115, 67, 31, 101, 14, 67, 59, 255, 112, 67, 45, 114, 21, 67, 225, 250, 108, 67, 254, 244, 25, 67, 99, 101, 0, 0};

    static const unsigned char cnxPathData[] = {110, 109, 0, 0, 128, 67, 0, 0, 0, 0, 98, 94, 58, 229, 66, 0, 0, 0, 0, 0, 0, 0, 0, 88, 57, 229, 66, 0, 0, 0, 0, 0, 0, 128, 67, 98, 0, 0, 0, 0, 170, 177, 198, 67, 94, 58, 229, 66, 0, 0, 0, 68, 0, 0, 128, 67, 0, 0, 0, 68, 98, 104, 177, 198, 67, 0, 0, 0, 68, 0, 0, 0, 68, 170, 177, 198, 67, 0, 0, 0, 68, 0, 0, 128, 67, 98, 0, 0, 0, 68, 88, 57, 229,
                                             66, 104, 177, 198, 67, 0, 0, 0, 0, 0, 0, 128, 67, 0, 0, 0, 0, 99, 109, 0, 0, 128, 67, 109, 119, 247, 67, 98, 123, 20, 248, 66, 109, 119, 247, 67, 55, 137, 136, 65, 68, 251, 193, 67, 55, 137, 136, 65, 0, 0, 128, 67, 98, 55, 137, 136, 65, 242, 18, 248, 66, 123, 20, 248, 66, 55, 137, 136, 65, 0, 0, 128, 67, 55, 137, 136, 65, 98,
                                             225, 250, 193, 67, 55, 137, 136, 65, 109, 119, 247, 67, 242, 18, 248, 66, 109, 119, 247, 67, 0, 0, 128, 67, 98, 109, 119, 247, 67, 68, 251, 193, 67, 225, 250, 193, 67, 109, 119, 247, 67, 0, 0, 128, 67, 109, 119, 247, 67, 99, 109, 0, 0, 128, 67, 205, 204, 76, 66, 98, 25, 228, 14, 67, 205, 204, 76, 66, 205, 204, 76, 66, 221,
                                             228, 14, 67, 205, 204, 76, 66, 0, 0, 128, 67, 98, 205, 204, 76, 66, 146, 141, 184, 67, 25, 228, 14, 67, 102, 102, 230, 67, 0, 0, 128, 67, 102, 102, 230, 67, 98, 244, 141, 184, 67, 102, 102, 230, 67, 102, 102, 230, 67, 145, 141, 184, 67, 102, 102, 230, 67, 0, 0, 128, 67, 98, 102, 102, 230, 67, 222, 228, 14, 67, 244, 141,
                                             184, 67, 205, 204, 76, 66, 0, 0, 128, 67, 205, 204, 76, 66, 99, 109, 0, 0, 128, 67, 211, 221, 221, 67, 98, 39, 81, 24, 67, 211, 221, 221, 67, 180, 136, 136, 66, 10, 215, 179, 67, 180, 136, 136, 66, 0, 0, 128, 67, 98, 180, 136, 136, 66, 236, 81, 24, 67, 39, 81, 24, 67, 180, 136, 136, 66, 0, 0, 128, 67, 180, 136, 136, 66, 98,
                                             108, 215, 179, 67, 180, 136, 136, 66, 211, 221, 221, 67, 170, 81, 24, 67, 211, 221, 221, 67, 0, 0, 128, 67, 98, 211, 221, 221, 67, 43, 215, 179, 67, 109, 215, 179, 67, 211, 221, 221, 67, 0, 0, 128, 67, 211, 221, 221, 67, 99, 109, 0, 0, 128, 67, 193, 170, 42, 67, 98, 53, 222, 80, 67, 193, 170, 42, 67, 192, 170, 42, 67, 54,
                                             222, 80, 67, 192, 170, 42, 67, 0, 0, 128, 67, 98, 192, 170, 42, 67, 229, 144, 151, 67, 53, 222, 80, 67, 160, 170, 170, 67, 0, 0, 128, 67, 160, 170, 170, 67, 98, 230, 144, 151, 67, 160, 170, 170, 67, 160, 170, 170, 67, 230, 144, 151, 67, 160, 170, 170, 67, 0, 0, 128, 67, 98, 160, 170, 170, 67, 53, 222, 80, 67, 229, 144, 151,
                                             67, 193, 170, 42, 67, 0, 0, 128, 67, 193, 170, 42, 67, 99, 109, 0, 0, 128, 67, 45, 34, 162, 67, 98, 68, 75, 90, 67, 45, 34, 162, 67, 166, 187, 59, 67, 127, 218, 146, 67, 166, 187, 59, 67, 0, 0, 128, 67, 98, 166, 187, 59, 67, 68, 75, 90, 67, 2, 75, 90, 67, 166, 187, 59, 67, 0, 0, 128, 67, 166, 187, 59, 67, 98, 94, 218, 146, 67,
                                             166, 187, 59, 67, 45, 34, 162, 67, 2, 75, 90, 67, 45, 34, 162, 67, 0, 0, 128, 67, 98, 45, 34, 162, 67, 94, 218, 146, 67, 94, 218, 146, 67, 45, 34, 162, 67, 0, 0, 128, 67, 45, 34, 162, 67, 99, 101, 0, 0};



    static const juce::String modulationCircleSvgPath(
        "M 12,0 C 5.367,0 0,5.367 0,12 0,18.633 5.367,24 12,24 "
        "18.633,24 24,18.633 24,12 24,5.367 18.633,0 12,0");
    static const juce::String modulationSymbolSvgPath(
        "M 19.683,16.283 C 18.45,16.217 17.833,14.75 17.533,13.85 "
        "17.267,13.083 16.867,11.733 16.567,10.917 16.217,9.967 "
        "16.117,9.9 15.917,9.9 c -0.417,0 -0.734,1.183 -1.05,2.067 "
        "-0.667,1.833 -1.167,3.85 -2.934,3.85 -1.533,0 -2.216,-1.184 "
        "-2.7,-1.884 C 8.783,13.267 8.517,13.117 8.1,13.117 "
        "7.567,13.117 7.317,13.583 6.833,14.4 6.55,14.867 6.233,15.35 "
        "5.867,15.667 5.767,15.75 4.933,16.4 4.15,16.3 "
        "3.7,16.233 3.383,15.967 3.383,15.517 c -0.617,0 0.684,-0.734 "
        "1.067,-0.884 0.333,-0.116 0.733,-0.716 0.933,-1.05 "
        "0.534,-0.916 1.217,-2.116 2.75,-2.116 1.35,0 2,0.866 2.5,1.55 "
        "0.45,0.616 0.717,1.116 1.234,1.133 0.433,0.017 1.033,-1.617 "
        "1.383,-2.75 0.533,-1.733 1.233,-3.333 2.633,-3.333 "
        "1.884,0 2.434,2.633 3.017,4.65 0.083,0.3 0.283,0.933 0.333,1.016 "
        "0.267,0.567 0.484,0.934 0.717,1.05 0.267,0.15 0.7,0.434 "
        "0.567,0.934 -0.084,0.383 -0.434,0.583 -0.834,0.566");
    static const juce::String ABComparisonSvgPath("M10.55 21H4.5c-0.4 0 -0.75 -0.15 -1.05 -0.45 -0.3 -0.3 -0.45 -0.65 -0.45 -1.05V4.5c0 -0.4 0.15 -0.75 0.45 -1.05C3.75 3.15 4.1 3 4.5 3h6.05V1.75c0 -0.2125 0.07235 -0.390665 0.217 -0.5345 0.1445 -0.143665 0.32365 -0.2155 0.5375 -0.2155 0.21365 0 0.39135 0.071835 0.533 0.2155 0.14165 0.143835 0.2125 0.322 0.2125 0.5345v20.5c0 0.2125 -0.07235 0.3906 -0.217 0.53425 -0.1445 0.14385 -0.32365 0.21575 -0.5375 0.21575 -0.21365 0 -0.39135 -0.0719 -0.533 -0.21575 -0.14165 -0.14365 -0.2125 -0.32175 -0.2125 -0.53425V21ZM4.5 18.45h6.05V11.525L4.5 18.45ZM13.55 21V11.625L19.5 18.45V4.5H13.55V3H19.5c0.4 0 0.75 0.15 1.05 0.45 0.3 0.3 0.45 0.65 0.45 1.05v15c0 0.4 -0.15 0.75 -0.45 1.05 -0.3 0.3 -0.65 0.45 -1.05 0.45H13.55Z");
    static const juce::String returnToInitSvgPath("m14.5,5c3.87,0 7,3.13 7,7 0,3.87-3.13,7-7,7-3.87,0-7-3.13-7-7H10L6.5,8.5 3,12h2.5c0,4.97 4.03,9 9,9 4.97,0 9-4.03 9-9 0-4.97-4.03-9-9-9z");
    static const juce::String previousSvgPath("M15.41 16.59L10.83 12L15.41 7.41L14 6L8 12L14 18Z");
    static const juce::String nextSvgPath("M8.59 16.59L13.17 12L8.59 7.41L10 6L16 12L10 18Z");
    static const juce::String HelmBoyFaceSvgPath("M4 12C4 7.58 7.58 4 12 4C16.42 4 20 7.58 20 12V18H16V12H18C18 8.69 15.31 6 12 6C8.69 6 6 8.69 6 12H8V18H4ZM9 8H11V11H13V8H15V16H13V13H11V16H9Z");

    logoPath = juce::Drawable::parseSVGPath(HelmBoyFaceSvgPath);
    cnxPath.loadPathFromData(cnxPathData, sizeof(cnxPathData));
    modulationCirclePath = juce::Drawable::parseSVGPath(modulationCircleSvgPath);
    modulationSymbolPath = juce::Drawable::parseSVGPath(modulationSymbolSvgPath);
    ABComparisonPath = juce::Drawable::parseSVGPath(ABComparisonSvgPath);
    returnToInitPath = juce::Drawable::parseSVGPath(returnToInitSvgPath);
    previousPath = juce::Drawable::parseSVGPath(previousSvgPath);
    nextPath = juce::Drawable::parseSVGPath(nextSvgPath);
    HelmBoyFacePath = juce::Drawable::parseSVGPath(HelmBoyFaceSvgPath);
}

void HelmBoyGraphics::paint(juce::Graphics& g)
{
    juce::Colour currentContourColour;

    auto bounds = getLocalBounds().toFloat();
    float faceDiameter = 0.8f * std::min(bounds.getWidth(), bounds.getHeight());
    float faceX = (bounds.getWidth() - faceDiameter) / 2.0f;
    float faceY = (bounds.getHeight() - faceDiameter) / 2.0f;
    float scaleX;
    float scaleY;
    juce::Rectangle<float> faceArea(faceX, faceY, faceDiameter, faceDiameter);

    if (identity == Identity::logo)
    {
        currentContourColour = Colors::amethyst;

        auto logoPathBounds = logoPath.getBounds();
        scaleX = faceArea.getWidth() / logoPathBounds.getWidth();
        scaleY = faceArea.getHeight() / logoPathBounds.getHeight();

        // Calculer la translation pour centrer le logo
        float centeredX = faceX + (faceArea.getWidth() - logoPathBounds.getWidth() * scaleX) / 2.0f - logoPathBounds.getX() * scaleX;
        float centeredY = faceY + (faceArea.getHeight() - logoPathBounds.getHeight() * scaleY) / 2.0f - logoPathBounds.getY() * scaleY;

        juce::AffineTransform transform = juce::AffineTransform::scale(scaleX, scaleY)
            .translated(centeredX, centeredY);

        logoPath.applyTransform(transform);
        g.setColour(currentContourColour);
        g.strokePath(logoPath, juce::PathStrokeType(1.0f));
    }
    else if (identity == Identity::ModulationLegacy) {
        if (active) {
            currentContourColour = Colors::topaze;
        }
        else {
            currentContourColour = inactiveContourColour;
        }
        auto cnxPathBounds = cnxPath.getBounds();
        scaleX = faceArea.getWidth() / cnxPathBounds.getWidth();
        scaleY = faceArea.getHeight() / cnxPathBounds.getHeight();

        // Calculer la translation pour centrer le cnx
        float centeredX = faceX + (faceArea.getWidth() - cnxPathBounds.getWidth() * scaleX) / 2.0f - cnxPathBounds.getX() * scaleX;
        float centeredY = faceY + (faceArea.getHeight() - cnxPathBounds.getHeight() * scaleY) / 2.0f - cnxPathBounds.getY() * scaleY;

        juce::AffineTransform transform = juce::AffineTransform::scale(scaleX, scaleY)
            .translated(centeredX, centeredY);
        cnxPath.applyTransform(transform);
        g.setColour(currentContourColour);
        g.strokePath(cnxPath, juce::PathStrokeType(1.0f));
        if (selected) {
            // Créer une copie du path pour le centrage
            juce::Path innerCnxPath = cnxPath;
            auto cnxTransformedBounds = cnxPath.getBounds();

            // Calculer l'échelle pour réduire de moitié
            float innerScaleX = 0.5f;
            float innerScaleY = 0.5f;

            // Calculer la position pour centrer le path intérieur sur le path extérieur
            float innerCenteredX = cnxTransformedBounds.getCentreX() - (cnxTransformedBounds.getWidth() * innerScaleX) / 2.0f;
            float innerCenteredY = cnxTransformedBounds.getCentreY() - (cnxTransformedBounds.getHeight() * innerScaleY) / 2.0f;

            // Appliquer la transformation au path intérieur
            juce::AffineTransform innerTransform = juce::AffineTransform::scale(innerScaleX, innerScaleY,
                                                                                 cnxTransformedBounds.getCentreX(),
                                                                                 cnxTransformedBounds.getCentreY());
            innerCnxPath.applyTransform(innerTransform);

            g.setColour(Colors::emerald);
            g.strokePath(innerCnxPath, juce::PathStrokeType(1.0f));
        }
    }
    else {
        paintModulationControl(g, getLocalBounds().toFloat(), selected, active);
    }

}

void HelmBoyGraphics::drawModulationControl(Graphics& g,
                                            Rectangle<float> bounds,
                                            bool selected, bool active) {
    static const HelmBoyGraphics pathSource;
    pathSource.paintModulationControl(g, bounds, selected, active);
}

void HelmBoyGraphics::paintModulationControl(Graphics& g,
                                             Rectangle<float> bounds,
                                             bool selected, bool active) const {
    const float face_diameter = 0.8f * std::min(bounds.getWidth(), bounds.getHeight());
    const float face_x = bounds.getX() + (bounds.getWidth() - face_diameter) / 2.0f;
    const float face_y = bounds.getY() + (bounds.getHeight() - face_diameter) / 2.0f;
    const auto circle_bounds = modulationCirclePath.getBounds();
    const float scale_x = face_diameter / circle_bounds.getWidth();
    const float scale_y = face_diameter / circle_bounds.getHeight();
    const float centered_x = face_x - circle_bounds.getX() * scale_x;
    const float centered_y = face_y - circle_bounds.getY() * scale_y;
    const auto transform = AffineTransform::scale(scale_x, scale_y)
                               .translated(centered_x, centered_y);

    Path circle = modulationCirclePath;
    circle.applyTransform(transform);
    g.setColour(active ? Colour(Colors::Color_ff000000) : Colour(0xff444444));
    g.fillPath(circle);
    g.setColour(active ? Colors::topaze : Colour(0xff555555));
    g.strokePath(circle, PathStrokeType(1.0f));

    Path symbol = modulationSymbolPath;
    const auto symbol_bounds = symbol.getBounds();
    const auto circle_centre = circle_bounds.getCentre();
    const auto symbol_centre = symbol_bounds.getCentre();
    symbol.applyTransform(AffineTransform::translation(
        circle_centre.x - symbol_centre.x, circle_centre.y - symbol_centre.y));
    symbol.applyTransform(transform);
    g.setColour(selected ? Colors::emerald : inactiveContourColour);
    g.fillPath(symbol);
    g.strokePath(symbol, PathStrokeType(1.0f));
}

// On appelle la fonction par "HelmGraphicObject.setIdentity(HelmBoyGraphics::Identity::Modulation);" par exemple.
void HelmBoyGraphics::setIdentity(Identity newIdentity)
{
    identity = newIdentity;
    repaint();
}

void HelmBoyGraphics::setForceSquareBounds(bool forceSquare)
{
    forceSquareBounds = forceSquare;
    resized();
}

void HelmBoyGraphics::resized()
{
    if (forceSquareBounds)
    {
        auto bounds = getBounds();
        int size = std::min(bounds.getWidth(), bounds.getHeight());
        setBounds(bounds.getX(), bounds.getY(), size, size);
    }
}

void HelmBoyGraphics::setSelected(bool isSelected)
{
    selected = isSelected;
    repaint();
}

void HelmBoyGraphics::setActivated(bool isActive)
{
    active = isActive;
}

bool HelmBoyGraphics::isActivated() const
{
    return active;
}

juce::Path HelmBoyGraphics::getIconPath(Icon icon) const
{
    const auto getBundledSvgPath = [](const char* resourceName) {
        int dataSize = 0;
        const char* data = BinaryData::getNamedResource(resourceName, dataSize);
        if (data == nullptr || dataSize <= 0)
            return juce::Path{};

        auto drawable = juce::Drawable::createFromImageData(data, static_cast<size_t>(dataSize));
        return drawable != nullptr ? drawable->getOutlineAsPath() : juce::Path{};
    };

    switch (icon) {
        case Icon::Previous: return previousPath;
        case Icon::Next: return nextPath;
        case Icon::Init: return returnToInitPath;
        case Icon::ABComparison: return ABComparisonPath;
        case Icon::PatchSave: return getBundledSvgPath("patch_save_svg");
        case Icon::PatchSaveAs: return getBundledSvgPath("patch_save_as_svg");
        case Icon::PatchExport: return getBundledSvgPath("patch_export_svg");
        case Icon::PatchBrowse: return getBundledSvgPath("patch_browse_svg");
        case Icon::PatchEdit: return getBundledSvgPath("patch_edit_svg");
        case Icon::PatchUndo: {
            juce::Path undoPath = getBundledSvgPath("patch_redo_svg");
            const auto bounds = undoPath.getBounds();
            undoPath.applyTransform(juce::AffineTransform::scale(-1.0f, 1.0f)
                                        .translated(bounds.getX() + bounds.getRight(), 0.0f));
            return undoPath;
        }
        case Icon::PatchRedo: return getBundledSvgPath("patch_redo_svg");
    }
    return {};
}

juce::Colour HelmBoyGraphics::toGrayScale(const juce::Colour& colour)
{
    int r = colour.getRed();
    int g = colour.getGreen();
    int b = colour.getBlue();

    juce::uint8 gray = static_cast<juce::uint8>(0.299f * r + 0.587f * g + 0.114f * b);
    return juce::Colour( gray, gray, gray);
}

juce::Image HelmBoyGraphics::createModulationImage(int size, bool selected, bool active)
{
    juce::Image image(juce::Image::ARGB, size, size, true);
    juce::Graphics g(image);

    HelmBoyGraphics graphics;
    graphics.setIdentity(Identity::Modulation);
    graphics.setSelected(selected);
    graphics.setActivated(active);
    graphics.setBounds(0, 0, size, size);

    graphics.paint(g);

    return image;
}

juce::Image HelmBoyGraphics::createLegacyModulationImage(int size, bool selected, bool active)
{
    juce::Image image(juce::Image::ARGB, size, size, true);
    juce::Graphics g(image);

    HelmBoyGraphics graphics;
    graphics.setIdentity(Identity::ModulationLegacy);
    graphics.setSelected(selected);
    graphics.setActivated(active);
    graphics.setBounds(0, 0, size, size);
    graphics.paint(g);

    return image;
}

void HelmBoyGraphics::mouseDown(const juce::MouseEvent& event)
{
    if (onClick)
        onClick();
}
