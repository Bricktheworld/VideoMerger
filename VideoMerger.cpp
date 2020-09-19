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

void run_ffmpeg(vector<dirent *> *, int, int);
string get_args(vector<dirent *> *, int, int);
vector<dirent *> *get_cropped_videos(vector<dirent *> *_files, string extension, int bWidth, int bHeight);
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
    vector<dirent *> files;

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
                files.push_back(ent);

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
        //Run the ffmpeg command with all of the files retreived
        run_ffmpeg(get_cropped_videos(&files, extension, width, height), rows, columns);
    }

    //If opendir returns a nuoll pointer, we want to fail the program
    else
    {
        perror("");
        return EXIT_FAILURE;
    }
}

void run_ffmpeg(vector<dirent *> *files, int rows, int columns)
{
    //Get the arguments for running the ffmpeg command
    string args = get_args(files, rows, columns);
    cout << "Running ffmpeg " + args << endl;
    system(("ffmpeg " + args).c_str());
}

string get_args(vector<dirent *> *files, int rows, int columns)
{
    string includes = "";
    string filter = " -filter_complex \"";
    string hstack = "";
    string vstack = "";
    string amerge = "";

    for (int i = 0; i < files->size(); i++)
    {
        dirent *file = files->at(i);
        cout << "Getting file " << static_cast<void *>(files->at(i)) << endl;
        includes += (" -i cropped/" + string(file->d_name));
    }

    int counter = 0;

    //Rows
    for (int i = 0; i < rows; i++)
    {
        int inputs = 0;
        if (files->size() <= counter)
            continue;
        //Columns
        for (int j = 0; j < columns; j++)
        {
            if (files->size() <= counter)
                break;
            hstack += "[" + to_string(counter) + ":v]";
            amerge += "[" + to_string(counter) + ":a]";
            counter++;
            inputs++;
        }
        hstack += "hstack=" + to_string(inputs) + "[";
        char c = i + 'i';
        hstack.push_back(c);
        hstack += "];";

        vstack += "[";
        vstack.push_back(c);
        vstack += "]";
    }

    vstack += "vstack[v];";
    amerge += "amerge=inputs=" + to_string(counter) + "[a]";

    delete files;

    return includes +
           filter + hstack + vstack + amerge + "\" -map \"[v] \" -map \"[a] \" -ac 2 -shortest output.mp4";
}

vector<dirent *> *get_cropped_videos(vector<dirent *> *_files, string extension, int bWidth, int bHeight)
{
    int status = system("mkdir cropped");
    cout << "Status: " + to_string(status) << endl;
    vector<dirent *> *files = new vector<dirent *>;
    if (status == 256)
    {
        char res;
        cout << "Unable to create directory as it already exists, would you like to delete and try again? (y/n): ";
        cin >> res;
        if (res == 'y')
        {
            system("rm -rf cropped");
            return get_cropped_videos(_files, extension, bWidth, bHeight);
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

                        files->push_back(ent);
                        cout << "Found cropped video " + string(ent->d_name) << endl;
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

                string res = exec(("ffprobe -v error -select_streams v:0 -show_entries stream=width,height -of csv=s=x:p=0 " + string(_files->at(i)->d_name)).c_str());

                int vWidth = atoi(res.substr(0, res.rfind("x")).c_str());
                int vHeight = atoi(res.substr(res.rfind("x") + 1, res.size() - 1).c_str());

                struct XYVector dimensions;
                dimensions.x = vWidth;
                dimensions.y = vHeight;

                double factor = double(bWidth * vHeight) / double(bHeight * vWidth);

                Side largerSide;
                if (vHeight <= vWidth)
                {
                    largerSide = Side::width;
                }
                else if (vHeight > vWidth)
                {
                    largerSide = Side::height;
                }

                (*get_side(&dimensions, largerSide)) *= factor;
                struct XYVector offset;
                offset.x = 0;
                offset.y = 0;

                get_offset(&offset, &dimensions, largerSide, vWidth, vHeight);

                string command = "ffmpeg -i " + string(_files->at(i)->d_name) + " -filter:v \"crop=";
                command.append(to_string(dimensions.x));
                command.append(":");
                command.append(to_string(dimensions.y));
                command.append(":");
                command.append(to_string(offset.x));
                command.append(":");
                command.append(to_string(offset.y));
                command.append("\" cropped/" + string(_files->at(i)->d_name));
                cout << "Running command " + command << endl;
                system(command.c_str());
            }

            while ((ent = readdir(croppedDir)) != NULL)
            {
                if (boost::ends_with(ent->d_name, extension))
                {

                    files->push_back(ent);
                    cout << "Found cropped video " + string(ent->d_name);
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

//ffmpeg -i video0.mov -i video1.mov -i video2.mov -i video3.mov -filter_complex "[0:v] setpts=PTS-STARTPTS, scale=qvga [a0];[1:v] setpts=PTS-STARTPTS, scale=qvga [a1];[2:v] setpts=PTS-STARTPTS, scale=qvga [a2];[3:v] setpts=PTS-STARTPTS, scale=qvga [a3];[a0][a1][a2][a3]xstack=inputs=4:layout=0_0|0_h0|w0_0|w0_h0[out]" -map "[out]" -c:v libx264 -t '30' -f matroska output.mov