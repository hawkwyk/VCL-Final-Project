#include "Assets/bundled.h"
#include "Labs/FinalProject/App.h"

namespace VCX::Labs::FinalProject {
    using namespace Assets;

    App::App() :
        _ui(Labs::Common::UIOptions { })
    {}

    void App::OnFrame() {
        _ui.Setup(_cases, _caseId);
    }
}