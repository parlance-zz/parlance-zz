#define UNITY_API extern "C" __declspec(dllexport)

#include <cstdint>
#include <intrin.h>
#include <vector>
#include <random>

using namespace std;

#define NULL_TILE_ID					0xFFFF
#define CONSTRAIN_FLAG_BANISH_ON_FAIL	0x1

struct Vector2Int
{
	int x;
	int y;
};

struct Vector3Int
{
	int x;
	int y;
	int z;
};

struct CompositeTile
{
	vector<uint16_t> adjacency[8];
	vector<uint32_t> adjacentWeights[8];
	int64_t numAdjacents[8];
	vector<Vector3Int> tileLocations;
	int64_t numTileLocations;

};

struct CompositeMap
{
	vector<uint16_t> map;
	int sizeX, sizeY;
};

struct Generator
{
	vector<CompositeTile> tiles;
	vector<CompositeMap> sampleMaps;
};

struct TileWeight
{
	uint64_t weight;
	Vector3Int context;
	uint16_t tileId;
};

struct SuperMap
{
	Generator *generator;

	vector<uint64_t> scratch;

	vector<uint64_t> superTiles;
	vector<uint64_t> superTilesBackup;
	vector<uint16_t> superTileCounts;
	vector<uint64_t> superTileDirtyBits;
	vector<uint64_t> superTileModifiedBits;
	
	vector<TileWeight> tileWeights;

	vector<uint16_t> targetMap;
	vector<Vector3Int> contextMap;

	vector<Vector3Int> contexts;

	mt19937_64 rng;

	int qwordsPerSuperTile;
	int sizeX, sizeY;
	uint16_t constraintPropMaxDensity;
};

const int adjX[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
const int adjY[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };

inline uint64_t GetBitfieldTrailerMask(size_t capacity)
{
	if (!(capacity & 0x3F)) return uint64_t(0xFFFFFFFFFFFFFFFF);
	return (uint64_t(1) << (capacity & 0x3F)) - 1;
}

inline void SetBit(vector<uint64_t> &buffer, int index)
{
	buffer[index >> 6] |= uint64_t(1) << (index & 0x3F);
}

inline uint64_t GetBit(vector<uint64_t> &buffer, int index)
{
	return (buffer[index >> 6] & (uint64_t(1) << (index & 0x3F)));
}

inline void ClearBit(vector<uint64_t> &buffer, int index)
{
	buffer[index >> 6] &= ~(uint64_t(1) << (index & 0x3F));
}

inline void RecountSuperTiles(SuperMap *map)
{
	for (int i = 0; i < map->sizeX * map->sizeY; i++)
	{
		uint64_t count = 0;
		for (int q = 0; q < map->qwordsPerSuperTile; q++) count += __popcnt64(map->superTiles[i * map->qwordsPerSuperTile + q]);
		map->superTileCounts[i] = (uint16_t)count;
	}
}

inline void RecoverSuperTiles(SuperMap *map)
{
	for (int i = 0; i < map->superTileModifiedBits.size(); i++)
	{
		unsigned long index;
		while (_BitScanForward64(&index, map->superTileModifiedBits[i]))
		{
			map->superTileModifiedBits[i] ^= uint64_t(1) << index;
			int64_t tileOffset = index + (i << 6);
			memcpy(&map->superTiles[tileOffset * map->qwordsPerSuperTile], &map->superTilesBackup[tileOffset * map->qwordsPerSuperTile], map->qwordsPerSuperTile << 3);

			uint64_t count = 0;
			for (int q = 0; q < map->qwordsPerSuperTile; q++) count += __popcnt64(map->superTiles[tileOffset * map->qwordsPerSuperTile + q]);
			map->superTileCounts[tileOffset] = (uint16_t)count;
		}
	}
}

/*
extern "C" int64_t __cdecl _MatchCount(uint16_t *s1, uint16_t *s2, Vector2Int dim, Vector2Int stride);
inline int64_t MatchCount(int width, int height, CompositeMap *map1, int x1, int y1, uint16_t *map2, int x2, int y2, int sizeX2, int sizeY2)
{
	int xMin1 = x1 - (width >> 1);  if (xMin1 < 0) { width += xMin1;  xMin1 = 0; }
	int yMin1 = y1 - (height >> 1); if (yMin1 < 0) { height += yMin1;  yMin1 = 0; }
	if ((xMin1 + width)  > map1->sizeX) width = map1->sizeX - xMin1;
	if ((yMin1 + height) > map1->sizeY) height = map1->sizeY - yMin1;
	int offset1 = xMin1 + yMin1 * map1->sizeX;

	int xMin2 = x2 - (width >> 1);  if (xMin2 < 0) { width += xMin2; xMin2 = 0; }
	int yMin2 = y2 - (height >> 1); if (yMin2 < 0) { height += yMin2;  yMin2 = 0; }
	if ((xMin2 + width) > sizeX2) width = sizeX2 - xMin2;
	if ((yMin2 + height) > sizeY2) height = sizeY2 - yMin2;
	int offset2 = xMin2 + yMin2 * sizeX2;

	if ((width <= 0) || (height <= 0)) return 0;

	Vector2Int dim; dim.x = width; dim.y = height;
	Vector2Int stride; stride.x = map1->sizeX - width; stride.y = sizeX2 - width;
	
	return _MatchCount(&map1->map[offset1], &map2[offset2], dim, stride);
}*/
/*
for (int i2 = 0; i2 < map->generator->tiles[tileId].numTileLocations; i2++)
{
Vector3Int loc = map->generator->tiles[tileId].tileLocations[i2];
int64_t matchCount = MatchCount(map->matchCountWidth, map->matchCountHeight, &map->generator->sampleMaps[loc.z], loc.x, loc.y, &map->targetMap[0], x, y, map->sizeX, map->sizeY);

weight += matchCount * matchCount * temperature * temperature;
}*/
/*
for (int i = 0; i < tileCount; i++)
{
int64_t exponent = map->tileWeights[i].weight - bestWeight + temperature; if (exponent < 0) exponent = 0;
uint64_t renormalizedWeight = uint64_t(1) << exponent; if (!map->tileWeights[i].tileId) renormalizedWeight = 0;
map->tileWeights[i].weight = renormalizedWeight;
totalWeight += renormalizedWeight;
}*/

UNITY_API Generator* __cdecl RMX_CreateGenerator(int numCompositeTiles, int numSampleMaps)
{
	Generator *gen = new Generator;

	gen->tiles.resize(numCompositeTiles);
	gen->sampleMaps.resize(numSampleMaps);

	return gen;
}

UNITY_API void __cdecl RMX_Generator_Destroy(Generator *gen)
{
	delete gen;
}

UNITY_API void __cdecl RMX_Generator_SetTileAdjacency(Generator *gen, int tile, int direction, int numAdjacents, const uint16_t *adjacentIds, const uint32_t *adjacentWeights)
{
	gen->tiles[tile].numAdjacents[direction] = numAdjacents;
	if (numAdjacents > 0)
	{
		gen->tiles[tile].adjacency[direction].resize(numAdjacents);
		memcpy(&gen->tiles[tile].adjacency[direction][0], adjacentIds, numAdjacents << 1);
		gen->tiles[tile].adjacentWeights[direction].resize(numAdjacents);
		memcpy(&gen->tiles[tile].adjacentWeights[direction][0], adjacentWeights, numAdjacents << 2);
	}
}

UNITY_API void __cdecl RMX_Generator_SetTileLocations(Generator *gen, int tile, int numLocations, const Vector3Int *locations)
{
	gen->tiles[tile].numTileLocations = numLocations;
	if (numLocations > 0)
	{
		gen->tiles[tile].tileLocations.resize(numLocations);
		memcpy(&gen->tiles[tile].tileLocations[0], locations, numLocations * sizeof(Vector3Int));
	}
}

UNITY_API void __cdecl RMX_Generator_SetSampleMap(Generator *gen, int map, int sizeX, int sizeY, const uint16_t *mapData)
{
	gen->sampleMaps[map].sizeX = sizeX;
	gen->sampleMaps[map].sizeY = sizeY;

	gen->sampleMaps[map].map.resize(sizeX * sizeY);
	memcpy(&gen->sampleMaps[map].map[0], mapData, (sizeX * sizeY) << 1);
}

UNITY_API SuperMap* __cdecl RMX_Generator_CreateSuperMap(Generator *gen, int sizeX, int sizeY, uint64_t seed)
{
	SuperMap *map = new SuperMap;
	map->generator = gen;

	map->sizeX = sizeX;
	map->sizeY = sizeY;

	map->constraintPropMaxDensity = (uint16_t)map->generator->tiles.size();

	map->qwordsPerSuperTile = int((gen->tiles.size() + 63) >> 6);
	map->scratch.resize(map->qwordsPerSuperTile);
	map->tileWeights.resize(gen->tiles.size());

	map->superTiles.resize(map->qwordsPerSuperTile * sizeX * sizeY);
	map->superTilesBackup.resize(map->qwordsPerSuperTile * sizeX * sizeY);

	map->superTileCounts.resize(sizeX * sizeY);
	map->targetMap.resize(sizeX * sizeY);
	map->contextMap.resize(sizeX * sizeY);

	map->superTileDirtyBits.resize((sizeX * sizeY + 63) >> 6);
	memset(&map->superTileDirtyBits[0], 0, map->superTileDirtyBits.size() << 3);
	map->superTileModifiedBits.resize((sizeX * sizeY + 63) >> 6);
	memset(&map->superTileModifiedBits[0], 0, map->superTileModifiedBits.size() << 3);

	uint64_t qwordPadMask = GetBitfieldTrailerMask(gen->tiles.size());
	for (int i = 0; i < sizeX * sizeY; i++)
	{
		for (int q = 0; q < map->qwordsPerSuperTile; q++) map->superTiles[i * map->qwordsPerSuperTile + q] = 0xFFFFFFFFFFFFFFFF;
		map->superTiles[i * map->qwordsPerSuperTile + map->qwordsPerSuperTile - 1] &= qwordPadMask;
		map->superTileCounts[i] = int(gen->tiles.size());

		map->targetMap[i] = NULL_TILE_ID;
		map->contextMap[i].x = 0; map->contextMap[i].y = 0; map->contextMap[i].z = -1;
	}

	map->rng.seed(seed);
	
	return map;
}

UNITY_API void __cdecl RMX_SuperMap_Destroy(SuperMap *map)
{
	delete map;
}

UNITY_API void __cdecl RMX_SuperMap_GetTargetMap(SuperMap *map, uint16_t *out)
{
	memcpy(out, &map->targetMap[0], (map->sizeX * map->sizeY) << 1);
}

UNITY_API void __cdecl RMX_SuperMap_GetContextMap(SuperMap *map, Vector3Int *out)
{
	memcpy(out, &map->contextMap[0], (map->sizeX * map->sizeY) * sizeof(Vector3Int));
}

UNITY_API void __cdecl RMX_SuperMap_GetSuperTiles(SuperMap *map, uint64_t *out)
{
	memcpy(out, &map->superTiles[0], map->superTiles.size() * sizeof(uint64_t));
}

UNITY_API void __cdecl RMX_SuperMap_SetTargetMap(SuperMap *map, const uint16_t *mapData)
{
	memcpy(&map->targetMap[0], mapData, (map->sizeX * map->sizeY) << 1);
}

UNITY_API void __cdecl RMX_SuperMap_SetContextMap(SuperMap *map, const Vector3Int *contextData)
{
	memcpy(&map->contextMap[0], contextData, (map->sizeX * map->sizeY) * sizeof(Vector3Int));
}

UNITY_API void __cdecl RMX_SuperMap_SetSuperTiles(SuperMap *map, const uint64_t *superTiles)
{
	memcpy(&map->superTiles[0], superTiles, map->superTiles.size() * sizeof(uint64_t));
	RecountSuperTiles(map);
}

UNITY_API void __cdecl RMX_SuperMap_GetDensityMap(SuperMap *map, float *out)
{
	float invNumTiles = 1.0f / (float)map->generator->tiles.size();
	for (int i = 0; i < map->superTileCounts.size(); i++) out[i] = map->superTileCounts[i] * invNumTiles;
}

UNITY_API Vector2Int __cdecl RMX_SuperMap_FindNextGenLocation(SuperMap *map)
{
	Vector2Int nextGenLoc; nextGenLoc.x = -1; nextGenLoc.y = -1;
	uint16_t lowestCount = 0xFFFF;
	int bestTileOffset = -1;

	for (int i = 0; i < map->superTileCounts.size(); i++)
	{
		if (map->targetMap[i] == NULL_TILE_ID)
		{
			if (map->superTileCounts[i] < lowestCount)
			{
				lowestCount = map->superTileCounts[i];
				bestTileOffset = i;
			}
		}
	}

	if (bestTileOffset >= 0)
	{
		nextGenLoc.y = bestTileOffset / map->sizeX;
		nextGenLoc.x = bestTileOffset - nextGenLoc.y * map->sizeX;
	}

	return nextGenLoc;
}

UNITY_API void __cdecl RMX_SuperMap_SetParams(SuperMap *map, int constraintPropMaxDensity)
{
	map->constraintPropMaxDensity = constraintPropMaxDensity;
}

UNITY_API int __cdecl RMX_SuperMap_Constrain(SuperMap *map, int numTiles, const Vector2Int *locations, const uint16_t *tileIds, uint32_t flags)
{
	memset(&map->superTileModifiedBits[0], 0, map->superTileModifiedBits.size() << 3);

	for (int t = 0; t < numTiles; t++)
	{
		int tileOffset = locations[t].y * map->sizeX + locations[t].x;
		SetBit(map->superTileDirtyBits, tileOffset);

		if (tileIds[t] != NULL_TILE_ID)
		{
			memcpy(&map->superTilesBackup[tileOffset * map->qwordsPerSuperTile], &map->superTiles[tileOffset * map->qwordsPerSuperTile], map->qwordsPerSuperTile << 3);
			SetBit(map->superTileModifiedBits, tileOffset);

			memset(&map->superTiles[tileOffset * map->qwordsPerSuperTile], 0, map->qwordsPerSuperTile << 3);
			SetBit(map->superTiles, ((tileOffset * map->qwordsPerSuperTile) << 6) + tileIds[t]);
			map->superTileCounts[tileOffset] = 1;
		}
	}

	while (true)
	{
		uint16_t lowestCount = 0xFFFF;
		int tileOffset;
		for (int i = 0; i < map->superTileDirtyBits.size(); i++)
		{
			uint64_t temp = map->superTileDirtyBits[i];
			unsigned long index;
			while (_BitScanForward64(&index, temp))
			{
				int dirtyTileOffset = index + (i << 6);
				if (map->superTileCounts[dirtyTileOffset] < lowestCount)
				{
					tileOffset = dirtyTileOffset;
					lowestCount = map->superTileCounts[dirtyTileOffset];
				}
				temp ^= uint64_t(1) << index;
			}
		}
		if (lowestCount >= map->constraintPropMaxDensity) break;

		ClearBit(map->superTileDirtyBits, tileOffset);

		int tileY = tileOffset / map->sizeX;
		int tileX = tileOffset - tileY * map->sizeX;

		for (int a = 0; a < 8; a++)
		{
			int otherY = tileY + adjY[a];
			int otherX = tileX + adjX[a];
			if ((otherX < 0) || (otherX >= map->sizeX) || (otherY < 0) || (otherY >= map->sizeY)) continue;

			int otherTileOffset = otherY * map->sizeX + otherX;
			if (map->targetMap[otherTileOffset] != NULL_TILE_ID) continue;

			if (!GetBit(map->superTileModifiedBits, otherTileOffset))
			{
				memcpy(&map->superTilesBackup[otherTileOffset * map->qwordsPerSuperTile], &map->superTiles[otherTileOffset * map->qwordsPerSuperTile], map->qwordsPerSuperTile << 3);
				SetBit(map->superTileModifiedBits, otherTileOffset);
			}

			memset(&map->scratch[0], 0, map->qwordsPerSuperTile << 3);
			uint64_t *super = &map->superTiles[tileOffset * map->qwordsPerSuperTile];

			for (int i2 = 0; i2 < map->qwordsPerSuperTile; i2++)
			{
				uint64_t temp = super[i2];
				unsigned long index2;
				while (_BitScanForward64(&index2, temp))
				{
					int tileId = index2 + (i2 << 6);
					for (int i3 = 0; i3 < map->generator->tiles[tileId].numAdjacents[a]; i3++) SetBit(map->scratch, map->generator->tiles[tileId].adjacency[a][i3]);
					temp ^= uint64_t(1) << index2;
				}
			}

			uint64_t preFilterCount = map->superTileCounts[otherTileOffset], postFilterCount = 0;
			uint64_t *otherSuper = &map->superTiles[otherTileOffset * map->qwordsPerSuperTile];
			for (int i2 = 0; i2 < map->qwordsPerSuperTile; i2++)
			{
				otherSuper[i2] &= map->scratch[i2];
				postFilterCount += __popcnt64(otherSuper[i2]);
			}

			if (postFilterCount < preFilterCount)
			{
				if (postFilterCount == 0)
				{
					memset(&map->superTileDirtyBits[0], 0, map->superTileDirtyBits.size() << 3);
					RecoverSuperTiles(map);

					if (flags & CONSTRAIN_FLAG_BANISH_ON_FAIL)
					{
						uint16_t lowestCount = 0xFFFF;

						for (int t = 0; t < numTiles; t++)
						{
							int tileOffset = locations[t].y * map->sizeX + locations[t].x;
							ClearBit(map->superTiles, ((tileOffset * map->qwordsPerSuperTile) << 6) + tileIds[t]);
							if ((--map->superTileCounts[tileOffset]) == 0) lowestCount = 0;
						}

						if (lowestCount) return -1;
						else return 0;
					}
					else
					{
						return 0;
					}
				}

				SetBit(map->superTileDirtyBits, otherTileOffset);
				map->superTileCounts[otherTileOffset] = (uint16_t)postFilterCount;
			}
		}
	}

	return 1;
}

UNITY_API int __cdecl RMX_SuperMap_ClearTiles(SuperMap *map, int numTiles, const Vector2Int *locations)
{
	for (int i = 0; i < numTiles; i++)
	{
		int tileOffset = locations[i].y * map->sizeX + locations[i].x;
		map->targetMap[tileOffset] = NULL_TILE_ID;
		map->contextMap[tileOffset].x = 0;
		map->contextMap[tileOffset].y = 0;
		map->contextMap[tileOffset].z = -1;
	}

	uint64_t qwordPadMask = GetBitfieldTrailerMask(map->generator->tiles.size());
	for (int i = 0; i < map->sizeX * map->sizeY; i++)
	{
		if (map->targetMap[i] == NULL_TILE_ID)
		{
			for (int q = 0; q < map->qwordsPerSuperTile; q++) map->superTiles[i * map->qwordsPerSuperTile + q] = 0xFFFFFFFFFFFFFFFF;
			map->superTiles[i * map->qwordsPerSuperTile + map->qwordsPerSuperTile - 1] &= qwordPadMask;
		}
	}

	uint16_t tileId = NULL_TILE_ID;
	for (int y = 0; y < map->sizeY; y++)
	{
		for (int x = 0; x < map->sizeX; x++)
		{
			int tileOffset = y * map->sizeX + x;
			if (map->targetMap[tileOffset] != NULL_TILE_ID)
			{
				Vector2Int location; location.x = x; location.y = y;
				if (!RMX_SuperMap_Constrain(map, 1, &location, &tileId, 0)) return 0;
			}
		}
	}

	return 1;
}

UNITY_API int __cdecl RMX_SuperMap_PaintTiles(SuperMap *map, int numTiles, Vector2Int *locations, uint16_t *tileIds, int temperature, uint32_t flags)
{
	vector<Vector3Int> &contexts = map->contexts;
	if (contexts.size() < numTiles) contexts.resize(numTiles);
	
	for (int t = 0; t < numTiles; t++)
	{
		contexts[t].x = 0; contexts[t].y = 0; contexts[t].z = -1;
		uint16_t tileId = tileIds[t];
		if (tileId == NULL_TILE_ID)
		{
			int x = locations[t].x, y = locations[t].y;
			int tileOffset = y * map->sizeX + x;
			uint64_t *super = &map->superTiles[tileOffset * map->qwordsPerSuperTile];

			int64_t tileCount = 0; uint64_t totalWeight = 0;
			for (int i = 0; i < map->qwordsPerSuperTile; i++)
			{
				uint64_t temp = super[i];
				unsigned long index;
				while (_BitScanForward64(&index, temp))
				{
					uint64_t tileId = index + (i << 6);
					if (tileId)
					{
						uint64_t randContextIndex = map->rng() % map->generator->tiles[tileId].numTileLocations;
						map->tileWeights[tileCount].context = map->generator->tiles[tileId].tileLocations[randContextIndex];

						uint64_t weight = 1; 
						for (int a = 0; a < 8; a++)
						{
							int otherX = x + adjX[a];
							int otherY = y + adjY[a];
							if ((otherX < 0) || (otherX >= map->sizeX) || (otherY < 0) || (otherY >= map->sizeY)) continue;
							int otherTileOffset = otherY * map->sizeX + otherX;

							for (int i2 = 0; i2 < map->qwordsPerSuperTile; i2++)
							{
								uint64_t temp2 = map->superTiles[otherTileOffset * map->qwordsPerSuperTile + i2];
								unsigned long index2;
								while (_BitScanForward64(&index2, temp2))
								{
									uint64_t otherTileId = index2 + (i2 << 6);
									for (int j = 0; j < map->generator->tiles[otherTileId].numAdjacents[7 - a]; j++)
									{
										if (map->generator->tiles[otherTileId].adjacency[7 - a][j] == tileId)
										{
											weight += uint64_t(map->generator->tiles[otherTileId].adjacentWeights[7 - a][j]) << 7;
											break;
										}
									}
									temp2 ^= uint64_t(1) << index2;
								}
							}

							if (map->contextMap[otherTileOffset].z != -1)
							{								
								Vector3Int otherContext = map->contextMap[otherTileOffset]; otherContext.x -= adjX[a]; otherContext.y -= adjY[a];
								if ((otherContext.x < 0) || (otherContext.y < 0) || (otherContext.x >= map->generator->sampleMaps[otherContext.z].sizeX) || (otherContext.y >= map->generator->sampleMaps[otherContext.z].sizeY)) continue;
								uint16_t contextTileId = map->generator->sampleMaps[otherContext.z].map[otherContext.y * map->generator->sampleMaps[otherContext.z].sizeX + otherContext.x];
								if (contextTileId == tileId)
								{
									if ((map->rng() & 0xFF) > temperature)
									{
										if (weight < uint64_t(0x0010000000000000)) weight <<= 9;
										map->tileWeights[tileCount].context = otherContext;
									}
								}	
							}
						}
						totalWeight += weight;
						map->tileWeights[tileCount].weight = weight;
						map->tileWeights[tileCount++].tileId = (uint16_t)tileId;
					}
					temp ^= uint64_t(1) << index;
				}
			}
			if (!totalWeight) return 0;

			uint64_t rand = map->rng() % totalWeight, weight = 0;
			for (int i = 0; i < tileCount; i++)
			{
				uint64_t nextWeight = weight + map->tileWeights[i].weight;
				if ((rand >= weight) && (rand < nextWeight))
				{
					tileId = map->tileWeights[i].tileId;
					contexts[t] = map->tileWeights[i].context;
					break;
				}
				weight = nextWeight;
			}
			tileIds[t] = tileId;
		}
	}

	int constrainResult = RMX_SuperMap_Constrain(map, numTiles, locations, tileIds, flags);
	if (constrainResult == 1)
	{
		for (int t = 0; t < numTiles; t++)
		{
			int x = locations[t].x, y = locations[t].y;
			int tileOffset = y * map->sizeX + x;
			map->targetMap[tileOffset] = tileIds[t];
			map->contextMap[tileOffset] = contexts[t];
		}
		return 1;
	}

	return constrainResult;
}