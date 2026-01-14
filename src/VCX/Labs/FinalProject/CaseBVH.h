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

namespace VCX::Labs::FinalProject 
{   
    class BackGroundRender
    {
    public:
        BackGroundRender();
        void render(Engine::GL::UniqueProgram & program);
    
    public:
        Engine::GL::UniqueIndexedRenderItem LineItem;
    };

    class SkeletonRender
    {
    public: 
        SkeletonRender();

        void render(Engine::GL::UniqueProgram & program);
        void load(const Skeleton & skele);
        void loadAll(const Skeleton & skele);
    
    public:
        Engine::GL::UniqueIndexedRenderItem LineItem;
        Engine::GL::UniqueRenderItem        PointItem;
    };

    class CaseBVH : public Common::ICase 
    {
    public:
        CaseBVH();

        virtual std::string_view const GetName() override { return "BVH Animation"; }

        virtual void OnSetupPropsUI() override;
        virtual Common::CaseRenderResult OnRender(std::pair<std::uint32_t, std::uint32_t> const desiredSize) override;
        virtual void OnProcessInput(ImVec2 const & pos) override;
    
    private:
        Engine::GL::UniqueProgram               _program;
        Engine::GL::UniqueRenderFrame           _frame;
        Engine::Camera                          _camera { .Eye = glm::vec3(-3, 3, 3) };
        Common::OrbitCameraManager              _cameraManager;
        bool                                    _stopped       { false };
        float                                   _speed         { 1.0f }; // Animation speed multiplier
        int                                     _aaSamples     { 1 };    // Anti-aliasing samples (1 = no AA, 2, 4, 8, 16)
        
        // Video export variables
        bool                                    _exporting     { false };
        std::string                             _exportDir     { "exported_frames" };
        int                                     _exportFrame   { 0 };
        int                                     _exportFps     { 30 };
        
        // Helper method for saving frames
        void SaveFrame(Engine::GL::UniqueTexture2D const & tex, std::pair<std::uint32_t, std::uint32_t> texSize);

        BackGroundRender                        BackGround;
        SkeletonRender                          skeletonRender;


        // self defined object 
        std::string                             _filePath = "assets/BVH_data/01_01.bvh";
        Skeleton                                _skeleton;
        Action                                  _action;
        BVHLoader                               _BVHLoader;
    };
}