//
//  Oscilloscope.h
//  interface/src/ui
//
//  Created by Philip on 1/28/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Oscilloscope_h
#define hifi_Oscilloscope_h

#include <cassert>

#include <QtCore/QObject>

class Oscilloscope : public QObject {
    Q_OBJECT
public:
    Oscilloscope(int width, int height, bool isEnabled);
    ~Oscilloscope();

    void render(int x, int y);

    // Switches: On/Off, Stop Time
    volatile bool enabled;
    volatile bool inputPaused;

    // Limits
    static unsigned const MAX_CHANNELS = 3;
    static unsigned const MAX_SAMPLES_PER_CHANNEL = 4096; 

    // Sets the color for a specific channel.
    void setColor(unsigned ch, unsigned rgb) { assert(ch < MAX_CHANNELS); if (! inputPaused) { _colors[ch] = rgb; } }

    // Controls a simple one pole IIR low pass filter that is provided to
    // reduce high frequencies aliasing (to lower ones) when downsampling.
    //
    // The parameter sets the influence of the input in respect to the
    // feed-back signal on the output.
    //
    //                           +---------+
    //         in O--------------|+ ideal  |--o--------------O out
    //                       .---|- op amp |  |    
    //                       |   +---------+  |
    //                       |                |
    //                       o-------||-------o
    //                       |                |
    //                       |              __V__
    //                        -------------|_____|-------+ 
    //                                     :     :       |
    //                                    0.0 - 1.0    (GND)
    //
    // The values in range 0.0 - 1.0 correspond to "all closed" (input has
    // no influence on the output) to "all open" (feedback has no influence
    // on the output) configurations.
    void setLowpassOpenness(float w) { assert(w >= 0.0f && w <= 1.0f); _lowPassCoeff = w; }

    // Sets the number of input samples per output sample. Without filtering
    // just uses every nTh sample.
    void setDownsampleRatio(unsigned n) { assert(n > 0); _downsampleRatio = n; }
public slots:
    void addSamples(const QByteArray& audioByteArray, bool isStereo, bool isInput);
private:
    // don't copy/assign
    Oscilloscope(Oscilloscope const&); // = delete;
    Oscilloscope& operator=(Oscilloscope const&); // = delete;    

    // state variables

    unsigned        _width;
    unsigned        _height;
    short*          _samples;
    short*          _vertices;
    unsigned        _writePos[MAX_CHANNELS];

    float           _lowPassCoeff;
    unsigned        _downsampleRatio;
    unsigned        _colors[MAX_CHANNELS];
};

#endif // hifi_Oscilloscope_h
