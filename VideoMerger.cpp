#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <boost/algorithm/string/predicate.hpp>
#include <filesystem>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

#include "XYVector.h"
#include "SideEnum.h"

using namespace std;

void run_ffmpeg(vector<string> *, int, int, int, int, string);
string get_args(vector<string> *, int, int, int, int, string);
vector<string> *get_cropped_videos(vector<string> *_files, double *longestVideoLength, string extension, int bWidth, int bHeight);
string exec(const char *cmd);

int main()
{

    // Stores current directory to be used in the readdir
    DIR *directory;

    //Stores the current entity from the readdir stream
    struct dirent *ent;

    //Generic file name to be used to find all of the videos
    string file_name;

    //Rows
    int rows;
    //Columns
    int columns;

    cout << "Please enter a generic video file name: ";
    cin >> file_name;

    //Pattern for all the names of the files without the extension
    string name = file_name.substr(0, file_name.find("."));

    //File Extension
    string extension = file_name.substr(file_name.find("."));

    //List of files to be retreived from the program
    vector<string> files;

    //Open the directory
    if ((directory = opendir(".")) != NULL)
    {
        //Read all the files in the dir
        while ((ent = readdir(directory)) != NULL)
        {

            //If the file name starts with the generic name and ends with the file extension
            string str = ent->d_name;
            if (str.rfind(name, 0) == 0 && boost::ends_with(str, extension))
            {
                //Add the file to the list of files
                files.push_back(ent->d_name);

                //Print out the retreived file
                cout << "Retreived File " << ent->d_name << endl;
            }
        }

        //Close the directory after finishing
        closedir(directory);

        cout << "Rows?: ";
        cin >> rows;
        cout << "Columns?: ";
        cin >> columns;

        if (columns * rows < files.size())
        {
            cout << "columns x Rows = " + to_string(rows * columns) + ", while the number of vides is " + to_string(files.size()) << endl;
            // perror("");
            return EXIT_FAILURE;
        }

        int width = 1920 / columns;
        int height = 1080 / rows;

        double longestVideoLength = 0;
        //Run the ffmpeg command with all of the files retreived
        vector<string> *cropped_files = get_cropped_videos(&files, &longestVideoLength, extension, width, height);

        system(("ffmpeg -t " + to_string(longestVideoLength) + "  -f lavfi -i color=c=black:s=640x480 -c:v libx264 -tune stillimage -pix_fmt yuv420p empty.mp4").c_str());
        run_ffmpeg(cropped_files, rows, columns, width, height, extension);
    }

    //If opendir returns a nuoll pointer, we want to fail the program
    else
    {
        perror("");
        return EXIT_FAILURE;
    }
}

void run_ffmpeg(vector<string> *files, int rows, int columns, int width, int height, string extension)
{
    //Get the arguments for running the ffmpeg command
    string args = get_args(files, rows, columns, width, height, extension);
    cout << "Running ffmpeg " + args << endl;
    system(("ffmpeg " + args).c_str());
    system("rm empty.mp4");
}

string get_args(vector<string> *files, int rows, int columns, int width, int height, string extension)
{
    string includes = "";
    string filter = " -filter_complex \"";
    string videos = "";
    string xstack = "";
    string amix = "";

    for (int i = 0; i < rows * columns; i++)
    {
        if (i >= files->size())
        {
            includes += (" -i empty.mp4");
        }
        else
        {
            cout << "Getting file " << files->at(i) << endl;
            string file_without_extension = files->at(i).substr(0, files->at(i).rfind("."));
            cout << "Removing file " << file_without_extension << "_borderless" << extension << endl;
            system(("rm " + file_without_extension + "_borderless" + extension).c_str());
            includes += (" -i cropped/" + files->at(i));
        }
    }
    for (int i = 0; i < rows * columns; i++)
    {
        videos += "[" + to_string(i) + ":v] setpts=PTS-STARTPTS, scale=" + to_string(width) + ":" + to_string(height) + " [a" + to_string(i) + "];";
    }
    for (int i = 0; i < rows * columns; i++)
    {
        xstack += "[a" + to_string(i) + "]";
    }

    xstack += "xstack=inputs=" + to_string(rows * columns) + ":layout=";

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < columns; j++)
        {
            if (j == 0)
            {
                xstack += "0";
            }
            else
            {
                for (int k = 0; k < j; k++)
                {
                    xstack += "w" + to_string(k);
                    if (k != j - 1)
                    {
                        xstack += "+";
                    }
                }
            }
            xstack += "_";
            if (i == 0)
            {
                xstack += "0";
            }
            else
            {
                for (int k = 0; k < i; k++)
                {
                    xstack += "h" + to_string(k);
                    if (k != i - 1)
                    {
                        xstack += "+";
                    }
                }
            }
            if (i != rows - 1 || j != columns - 1)
            {
                xstack += "|";
            }
            else
            {
                xstack += "[out];";
            }
        }
    }

    amix += "amix=inputs=" + to_string(files->size());

    delete files;

    return includes + filter + videos + xstack + amix + "\" -map \"[out]\" output.mp4";
}

vector<string> *get_cropped_videos(vector<string> *_files, double *longestVideoLength, string extension, int bWidth, int bHeight)
{
    int status = system("mkdir cropped");
    cout << "Status: " + to_string(status) << endl;
    vector<string> *files = new vector<string>;
    if (status == 256)
    {
        char res;
        cout << "Unable to create directory as it already exists, would you like to delete and try again? (y/n): ";
        cin >> res;
        if (res == 'y')
        {
            system("rm -rf cropped");
            delete files;
            return get_cropped_videos(_files, longestVideoLength, extension, bWidth, bHeight);
        }
        else
        {

            DIR *croppedDir;
            struct dirent *ent;
            if ((croppedDir = opendir("./cropped")) != NULL)
            {
                while ((ent = readdir(croppedDir)) != NULL)
                {
                    if (boost::ends_with(ent->d_name, extension))
                    {
                        string res = exec(("ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 " + string(ent->d_name)).c_str());
                        double duration = atof(res.c_str());
                        if (duration > *longestVideoLength)
                        {
                            *longestVideoLength = duration;
                        }
                        files->push_back(ent->d_name);
                        cout << "Adding file " << ent->d_name << endl;
                    }
                }
                closedir(croppedDir);
                return files;
            }
            else
            {
                cout << "FAILED TO OPEN CROPPED DIRECTORY";
                return nullptr;
            }
        }
    }
    else
    {
        DIR *croppedDir;
        struct dirent *ent;
        if ((croppedDir = opendir("./cropped")) != NULL)
        {
            for (int i = 0; i < _files->size(); i++)
            {
                string file_name = _files->at(i);
                string new_file_name = file_name.substr(0, file_name.size() - extension.size()) + "_borderless" + extension;
                string crop_detect = exec(("ffmpeg -i " + _files->at(i) + " -t 1 -vf cropdetect -f null - 2>&1 | awk '/crop/ { print $NF }' | tail -1").c_str());
                system(("ffmpeg -i " + _files->at(i) + " -filter:v \"" + crop_detect + "\" " + new_file_name).c_str());

                string res = exec(("ffprobe -v error -select_streams v:0 -show_entries stream=width,height -of csv=s=x:p=0 " + new_file_name).c_str());

                int vWidth = atoi(res.substr(0, res.rfind("x")).c_str());
                int vHeight = atoi(res.substr(res.rfind("x") + 1, res.size() - 1).c_str());

                struct XYVector dimensions;
                dimensions.x = vWidth;
                dimensions.y = vHeight;

                double factor = double(bWidth * vHeight) / double(bHeight * vWidth);
                cout << "dimensions are " << res << endl;

                Side largerSide;
                if (vHeight <= vWidth)
                {
                    cout << "Larger side is width" << endl;
                    largerSide = Side::width;
                }
                else if (vHeight > vWidth)
                {
                    cout << "Larger side is height" << endl;
                    largerSide = Side::height;
                }
                else
                {
                    cout << "Sides are equal" << endl;
                }

                (*get_side(&dimensions, largerSide)) *= largerSide == Side::width ? factor : 1 / factor;
                struct XYVector offset;
                offset.x = 0;
                offset.y = 0;

                get_offset(&offset, &dimensions, largerSide, vWidth, vHeight);

                string command = "ffmpeg -i " + new_file_name + " -filter:v \"crop=";
                command.append(to_string(dimensions.x));
                command.append(":");
                command.append(to_string(dimensions.y));
                command.append(":");
                command.append(to_string(offset.x));
                command.append(":");
                command.append(to_string(offset.y));
                command.append("\" cropped/" + _files->at(i));
                cout << "Running command " + command << endl;
                system(command.c_str());
            }

            while ((ent = readdir(croppedDir)) != NULL)
            {
                if (boost::ends_with(ent->d_name, extension))
                {
                    string res = exec(("ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 " + string(ent->d_name)).c_str());
                    double duration = atof(res.c_str());
                    if (duration > *longestVideoLength)
                    {
                        *longestVideoLength = duration;
                    }

                    files->push_back(ent->d_name);
                }
            }
            closedir(croppedDir);
            for (int i = 0; i < files->size(); i++)
            {
                cout << "File " << files->at(i) << endl;
            }
            return files;
        }
        else
        {
            cout << "FAILED TO OPEN CROPPED DIRECTORY";
            return nullptr;
        }
    }
    return nullptr;
}

std::string exec(const char *cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return result;
}
