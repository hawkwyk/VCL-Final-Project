#include "Labs/FinalProject/Skeleton.h"

namespace VCX::Labs::FinalProject
{
    Joint::Joint(){}    Skeleton::Skeleton() : Root(nullptr) {}
    
    Skeleton::~Skeleton() {
        Clear();
    }
    
    void Skeleton::Clear() {
        if (Root) {
            ClearJoint(Root);
            Root = nullptr;
        }
    }
    
    void Skeleton::ClearJoint(Joint* ptr) {
        if (!ptr) return;
        
        // Clear all children first
        Joint* child = ptr->ChiPtr;
        while (child) {
            Joint* nextChild = child->BroPtr;
            ClearJoint(child);
            child = nextChild;
        }
        
        // Delete the current joint
        delete ptr;
    }

    std::pair<std::vector<glm::vec3>, std::vector<std::uint32_t>> Skeleton::Convert() const
    {
        std::vector<glm::vec3>     Positions;
        std::vector<std::uint32_t> Indices;

        Construct(Root, Positions, Indices);

        return {Positions, Indices};
    }

    void Skeleton::Construct(const Joint *ptr, std::vector<glm::vec3> & Positions, std::vector<std::uint32_t> & Indices) const
    {
        std::uint32_t FatherIdx = Positions.size();
        Positions.push_back(ptr->GlobalPosition);

        Joint * NextPtr = ptr->ChiPtr; 
        while(NextPtr != nullptr)
        {
            std::uint32_t ChildIdx = Positions.size();
            Positions.push_back(NextPtr->GlobalPosition);
            Indices.push_back(FatherIdx);
            Indices.push_back(ChildIdx);
            Construct(NextPtr, Positions, Indices);

            NextPtr = NextPtr->BroPtr;
        }
    }

    void Skeleton::ForwardKinematics()
    {
        // Init Root
        Root->GlobalPosition = Root->LocalOffset;
        Root->GlobalRotation = Root->LocalRotation;

        // Forward Kinematics
        ItsMyGo(Root);

        Adjust(Root);
    }

    void Skeleton::ItsMyGo(Joint * ptr)
    {
        Joint* NextPtr = ptr->ChiPtr;
        while(NextPtr != nullptr)
        {
            NextPtr->GlobalRotation = ptr->GlobalRotation * NextPtr->LocalRotation;
            NextPtr->GlobalPosition = ptr->GlobalPosition + ptr->GlobalRotation * NextPtr->LocalOffset;
            ItsMyGo(NextPtr);

            NextPtr = NextPtr->BroPtr;
        }
    }

    void Skeleton::Adjust(Joint* ptr)
    {
        ptr->GlobalPosition = Scale * ptr->GlobalPosition + Offset;

        Joint* NextPtr = ptr->ChiPtr;
        while(NextPtr != nullptr)
        {
            Adjust(NextPtr);
            NextPtr = NextPtr->BroPtr;
        }
    }

    int Skeleton::GetJointCount() const
    {
        int count = 0;
        if (Root) {
            CountJoints(Root, count);
        }
        return count;
    }

    void Skeleton::CountJoints(const Joint* ptr, int& count) const
    {
        if (!ptr) return;
        
        count++;
        
        // Count child joints
        Joint* child = ptr->ChiPtr;
        while (child) {
            CountJoints(child, count);
            child = child->BroPtr;
        }
    }

    std::string Skeleton::GetJointName(int index) const
    {
        std::vector<std::string> jointNames;
        if (Root) {
            // Use the same traversal order as Construct()
            std::vector<glm::vec3> dummyPositions;
            std::vector<std::uint32_t> dummyIndices;
            Construct(Root, dummyPositions, dummyIndices);
            
            // Now collect names in the exact same order as positions
            std::vector<std::string> allNames;
            std::vector<glm::vec3> namePositions;
            std::vector<std::uint32_t> nameIndices;
            CollectJointNames(Root, allNames, namePositions, nameIndices);
            
            // Create a mapping from position to name
            for (size_t i = 0; i < dummyPositions.size(); i++) {
                // Find the joint with this position
                for (size_t j = 0; j < namePositions.size(); j++) {
                    if (glm::distance(dummyPositions[i], namePositions[j]) < 0.001f) {
                        jointNames.push_back(allNames[j]);
                        break;
                    }
                }
            }
        }
        
        if (index >= 0 && index < jointNames.size()) {
            return jointNames[index];
        }
        return "";
    }

    void Skeleton::CollectJointNames(const Joint* ptr, std::vector<std::string>& names, std::vector<glm::vec3>& positions, std::vector<std::uint32_t>& indices) const
    {
        if (!ptr) return;
        
        // Add current joint
        names.push_back(ptr->Name);
        positions.push_back(ptr->GlobalPosition);
        
        Joint* NextPtr = ptr->ChiPtr;
        while(NextPtr != nullptr)
        {
            // Add child joint
            names.push_back(NextPtr->Name);
            positions.push_back(NextPtr->GlobalPosition);
            
            // Recursively process child
            CollectJointNames(NextPtr, names, positions, indices);

            NextPtr = NextPtr->BroPtr;
        }
    }
}