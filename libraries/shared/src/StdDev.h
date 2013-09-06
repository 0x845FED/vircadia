//
//  StdDev.h
//  hifi
//
//  Created by Philip Rosedale on 3/12/13.
//
//

#ifndef __hifi__StdDev__
#define __hifi__StdDev__

class StDev {
    public:
        StDev();
        void reset();
        void addValue(float v);
        float getAverage();
        float getStDev();
        int getSamples() const { return sampleCount; }
    private:
        float * data;
        int sampleCount;
};

#endif /* defined(__hifi__StdDev__) */
