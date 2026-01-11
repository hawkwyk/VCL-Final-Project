#pragma once

#include <vector>

#include "Engine/app.h"
#include "Labs/Final_Project/CaseBVH.h"
#include "Labs/Common/UI.h"

namespace VCX::Labs::Final_Project {

    class App : public Engine::IApp {
    private:
        Common::UI             _ui;

        CaseBVH                _caseBVH;

        std::size_t        _caseId = 0;

        std::vector<std::reference_wrapper<Common::ICase>> _cases = {
            _caseBVH
        };

    public:
        App();
        void OnFrame() override;
    };
}