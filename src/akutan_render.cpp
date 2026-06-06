/**
 * @file akutan_render.cpp
 * @brief Seismic Data Sonification engine for Akutan Volcano Data.
 * 
 * This program implements a granular synthesis engine using the Synthesis Tool Kit (STK)
 * to sonify volcanic seismic data (1997-2017). It processes CSV-formatted seismic
 * parameters and maps them to real time audio-synthesis modules including
 * Granulate, BiQuad Filters, and Pitch Shifting.
 * *@author Jacob Mitani
 * @date 2026-06-06
 * @project MUS 410 - University of Oregon
 * * @note Dataset provided by Hannah Mark (Lamont Assistant Research Professor, 
 * Marine & Polar Geophysics).
 *
 * @dependencies
 * - Synthesis ToolKit (STK)
 * - Standard C++17 library
 */


#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>

#include <Stk.h>
#include <Granulate.h>
#include <BiQuad.h>
#include <PitShift.h>
#include <FileWvOut.h>

const double PI = 3.141592653589793;

class AleutianSonifier {
private:
    stk::Granulate granulator;
    stk::BiQuad filter;
    stk::PitShift pitchShifter;
    
    float currentGain; 

public:
    AleutianSonifier(const std::string& sourceAudioPath) {
        try {
            granulator.openFile(sourceAudioPath);
        } catch (stk::StkError &error) {
            error.printMessage();
            exit(1);
        }
        granulator.setVoices(12);
        granulator.setStretch(1);
    }

    void updateParameters(double apf, double fi, double mag, double depth, 
                          double lon, double skew, double kurt) {
        
        
        int grainDurationMs = 40 + static_cast<int>(mag * 250.0);
        int rampPercent = 5 + static_cast<int>(depth * 45.0);
        int delayMs = static_cast<int>((1.0 - fi) * 50.0);
        int offsetMs = static_cast<int>(fi * 20.0);
        
        granulator.setGrainParameters(grainDurationMs, rampPercent, offsetMs, delayMs);

        
        int stretch = 1 + static_cast<int>(apf * 10.0);
        granulator.setStretch(stretch);


        stk::StkFloat cutoffFreq = 100 + (skew * 8000.0);
        
        stk::StkFloat resonance = 0.5 + (kurt * 0.42);
        
        filter.setResonance(cutoffFreq, resonance, true);

        this->currentGain = static_cast<float>(apf) * 0.4f + 0.1f;
    }


    void setPitchShift(double ratio) {
        pitchShifter.setShift(ratio);
    }

    void tick(stk::StkFrames& frame) {

        stk::StkFloat monoSample = granulator.tick() * currentGain;


        monoSample = pitchShifter.tick(monoSample);


        monoSample = filter.tick(monoSample);


        monoSample = std::tanh(monoSample);

        frame[0] = monoSample;
    }
};

int main() {
    stk::Stk::setSampleRate(44100.0);
    
    AleutianSonifier aleutianEngine("IR/fm-ir1.wav");
    
    stk::FileWvOut output;
    output.openFile("audio_output/akutan_sonification.wav", 1, stk::FileWrite::FILE_WAV, stk::Stk::STK_SINT16);

    std::ifstream file("data/akutan_cleaned.csv");
    if (!file.is_open()) {
        std::cerr << "could not open akutan_cleaned.csv." << std::endl;
        return 1;
    }

    std::string line;
    std::getline(file, line);


    stk::StkFrames monoFrame(1, 1);

    int previousYear = -1;
    double rootPitchRatio = 1.0;
    double currentPitchRatio = rootPitchRatio;

    std::vector<int> dorianScale = {0, 2, 3, 5, 7, 9, 10}; 
    int yearIndex = 0;


    while (std::getline(file, line)) {
        if (line.empty() || line == "\r") continue; 

        std::stringstream ss(line);
        std::string val;
        std::vector<double> row;

        while (std::getline(ss, val, ',')) {
            try {
                if (!val.empty()) row.push_back(std::stod(val));
            } catch (const std::invalid_argument& e) {
                continue; 
            }
        }

        if (row.size() >= 11) {

            int currentYear = static_cast<int>(row[0]); 

            if (previousYear == -1) {
                previousYear = currentYear;
                aleutianEngine.setPitchShift(currentPitchRatio);
            } 
            else if (currentYear > previousYear) {

                yearIndex++; 

                int octave = yearIndex / dorianScale.size();

                int scaleDegree = yearIndex % dorianScale.size();

                int semitones = (octave * 12) + dorianScale[scaleDegree];

                currentPitchRatio = rootPitchRatio * std::pow(2.0, semitones / 12.0);
                
                aleutianEngine.setPitchShift(currentPitchRatio);
                
                std::cout << "Current Year: " << currentYear 
                          << " | Scale Degree: " << scaleDegree 
                          << " | Pitch Ratio: " << currentPitchRatio << std::endl;
                
                previousYear = currentYear;
            }
            // row mapping order ()
            aleutianEngine.updateParameters(row[7], row[6], row[5], row[4], row[2], row[8], row[10]);
            
            int samplesPerRow = 3274;
            
            for (int i = 0; i < samplesPerRow; ++i) {
                aleutianEngine.tick(monoFrame);
                output.tick(monoFrame);
            }
        }
    }

    std::cout << "Sonification completed." << std::endl;
    return 0;
}