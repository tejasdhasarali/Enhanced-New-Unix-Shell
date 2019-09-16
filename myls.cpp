#include <iostream>
#include<string>
#include <vector>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <algorithm>

using namespace std;
int main(){

    vector<string> fileNameList;
    DIR *d;
    struct dirent *dir;
    struct stat fileStat;
    char time[80];

    d = opendir(".");                   //Get all the files in current directory
    if (d) {
        while ((dir = readdir(d)) != NULL) {
                fileNameList.push_back(dir->d_name);       //Add all the file names to vector
        }
        closedir(d);
    }
    else{
        cout<<"Failed to Open the Current Directory. Error!!!"<<endl;
    }

    sort(fileNameList.begin(),fileNameList.end());


    for(size_t i=0;i<fileNameList.size();i++) {

        if (stat(fileNameList[i].c_str(), &fileStat) < 0) {                             //Get all the file stats
            cout << "Error in getting information of file "<< fileNameList[i] << endl;
        }
        else {
            cout<<(S_ISDIR(fileStat.st_mode) ? "d" : "-");                              //Print all the file stats
            cout<<((fileStat.st_mode & S_IRUSR) ? "r" : "-");
            cout<<((fileStat.st_mode & S_IWUSR) ? "w" : "-");
            cout<<((fileStat.st_mode & S_IXUSR) ? "x" : "-");
            cout<<((fileStat.st_mode & S_IRGRP) ? "r" : "-");
            cout<<((fileStat.st_mode & S_IWGRP) ? "w" : "-");
            cout<<((fileStat.st_mode & S_IXGRP) ? "x" : "-");
            cout<<((fileStat.st_mode & S_IROTH) ? "r" : "-");
            cout<<((fileStat.st_mode & S_IWOTH) ? "w" : "-");
            cout<<((fileStat.st_mode & S_IXOTH) ? "x" : "-");
            cout<<"\t";
            cout<<fileStat.st_nlink;
            cout<<"\t";
            cout<<getpwuid(fileStat.st_uid)->pw_name;
            cout<<"\t";
            cout<<getgrgid(fileStat.st_gid)->gr_name;
            cout<<"\t";
            cout<<fileStat.st_size;
            cout<<"\t";
            strftime(time, 80, "%b %d %I:%M", localtime(&fileStat.st_mtime));
            cout<<time;
            cout<<"\t";
            cout<<fileNameList[i];
            cout<<endl;

        }
    }

    return 0;
}
