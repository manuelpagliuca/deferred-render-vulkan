#pragma once

#include <fstream>

const std::vector<const char*> deviceExtensions = 
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME			// SwapChain
};

// Indici contententi le posizioni delle Queue Families nel device fisico.
struct QueueFamilyIndices {

	// Famiglia che riguarda la grafica
	int graphicsFamily	   = -1;		// Location of Graphics Family Queue
	int presentationFamily = -1;		// Posizione della Presenation Queue Family (stesso tipo delle GraphicsFamily)
									
	bool isValid()	// Controlla se gli indici delle Queue Families sono validi
	{
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

// Informazioni della surface che la swapchain può supportare
struct SwapChainDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities;	 // Proprietà della Surface, e.g. dimensioni immagine / estensioni
	std::vector<VkSurfaceFormatKHR> formats;		 // Lista dei formati immagini supportati dalla Surface (RGBA, ...)
	std::vector<VkPresentModeKHR> presentationModes; // Surface presentation modes (how the image is presented to screen)
};

struct SwapChainImage {
	VkImage image;			// Immagine fisica che verrà mostrata sul device
	VkImageView imageView;	// Interfaccia logica 
};

static std::vector<char> readFile(const std::string& filename)
{
	// Apre lo stream dal file fornito, come binario e partendo dal fondo (cosi' da poter calcolarne la dimensione)
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	// Controlliamo se il filestream e' stato aperto correttamente
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open a file!");
	}

	// Costruisce il fileBuffer con la dimensione del file SPIR-V
	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> fileBuffer(fileSize);

	// Imposto il cursore del file all'inizio del file
	file.seekg(0);

	// Legge il contenuto del file e lo salva sul fileBuffer (ne legge ;fileSize' byte)
	file.read(fileBuffer.data(), fileSize);

	return fileBuffer;
}