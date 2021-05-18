#pragma once

#include "pch.h"

#include "Mesh.h"

#include <assimp/scene.h>

class MeshModel
{
public:
	MeshModel() = default;
	MeshModel(const std::vector<Mesh>& meshList);
	~MeshModel() = default;

	size_t GetMeshCount() const;
	Mesh* GetMesh(const size_t index);
	glm::mat4 GetModel();
	void SetModel(const glm::mat4& model);
	void DestroyMeshModel();

	static std::vector<std::string> LoadMaterials(const aiScene *scene);
	static std::vector<Mesh> LoadNode(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,
		aiNode* node, const aiScene* scene, std::vector<int> matToTex);
	static Mesh LoadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,
		aiMesh* mesh, const aiScene* scene, std::vector<int> matToTex);

private:
	std::vector<Mesh> m_MeshList;
	glm::mat4 m_Model;
};

