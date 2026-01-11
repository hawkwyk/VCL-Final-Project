#include "Assets/bundled.h"
#include "Labs/Final_Project/App.h"

int main() {
    using namespace VCX;
    return Engine::RunApp<Labs::Final_Project::App>(Engine::AppContextOptions {
        .Title         = "VCX Labs Final Project: BVH Animation",
        .WindowSize    = { 1024, 768 },
        .FontSize      = 16,
        .IconFileNames = Assets::DefaultIcons,
        .FontFileNames = Assets::DefaultFonts,
    });
}
