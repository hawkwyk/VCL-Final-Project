#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Engine/GL/Frame.hpp"
#include "Engine/GL/Program.h"
#include "Engine/GL/RenderItem.h"
#include "Labs/Common/OrbitCameraManager.h"
#include "Labs/Common/ICase.h"
#include "Labs/Common/ImageRGB.h"

#include "Labs/FinalProject/Skeleton.h"
#include "Labs/FinalProject/Player.h"
#include "Labs/FinalProject/BVHLoader.h"
#include "Labs/FinalProject/CaseBVH.h"

namespace VCX::Labs::FinalProject 
{
    class CaseSkeleton : public Common::ICase 
    {
    public:
        CaseSkeleton();

        virtual std::string_view const GetName() override { return "Skeleton Structure"; }

        virtual void OnSetupPropsUI() override;
        virtual Common::CaseRenderResult OnRender(std::pair<std::uint32_t, std::uint32_t> const desiredSize) override;
        virtual void OnProcessInput(ImVec2 const & pos) override;
    
    private:
        Engine::GL::UniqueProgram               _program;
        Engine::GL::UniqueRenderFrame           _frame;
        Engine::Camera                          _camera { .Eye = glm::vec3(-3, 3, 3) };
        Common::OrbitCameraManager              _cameraManager;
        int                                     _aaSamples     { 1 };    // Anti-aliasing samples (1 = no AA, 2, 4, 8, 16)
        
        // Skeleton data
        std::string                             _filePath = "assets/BVH_data/01_01.bvh";
        Skeleton                                _skeleton;
        Action                                  _action;
        BVHLoader                               _BVHLoader;
        
        // Hover detection
        ImVec2                                  _lastMousePos;
        int                                     _hoveredJointIndex { -1 };
        std::string                             _hoveredJointName;
        
        // Helper method for checking mouse hover on joints
        void CheckJointHover(std::pair<std::uint32_t, std::uint32_t> const desiredSize, ImVec2 const & mousePos);

        BackGroundRender                        BackGround;
        SkeletonRender                          skeletonRender;
    };
}