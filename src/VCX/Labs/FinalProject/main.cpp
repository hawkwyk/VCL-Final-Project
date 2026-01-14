#include "Assets/bundled.h"
#include "Labs/FinalProject/App.h"

int main()
{
    using namespace VCX;
    return Engine::RunApp<Labs::FinalProject::App>(Engine::AppContextOptions {
        .Title = "VCX Final Project",
        .WindowSize = { 1024, 768 },
        .FontSize = 16,

        .IconFileNames = Assets::DefaultIcons,
        .FontFileNames = Assets::DefaultFonts,
    });
}