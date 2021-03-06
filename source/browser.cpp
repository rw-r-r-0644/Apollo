#include "browser.h"
#include "app.h"
#include "gui.h"
#include "fs.h"
#include "utils.h"

extern MainApplication *app;

Browser::Browser()
{
    this->LoadFiles(currentPath);
}

Browser::~Browser()
{
}

void Browser::LoadFiles(const std::string &_pathname)
{
    // If it is dir load files into vector - currentFiles, then change currentPath
    this->currentFiles = fs::OpenDir(_pathname);

    // to do: if openDir returns error then do not clear items and make error message
    // Sort elements from currentFiles and then load them into filesMenu
    SortFiles(this->currentFiles, this->sortType);
    app->GetMainLayout()->LoadItems(this->currentFiles);

    // Change current location to _pathname
    this->currentPath = _pathname;
    app->GetMainLayout()->SetLocationBarText(this->currentPath);

    // Set number of selected items to zero
    this->numberOfSelected = 0;
}

void Browser::Open()
{
    switch (this->GetFileType())
    {
        case 0:
        {
            this->OpenFile();
        }
        break;

        case 1:
        {
            u32 index = app->GetMainLayout()->GetSelectedIndex();
            u32 scroll = app->GetMainLayout()->GetScrollIndex();
            this->lastCursorPosition.push_back(index);
            this->lastScrollPosition.push_back(scroll);
            this->LoadFiles(this->GetFilePathName());
        }
        break;
    }
}

void Browser::OpenFile()
{
    if (this->GetNumberOfSelected() > 1)
        return;

    bool isbin = fs::IsFileBinary(this->GetFilePathName());
    std::string ext = this->GetFileExtension();

    if (ext == "png" || ext == "jpeg" || ext == "jpg")
    {

    }
    else if (ext == "zip" || ext == "rar")
    {

    }
    else if (ext == "bin")
    {

    }
    else if (!isbin)
    {
        int c = app->CreateShowDialog("Open in", "", {"Text viewer", "Cancel"}, true);
        if (c == 0)
        {
            app->LoadLayout(app->GetTextLayout());
            app->GetTextLayout()->Start(fs::ReadTextFile(this->GetFilePathName()), this->GetFilePathName());
        }
    }
}

void Browser::Refresh()
{
    u32 index = app->GetMainLayout()->GetSelectedIndex();
    u32 scroll = app->GetMainLayout()->GetScrollIndex();
    this->LoadFiles(this->currentPath);
    app->GetMainLayout()->SetCursorPosition(index, scroll);
}

void Browser::NavigateBack()
{
    std::size_t n = std::count(this->currentPath.begin(), this->currentPath.end(), '/');
    std::size_t d = this->currentPath.find_last_of("/\\");

    if (n > 1)
    {
        this->currentPath.erase(this->currentPath.begin()+d, this->currentPath.end());
        this->LoadFiles(this->currentPath);
        app->GetMainLayout()->SetCursorPosition(this->lastCursorPosition.back(), this->lastScrollPosition.back());
        this->lastCursorPosition.pop_back();
        this->lastScrollPosition.pop_back();
    }
}

void Browser::SelectFile()
{
    // Select or unselect if already selected
    u32 index = app->GetMainLayout()->GetSelectedIndex();
    this->currentFiles.at(index).selected = !this->currentFiles.at(index).selected;

    switch (this->currentFiles.at(index).selected)
    {
        case 0: // unselected
            app->GetMainLayout()->SetMenuElementIndexColor({10, 10, 10, 255}); // black
            this->numberOfSelected--;
        break;

        case 1: // selected
            app->GetMainLayout()->SetMenuElementIndexColor({213, 160, 29, 255}); // gold
            this->numberOfSelected++;
        break;
    }

    if (this->GetNumberOfSelected() == 1)
    {
        for (auto &f : this->currentFiles)
        {
            if (f.selected == true)
            {
                this->firstSelected = f;
                break;
            }
        }
    }
}

void Browser::RenameFile()
{
    std::string pathname = this->GetFilePathName();
    std::string path = this->GetFilePath();
    std::string oldname = this->GetFileName();
    std::string newname = sys::GetTextFromSoftwareKeyboard("Error: Couldn't rename the file, the name was too long", "Enter new filename", oldname);
    fs::Rename(pathname, path + R"(/)" + newname);
    this->Refresh();
}

void Browser::RemoveFiles()
{
    app->LoadLayout(app->GetDeleteLayout());

    if (this->GetNumberOfSelected() > 1)
    {
        u32 number = 0;
        for (auto &f : this->currentFiles)
        {
            if (f.selected == true)
            {
                auto [nodirs, nofiles] = fs::CountFilesAndDirsRecursive(f.pathname);
                number += nodirs;
                number += (f.type == 0) ? nofiles + 1 : nofiles + 0;
            }
        }

        app->GetDeleteLayout()->Start(number);
        app->CallForRender();

        for (auto &f : this->currentFiles)
        {
            if (f.selected == true)
            {
                if (fs::DeleteDirRecursive(f.pathname) != 0)
                    app->SetDebugText("Error while removing: " + f.pathname);
                else
                    app->SetDebugText("Removed: " + f.pathname);
            }
        }
    }
    else
    {
        auto [nodirs, nofiles] = fs::CountFilesAndDirsRecursive(this->GetFilePathName());
        nofiles += (this->GetFileType() == 0) ? 1 : 0;
        app->GetDeleteLayout()->Start(nodirs + nofiles);
        app->CallForRender();

        std::string pathname = this->GetFilePathName();
        if (fs::DeleteDirRecursive(pathname) != 0)
            app->SetDebugText("Error while removing: " + pathname);
        else
            app->SetDebugText("Removed: " + pathname);
    }

    this->Refresh();
    app->LoadLayout(app->GetMainLayout());
}

void Browser::CopyFiles()
{
    clipboard.clear();

    if (this->GetNumberOfSelected() > 1)
    {
        for (auto &f : this->currentFiles)
        {
            if (f.selected == true)
            {
                ClipboardNode node
                (
                    f.path,                                     // base
                    R"(/)" + f.name,                            // pathname
                    fs::IsDir(f.path + R"(/)" + f.name)         // directory
                );
                this->clipboard.push_back(node);
            }
        }
    }
    else
    {
        ClipboardNode node
        (
            this->GetFilePath(),                        // base
            R"(/)" + this->GetFileName(),               // pathname
            fs::IsDir(this->GetFilePathName())          // directory
        );
        this->clipboard.push_back(node);
    }

    this->moveFlag = 0;
}

void Browser::MoveFiles()
{
    this->CopyFiles();
    this->moveFlag = 1;
}

void Browser::PasteFiles()
{
    // Check if the destined folder is the source folder
    if (clipboard.at(0).base == this->currentPath)
        return;

    u32 number = 0;
    for (auto &f : this->clipboard)
    {
        // Check if the destined folder is the subfolder of the source folder
        std::size_t pos = this->currentPath.find(f.base + f.path);
        if (pos == 0)
        {
            app->CreateShowDialog("Cannot paste files!", "The destined directory is the subdirectory of the source folder.", {"Ok"}, true);
            return;
        }

        // Count files
        auto [nodirs, nofiles] = fs::CountFilesAndDirsRecursive(f.base + f.path);
        number += nodirs;
        number += (f.directory == false) ? nofiles + 1 : nofiles + 0;
    }

    app->GetCopyLayout()->Reset();
    app->LoadLayout(app->GetCopyLayout());
    app->GetCopyLayout()->Start(number, this->moveFlag);

    for (auto &f : this->clipboard)
    {
        this->CopyFileOrDirOverwrite(f.base + f.path, this->currentPath + f.path, f.directory, moveFlag);
    }

    this->moveFlag = 0;
    this->clipboard.clear();
    this->Refresh();
    app->LoadLayout(app->GetMainLayout());
}

void Browser::ChangeSortType(SortType _sorttype)
{
    this->sortType = _sorttype;
    this->Refresh();
}

u32 Browser::GetNumberOfSelected()
{
    return this->numberOfSelected;
}

u32 Browser::GetClipboardSize()
{
    return this->clipboard.size();
}

std::string Browser::GetFileName()
{
    if (this->GetNumberOfSelected() == 0)
        return this->currentFiles.at(app->GetMainLayout()->GetSelectedIndex()).name;
    else
        return this->firstSelected.name;
}

std::string Browser::GetFilePath()
{
    if (this->GetNumberOfSelected() == 0)
        return this->currentFiles.at(app->GetMainLayout()->GetSelectedIndex()).path;
    else
        return this->firstSelected.path;
}

std::string Browser::GetFilePathName()
{
    if (this->GetNumberOfSelected() == 0)
        return this->currentFiles.at(app->GetMainLayout()->GetSelectedIndex()).pathname;
    else
        return this->firstSelected.pathname;
}

std::string Browser::GetFilePermissions()
{
    if (this->GetNumberOfSelected() == 0)
        return fs::GetPermissions(this->currentFiles.at(app->GetMainLayout()->GetSelectedIndex()).pathname);
    else
        return fs::GetPermissions(this->firstSelected.pathname);
}

std::string Browser::GetFileExtension()
{
    std::string currentfile = "";
    if (this->GetNumberOfSelected() == 0)
        currentfile = this->currentFiles.at(app->GetMainLayout()->GetSelectedIndex()).name;
    else
        currentfile = this->firstSelected.name;
    return currentfile.substr(currentfile.find_last_of(".") + 1);
}

u32 Browser::GetFilesSize()
{
    u32 allsize = 0;
    if (this->GetNumberOfSelected() == 0)
    {
        if (this->GetFileType() == 1)
            allsize = fs::GetDirSizeRecursive(currentFiles.at(app->GetMainLayout()->GetSelectedIndex()).pathname);
        else
            allsize = fs::GetSize(currentFiles.at(app->GetMainLayout()->GetSelectedIndex()).pathname);
    }
    else
    {

        for (auto &f : this->currentFiles)
        {
            if (f.selected == true)
            {
                if (f.type == 1)
                    allsize += fs::GetDirSizeRecursive(f.pathname);
                else
                    allsize += fs::GetSize(f.pathname);
            }
        }
    }
    return allsize;
}

bool Browser::GetFileType()
{
    if (this->GetNumberOfSelected() == 0)
        return this->currentFiles.at(app->GetMainLayout()->GetSelectedIndex()).type;
    else
        return this->firstSelected.type;
}

std::pair<u32, u32> Browser::CountMultipleFilesType()
{
    u32 files = 0;
    u32 dirs = 0;
    if (this->GetNumberOfSelected() == 0)
    {
        if (this->GetFileType() == 1)
        {
            auto [nodirs, nofiles] = fs::CountFilesAndDirsRecursive(this->GetFilePathName());
            dirs += nodirs;
            files += nofiles;
        }
        else
            files++;
    }
    else
    {
        for (auto &f : this->currentFiles)
        {
            if (f.selected == true)
            {
                if (f.type == 1)
                {
                    auto [nodirs, nofiles] = fs::CountFilesAndDirsRecursive(f.pathname);
                    dirs += nodirs;
                    files += nofiles;
                }
                else
                    files++;
            }
        }
    }
    return std::make_pair(dirs, files);
}

inline void Browser::CopyFileOrDir(std::string _source, std::string _dest, bool _type)
{
    if (_type)
        fs::CopyDir(_source, _dest);
    else
        fs::CopyFile(_source, _dest);
}

inline void Browser::CopyFileOrDirOverwrite(std::string _source, std::string _dest, bool _type, bool _moveflag)
{
    if (_type)
        fs::CopyDirOverwrite(_source, _dest, _moveflag);
    else
        fs::CopyFileOverwrite(_source, _dest, _moveflag);
}
