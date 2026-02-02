#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>

#include "../Offsets/offsets.h"
#include "../Misc/structures.h"
#include "../Memory/MemoryManager.h"

class RobloxInstance
{
public:
	uintptr_t address;

	RobloxInstance(uintptr_t addy)
	{
		address = addy;
	}

	operator bool() const
	{
		return address != 0;
	}

	inline std::string Name() const
	{
		return Memory->readString(Memory->read<uintptr_t>(address + offsets::Name));
	}

	inline std::string Class() const
	{
		return Memory->readString(Memory->read<uintptr_t>(Memory->read<uintptr_t>(address + offsets::ClassDescriptor) + offsets::ClassDescriptorToClassName));
	}

	static std::string GetTheClassName(uintptr_t address)
	{
		uintptr_t class_ptr = Memory->read<uintptr_t>(address + offsets::ClassDescriptor);
		uintptr_t class_name_ptr = Memory->read<uintptr_t>(class_ptr + 0x8);
		std::string name = Memory->readString(class_name_ptr);

		return name;
	}


	RobloxInstance FindFirstChildOfClass(const std::string& Name)
	{
		std::vector<RobloxInstance> children = this->GetChildren();

		for (auto& object : children)
		{
			if (object.Class() == Name)
			{
				return object;
			}
		}
		return false;
	}

	inline bool IsA(std::string className) const
	{
		if (Class() == className)
		{
			return true;
		}
		return false;
	}

	inline std::vector<RobloxInstance> GetChildren() const
	{
		uintptr_t childrenStart = Memory->read<uintptr_t>(address + offsets::Children);
		uintptr_t childrenEnd = Memory->read<uintptr_t>(childrenStart + offsets::ChildrenEnd);

		std::vector<RobloxInstance> returnVector;

		for (uintptr_t child = Memory->read<uintptr_t>(childrenStart); child < childrenEnd; child += 0x10)
		{
			returnVector.emplace_back(RobloxInstance(Memory->read<uintptr_t>(child)));
		}

		return returnVector;
	}

	inline RobloxInstance FindFirstChild(std::string name = "") const
	{
		for (auto& child : this->GetChildren())
		{
			if (name == "")
				return child;

			if (child.Name() == name)
				return child;
		}
		return 0;
	}

	inline RobloxInstance FindFirstChildWhichIsA(std::string className = "") const
	{
		for (auto& child : this->GetChildren())
		{
			if (child.Class() == className)
				return child;
		}
		return 0;
	}

	std::vector<RobloxInstance> GetDescendants() const {
		std::vector<RobloxInstance> descendants;

		auto children = GetChildren();
		for (auto& child : children) {
			descendants.push_back(child);

			auto sub_descendants = child.GetDescendants();
			descendants.insert(descendants.end(), sub_descendants.begin(), sub_descendants.end());
		}

		return descendants;
	}

	inline Vectors::Vector3 Position() const
	{
		return Memory->read<Vectors::Vector3>(Memory->read<uintptr_t>(address + offsets::Primitive) + offsets::Position);
	}

	inline float Transparency() const
	{
		return Memory->read<float>(address + offsets::Transparency);
	}

	inline Vectors::Vector3 Size() const
	{
		return Memory->read<Vectors::Vector3>(Memory->read<uintptr_t>(address + offsets::Primitive) + offsets::PartSize);
	}

	inline sCFrame CFrame() const
	{
		if (Class() == "Camera")
		{
			auto rotation = Memory->read<Matrixes::Matrix3x3>(address + offsets::CameraRotation);
			auto position = Memory->read<Vectors::Vector3>(address + offsets::CameraPos);

			sCFrame newCFrame
			{
				rotation.r00, rotation.r01, rotation.r02,
				rotation.r10, rotation.r11, rotation.r12,
				rotation.r20, rotation.r21, rotation.r22,
				position.x, position.y, position.z
			};
			return newCFrame;
		}
		else
		{
			uintptr_t primitiveAddr = Memory->read<uintptr_t>(address + offsets::Primitive);

			auto rotation = Memory->read<Matrixes::Matrix3x3>(primitiveAddr + offsets::Rotation);
			auto position = Memory->read<Vectors::Vector3>(primitiveAddr + offsets::Position);

			sCFrame newCFrame
			{
				rotation.r00, rotation.r01, rotation.r02,
				rotation.r10, rotation.r11, rotation.r12,
				rotation.r20, rotation.r21, rotation.r22,
				position.x, position.y, position.z
			};
			return newCFrame;
		}
	}

	Matrixes::RMatrix3 GetCameraRotation() {
		if (!address)
			return Matrixes::RMatrix3{};

		return Memory->read<Matrixes::RMatrix3>(address + offsets::CameraRotation);
	}

	inline RobloxInstance Character() const
	{
		return RobloxInstance(Memory->read<uintptr_t>(address + offsets::ModelInstance));
	}

	inline float Health() const // this way you can get it from the player instead of getting it from the humanoid
	{
		auto character = Character();
		auto humanoid = character.FindFirstChildWhichIsA("Humanoid");

		return Memory->read<float>(humanoid.address + offsets::Health);
	}

	inline float MaxHealth() const // same here
	{
		auto character = Character();
		auto humanoid = character.FindFirstChildWhichIsA("Humanoid");

		return Memory->read<float>(humanoid.address + offsets::MaxHealth);
	}

	inline RobloxInstance Team() const
	{
		return RobloxInstance(Memory->read<uintptr_t>(address + offsets::Team));
	}

	inline uint64_t UserID() const
	{
		return static_cast<uint64_t>(Memory->read<int>(address + offsets::UserId));
	}

	inline int RigType() const
	{
		return Memory->read<int>(address + offsets::RigType);
	}

	void SetPosition(Vectors::Vector3 value) {
		auto prim = Memory->read<uintptr_t>(address + offsets::Primitive);
		Memory->write<Vectors::Vector3>(prim + offsets::Position, value);
	}

	void SetRotation(Matrixes::RMatrix3 value) {
		Memory->write<Matrixes::RMatrix3>(this->address + offsets::CameraRotation, value);
	}

	inline void SetTransparency(float value)
	{
		Memory->write<float>(address + offsets::Transparency, value);
	}

	inline void SetWalkspeed(float value)
	{
		value = std::round(value);
		Memory->write(address + offsets::WalkSpeedCheck, value);
		Memory->write(address + offsets::WalkSpeed, value);
	}

	inline void SetJumpPower(float value)
	{
		value = std::round(value);
		Memory->write(address + offsets::JumpPower, value);
	}

	inline float GetWalkspeed()
	{
		return std::round(Memory->read<float>(address + offsets::WalkSpeed));
	}

	inline float GetJumpPower()
	{
		return std::round(Memory->read<float>(address + offsets::JumpPower));
	}

	inline float GetFOV()
	{
		auto radiantsFOV = Memory->read<float>(address + offsets::FOV);

		//gotta convert radiants to degrees

		auto degreesFOV = radiantsFOV * 180 / 3.1415926535;

		return std::round(degreesFOV);

	}

	inline void SetFOV(float value)
	{
		value = std::round(value);
		//gotta convert degrees to radiants

		auto radiantsValue = value * 3.1415926535 / 180;

		Memory->write<float>(address + offsets::FOV, radiantsValue);
	}

};