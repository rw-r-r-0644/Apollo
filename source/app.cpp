#include <iostream>
#include <vector>
#include <string>
#include "app.h"
#include "fs.h"
#include "utils.h"

MainApplication *app;

int Initialize()
{
    void *addr = NULL;
    if (sys::IsNRO() == 0)
        svcSetHeapSize(&addr, 0x10000000);
    if (R_FAILED(psmInitialize()))  return 1;
    if (R_FAILED(nifmInitialize())) return 1;
    return 0;
}

void Finalize()
{
    psmExit();
    nifmExit();
}

void MainApplication::SetDebugText(std::string _text)
{
    this->debugText->SetText(_text);
}
/*
#include <fstream>
void MainApplication::LogDebugText(std::string _text)
{
    std::ofstream debug(R"(sdmc:/Apollo_log.txt)", std::ios::app);
    debug << _text << std::endl;
}
*/
MainApplication::MainApplication()
{
    app = this;
    this->SetTouch(false);
    this->InitAllLayouts();
    this->InitAllModules();
    this->AddThread(std::bind(&MainApplication::Update, this));
}

MainApplication::~MainApplication()
{
    /*
    for (auto &i : allMenuItems)
    {
        delete i;
    }

    for (auto &e : allElements)
    {
        delete e;
    }
*/
    for (auto &l : allLayouts)
    {
        delete l;
    }

    delete this->browser;
}

void MainApplication::Update()
{
    std::string time = sys::GetCurrentTime();
    if (time != this->pretime)
    {
        this->clockText->SetText(time);
        this->pretime = time;
    }

    u32 battery = sys::GetBatteryLevel() / 10;
    if (battery != this->prebatstatus)
    {
        this->batteryStatus->SetImage(GetRomFsResource(std::to_string(battery), "Battery"));
        this->prebatstatus = battery;
    }

    bool isbatterycharging = sys::IsBatteryCharging();
    if (isbatterycharging != this->prebatcharg)
    {
        this->batteryCharging->SetVisible(isbatterycharging);
        this->prebatcharg = isbatterycharging;
    }

    u32 wifi = sys::GetWiFiConnectionSignal();
    if (wifi != this->prewifistatus)
    {
        this->wifiStatus->SetImage(GetRomFsResource(std::to_string(wifi), "WiFi"));
        this->prewifistatus = wifi;
    }

    if (this->prehelpinfo != this->helpinfo)
    {
        std::string help = "";
        if (this->helpinfo == true)
            help = "_help";

        this->copyButton->SetIcon(GetRomFsResource("copy" + help));
        this->pasteButton->SetIcon(GetRomFsResource("paste" + help));
        this->deleteButton->SetIcon(GetRomFsResource("delete" + help));
        this->renameButton->SetIcon(GetRomFsResource("rename" + help));
        this->exitButton->SetIcon(GetRomFsResource("exit" + help));

        this->prehelpinfo = this->helpinfo;
    }
}

ui::MainLayout *MainApplication::GetMainLayout()
{
    return this->mainLayout;
}

ui::CopyLayout *MainApplication::GetCopyLayout()
{
    return this->copyLayout;
}

ui::DeleteLayout *MainApplication::GetDeleteLayout()
{
    return this->deleteLayout;
}

ui::TextLayout *MainApplication::GetTextLayout()
{
    return this->textLayout;
}

void MainApplication::mainLayoutInput(u64 Down, u64 Up, u64 Held)
{
    if (Down & KEY_MINUS)           this->ExitAction();
    if (Down & KEY_PLUS)            this->InfoAction();
    if (Down & KEY_B)               this->NavigateBackAction();
    if (Down & KEY_X)               this->RenameAction();
    if (Down & KEY_Y)               this->SelectAction();
    if (Down & KEY_L)               this->CopyAction();
    if (Down & KEY_R)               this->PasteAction();
    if (Down & KEY_ZL)              this->MoveAction();
    if (Down & KEY_ZR)              this->DeleteAction();
    if (Down & KEY_LSTICK)          this->SortAction();
    if (Down & KEY_RSTICK)          this->HelpAction();
    if (Down & KEY_LSTICK_LEFT) // focus on sidebar
    {
        this->sidebarMenu->SetOnFocus(true);
        this->GetMainLayout()->SetOnFocus(false);
    }
    if (Down & KEY_LSTICK_RIGHT) // focus on main layout
    {
        this->sidebarMenu->SetOnFocus(false);
        this->GetMainLayout()->SetOnFocus(true);
    }
}

void MainApplication::copyLayoutInput(u64 Down, u64 Up, u64 Held)
{

}

void MainApplication::deleteLayoutInput(u64 Down, u64 Up, u64 Held)
{

}

void MainApplication::textLayoutInput(u64 Down, u64 Up, u64 Held)
{
    if (Down & KEY_MINUS)           this->ExitAction();
    if (Down & KEY_B)               this->GetTextLayout()->End();
    if (Down & KEY_LSTICK_UP ||
        Held & KEY_RSTICK_UP ||
        Down & KEY_DUP)             this->GetTextLayout()->ScrollUp();
    if (Down & KEY_LSTICK_DOWN ||
        Held & KEY_RSTICK_DOWN ||
        Down & KEY_DDOWN)           this->GetTextLayout()->ScrollDown();
}


Browser *MainApplication::GetBrowser()
{
    return this->browser;
}

void MainApplication::InitAllLayouts()
{
    /// Helpful variables
    this->helpinfo = false;

    // previous values
    this->prehelpinfo = false;
    this->prebatcharg = false;
    this->prebatstatus = 0;
    this->pretime = "00:00:00";
    this->prewifistatus = -1;

    /// Layouts
    this->mainLayout = new ui::MainLayout();
    allLayouts.push_back(this->mainLayout);
    mainLayout->SetOnInput(std::bind(&MainApplication::mainLayoutInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    this->copyLayout = new ui::CopyLayout();
    allLayouts.push_back(this->copyLayout);
    copyLayout->SetOnInput(std::bind(&MainApplication::copyLayoutInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    this->deleteLayout = new ui::DeleteLayout();
    allLayouts.push_back(this->deleteLayout);
    deleteLayout->SetOnInput(std::bind(&MainApplication::deleteLayoutInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    this->textLayout = new ui::TextLayout();
    allLayouts.push_back(this->textLayout);
    textLayout->SetOnInput(std::bind(&MainApplication::textLayoutInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    /// Elements
    // details
    this->footerBottom = new pu::element::Rectangle(0, 647, 1280, 73, {235, 235, 235, 255});
    allElements.push_back(this->footerBottom);

    this->footerLine = new pu::element::Rectangle(146, 647, 1280-146-30, 1, {45, 45, 45, 255});
    allElements.push_back(this->footerLine);

    // status bar
    this->clockText = new pu::element::TextBlock(984, 53, sys::GetCurrentTime());
    this->clockText->SetColor({10, 10, 10, 255}); // black
    allElements.push_back(this->clockText);
    this->batteryStatus = new pu::element::Image(1181, 55, GetRomFsResource("10", "Battery"));
    allElements.push_back(this->batteryStatus);
    this->batteryCharging = new pu::element::Image(1217, 55, GetRomFsResource("charging", "Battery"));
    allElements.push_back(this->batteryCharging);
    this->batteryCharging->SetVisible(false);
    allElements.push_back(this->batteryCharging);
    this->wifiStatus = new pu::element::Image(1126, 55, GetRomFsResource("3", "WiFi"));
    allElements.push_back(this->wifiStatus);

    // sidebar
    this->sidebarBackground = new pu::element::Rectangle(0, 0, 116, 720, {45, 45, 54, 255});
    this->sidebarBackground->SetDrawSideShadow(true, 20, 120);
    allElements.push_back(this->sidebarBackground);

    this->sidebarMenu = new pu::element::Menu(0, 120, 116, {0, 0, 0, 0}, 120, 5);
    this->sidebarMenu->SetOnFocusColor({36, 35, 39, 255});
    this->sidebarMenu->SetScrollbarColor({36, 35, 39, 255});
    this->sidebarMenu->SetScrollbarWidth(5);
    this->sidebarMenu->SetScrollbarPosition(1);
    this->sidebarMenu->SetDrawShadow(false);
    this->sidebarMenu->SetOnFocus(false);
    this->sidebarMenu->SetIconOnly(true);
    this->sidebarMenu->SetIconScale(0.8);
    allElements.push_back(this->sidebarMenu);

    this->sidebarLogo = new pu::element::Image(27, 29, GetRomFsResource("logo"));
    allElements.push_back(this->sidebarLogo);

    this->copyButton = new pu::element::MenuItem("");
    this->copyButton->SetIcon(GetRomFsResource("copy"));
    this->copyButton->AddOnClick(std::bind(&MainApplication::CopyAction, this), KEY_A);
    this->sidebarMenu->AddItem(copyButton);
    allMenuItems.push_back(this->copyButton);

    this->pasteButton = new pu::element::MenuItem("");
    this->pasteButton->SetIcon(GetRomFsResource("paste"));
    this->pasteButton->AddOnClick(std::bind(&MainApplication::PasteAction, this), KEY_A);
    this->sidebarMenu->AddItem(pasteButton);
    allMenuItems.push_back(this->pasteButton);

    this->deleteButton = new pu::element::MenuItem("");
    this->deleteButton->SetIcon(GetRomFsResource("delete"));
    this->deleteButton->AddOnClick(std::bind(&MainApplication::DeleteAction, this), KEY_A);
    this->sidebarMenu->AddItem(deleteButton);
    allMenuItems.push_back(this->deleteButton);

    this->renameButton = new pu::element::MenuItem("");
    this->renameButton->SetIcon(GetRomFsResource("rename"));
    this->renameButton->AddOnClick(std::bind(&MainApplication::RenameAction, this), KEY_A);
    this->sidebarMenu->AddItem(renameButton);
    allMenuItems.push_back(this->renameButton);

    this->exitButton = new pu::element::MenuItem("");
    this->exitButton->SetIcon(GetRomFsResource("exit"));
    this->exitButton->AddOnClick(std::bind(&MainApplication::ExitAction, this), KEY_A); // to do: exit action
    this->sidebarMenu->AddItem(exitButton);
    allMenuItems.push_back(this->exitButton);

    // buttons
    int btnSize = 25;
    u32 btnFntSize = 23;
    u32 btnAx = 1073;
    u32 btnAy = 652;
    u32 btnAx_img = 1088;
    int btnYx_img_fix = -6;
    u32 btnh = 65;
    u32 btnw = 130;
    u32 btny = btnAy + (btnh/2) - (btnSize/2) - 2;

    this->button_A = new pu::element::Button(btnAx, btnAy, btnw, btnh, "    Open", {0, 0, 0, 0}, {0, 0, 0, 0}, btnFntSize);
    allElements.push_back(this->button_A);
    this->button_A_icon = new pu::element::Image(btnAx_img, btny, GetRomFsResource("a_black", "Buttons"));
    allElements.push_back(this->button_A_icon);

    this->button_B = new pu::element::Button(btnAx - btnw, btnAy-2, btnw, btnh, "    Back", {0, 0, 0, 0}, {0, 0, 0, 0}, btnFntSize);
    allElements.push_back(this->button_B);
    this->button_B_icon = new pu::element::Image(btnAx_img - btnw, btny, GetRomFsResource("b_black", "Buttons"));
    allElements.push_back(this->button_B_icon);

    this->button_Y = new pu::element::Button(btnAx - 2*btnw, btnAy-2, btnw, btnh, "    Select", {0, 0, 0, 0}, {0, 0, 0, 0}, btnFntSize);
    allElements.push_back(this->button_Y);
    this->button_Y_icon = new pu::element::Image(btnAx_img - 2*btnw + btnYx_img_fix, btny, GetRomFsResource("y_black", "Buttons"));
    allElements.push_back(this->button_Y_icon);//

    this->button_R_STICK = new pu::element::Button(btnAx - 3*btnw, btnAy, btnw, btnh, "    Help", {0, 0, 0, 0}, {0, 0, 0, 0}, btnFntSize);
    allElements.push_back(this->button_R_STICK);
    this->button_R_STICK_icon = new pu::element::Image(btnAx_img - 3*btnw, btny, GetRomFsResource("r_stick_black", "Buttons"));
    allElements.push_back(this->button_R_STICK_icon);

    this->button_L_STICK = new pu::element::Button(btnAx - 4*btnw, btnAy-2, btnw, btnh, "   Sort", {0, 0, 0, 0}, {0, 0, 0, 0}, btnFntSize);
    allElements.push_back(this->button_L_STICK);
    this->button_L_STICK_icon = new pu::element::Image(btnAx_img - 4*btnw, btny, GetRomFsResource("l_stick_black", "Buttons"));
    allElements.push_back(this->button_L_STICK_icon);

    this->debugText = new pu::element::TextBlock(1, 1, "debug");
    debugText->SetColor({242, 38, 19, 1});
    //allElements.push_back(this->debugText);

    // load layouts and give them all the necessary elements
    this->LoadLayout(this->mainLayout);
    for (auto &l : allLayouts)
    {
        for (auto &e : allElements)
        {
            l->Add(e);
        }
    }
}

void MainApplication::InitAllModules()
{
    this->browser = new Browser();
}

void MainApplication::CopyAction()
{
    if (!this->GetMainLayout()->IsEmptyDirTextShown())
        this->browser->CopyFiles();
}

void MainApplication::MoveAction()
{
    if (!this->GetMainLayout()->IsEmptyDirTextShown())
        this->browser->MoveFiles();
}

void MainApplication::PasteAction()
{
    int choice = 0;
    if (this->GetBrowser()->GetClipboardSize() > 0)
        choice = this->CreateShowDialog("Do you want to paste the clipboard's content here?", "", {"No", "Yes"}, false);
    if (choice == 1)
        browser->PasteFiles();
}

void MainApplication::RenameAction()
{
    if (!this->GetMainLayout()->IsEmptyDirTextShown())
        this->browser->RenameFile();
}

void MainApplication::DeleteAction()
{
    if (!this->GetMainLayout()->IsEmptyDirTextShown())
    {
        int choice = 0;
        if (this->GetBrowser()->GetNumberOfSelected() > 1)
            choice = this->CreateShowDialog("Do you want to delete these items?", "", {"No", "Yes"}, false);
        else
            choice = this->CreateShowDialog("Do you want to delete this item?", this->GetBrowser()->GetFileName(), {"No", "Yes"}, false);
        if (choice == 1)
            browser->RemoveFiles();
    }
}

void MainApplication::NavigateBackAction()
{
    this->browser->NavigateBack();
}

void MainApplication::SelectAction()
{
    if (!this->GetMainLayout()->IsEmptyDirTextShown())
        this->browser->SelectFile();
}

void MainApplication::SortAction()
{
    int choice = this->CreateShowDialog("Choose file sorting method", "", {"By name", "By name reversed", "By size", "By size reversed"}, false);
    if (!this->GetMainLayout()->IsEmptyDirTextShown())
    {
        switch (choice)
        {
            case 0:
                browser->ChangeSortType(SortType::Alphabetical);
            break;

            case 1:
                browser->ChangeSortType(SortType::Alphabetical_Reversed);
            break;

            case 2:
                browser->ChangeSortType(SortType::By_Size);
            break;

            case 3:
                browser->ChangeSortType(SortType::By_Size_Reversed);
            break;
        }
    }
}

void MainApplication::InfoAction()
{
    if (!this->GetMainLayout()->IsEmptyDirTextShown())
    {
        if (this->GetBrowser()->GetNumberOfSelected() > 1)
        {
            u32 s  = this->GetBrowser()->GetFilesSize();
            auto [nodirs, nofiles] = this->GetBrowser()->CountMultipleFilesType();
            std::string name = "Multiple files\n";
            std::string path = "\nPath: " + WrapText(this->GetBrowser()->GetFilePath(), 80);
            std::string cont = "\nContains: " + std::to_string(nofiles) + " Files, " + std::to_string(nodirs) + " Folders";
            std::string size = "\nSize: " + FormatSize(s) + " (" + FormatNumber(s) + " Bytes)";
            this->CreateShowDialog("Properties", name + path + cont + size, {"Cancel"}, true);
        }
        else
        {
            bool t = this->GetBrowser()->GetFileType();
            u32 s  = this->GetBrowser()->GetFilesSize();
            auto [nodirs, nofiles] = this->GetBrowser()->CountMultipleFilesType();
            std::string name = "Name: " + WrapText(this->GetBrowser()->GetFileName(), 80);
            std::string path = "\nPath: " + WrapText(this->GetBrowser()->GetFilePath(), 80);
            std::string type = (t == 0) ? "\nType: File" : "\nType: Directory";
            std::string cont = (t == 0) ? "" : "\nContains: " + std::to_string(nofiles) + " Files, " + std::to_string(nodirs) + " Folders";
            std::string size = "\nSize: " + FormatSize(s) + " (" + FormatNumber(s) + " Bytes)";
            std::string perm = "\nPermissions: " + this->GetBrowser()->GetFilePermissions();
            this->CreateShowDialog("Properties", name + path + type + cont + size + perm, {"Cancel"}, true);
        }
    }
}

void MainApplication::HelpAction()
{
    this->helpinfo = !this->helpinfo;
}

void MainApplication::ExitAction()
{
    this->Close();//WithFadeOut();
}
