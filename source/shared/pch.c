#include "pch.h"

#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef PLATFORM_SDL
#define DATA_PATH "data/3dmaze"
#elifdef PLATFORM_NDS
#define DATA_PATH "/data/3dmaze"
#endif

Texture gTextures[NUM_TEXTURES] = {0};

void getIniSettings()
{
	if (chdir(DATA_PATH))
		return;

	FILE* f = fopen("textures.cfg", "r");
	if (!f)
		return;

	for (int i=0; i<NUM_TEXTURES; i++)
	{
		char textureFile[128];
		fgets(textureFile, sizeof(textureFile), f);
		for (int j=0; j<sizeof(textureFile); j++)
		{
			if (textureFile[j] != '\n') continue;
			textureFile[j] = 0;
			break;
		}
		gTextures[i].data = stbi_load(textureFile, &gTextures[i].w, &gTextures[i].h, &gTextures[i].n, 0);
		if (gTextures[i].data)
			printf("loaded %s\n", textureFile);
	}

	fclose(f);

	// get wall/floor/ceiling texture enables
	// For now, texturing always on
	/*
	for( i = 0; i < NUM_SURFACES; i++ )
		gTexInfo[i].bTex = TRUE;

	option = ss_GetRegistryInt( IDS_DEFAULT_TEXTURE_ENABLE, (1 << NUM_SURFACES)-1 );
	for( i = 0; i < NUM_SURFACES; i++, option >>= 1 )
		gTexInfo[i].bDefTex = option & 1;

	// get default texture indices

	for( i = 0; i < NUM_SURFACES; i++ ) {
		gTexInfo[i].iDefTex = 
				ss_GetRegistryInt( gTex[i].defTexIDS, gTex[i].iDefTex );
		SS_CLAMP_TO_RANGE2( gTexInfo[i].iDefTex, 0, 
											NUM_DEF_SURFACE_TEXTURES-1 );
	}

	// get user texture files

	for( i = 0; i < NUM_SURFACES; i++ ) {
		ss_GetRegistryString( gTex[i].fileIDS, 0, 
							  gTexInfo[i].texFile.szPathName, MAX_PATH);
		gTexInfo[i].texFile.nOffset = ss_GetRegistryInt( gTex[i].offsetIDS, 0 );
	}

	// get overlay
	maze_options.top_view = ss_GetRegistryInt( IDS_OVERLAY, 0 );

	// Get rat population
	maze_options.nrats = ss_GetRegistryInt(IDS_NRATS, 1);
	
	// get image quality

	giImageQual = ss_GetRegistryInt( IDS_IMAGEQUAL, IMAGEQUAL_DEFAULT );
	SS_CLAMP_TO_RANGE2( giImageQual, IMAGEQUAL_DEFAULT, IMAGEQUAL_HIGH );

	// get size

	giSize = ss_GetRegistryInt( IDS_SIZE, 0 );
	SS_CLAMP_TO_RANGE2( giSize, MIN_SLIDER, MAX_SLIDER );

	// get turbo mode
	gbTurboMode = ss_GetRegistryInt( IDS_TURBOMODE, 1 );
	*/
}
