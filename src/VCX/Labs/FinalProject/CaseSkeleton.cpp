#include "Engine/app.h"
#include "Labs/FinalProject/CaseSkeleton.h"
#include "Labs/Common/ImGuiHelper.h"
#include <stb_image_write.h>
#include <filesystem>
#include <algorithm>

namespace VCX::Labs::FinalProject 
{
    CaseSkeleton::CaseSkeleton() :
        _program(
            Engine::GL::UniqueProgram({
                Engine::GL::SharedShader("assets/shaders/flat.vert"),
                Engine::GL::SharedShader("assets/shaders/flat.frag")}))
        {
            _cameraManager.AutoRotate = false;
            _cameraManager.Save(_camera);

            _BVHLoader.Load(_filePath.c_str(), _skeleton, _action);
            _skeleton.ForwardKinematics();
            skeletonRender.loadAll(_skeleton);
        }

        void CaseSkeleton::OnSetupPropsUI()
        {
            // Camera controls note at the top
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Note:");
            ImGui::TextWrapped("Use left button to rotate, right button to move camera position, and wheel to zoom in/out. Hover over joints to see their names.");
            ImGui::Separator();
            
            ImGui::Text("BVH Skeleton Viewer");
            
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
                    _skeleton.ForwardKinematics();
                    skeletonRender.loadAll(_skeleton);
                    _action.Reset();
                    _hoveredJointIndex = -1;
                    _hoveredJointName.clear();
                }
            }
            
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
            
            // Display hovered joint info
            if (_hoveredJointIndex != -1) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Hovered Joint:");
                ImGui::Text("Index: %d", _hoveredJointIndex);
                ImGui::Text("Name: %s", _hoveredJointName.c_str());
            }
        }

        Common::CaseRenderResult CaseSkeleton::OnRender(std::pair<std::uint32_t, std::uint32_t> const desiredSize)
        {
            skeletonRender.load(_skeleton);

            _frame.Resize(desiredSize, _aaSamples);

            _cameraManager.Update(_camera);

            _program.GetUniforms().SetByName("u_Projection", _camera.GetProjectionMatrix((float(desiredSize.first) / desiredSize.second)));
            _program.GetUniforms().SetByName("u_View"      , _camera.GetViewMatrix());

            gl_using(_frame);

            BackGround.render(_program);
            skeletonRender.render(_program);

            glPointSize(1.f);
            
            // Check for joint hover
            CheckJointHover(desiredSize, _lastMousePos);
            
            return Common::CaseRenderResult{
                .Fixed      = false,
                .Flipped    = true,
                .Image      = _frame.GetColorAttachment(),
                .ImageSize  = desiredSize,
            };
        }

        void CaseSkeleton::CheckJointHover(std::pair<std::uint32_t, std::uint32_t> const desiredSize, ImVec2 const & mousePos)
        {
            if (mousePos.x < 0 || mousePos.y < 0 || mousePos.x >= desiredSize.first || mousePos.y >= desiredSize.second) {
                _hoveredJointIndex = -1;
                _hoveredJointName.clear();
                return;
            }

            // Get the current joint positions
            auto res = _skeleton.Convert();
            const auto& jointPositions = res.first;
            
            // Projection matrix
            glm::mat4 projection = _camera.GetProjectionMatrix((float(desiredSize.first) / desiredSize.second));
            glm::mat4 view = _camera.GetViewMatrix();
            glm::mat4 model = glm::mat4(1.0f);
            glm::mat4 mvp = projection * view * model;
            
            float closestDistance = FLT_MAX;
            int closestJoint = -1;
            
            // Convert mouse position to normalized device coordinates
            float x = (2.0f * mousePos.x) / desiredSize.first - 1.0f;
            float y = 1.0f - (2.0f * mousePos.y) / desiredSize.second;
            
            for (size_t i = 0; i < jointPositions.size(); i++) {
                // Project joint position to screen space
                glm::vec4 clipPos = mvp * glm::vec4(jointPositions[i], 1.0f);
                
                // Perspective division
                if (clipPos.w > 0.0f) {
                    glm::vec3 ndcPos = glm::vec3(clipPos) / clipPos.w;
                    
                    // Calculate distance from mouse to joint in screen space
                    float dx = ndcPos.x - x;
                    float dy = ndcPos.y - y;
                    float distance = dx * dx + dy * dy;
                    
                    // Check if this joint is closer than the previous closest
                    if (distance < closestDistance && distance < 0.1f) { // Increased threshold for better detection of smaller/farther joints like arms and hands
                        closestDistance = distance;
                        closestJoint = static_cast<int>(i);
                    }
                }
            }
            
            // Update hovered joint info
            if (closestJoint != -1 && closestJoint < jointPositions.size()) {
                _hoveredJointIndex = closestJoint;
                _hoveredJointName = _skeleton.GetJointName(closestJoint);
            } else {
                _hoveredJointIndex = -1;
                _hoveredJointName.clear();
            }
        }
        
        void CaseSkeleton::OnProcessInput(ImVec2 const & pos)
        {
            _cameraManager.ProcessInput(_camera, pos);
            _lastMousePos = pos;
        }
}