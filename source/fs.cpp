#include <string.h>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "fs.h"
#include "app.h"

extern MainApplication *app;

namespace fs
{
    inline bool Filter(char *_name) // is it necessary?
    {
        return (strcmp(_name, ".") && strcmp(_name, ".."));
    }

    inline void MakeDir(const std::string &_path)
    {
        mkdir(_path.c_str(), 0777);
    }

    std::vector<File> OpenDir(const std::string &_path)
    {
        DIR* dir;
        struct dirent* ent;
        std::vector<File> files;

        dir = opendir(_path.c_str());
        if (dir!=NULL)
        {
            while ((ent = readdir(dir)))
            {
                File _f
                (
                    _path + R"(/)" + ent->d_name,               // pathname
                    ent->d_name,                                // name
                    _path,                                      // path
                    IsDir(_path + R"(/)" + ent->d_name)         // type
                );

                files.push_back(File(_f));
            }
            app->SetDebugText("Loaded: " + _path);
        }
        else
            app->SetDebugText("Error loading: " + _path);

        closedir(dir);
        return files;
    }

    bool IsDir(const std::string &_path)
    {
        struct stat buf;
        stat((char *)_path.c_str(), &buf);
        return S_ISDIR(buf.st_mode);
    }

    bool Exists(const std::string &_pathname)
    {
        struct stat buff;
        return (stat(_pathname.c_str(), &buff) == 0);
    }

    std::pair<std::vector<std::string>, std::vector<std::string>> GetContents(const std::string &_path)
    {
        std::vector<std::string> directories;
        std::vector<std::string> files;

        DIR *dir;
        struct dirent *ent;
        dir = opendir(_path.c_str());

        if (dir)
        {
            while ((ent = readdir(dir)))
            {
                if (Filter(ent->d_name))
                {
                    std::string p = ent->d_name;

                    switch(IsDir(_path + R"(/)" + p))
                    {
                        case 0:
                            files.push_back(p);
                        break;

                        case 1:
                            directories.push_back(p);
                        break;
                    }
                }
            }
        }
        closedir(dir);
        return std::make_pair(directories, files);
    }

    #include <fcntl.h>   // open
    #include <unistd.h>  // read, write, close
    #include <cstdio>    // BUFSIZ
    #include <ctime>

    void CopyFile(const std::string &_source, const std::string &_dest)
    {
        std::ifstream source(_source, std::ios::binary);
        std::ofstream dest(_dest, std::ios::binary);

        source.seekg(0, std::ios::end);
        std::ifstream::pos_type size = source.tellg();
        source.seekg(0);

        char* buffer = new char[size];

        source.read(buffer, size);
        dest.write(buffer, size);

        delete[] buffer;
        source.close();
        dest.close();
    }

    void CopyDir(const std::string &_source, const std::string &_dest)
    {
        MakeDir(_dest);
        auto [directories, files] = GetContents(_source);

        if (!directories.empty())
        {
            for (auto &d : directories)
            {
                app->GetCopyLayout()->Update(_source, _dest);
                CopyDir(_source + R"(/)" + d, _dest + R"(/)" + d);
            }
        }

        if (!files.empty())
        {
            for (auto &f : files)
            {
                app->GetCopyLayout()->Update(_source + R"(/)" + f, _dest + R"(/)" + f);
                CopyFile(_source + R"(/)" + f, _dest + R"(/)" + f);
            }
        }
    }

    int RenameFile(const std::string &_pathname, const std::string &_newname)
    {
        return (rename(_pathname.c_str(), _newname.c_str()));
    }

    int Remove(const std::string &_filepath)
    {
        int result;
        switch (IsDir(_filepath))
        {
            case 0:
                result = remove(_filepath.c_str());
            break;

            case 1:
                result = rmdir(_filepath.c_str());
            break;
        }
        return result;
    }

    int DeleteDirRecursive(const std::string &_pathname)
    {
        int result;
        DIR *dir = opendir(_pathname.c_str());
        struct dirent *ent;

        while ((ent = readdir(dir)) != NULL)
        {
            if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
            {
                std::string filepath = _pathname + R"(/)" + ent->d_name;
                result = Remove(filepath);

                if (result != 0)
                {
                    DeleteDirRecursive(filepath);
                    result = Remove(filepath); // is it necessery?
                }

                app->GetDeleteLayout()->Update(_pathname);
                app->CallForRender();
            }
        }
        closedir(dir);

        result = Remove(_pathname); // got this, so the above seems useless
        app->GetDeleteLayout()->Update(_pathname);
        app->CallForRender();
        return result;
    }

    void ListFilesRecursive(const std::string &_baselocation, const std::string &_filename, std::vector<ClipboardNode> &_filelist, const std::string &_pathname)
    {
        std::string path;

        DIR *dir = opendir((_baselocation + _filename + _pathname).c_str());
        struct dirent *ent;

        if (!dir)
            return;

        while ((ent = readdir(dir)) != NULL)
        {
            if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
            {
                path = _pathname + R"(/)" + ent->d_name;
                ClipboardNode node
                (
                    _baselocation,                                      // base
                    R"(/)" + _filename + path,                          // pathname
                    IsDir(_baselocation + _filename + path)             // directory
                );
                _filelist.push_back(node);

                ListFilesRecursive(_baselocation, _filename, _filelist, path);
            }
        }
        closedir(dir);
    }

    u32 CountFilesRecursive(const std::string &_pathname, u32 _number)
    {
        DIR *dir = opendir((_pathname).c_str());
        struct dirent *ent;

        if (!dir)
            return 0;

        while ((ent = readdir(dir)) != NULL)
        {
            if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
            {
                _number++;
                _number += CountFilesRecursive(_pathname + R"(\)" + ent->d_name);
            }
        }
        closedir(dir);

        return _number;
    }
}
