#include <filesystem>
#include <stdio.h>
#include <fstream>
#include <Shlwapi.h>
#include "fpng/fpng.cpp" // Yes cpp so we don't have to change project settings

#define ASINAME L"PNGScreenshots"
#define MYHOOK "PNGScreenShots"
#define VERSION L"1.0.0"

#include "../../Shared-ASI/Interface.h"

#ifdef GAMELE1
SPI_PLUGINSIDE_SUPPORT(ASINAME, VERSION, L"ME3Tweaks", SPI_GAME_LE1, SPI_VERSION_ANY);
#endif
#ifdef GAMELE2
SPI_PLUGINSIDE_SUPPORT(ASINAME, VERSION, L"ME3Tweaks", SPI_GAME_LE2, SPI_VERSION_ANY);
#endif
#ifdef GAMELE3
SPI_PLUGINSIDE_SUPPORT(ASINAME, VERSION, L"ME3Tweaks", SPI_GAME_LE3, SPI_VERSION_ANY);
#endif

#include "../../Shared-ASI/Common.h"
#include "../../Shared-ASI/ME3Tweaks/ME3TweaksHeader.h"

SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;

// ======================================================================
// appCreateBitmap hook
// ======================================================================

// Prototypes for appCreateBitmap
typedef void (*tAppCreateBitmap)(wchar_t* pattern, int width, int height, FColor* data, void* fileManager);
tAppCreateBitmap appCreateBitmap = nullptr;
tAppCreateBitmap appCreateBitmap_orig = nullptr;

bool fpngInitialized = false;

// The initial screenshot index we should check against. As we take screenshots we know the previous images will always exist
// and as such we can just increment this on auto-generated screenshots to match the game's incremental system
int cachedScreenshotIndex = 1;

// Hopefully user never gets here but this is so it doesn't make an infinite loop
int maxScreenshotIndex = 99999;

// data is an array of FColor structs (BGRA) of size width * height
void appCreateBitmap_hook(wchar_t* inputBaseName, int width, int height, FColor* data, void* fileManager)
{
	if (!fpngInitialized)
	{
		// Initialize the library before use
		fpng::fpng_init();
		fpngInitialized = true;
	}

	long byteCount = width * height * 3;

	// Color order needs swapped around for FPNG to access data
	// since nothing supports BRGA.
	std::vector<unsigned char> newImageData(byteCount);
	int pixelIndex = 0; // Which pixel we are one
	long totalCount = width * height; // how many pixels
	long position = 0; // Where to place into vector
	for (long i = 0; i < totalCount; i++)
	{
		auto color = &data[pixelIndex];
		newImageData[position] = color->R;
		newImageData[position + 1L] = color->G;
		newImageData[position + 2L] = color->B;

		pixelIndex++;
		position = pixelIndex * 3;
	}

	// Determine output filename.
	auto path = std::filesystem::path(inputBaseName);
	auto extension = path.extension();
	if (extension != ".png")
	{
		// Calculate a new name
		auto outPath = std::filesystem::path(inputBaseName);
		std::wstring newFname;
		while (cachedScreenshotIndex < maxScreenshotIndex)
		{
#ifdef GAMELE1
			newFname = wstring_format(L"PNGLE1ScreenShot%05i.png", cachedScreenshotIndex);
#elif defined(GAMELE2)
			newFname = wstring_format(L"PNGLE2ScreenShot%05i.png", cachedScreenshotIndex);
#elif defined(GAMELE3)
			newFname = wstring_format(L"PNGLE3ScreenShot%05i.png", cachedScreenshotIndex);
#endif
			cachedScreenshotIndex++; // Increment the cached index immediately.
			outPath.replace_filename(newFname);

			// Check if file exists
			if (!std::filesystem::exists(outPath))
			{
				// File doesn't exist. Use this one
				path = outPath;
				break;
			}
			// File exists, go to next one
		}

		if (cachedScreenshotIndex == maxScreenshotIndex)
			return; // Can't take any more screenshots
	}

	// Save the image data using the fpng library.
	fpng::fpng_encode_image_to_wfile(path.c_str(), newImageData.data(), width, height, 3, 0U); // 3bpp, no flags
}


SPI_IMPLEMENT_ATTACH
{
	// Hook appCreateBitmap and provide our own implementation of saving the bitmap data to disk.
	auto _ = SDKInitializer::Instance();
#ifdef GAMELE1
	INIT_FIND_PATTERN_POSTHOOK(appCreateBitmap, /*40 55 53 56 57*/ "41 54 41 55 41 56 41 57 48 8d ac 24 38 f8 ff ff 48 81 ec c8 08 00 00 48 c7 44 24 70 fe ff ff ff 48 8b 05 3c b6 55 01 48 33 c4 48 89 85 b0 07 00 00 4c 89 4c 24 48 44 89 44 24 58");
#elif defined(GAMELE2)
	INIT_FIND_PATTERN_POSTHOOK(appCreateBitmap, /*40 55 53 56 57 */ "41 54 41 55 41 56 41 57 48 8d ac 24 38 f8 ff ff 48 81 ec c8 08 00 00 48 c7 44 24 70 fe ff ff ff 48 8b 05 ec 2a 58 01");
#elif defined(GAMELE3)
	INIT_FIND_PATTERN_POSTHOOK(appCreateBitmap, /*40 55 53 56 57 */ "41 54 41 55 41 56 41 57 48 8d ac 24 78 f8 ff ff 48 81 ec 88 08 00 00 48 c7 44 24 60 fe ff ff ff");
#endif

	INIT_HOOK_PATTERN(appCreateBitmap);
	return true;
}

SPI_IMPLEMENT_DETACH
{
	return true;
}