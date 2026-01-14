#pragma once

#include <vector>

#include "Engine/app.h"

#include "Labs/FinalProject/CaseBVH.h"
#include "Labs/FinalProject/CaseSkeleton.h"
#include "Labs/Common/UI.h"

namespace VCX::Labs::FinalProject {
    class App : public Engine::IApp {
    private:
        Common::UI      _ui;

        std::size_t     _caseId = 0;

        CaseSkeleton _caseSkeleton;
        CaseBVH      _caseBVH;

        std::vector<std::reference_wrapper<Common::ICase>> _cases = {
            _caseSkeleton,
            _caseBVH
        };
    public:
        App();

        void OnFrame() override;
    };
}


