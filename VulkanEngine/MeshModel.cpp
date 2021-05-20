#include "pch.h"
#include "MeshModel.h"

MeshModel::MeshModel(const std::vector<Mesh>& meshList)
{
	m_MeshList = meshList;
	m_Model = glm::mat4(1.0f);
}

size_t MeshModel::GetMeshCount() const
{
	return m_MeshList.size();
}

Mesh* MeshModel::GetMesh(const size_t index)
{
	if (index >= m_MeshList.size())
	{
		throw std::runtime_error("Attempted to access invalid mesh index.");
	}

	return &m_MeshList[index];
}

glm::mat4 MeshModel::GetModel()
{
	return m_Model;
}

void MeshModel::SetModel(const glm::mat4& model)
{
	m_Model = model;
}

void MeshModel::DestroyMeshModel()
{
	for (auto& mesh : m_MeshList)
	{
		mesh.destroyBuffers();
	}
}

std::vector<std::string> MeshModel::LoadMaterials(const aiScene* scene)
{
	// Creazione 1:1 lista di textures
	std::vector<std::string>texture_list(scene->mNumMaterials);

	// Copia del nome della texture (nel caso in cui esista)
	for (size_t i = 0; i < scene->mNumMaterials; i++)
	{
		aiMaterial* material = scene->mMaterials[i];

		texture_list[i] = "";

		//material->Get(AI_MATKEY_TEXTURE(aitexc))


		// Controllo l'esistenza di una texture diffusa (colore semplice)
		if (material->GetTextureCount(aiTextureType_DIFFUSE))
		{
			// Prelevamento della path
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
			{
				// Eliminazione delle relative paths
				const int idx = std::string(path.data).rfind("\\");
				std::string file_name = std::string(path.data).substr(idx + 1);
				texture_list[i] = file_name;
			}
		}

		// Controllo l'esistenza di una texture diffusa (colore semplice)
		if (material->GetTextureCount(aiTextureType_BASE_COLOR))
		{
			// Prelevamento della path
			aiString path;
			if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &path) == AI_SUCCESS)
			{
				// Eliminazione delle relative paths
				const int idx = std::string(path.data).rfind("\\");
				std::string file_name = std::string(path.data).substr(idx + 1);
				texture_list[i] = file_name;
			}
		}

		// Controllo l'esistenza di una texture diffusa (colore semplice)
		if (material->GetTextureCount(aiTextureType_AMBIENT))
		{
			// Prelevamento della path
			aiString path;
			if (material->GetTexture(aiTextureType_AMBIENT, 0, &path) == AI_SUCCESS)
			{
				// Eliminazione delle relative paths
				const int idx = std::string(path.data).rfind("\\");
				std::string file_name = std::string(path.data).substr(idx + 1);
				texture_list[i] = file_name;
			}
		}



		// Se non è presente alcuna texture, utilizza miss.png
		if (texture_list[i].empty())
		{
			texture_list[i] = "miss.png";
		}

	}
	return texture_list;
}

std::vector<Mesh> MeshModel::LoadNode(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiNode* node, const aiScene* scene, std::vector<int> matToTex)
{
	std::vector<Mesh> mesh_list;

	// Attraversa ogni mesh per ciascun nodo ed aggiungerla alla mesh list
	for (size_t i = 0; i < node->mNumMeshes; i++)
	{
		// Caricamento della mesh
		mesh_list.push_back(LoadMesh(newPhysicalDevice, newDevice, transferQueue, transferCommandPool,
			scene->mMeshes[i], scene, matToTex));
	}

	// Attraversa ad ogni nodo connesso a questo nodo e caricalo, dopo dichè append la relativa mesh alla lista.

	for (size_t i = 0; i < node->mNumChildren; i++)
	{
		std::vector<Mesh> new_list = LoadNode(newPhysicalDevice, newDevice, transferQueue, transferCommandPool, 
			node->mChildren[i], scene, matToTex);

		mesh_list.insert(mesh_list.end(), new_list.begin(), new_list.end());
	}

	return mesh_list;
}

Mesh MeshModel::LoadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, 
	VkCommandPool transferCommandPool, aiMesh* mesh, const aiScene* scene, std::vector<int> matToTex)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	vertices.resize(mesh->mNumVertices);

	// Attraversa ogni vertice e copia lungo i vertici
	for (size_t i = 0; i < mesh->mNumVertices; i++)
	{
		// Impostazione della posizione
		vertices[i].pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

		// Set tex coords
		if (mesh->mTextureCoords[0])
		{
			vertices[i].tex = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		}
		else
		{
			vertices[i].tex = { 0.0f, 0.0f };
		}

		// Set color (not really used right now in code)
		vertices[i].col = { 1.0f, 1.0f, 1.0f };
	}

	for (size_t i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];

		for (size_t j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	MainDevice m = {newPhysicalDevice, newDevice};

	return Mesh(m, transferQueue, transferCommandPool, &vertices, &indices, matToTex[mesh->mMaterialIndex]);
}