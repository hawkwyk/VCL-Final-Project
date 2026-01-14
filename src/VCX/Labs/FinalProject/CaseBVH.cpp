#include "Engine/app.h"
#include "Labs/FinalProject/CaseBVH.h"
#include "Labs/Common/ImGuiHelper.h"
#include <stb_image_write.h>
#include <filesystem>
#include <algorithm>

namespace VCX::Labs::FinalProject 
{
    /**
     * BackGround Section
    */
    BackGroundRender::BackGroundRender():
        LineItem(Engine::GL::VertexLayout().Add<glm::vec3>("position", Engine::GL::DrawFrequency::Stream, 0), Engine::GL::PrimitiveType::Triangles)
    {
        std::vector<glm::vec3> poses;
        std::vector<std::uint32_t> indices;

        // Create a large rectangle floor (two triangles)
        float size = 30.0f; // Size of the floor
        poses.push_back({ -size, 0.0f, -size }); // Bottom-left
        poses.push_back({  size, 0.0f, -size }); // Bottom-right
        poses.push_back({  size, 0.0f,  size }); // Top-right
        poses.push_back({ -size, 0.0f,  size }); // Top-left

        // Two triangles forming a rectangle
        indices.push_back(0); indices.push_back(1); indices.push_back(2);
        indices.push_back(0); indices.push_back(2); indices.push_back(3);

        // Update
        LineItem.UpdateVertexBuffer("position", Engine::make_span_bytes<glm::vec3>(poses));
        LineItem.UpdateElementBuffer(indices);
    };

    void BackGroundRender::render(Engine::GL::UniqueProgram & program)
    {
        program.GetUniforms().SetByName("u_Color", glm::vec3( 128.0f/255, 128.0f/255, 128.0f/255 )); // Neutral gray color
        LineItem.Draw({ program.Use() });
    }
    // BackGround End

    /**
     * SkeletonRender Section
    */
    SkeletonRender::SkeletonRender():
        LineItem(Engine::GL::VertexLayout().Add<glm::vec3>("position", Engine::GL::DrawFrequency::Stream, 0), Engine::GL::PrimitiveType::Lines),
        PointItem(Engine::GL::VertexLayout().Add<glm::vec3>("position", Engine::GL::DrawFrequency::Stream, 0), Engine::GL::PrimitiveType::Points)
    {}

    void SkeletonRender::render(Engine::GL::UniqueProgram & program)
    {
        program.GetUniforms().SetByName("u_Color", glm::vec3( 1.0f, 0.0f, 0.0f ));
        glPointSize(10.f);
        PointItem.Draw({ program.Use() });
        glPointSize(1.f);

        program.GetUniforms().SetByName("u_Color", glm::vec3( 1.0f, 1.0f, 1.0f ));
        glLineWidth(3.f);
        LineItem.Draw({ program.Use() });
        glLineWidth(1.f);
    }

    void SkeletonRender::load(const Skeleton & skele)
    {
        auto res = skele.Convert();
        LineItem.UpdateVertexBuffer("position", Engine::make_span_bytes<glm::vec3>(res.first));

        PointItem.UpdateVertexBuffer("position", Engine::make_span_bytes<glm::vec3>(res.first));
    }
    void SkeletonRender::loadAll(const Skeleton & skele)
    {
        auto res = skele.Convert();
        LineItem.UpdateElementBuffer(res.second);
        LineItem.UpdateVertexBuffer("position", Engine::make_span_bytes<glm::vec3>(res.first));

        PointItem.UpdateVertexBuffer("position", Engine::make_span_bytes<glm::vec3>(res.first));
    }
    // SkeletonRender End



    CaseBVH::CaseBVH() :
        _program(
            Engine::GL::UniqueProgram({
                Engine::GL::SharedShader("assets/shaders/flat.vert"),
                Engine::GL::SharedShader("assets/shaders/flat.frag")}))
        {
            _cameraManager.AutoRotate = false;
            _cameraManager.Save(_camera);

            _BVHLoader.Load(_filePath.c_str(), _skeleton, _action);

            skeletonRender.loadAll(_skeleton);
        }

        void CaseBVH::OnSetupPropsUI()
        {
            // Camera controls note at the top
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Note:");
            ImGui::TextWrapped("Use left button to rotate, right button to move camera position, and wheel to zoom in/out.");
            ImGui::Separator();
            
            ImGui::Text("BVH Animation Player");
            
            // File selection dropdown
            ImGui::Text("Select BVH File:");
            
            // Dynamically load BVH files from directory
            static std::vector<std::string> bvh_files;
            static bool files_loaded = false;
            
            if (!files_loaded) {
                // Load all BVH files from the directory
                namespace fs = std::filesystem;
                std::string bvh_dir = "assets/BVH_data";
                
                if (fs::exists(bvh_dir) && fs::is_directory(bvh_dir)) {
                    for (const auto& entry : fs::directory_iterator(bvh_dir)) {
                        if (entry.is_regular_file() && entry.path().extension() == ".bvh") {
                            // Convert to forward slashes for consistency with the rest of the codebase
                            std::string path_str = entry.path().string();
                            std::replace(path_str.begin(), path_str.end(), '\\', '/');
                            bvh_files.push_back(path_str);
                        }
                    }
                }
                
                files_loaded = true;
                
                // Set default file if available
                if (!bvh_files.empty()) {
                    _filePath = bvh_files[0];
                }
            }
            
            // Create a list of const char* for ImGui combo
            static std::vector<const char*> bvh_files_ptr;
            if (!bvh_files_ptr.empty()) {
                bvh_files_ptr.clear();
            }
            for (const auto& file : bvh_files) {
                bvh_files_ptr.push_back(file.c_str());
            }
            
            static int current_file = 0;
            if (ImGui::Combo("##bvh_file_combo", &current_file, bvh_files_ptr.data(), static_cast<int>(bvh_files_ptr.size()))) {
                if (current_file >= 0 && current_file < static_cast<int>(bvh_files.size())) {
                    _filePath = bvh_files[current_file];
                    _BVHLoader.Load(_filePath.c_str(), _skeleton, _action);
                    skeletonRender.loadAll(_skeleton);
                    _action.Reset();
                }
            }
            
            // Animation control buttons
            ImGui::Separator();
            ImGui::Text("Animation Controls:");
            
            if (ImGui::Button(_stopped ? "Play" : "Pause")) {
                _stopped = !_stopped;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                _action.Reset();
                _stopped = true;
            }
            
            // Animation progress
            static float progress = 0.0f;
            if (!_stopped) {
                progress = static_cast<float>(_action.TimeIndex) / _action.Frames;
            }
            ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), nullptr);
            
            // Frame counter
            ImGui::Text("Frame: %d / %d", _action.TimeIndex, _action.Frames);
            
            // Animation speed control
            ImGui::Separator();
            ImGui::Text("Animation Speed:");
            ImGui::SliderFloat("Speed", &_speed, 0.1f, 3.0f, "%.1f");
            ImGui::Text("Speed: %.1fx", _speed);
            
            ImGui::Separator();
            ImGui::Text("Anti-aliasing:");
            // Anti-aliasing sample count options
            static const char* aaOptions[] = {"Off (1x)", "2x MSAA", "4x MSAA", "8x MSAA", "16x MSAA"};
            static const int aaValues[] = {1, 2, 4, 8, 16};
            static int currentAA = 0; // Default to no AA
            
            // Find current index based on _aaSamples
            for (int i = 0; i < 5; i++) {
                if (aaValues[i] == _aaSamples) {
                    currentAA = i;
                    break;
                }
            }
            
            if (ImGui::Combo("Quality", &currentAA, aaOptions, 5)) {
                _aaSamples = aaValues[currentAA];
                // Force frame resize with new sample count
                auto size = _frame.GetSize();
                _frame.Resize(size, _aaSamples);
            }
            
            ImGui::Separator();
            ImGui::Text("Video Export:");
            
            // Export directory input
            static char exportDir[256] = "exported_frames";
            ImGui::Text("Export Directory:");
            ImGui::InputText("##ExportDir", exportDir, IM_ARRAYSIZE(exportDir));
            
            // Export FPS control
            int exportFps = _exportFps;
            ImGui::SliderInt("FPS", &exportFps, 10, 60, "%d");
            if (exportFps != _exportFps) {
                _exportFps = exportFps;
            }
            
            // Export control buttons
            if (_exporting) {
                if (ImGui::Button("Stop Export")) {
                    _exporting = false;
                    ImGui::Text("Export completed: %d frames saved", _exportFrame);
                }
                ImGui::Text("Exporting... Frame: %d", _exportFrame);
            } else {
                if (ImGui::Button("Start Export")) {
                    _exporting = true;
                    _exportDir = exportDir;
                    _exportFrame = 0;
                    // Reset animation to beginning
                    _action.Reset();
                    _stopped = false;
                }
            }
        }

        Common::CaseRenderResult CaseBVH::OnRender(std::pair<std::uint32_t, std::uint32_t> const desiredSize)
        {
            if (!_stopped)
            {
                _action.Load(_skeleton, Engine::GetDeltaTime() * _speed);
            }

            skeletonRender.load(_skeleton);

            _frame.Resize(desiredSize, _aaSamples);

            _cameraManager.Update(_camera);

            _program.GetUniforms().SetByName("u_Projection", _camera.GetProjectionMatrix((float(desiredSize.first) / desiredSize.second)));
            _program.GetUniforms().SetByName("u_View"      , _camera.GetViewMatrix());

            gl_using(_frame);

            BackGround.render(_program);
            skeletonRender.render(_program);

            glPointSize(1.f);
            
            // Save frame if exporting
            if (_exporting) {
                SaveFrame(_frame.GetColorAttachment(), desiredSize);
                
                // Check if animation is complete
                if (_action.TimeIndex >= _action.Frames - 1) {
                    _exporting = false;
                    _stopped = true;
                }
            }
            
            return Common::CaseRenderResult{
                .Fixed      = false,
                .Flipped    = true,
                .Image      = _frame.GetColorAttachment(),
                .ImageSize  = desiredSize,
            };
        }

        void CaseBVH::SaveFrame(Engine::GL::UniqueTexture2D const & tex, std::pair<std::uint32_t, std::uint32_t> texSize)
        {
            // Create export directory if it doesn't exist
            std::filesystem::create_directories(_exportDir);
            
            // Generate filename with frame number (e.g., frame_0001.png)
            std::string filename = _exportDir + "/frame_" + fmt::format("{:04d}", _exportFrame) + ".png";
            
            // Save the frame
            gl_using(tex);
            std::vector<unsigned char> rawImg(texSize.first * texSize.second * 3);
            
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, rawImg.data());
            glPixelStorei(GL_PACK_ALIGNMENT, 4);
            
            // Flip vertically since OpenGL textures are stored upside down
            stbi_flip_vertically_on_write(true);
            stbi_write_png(filename.c_str(), texSize.first, texSize.second, 3, rawImg.data(), 3 * texSize.first);
            
            _exportFrame++;
        }
        
        void CaseBVH::OnProcessInput(ImVec2 const & pos)
        {
            _cameraManager.ProcessInput(_camera, pos);
        }
}

