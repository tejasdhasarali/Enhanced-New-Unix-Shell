#include <iostream>
#include<string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <algorithm>

#define PATH_MAX 1024

using namespace std;

/*Handler to remove the child process from process
    table when it completes execution.
        To avoid Zombie Process*/

void handler(int signum)
{
    wait(NULL);
}


/* Reads the input from the terminal*/

string readLines(){
    string input;
    getline(cin,input);

    if(!cin){
        cout<<"Ctrl+D pressed exiting the Shell"<<endl;
        exit(1);
    }
    return input;
}

/* Split the input by space and convert it into vector of string and
 *     return to the main*/

vector<string> tokenize(string input){
    vector <string> args;
    string intermediate;
    stringstream inputStream(input);

    while(getline(inputStream,intermediate, ' ')){
        args.push_back(intermediate);
    }
    for(size_t i=0;i<args.size();i++){
        if(args[i].empty())
            args.erase(args.begin()+i);
    }
    return args;
}

/* Runs all the commands both in foreground and background
 * Doesn't Handle IO-redirection and Pipe execution*/

int runTheCommand(vector<string> args,bool isBackground)
{
    pid_t pid, wpid;
    int status;
    char *argsPointer[args.size()];

    for(size_t i=0; i<args.size();i++){
        argsPointer[i]= const_cast<char *>(args[i].c_str());            //Convert vector to constant character pointer
    }
    argsPointer[args.size()]=NULL;

    pid = fork();

    if (pid == 0) {

        if(isBackground) {
            setpgid(0, 0);                              //Set a different group Id to child to give up
        }                                                   // terminal foreground to the parent

        FILE *fp;

        if ((fp = fopen("Mypath.txt", "r")) != NULL && args[0]=="myls")     //Check Mypath is set for myls
        {                                                                   //MyPath is saved in a file
            int n=0;
            while(fgetc(fp) !=EOF){
                n=n+1;
            }

            char c[n];
            fseek(fp, 0, SEEK_SET);
            fscanf(fp, "%s", c);
            fclose(fp);
            c[n]='\0';
            //char *mylsArgs[]={c, NULL};
            if(execvp(c,argsPointer)==-1)             //execute myls
            {
                perror("Myshell");
            }
            exit(EXIT_FAILURE);
        }

        else if(args[0]=="myls"){
            cout<<"Please set the path of myls"<<endl;
        }

        else{
            if (execvp(argsPointer[0], argsPointer) == -1) {            //Execute all the commands
                perror("MyShell");
            }
            exit(EXIT_FAILURE);
        }

    } else if (pid < 0) {
        perror("MyShell");

    } else {
        if(isBackground){                                           //Signal the handler to manage the child process
            signal(SIGCHLD, handler);                               //Return immediately
            wpid = waitpid(pid, &status, WNOHANG);
        }
        else{
            do {
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));       //wait till child finished execution
        }
    }
    return 0;
}

/* Handles I/O Redirection and executes both in foreground and background*/

int IORedirection(vector<string> args, bool isBackground){

    pid_t pid, wpid;
    int status;
    vector <string>::iterator itGreat;
    vector <string>::iterator itLess;

    pid = fork();

    if (pid == 0) {

        if (isBackground) {
            setpgid(0, 0);
        }

        char *argsPointer[args.size()];

        itLess=find(args.begin(),args.end(),"<");

        if(itLess != args.end()){                   //Handles input redirection
            args.erase(itLess);
        }

        itGreat=find(args.begin(),args.end(),">");

        if(itGreat != args.end()){                  //Handles output redirection by changing the stdin and stdout
            int in;
            int out;
            long index=distance(args.begin(),itGreat);

            in=open(args[index-1].c_str(),O_RDONLY);

            if(in<0){
                cout<<"The input file " <<args[index-1]<<" is missing. Error!!!"<<endl;
                exit(EXIT_FAILURE);
            }

            out=open(args.back().c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            dup2(in,fileno(stdin));
            dup2(out,fileno(stdout));
            close(in);
            close(out);

            args.erase(itGreat+1);
            args.erase(itGreat);

        }


        for(size_t i=0; i<args.size();i++){
            argsPointer[i]= const_cast<char *>(args[i].c_str());
        }

        argsPointer[args.size()]=NULL;

        if (execvp(argsPointer[0], argsPointer) == -1) {
            perror("MyShell");

        }
        exit(EXIT_FAILURE);

    }
    else if (pid < 0) {
        perror("MyShell");
    }
    else {
        if(isBackground){
            signal(SIGCHLD, handler);
            wpid = waitpid(pid, &status, WNOHANG);
        }
        else{
            do {
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }

    }
    return 0;

}

/* Executes all the piped commands only in foreground*/

int pipeExecute(string input){

    stringstream inputStream(input);
    string intermediate;
    int pipefd[2];
    int status;
    vector <string> args;
    pid_t pid, wpid;
    int fd=0;
    int count=0;
    long n = std::count(input.begin(), input.end(), '|');


    while(getline(inputStream,intermediate , '|')) {

        count++;
        args = tokenize(intermediate);

        char *argsPointer[args.size()];
        for (size_t i = 0; i < args.size(); i++) {
            argsPointer[i] = const_cast<char *>(args[i].c_str());
        }
        argsPointer[args.size()] = NULL;

        pipe(pipefd);                                   //Pipe the input and output
        pid = fork();

        if (pid == 0) {
            close(pipefd[0]);
            dup2(fd,fileno(stdin));                 //set the previous output as current stdin

            if(count!=n+1){                         //Makes sure the last output is printed to stdout
                dup2(pipefd[1],fileno(stdout));     //Set the pipe as stdout
            }

            FILE *fp;

            if ((fp = fopen("Mypath.txt", "r")) != NULL && args[0]=="myls")     //Handles myls inside Pipe
            {
                int n=0;
                while(fgetc(fp) !=EOF){
                    n=n+1;
                }

                char c[n];
                fseek(fp, 0, SEEK_SET);
                fscanf(fp, "%s", c);
                fclose(fp);
                c[n]='\0';
                //char *mylsArgs[]={c, NULL};

                if(execvp(c,argsPointer)==-1)
                {
                    perror("Myshell");
                }
                exit(EXIT_FAILURE);
            }

            else if(args[0]=="myls"){
                cout<<"Please set the path of myls"<<endl;
            }
            else {
                if (execvp(argsPointer[0], argsPointer) == -1) {
                    perror("MyShell");
                }
                exit(EXIT_FAILURE);
            }
        }
        else if (pid < 0) {
            perror("MyShell");
        }
        else {
            do {
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));

            close(pipefd[1]);
            fd=pipefd[0];
        }
    }
    return 0;
}

/*Executes the change director command*/

int executeCD(vector<string> args){

    int ret;
    char cwd[PATH_MAX];

    if(args[1].empty()){
        cout<<"Missing directory. Error!!!"<<endl;
        return -1;
    }
    if(args[1][0]=='/'){
        ret=chdir(args[1].c_str());
    }
    else if (getcwd(cwd, sizeof(cwd)) != NULL) {
            string str=cwd;
            ret=chdir((str+"/"+args[1]).c_str());
    }

    else{
        ret=-1;
    }

    if(ret==0){
        cout<<"Directory Successfully changed to"<<endl;
        cout<<args[1]<<endl;
    }
    else{
        cout<<"Failed to Change the Directory. Error!!!"<<endl;
        cout<<"Error Code :"<<ret<<endl;
    }

    return ret;

}

int main() {

    string input;
    vector <string> args;
    int status;
    bool isBackground;
    vector <string>::iterator itGreat;
    vector <string>::iterator itLess;


    do {

        cout << "$ ";
        input = readLines();
        isBackground = false;

        size_t isPiped = string::npos;
        isPiped = input.find("|");

        if (isPiped != string::npos) {      //Handle piped commands
            pipeExecute(input);
            continue;
        }

        args = tokenize(input);                 //Handle background process
        if (args.back() == "&") {
            args.pop_back();
            isBackground = true;
        }

        itGreat = find(args.begin(), args.end(), ">");
        itLess = find(args.begin(), args.end(), "<");

        if (itGreat != args.end() || itLess != args.end()) {        //Handles IO redirection
            status = IORedirection(args, isBackground);
            continue;
        }

        if (args[0] == "exit") {                            //exit the shell
            cout << "Exiting the Shell" << endl;
            exit(0);
        }
        else if (args[0] == "cd") {                         //Change the directory
            executeCD(args);
        }
        else if (args[0] == "pwd") {                        //Display Current directory
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                cout << "The current working directory is: " << endl << cwd << endl;
            }
            else {
                cout << "There was a error in getcwd()!!!" << endl;
            }

        }
        else if (args[0] == "set") {                //Set the my path by saving in the file

            FILE *fp;
            fp = fopen("Mypath.txt", "w");

            if (fp != NULL) {
                size_t index = args[1].find("=");
                if (index != string::npos) {
                    fputs((args[1].substr((index + 1), (args[1].size() - 1))).c_str(), fp);
                    cout << "Path was set successfully" << endl;
                    fclose(fp);
                }
                else {
                    cout << "Error in setting the path. The format is set MYPATH=path1" << endl;
                }
            }
            else {
                cout << "Error while setting the path. Please try again" << endl;
            }
        }
        else {
            status = runTheCommand(args, isBackground);     //Runs all the other commands
        }

    }while(true);

    return 0;
}