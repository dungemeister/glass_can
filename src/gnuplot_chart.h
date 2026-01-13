#pragma once
#include <fstream>
#include <vector>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <string>
#include <errno.h>
#include <sys/wait.h>

#include "price_history_parser.h"

namespace GnuplotChart{
    
    static bool saveGnuplotDataFile(const std::vector<PriceHistory::PricePoint>& points, const std::string& points_data_filename){
        std::ofstream gnu_data(points_data_filename, std::ios_base::trunc);
        if(!gnu_data.is_open()){
            std::cerr << "ERROR: fail to open " << points_data_filename << std::endl;
            return false;
        }
        for(auto& point: points){
            auto length = point.date.length();
            auto date = point.date.substr(0, 14) + " ";
            gnu_data << std::fixed << std::setprecision(4) << date << point.median << std::endl;
        }
        gnu_data.close();
        return true;
    }

    static bool saveGnuplotScriptFile(const std::string& gnuplot_script_filename,
                                      const std::string& points_data_file,
                                      const std::string& output_filename,
                                      PriceHistory::eTimePeriod period){

        std::ofstream gnuplot_file(gnuplot_script_filename);
        if(!gnuplot_file.is_open()){
            std::cerr << "ERROR: fail to open " << gnuplot_script_filename << std::endl;
            return false;
        }
        size_t xtics = 3 * 30 * 86400;
        std::string timefmt;

        timefmt = "%b %d %Y %H";
        switch(period){
            case PriceHistory::eTimePeriod::ALL_TIME:
                xtics = 3 * 30*86400;
            break;
            case PriceHistory::eTimePeriod::WEEK:
                xtics = 86400;
            break;
            case PriceHistory::eTimePeriod::MONTH:
                xtics = 12 * 86400;
            break;
            case PriceHistory::eTimePeriod::YEAR:
                xtics = 96 * 86400;
            break;

        }
        gnuplot_file  << "set title \"Price chart " << points_data_file << "\"\n" <<
                "set terminal pngcairo size 1920,1080\n" <<
                "set output '" << output_filename <<"'\n" <<
                "set xdata time\n" <<
                "set timefmt \"" << timefmt << "\"\n" <<
                "set format x \"%b %d\"\n" <<
                "set xtics " << xtics << " nomirror\n" <<
                "plot '" << points_data_file << "' using 1:5 with lines title \"\""  << std::endl;
        gnuplot_file.close();
        return true;
        
    }

    static bool generateGnuplotPngImage(const std::string& gnu_script_filename){
        std::vector<const char*> args = {
            "gnuplot",
            gnu_script_filename.c_str(),
            nullptr};
        pid_t pid = fork();
        if(pid == -1){
            throw std::runtime_error("generateGnuplotPngImage: Fail to fork");
        }
        else if(pid == 0){
            //Child proc
            std::cout << "Runned gnuplot child proc " << getpid() << std::endl;
            execvp(args[0], const_cast<char* const*>(args.data()));

            throw std::runtime_error("ERROR: generateGnuplotPngImage: ");
            exit(-1);
        }
        else{
            std::cout << "Runned gnuplot proc " << getpid() << std::endl;
            int status;
            waitpid(pid, &status, 0);
            
            std::cout << "Child gnuplot finished" << std::endl;
        }
        return true;
    }

    static bool createChartPngImage(const std::vector<PriceHistory::PricePoint>& points,
                                    const std::string& points_data_file,
                                    const std::string& gnuplot_script_filename,
                                    const std::string& output_filename,
                                    PriceHistory::eTimePeriod period=PriceHistory::eTimePeriod::ALL_TIME){
        
        bool res = saveGnuplotDataFile(points, points_data_file);
        res     &= saveGnuplotScriptFile(gnuplot_script_filename, points_data_file, output_filename, period);
        res     &= generateGnuplotPngImage(gnuplot_script_filename);
        return res;
    }
};